/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)

 *        Driver for program to modify an exiting user
 *
 * Authors:
 *
 *        Krishna Ganugapati (krishnag@likewisesoftware.com)
 *        Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "mod_user_includes.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
MapErrorCode(
    DWORD dwError
    );

static
DWORD
ModifyUser(
    PCSTR pszUid,
    PCSTR pszLoginId,
    PDLINKEDLIST pTaskList
    );

static
DWORD
ParseArgs(
    int   argc,
    char* argv[],
    PSTR* ppszUid,
    PSTR* ppszLoginId,
    PDLINKEDLIST* ppTaskList
    );

static
VOID
FreeTasksInList(
    PVOID pTask,
    PVOID pUserData
    );

static
VOID
FreeTask(
    PUSER_MOD_TASK pTask
    );

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

static
BOOLEAN
ValidateArgs(
    PCSTR pszUid,
    PCSTR pszLoginId,
    PDLINKEDLIST pTaskList
    );

static
DWORD
BuildUserModInfo(
    uid_t        uid,
    PDLINKEDLIST pTaskList,
    PLSA_USER_MOD_INFO* ppUserModInfo
    );

static
DWORD
LsaModUserMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pTaskList = NULL;
    PSTR pszUid = NULL;
    PSTR pszLoginId = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

     if (geteuid() != 0) {
         fprintf(stderr, "This program requires super-user privileges.\n");
         dwError = LW_ERROR_ACCESS_DENIED;
         BAIL_ON_LSA_ERROR(dwError);
     }

     dwError = ParseArgs(
                     argc,
                     argv,
                     &pszUid,
                     &pszLoginId,
                     &pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

     dwError = ModifyUser(
                     pszUid,
                     pszLoginId,
                     pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

 cleanup:

     if (pTaskList) {
         LsaDLinkedListForEach(pTaskList, &FreeTasksInList, NULL);
         LsaDLinkedListFree(pTaskList);
     }

     LW_SAFE_FREE_STRING(pszUid);
     LW_SAFE_FREE_STRING(pszLoginId);

     return dwError;

 error:

     dwError = MapErrorCode(dwError);

     dwErrorBufferSize = LwGetErrorString(dwError, NULL, 0);

     if (dwErrorBufferSize > 0)
     {
         DWORD dwError2 = 0;
         PSTR   pszErrorBuffer = NULL;

         dwError2 = LwAllocateMemory(
                     dwErrorBufferSize,
                     (PVOID*)&pszErrorBuffer);

         if (!dwError2)
         {
             DWORD dwLen = LwGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);

             if ((dwLen == dwErrorBufferSize) && !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
             {
                 fprintf(stderr,
                         "Failed to modify user.  Error code %u (%s).\n%s\n",
                         dwError,
                         LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                         pszErrorBuffer);
                 bPrintOrigError = FALSE;
             }
         }

         LW_SAFE_FREE_STRING(pszErrorBuffer);
     }

     if (bPrintOrigError)
     {
         fprintf(stderr,
                 "Failed to modify user.  Error code %u (%s).\n",
                 dwError,
                 LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
     }

     goto cleanup;
}

static
DWORD
ParseArgs(
    int   argc,
    char* argv[],
    PSTR* ppszUid,
    PSTR* ppszLoginId,
    PDLINKEDLIST* ppTaskList
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_ADD_TO_GROUPS,
        PARSE_MODE_REMOVE_FROM_GROUPS,
        PARSE_MODE_SET_EXPIRY_DATE,
        PARSE_MODE_SET_PRIMARY_GROUP,
        PARSE_MODE_SET_NT_PASSWORD_HASH,
        PARSE_MODE_SET_LM_PASSWORD_HASH,
        PARSE_MODE_SET_PASSWORD,
        PARSE_MODE_SET_HOMEDIR,
        PARSE_MODE_SET_SHELL,
        PARSE_MODE_SET_GECOS,
        PARSE_MODE_UID,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PDLINKEDLIST pTaskList = NULL;
    PUSER_MOD_TASK pTask = NULL;
    PSTR    pszUid = NULL;
    PSTR    pszLoginId = NULL;
    PSTR    pszPassword = NULL;

    do {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
          break;
        }

        switch(parseMode) {
            case PARSE_MODE_OPEN:
            {
                if (!strcmp(pArg, "--enable-user"))
                {
                   dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                   BAIL_ON_LSA_ERROR(dwError);
                   pTask->taskType = UserModTask_EnableUser;

                   dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                   BAIL_ON_LSA_ERROR(dwError);

                   pTask = NULL;
                }
                else if (!strcmp(pArg, "--disable-user"))
                {

                    dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_DisableUser;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0))
                {
                  ShowUsage(GetProgramName(argv[0]));
                  exit(0);
                }
                else if (!strcmp(pArg, "--change-password-at-next-logon"))
                {

                    dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_ChangePasswordAtNextLogon;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--unlock")) {

                    dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_UnlockUser;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--password-never-expires"))
                {

                    dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_SetPasswordNeverExpires;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--password-must-expire"))
                {

                    dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                    BAIL_ON_LSA_ERROR(dwError);
                    pTask->taskType = UserModTask_SetPasswordMustExpire;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    pTask = NULL;
                }
                else if (!strcmp(pArg, "--add-to-groups"))
                {
                    parseMode = PARSE_MODE_ADD_TO_GROUPS;
                }
                else if (!strcmp(pArg, "--remove-from-groups"))
                {
                    parseMode = PARSE_MODE_REMOVE_FROM_GROUPS;
                }
                else if (!strcmp(pArg, "--set-nt-password-hash"))
                {
                    parseMode = PARSE_MODE_SET_NT_PASSWORD_HASH;
                }
                else if (!strcmp(pArg, "--set-lm-password-hash"))
                {
                    parseMode = PARSE_MODE_SET_LM_PASSWORD_HASH;
                }
                else if (!strcmp(pArg, "--set-password"))
                {
                    parseMode = PARSE_MODE_SET_PASSWORD;
                }
                else if (!strcmp(pArg, "--set-homedir"))
                {
                    parseMode = PARSE_MODE_SET_HOMEDIR;
                }
                else if (!strcmp(pArg, "--set-shell"))
                {
                    parseMode = PARSE_MODE_SET_SHELL;
                }
                else if (!strcmp(pArg, "--set-gecos"))
                {
                    parseMode = PARSE_MODE_SET_GECOS;
                }
                else if (!strcmp(pArg, "--set-account-expiry"))
                {
                    parseMode = PARSE_MODE_SET_EXPIRY_DATE;
                }
                else if (!strcmp(pArg, "--set-primary-group"))
                {
                    parseMode = PARSE_MODE_SET_PRIMARY_GROUP;
                }
                else if (!strcmp(pArg, "--uid")) {
                    parseMode = PARSE_MODE_UID;
                }
                else
                {
                    dwError = LwAllocateString(pArg, &pszLoginId);
                    BAIL_ON_LSA_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;
            }

            case PARSE_MODE_REMOVE_FROM_GROUPS:
            {
                 dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 pTask->taskType = UserModTask_RemoveFromGroups;

                 dwError = LwAllocateString(pArg, &pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 parseMode = PARSE_MODE_OPEN;

                 break;
            }

            case PARSE_MODE_ADD_TO_GROUPS:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_AddToGroups;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_NT_PASSWORD_HASH:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetNtPasswordHash;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_LM_PASSWORD_HASH:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetLmPasswordHash;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_PASSWORD:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetPassword;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_EXPIRY_DATE:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetExpiryDate;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_PRIMARY_GROUP:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetPrimaryGroup;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_HOMEDIR:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetHomedir;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_SHELL:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetShell;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_SET_GECOS:
            {
                dwError = LwAllocateMemory(sizeof(USER_MOD_TASK), (PVOID*)&pTask);
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = UserModTask_SetGecos;

                dwError = LwAllocateString(pArg, &pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_UID:
            {
                if (!IsUnsignedInteger(pArg))
                {
                    fprintf(stderr, "Please specifiy a uid that is an unsigned integer\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }

                dwError = LwAllocateString(pArg, &pszUid);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
            }

            case PARSE_MODE_DONE:
            {

                ShowUsage(GetProgramName(argv[0]));
                exit(1);
            }
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    if (!ValidateArgs(pszUid, pszLoginId, pTaskList)) {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    *ppszUid = pszUid;
    *ppszLoginId = pszLoginId;
    *ppTaskList = pTaskList;

cleanup:

    LW_SECURE_FREE_STRING (pszPassword);

    return dwError;

error:

    *ppTaskList = NULL;

    if (pTaskList) {
        LsaDLinkedListForEach(pTaskList, FreeTasksInList, NULL);
        LsaDLinkedListFree(pTaskList);
    }

    if (pTask) {
        FreeTask(pTask);
    }

    LW_SAFE_FREE_STRING(pszUid);
    LW_SAFE_FREE_STRING(pszLoginId);

    goto cleanup;
}

static
BOOLEAN
ValidateArgs(
    PCSTR pszUid,
    PCSTR pszLoginId,
    PDLINKEDLIST pTaskList
    )
{
    BOOLEAN bValid = FALSE;

    PDLINKEDLIST pListMember = NULL;
    BOOLEAN bEnableUser = FALSE;
    BOOLEAN bDisableUser = FALSE;
    BOOLEAN bSetChangePasswordAtNextLogon = FALSE;
    BOOLEAN bSetPasswordNeverExpires = FALSE;
    BOOLEAN bSetPassword = FALSE;
    PSTR pszNtPasswordHash = NULL;
    PSTR pszLmPasswordHash = NULL;

    for (pListMember = pTaskList; pListMember; pListMember = pListMember->pNext)
    {
        PUSER_MOD_TASK pTask = (PUSER_MOD_TASK)pListMember->pItem;
        if (pTask) {
           switch(pTask->taskType)
           {
               case UserModTask_EnableUser:
               {
                   bEnableUser = TRUE;
                   break;
               }
               case UserModTask_DisableUser:
               {
                   bDisableUser = TRUE;
                   break;
               }
               case UserModTask_SetPasswordNeverExpires:
               {
                   bSetPasswordNeverExpires = TRUE;
                   break;
               }
               case UserModTask_ChangePasswordAtNextLogon:
               {
                   bSetChangePasswordAtNextLogon = TRUE;
                   break;
               }
               case UserModTask_SetNtPasswordHash:
               {
                   pszNtPasswordHash = pTask->pszData;
                   break;
               }
               case UserModTask_SetLmPasswordHash:
               {
                   pszLmPasswordHash = pTask->pszData;
                   break;
               }
               case UserModTask_SetPassword:
               {
                   bSetPassword = TRUE;
                   break;
               }
               default:
                   break;
           }
        }
    }

    if (bEnableUser && bDisableUser)
    {
        fprintf(stderr, "Error: Both --enable-user and --disable-user cannot be specified.\n");
        goto cleanup;
    }

    if (bSetPasswordNeverExpires && bSetChangePasswordAtNextLogon)
    {
        fprintf(stderr, "Error: The options --password-never-expires and\n");
        fprintf(stderr, "       --change-password-at-next-logon cannot be specified together.\n");
        goto cleanup;
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszLoginId) &&
        LW_IS_NULL_OR_EMPTY_STR(pszUid))
    {
        fprintf(stderr, "Error: A valid user id or user login id must be specified.\n");
        goto cleanup;
    }

    if (pszNtPasswordHash &&
        !(strlen(pszNtPasswordHash) == 32 ||
          strlen(pszNtPasswordHash) == 0))
    {
        fprintf(stderr, "Error: NT password hash must be zero or 32 characters long.\n");
        goto cleanup;
    }

    if (pszLmPasswordHash &&
        !(strlen(pszLmPasswordHash) == 32 ||
          strlen(pszLmPasswordHash) == 0))
    {
        fprintf(stderr, "Error: LM password hash must be zero or 32 characters long.\n");
        goto cleanup;
    }

    if (bSetPassword && pszNtPasswordHash)
    {
        fprintf(stderr, "Error: the password and NT password hash cannot be specified together.\n");
        goto cleanup;
    }

    if (bSetPassword && pszLmPasswordHash)
    {
        fprintf(stderr, "Error: the password and LM password hash cannot be specified together.\n");
        goto cleanup;
    }

    bValid = TRUE;

cleanup:

    return bValid;
}

static
VOID
FreeTasksInList(
    PVOID pTask,
    PVOID pUserData
    )
{
    if (pTask) {
       FreeTask((PUSER_MOD_TASK)pTask);
    }
}

static
VOID
FreeTask(
    PUSER_MOD_TASK pTask
    )
{
    LW_SAFE_FREE_STRING(pTask->pszData);
    LwFreeMemory(pTask);
}

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

static
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s {modification options} ( <user login id> | --uid <uid> )\n\n", pszProgramName);

    fprintf(stdout, "\nModification options:\n");
    fprintf(stdout, "{ --help }\n");
    fprintf(stdout, "{ --disable-user | --enable-user }\n");
    fprintf(stdout, "{ --unlock }\n");
    fprintf(stdout, "{ --change-password-at-next-logon }\n");
    fprintf(stdout, "{ --password-never-expires }\n");
    fprintf(stdout, "{ --password-must-expire }\n");
    fprintf(stdout, "{ --add-to-groups nt4-style-group-name }\n");
    fprintf(stdout, "{ --remove-from-groups nt4-style-group-name }\n");
    fprintf(stdout, "{ --set-nt-password-hash password-hash-hex }\n");
    fprintf(stdout, "{ --set-lm-password-hash password-hash-hex }\n");
    fprintf(stdout, "{ --set-homedir home-directory }\n");
    fprintf(stdout, "{ --set-shell shell }\n");
    fprintf(stdout, "{ --set-gecos gecos }\n");
    fprintf(stdout, "{ --set-account-expiry expiry-date (YYYY-MM-DD format) }\n");
    fprintf(stdout, "{ --set-primary-group gid }\n");
    fprintf(stdout, "{ --set-password password }\n");

    fprintf(stdout, "\nNotes:\n");
    fprintf(stdout, "a) Set the expiry-date to 0 for an account that must never expire.\n");
    fprintf(stdout, "b) If both --remove-from-group and --add-to-group are specified,\n");
    fprintf(stdout, "   the user is removed from the specified group first.\n");
    fprintf(stdout, "c) The options ""--change-password-at-next-logon"" and \n");
    fprintf(stdout, "   ""--password-never-expires"" cannot be set simultaneously.\n");

}

static
DWORD
ModifyUser(
    PCSTR pszUid,
    PCSTR pszLoginId,
    PDLINKEDLIST pTaskList
    )
{
    DWORD dwError = 0;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;
    uid_t uid = 0;
    int   nRead = 0;
    LSA_FIND_FLAGS findFlags = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_QUERY_LIST Query = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszUid))
    {
        nRead = sscanf(pszUid, "%u", (unsigned int*)&uid);
        if ((nRead == EOF) || (nRead == 0)) {
            fprintf(stderr, "An invalid user id [%s] was specified.", pszUid);

            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else if (!LW_IS_NULL_OR_EMPTY_STR(pszLoginId))
    {
        Query.ppszStrings = &pszLoginId;

        dwError = LsaFindObjects(
                       hLsaConnection,
                       NULL,
                       findFlags,
                       LSA_OBJECT_TYPE_USER,
                       LSA_QUERY_TYPE_BY_NAME,
                       1,
                       Query,
                       &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_USER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        uid = ppObjects[0]->userInfo.uid;
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = BuildUserModInfo(
                    uid,
                    pTaskList,
                    &pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaModifyUser(
                    hLsaConnection,
                    pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout,
            "Successfully modified user %s\n",
            pszLoginId ? pszLoginId : pszUid);

cleanup:

    if (pUserModInfo) {
        LsaFreeUserModInfo(pUserModInfo);
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }

    if (hLsaConnection != (HANDLE)NULL) {
       LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
BuildUserModInfo(
    uid_t        uid,
    PDLINKEDLIST pTaskList,
    PLSA_USER_MOD_INFO* ppUserModInfo
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pListMember = pTaskList;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;

    dwError = LsaBuildUserModInfo(uid, &pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; pListMember; pListMember = pListMember->pNext)
    {
        PUSER_MOD_TASK pTask = (PUSER_MOD_TASK)pListMember->pItem;
        switch(pTask->taskType)
        {
            case UserModTask_EnableUser:
            {
                 dwError = LsaModifyUser_EnableUser(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_DisableUser:
            {
                 dwError = LsaModifyUser_DisableUser(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_UnlockUser:
            {
                 dwError = LsaModifyUser_Unlock(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_ChangePasswordAtNextLogon:
            {
                 dwError = LsaModifyUser_ChangePasswordAtNextLogon(pUserModInfo, TRUE);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_AddToGroups:
            {
                 dwError = LsaModifyUser_AddToGroups(pUserModInfo, pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_RemoveFromGroups:
            {
                 dwError = LsaModifyUser_RemoveFromGroups(pUserModInfo, pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case UserModTask_SetPasswordNeverExpires:
            {
                dwError = LsaModifyUser_SetPasswordNeverExpires(pUserModInfo, TRUE);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
            case UserModTask_SetPasswordMustExpire:
            {
                dwError = LsaModifyUser_SetPasswordMustExpire(pUserModInfo, TRUE);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
            case UserModTask_SetNtPasswordHash:
            {
                dwError = LsaModifyUser_SetNtPasswordHash(pUserModInfo,
                                                          pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case UserModTask_SetLmPasswordHash:
            {
                dwError = LsaModifyUser_SetLmPasswordHash(pUserModInfo,
                                                          pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case UserModTask_SetExpiryDate:
            {
                dwError = LsaModifyUser_SetExpiryDate(pUserModInfo,
                                                      pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case UserModTask_SetPrimaryGroup:
            {
                dwError = LsaModifyUser_SetPrimaryGroup(pUserModInfo,
                                                        pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case UserModTask_SetHomedir:
            {
                dwError = LsaModifyUser_SetHomedir(pUserModInfo,
                                                   pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case UserModTask_SetShell:
            {
                dwError = LsaModifyUser_SetShell(pUserModInfo,
                                                 pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case UserModTask_SetGecos:
            {
                dwError = LsaModifyUser_SetGecos(pUserModInfo,
                                                 pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
            case UserModTask_SetPassword:
            {
                dwError = LsaModifyUser_SetPassword(pUserModInfo,
                                                    pTask->pszData);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }
        }
    }

    *ppUserModInfo = pUserModInfo;

cleanup:

    return dwError;

error:

    *ppUserModInfo = NULL;

    if (pUserModInfo) {
       LsaFreeUserModInfo(pUserModInfo);
    }

    goto cleanup;
}

static
DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;

    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:

            dwError2 = LW_ERROR_LSA_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}

int
mod_user_main(
    int argc,
    char* argv[]
    )
{
    return LsaModUserMain(argc, argv);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
