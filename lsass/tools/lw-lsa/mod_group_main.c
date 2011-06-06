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
 *
 *        Driver for program to modify an existing group
 *
 * Authors:
 *          Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "mod_group_includes.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
LsaModGroupMain(
    int argc,
    char* argv[]
    );

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszGid,
    PSTR* ppszGroupName,
    PDLINKEDLIST* ppTaskList
    );

static
BOOLEAN
ValidateArgs(
    PCSTR        pszGid,
    PCSTR        pszGroupName,
    PDLINKEDLIST pTaskList
    );

static
VOID
FreeTasksInList(
    PVOID pListMember,
    PVOID pGroupData
    );

static
VOID
FreeTask(
    PGROUP_MOD_TASK pTask
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
DWORD
ModifyGroup(
    PCSTR pszGid,
    PCSTR pszGroupName,
    PDLINKEDLIST pTaskList
    );

static
DWORD
ResolveNames(
    HANDLE hLsaConnection,
    PDLINKEDLIST pTaskList
    );

static
DWORD
BuildGroupModInfo(
    gid_t        gid,
    PDLINKEDLIST pTaskList,
    PLSA_GROUP_MOD_INFO* ppGroupModInfo
    );

int
mod_group_main(
    int argc,
    char* argv[]
    )
{
    return LsaModGroupMain(argc, argv);
}

static
DWORD
MapErrorCode(
    DWORD dwError
    );

static
DWORD
LsaModGroupMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pTaskList = NULL;
    PSTR pszGid = NULL;
    PSTR pszGroupName = NULL;
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
                     &pszGid,
                     &pszGroupName,
                     &pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

     dwError = ModifyGroup(
                     pszGid,
                     pszGroupName,
                     pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

 cleanup:

     if (pTaskList) {
         LsaDLinkedListForEach(pTaskList, &FreeTasksInList, NULL);
         LsaDLinkedListFree(pTaskList);
     }

     LW_SAFE_FREE_STRING(pszGid);
     LW_SAFE_FREE_STRING(pszGroupName);

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
                         "Failed to modify group.  Error code %u (%s).\n%s\n",
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
                 "Failed to modify group.  Error code %u (%s).\n",
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
    PSTR* ppszGid,
    PSTR* ppszGroupName,
    PDLINKEDLIST* ppTaskList
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_ADD_TO_GROUPS,
        PARSE_MODE_REMOVE_FROM_GROUPS,
        PARSE_MODE_GID,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PDLINKEDLIST pTaskList = NULL;
    PGROUP_MOD_TASK pTask = NULL;
    PSTR pszGid = NULL;
    PSTR pszGroupName = NULL;
    PSTR pszAddMembers = NULL;
    PSTR pszRemoveMembers = NULL;

    do {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
          break;
        }

        switch(parseMode) {
            case PARSE_MODE_OPEN:
            {
                if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0))
                {
                  ShowUsage(GetProgramName(argv[0]));
                  exit(0);
                }
                else if (!strcmp(pArg, "--add-members"))
                {
                    parseMode = PARSE_MODE_ADD_TO_GROUPS;
                }
                else if (!strcmp(pArg, "--remove-members"))
                {
                    parseMode = PARSE_MODE_REMOVE_FROM_GROUPS;
                }
                else if (!strcmp(pArg, "--gid"))
                {
                    parseMode = PARSE_MODE_GID;
                }
                else
                {
                    dwError = LwAllocateString(pArg, &pszGroupName);
                    BAIL_ON_LSA_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;
            }

            case PARSE_MODE_REMOVE_FROM_GROUPS:
            {
                 dwError = LwAllocateMemory(sizeof(GROUP_MOD_TASK), (PVOID*)&pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 pTask->taskType = GroupModTask_RemoveMembers;

                 dwError = LwAllocateString(pArg, &pszRemoveMembers);
                 BAIL_ON_LSA_ERROR(dwError);

                 pTask->pData = pszRemoveMembers;

                 dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 parseMode = PARSE_MODE_OPEN;

                 break;
            }

            case PARSE_MODE_ADD_TO_GROUPS:
            {
                  dwError = LwAllocateMemory(sizeof(GROUP_MOD_TASK), (PVOID*)&pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  pTask->taskType = GroupModTask_AddMembers;

                  dwError = LwAllocateString(pArg, &pszAddMembers);
                  BAIL_ON_LSA_ERROR(dwError);

                  pTask->pData = pszAddMembers;

                  dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  parseMode = PARSE_MODE_OPEN;

                 break;
            }

            case PARSE_MODE_GID:
            {
                if (!IsUnsignedInteger(pArg))
                {
                    fprintf(stderr, "Please specifiy a gid that is an unsigned integer\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }

                dwError = LwAllocateString(pArg, &pszGid);
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

    if (!ValidateArgs(pszGid, pszGroupName, pTaskList)) {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    *ppszGid = pszGid;
    *ppszGroupName = pszGroupName;
    *ppTaskList = pTaskList;

cleanup:

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

    LW_SAFE_FREE_STRING(pszGid);
    LW_SAFE_FREE_STRING(pszGroupName);

    goto cleanup;
}

static
BOOLEAN
ValidateArgs(
    PCSTR        pszGid,
    PCSTR        pszGroupName,
    PDLINKEDLIST pTaskList
    )
{
    BOOLEAN bValid = FALSE;

    PDLINKEDLIST pListMember = NULL;

    for (pListMember = pTaskList; pListMember; pListMember = pListMember->pNext)
    {
        PGROUP_MOD_TASK pTask = (PGROUP_MOD_TASK)pListMember->pItem;
        if (pTask) {
           switch(pTask->taskType)
           {
               default:
                   break;
           }
        }
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszGid) &&
        LW_IS_NULL_OR_EMPTY_STR(pszGroupName))
    {
        fprintf(stderr, "Error: A valid group id or group name must be specified.\n");
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
    PVOID pGroupData
    )
{
    if (pTask) {
       FreeTask((PGROUP_MOD_TASK)pTask);
    }
}

static
VOID
FreeTask(
    PGROUP_MOD_TASK pTask
    )
{
    PSTR pszMember = pTask->pData;

    LW_SAFE_FREE_STRING(pszMember);
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
    fprintf(stdout, "Usage: %s {modification options} ( <group name> | --gid <gid> )\n\n", pszProgramName);

    fprintf(stdout, "\nModification options:\n");
    fprintf(stdout, "{ --help }\n");
    fprintf(stdout, "{ --add-members nt4-style-name }\n");
    fprintf(stdout, "{ --remove-members nt4-style-name }\n");
}

static
DWORD
ModifyGroup(
    PCSTR pszGid,
    PCSTR pszGroupName,
    PDLINKEDLIST pTaskList
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MOD_INFO pGroupModInfo = NULL;
    gid_t gid = 0;
    int   nRead = 0;
    LSA_FIND_FLAGS findFlags = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_QUERY_LIST Query = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszGid))
    {
        nRead = sscanf(pszGid, "%u", (unsigned int*)&gid);
        if ((nRead == EOF) || (nRead == 0)) {
            fprintf(stderr, "An invalid group id [%s] was specified.", pszGid);

            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else if (!LW_IS_NULL_OR_EMPTY_STR(pszGroupName))
    {
        Query.ppszStrings = &pszGroupName;

        dwError = LsaFindObjects(
                       hLsaConnection,
                       NULL,
                       findFlags,
                       LSA_OBJECT_TYPE_GROUP,
                       LSA_QUERY_TYPE_BY_NAME,
                       1,
                       Query,
                       &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        if (ppObjects[0] == NULL)
        {
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }

        gid = ppObjects[0]->groupInfo.gid;
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ResolveNames(
                    hLsaConnection,
                    pTaskList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = BuildGroupModInfo(
                    gid,
                    pTaskList,
                    &pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaModifyGroup(
                    hLsaConnection,
                    pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout,
            "Successfully modified group %s\n",
            pszGroupName ? pszGroupName : pszGid);

cleanup:

    if (pGroupModInfo) {
        LsaFreeGroupModInfo(pGroupModInfo);
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
ResolveNames(
    HANDLE hLsaConnection,
    PDLINKEDLIST pTaskList
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDLINKEDLIST pListMember = pTaskList;
    LSA_QUERY_LIST Query = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PGROUP_MOD_TASK pTask = NULL;
    PCSTR pszName = NULL;
    PSTR pszSID = NULL;
    PSID pSid = NULL;

    for (; pListMember; pListMember = pListMember->pNext)
    {
        pTask = (PGROUP_MOD_TASK)pListMember->pItem;
        if (pTask->taskType == GroupModTask_AddMembers ||
            pTask->taskType == GroupModTask_RemoveMembers)
        {
            pszName = (PCSTR)pTask->pData;

            ntStatus = RtlAllocateSidFromCString(&pSid, pszName);
            if (ntStatus == STATUS_SUCCESS)
            {
                dwError = LwAllocateString(pszName, &pszSID);
                BAIL_ON_LSA_ERROR(dwError);

            }
            else
            {
                Query.ppszStrings = &pszName;

                dwError = LsaFindObjects(
                                  hLsaConnection,
                                  NULL,
                                  0,
                                  LSA_OBJECT_TYPE_GROUP,
                                  LSA_QUERY_TYPE_BY_NAME,
                                  1,
                                  Query,
                                  &ppObjects);
                BAIL_ON_LSA_ERROR(dwError);
                                         
                if (ppObjects[0])
                {
                    dwError = LwAllocateString(
                                     ppObjects[0]->pszObjectSid,
                                     &pszSID);
                    BAIL_ON_LSA_ERROR(dwError);

                }
                else
                {
                    dwError = LsaFindObjects(
                                      hLsaConnection,
                                      NULL,
                                      0,
                                      LSA_OBJECT_TYPE_USER,
                                      LSA_QUERY_TYPE_BY_NAME,
                                      1,
                                      Query,
                                      &ppObjects);
                    BAIL_ON_LSA_ERROR(dwError);

                    if (ppObjects[0])
                    {
                        dwError = LwAllocateString(
                                         ppObjects[0]->pszObjectSid,
                                         &pszSID);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                    else
                    {
                        dwError = ERROR_INVALID_ACCOUNT_NAME;
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                }
            }

            LW_SAFE_FREE_STRING(pTask->pData);
            pTask->pData = NULL;

            pTask->pData = pszSID;

            if (ppObjects)
            {
                LsaFreeSecurityObjectList(1, ppObjects);
                ppObjects = NULL;
            }

            RTL_FREE(&pSid);
        }
    }

cleanup:
    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
        ppObjects = NULL;
    }

    RTL_FREE(&pSid);

    return dwError;

error:
    LW_SAFE_FREE_STRING(pTask->pData);

    goto cleanup;
}

static
DWORD
BuildGroupModInfo(
    gid_t        gid,
    PDLINKEDLIST pTaskList,
    PLSA_GROUP_MOD_INFO* ppGroupModInfo
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pListMember = pTaskList;
    PLSA_GROUP_MOD_INFO pGroupModInfo = NULL;

    dwError = LsaBuildGroupModInfo(gid, &pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; pListMember; pListMember = pListMember->pNext)
    {
        PGROUP_MOD_TASK pTask = (PGROUP_MOD_TASK)pListMember->pItem;
        switch(pTask->taskType)
        {
            case GroupModTask_AddMembers:
            {
                 dwError = LsaModifyGroup_AddMembers(pGroupModInfo, pTask->pData);
                 BAIL_ON_LSA_ERROR(dwError);
                 break;
            }
            case GroupModTask_RemoveMembers:
            {
                 dwError = LsaModifyGroup_RemoveMembers(pGroupModInfo, pTask->pData);
                 BAIL_ON_LSA_ERROR(dwError);
                 break;
            }
        }
    }

    *ppGroupModInfo = pGroupModInfo;

cleanup:

    return dwError;

error:

    *ppGroupModInfo = NULL;

    if (pGroupModInfo) {
       LsaFreeGroupModInfo(pGroupModInfo);
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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
