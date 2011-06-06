/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        account_rights_main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Driver for program to add/remove/enumerate account rights of
 *        an existing user or group
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "account_rights_includes.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
LsaAccountRightsMain(
    int argc,
    char* argv[]
    );

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszLoginId,
    PDLINKEDLIST* ppTaskList
    );

static
DWORD
GetStringListFromString(
    PSTR pszStr,
    CHAR Separator,
    PSTR **pppszList,
    PDWORD pNumElements
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
    PACCOUNT_RIGHTS_TASK pTask
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
ProcessAccountRights(
    PCSTR pszId,
    PDLINKEDLIST pTaskList
    );

int
account_rights_main(
    int argc,
    char* argv[]
    )
{
    return LsaAccountRightsMain(argc, argv);
}

static
DWORD
MapErrorCode(
    DWORD dwError
    );

static
DWORD
LsaAccountRightsMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pTaskList = NULL;
    PSTR pszId = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

    dwError = ParseArgs(argc, argv, &pszId, &pTaskList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ProcessAccountRights(
                     pszId,
                     pTaskList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pTaskList)
    {
        LsaDLinkedListForEach(pTaskList, &FreeTasksInList, NULL);
        LsaDLinkedListFree(pTaskList);
    }

    LW_SAFE_FREE_STRING(pszId);

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
                        "Failed to change account rights.  Error code %u (%s).\n%s\n",
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
                "Failed to change account rights.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int   argc,
    char* argv[],
    PSTR* ppszId,
    PDLINKEDLIST* ppTaskList
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_ADD_ACCOUNT_RIGHTS,
        PARSE_MODE_REMOVE_ACCOUNT_RIGHTS,
        PARSE_MODE_ENUM_ACCOUNT_RIGHTS,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PDLINKEDLIST pTaskList = NULL;
    PACCOUNT_RIGHTS_TASK pTask = NULL;
    PSTR pszId = NULL;
    PACCOUNT_RIGHTS_DATA pData = NULL;
    PSTR* ppszAccountRights = NULL;
    DWORD i = 0;
    BOOLEAN Enumerate = TRUE;
    BOOLEAN removeAll = FALSE;

    do {
        pArg = argv[iArg++];
        if (LW_IS_NULL_OR_EMPTY_STR(pArg))
        {
            ShowUsage(GetProgramName(argv[0]));
            exit(0);
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
                else if (!strcmp(pArg, "--add"))
                {
                    parseMode = PARSE_MODE_ADD_ACCOUNT_RIGHTS;
                }
                else if (!strcmp(pArg, "--remove"))
                {
                    parseMode = PARSE_MODE_REMOVE_ACCOUNT_RIGHTS;
                }
                else if (!strcmp(pArg, "--remove-all"))
                {
                    removeAll = TRUE;
                    parseMode = PARSE_MODE_REMOVE_ACCOUNT_RIGHTS;
                }
                else if (!strcmp(pArg, "--enum"))
                {
                    parseMode = PARSE_MODE_ENUM_ACCOUNT_RIGHTS;
                }
                else
                {
                    dwError = LwAllocateString(pArg, &pszId);
                    BAIL_ON_LSA_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;
            }

            case PARSE_MODE_ADD_ACCOUNT_RIGHTS:
            {
                dwError = LwAllocateMemory(
                                sizeof(ACCOUNT_RIGHTS_TASK),
                                OUT_PPVOID(&pTask));
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LwAllocateMemory(
                                sizeof(ACCOUNT_RIGHTS_DATA),
                                OUT_PPVOID(&pData));
                BAIL_ON_LSA_ERROR(dwError);

                dwError = GetStringListFromString(pArg,
                                                  SEPARATOR_CHAR,
                                                  &ppszAccountRights,
                                                  &pData->NumAccountRights);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LwAllocateMemory(
                                sizeof(PWSTR) * pData->NumAccountRights,
                                OUT_PPVOID(&pData->ppwszAccountRights));

                for (i = 0; i < pData->NumAccountRights; i++)
                {
                    dwError = LwMbsToWc16s(
                                ppszAccountRights[i],
                                &pData->ppwszAccountRights[i]);
                    BAIL_ON_LSA_ERROR(dwError);
                }

                pTask->taskType = AccountRightsTask_Add;
                pTask->pData    = pData;

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                Enumerate = FALSE;
                parseMode = PARSE_MODE_OPEN;
                pData     = NULL;

                break;
            }

            case PARSE_MODE_REMOVE_ACCOUNT_RIGHTS:
            {
                dwError = LwAllocateMemory(
                                sizeof(ACCOUNT_RIGHTS_TASK),
                                OUT_PPVOID(&pTask));
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LwAllocateMemory(
                                sizeof(ACCOUNT_RIGHTS_DATA),
                                OUT_PPVOID(&pData));
                BAIL_ON_LSA_ERROR(dwError);

                if (removeAll)
                {
                    pData->RemoveAll = TRUE;

                    pTask->taskType = AccountRightsTask_Remove;
                    pTask->pData    = pData;

                    dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LwAllocateString(pArg, &pszId);
                    BAIL_ON_LSA_ERROR(dwError);

                    Enumerate = FALSE;
                    parseMode = PARSE_MODE_OPEN;
                    break;
                }

                dwError = GetStringListFromString(pArg,
                                                  SEPARATOR_CHAR,
                                                  &ppszAccountRights,
                                                  &pData->NumAccountRights);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LwAllocateMemory(
                                sizeof(PWSTR) * pData->NumAccountRights,
                                OUT_PPVOID(&pData->ppwszAccountRights));

                for (i = 0; i < pData->NumAccountRights; i++)
                {
                    dwError = LwMbsToWc16s(
                                ppszAccountRights[i],
                                &pData->ppwszAccountRights[i]);
                    BAIL_ON_LSA_ERROR(dwError);
                }

                pTask->taskType = AccountRightsTask_Remove;
                pTask->pData    = pData;

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                Enumerate = FALSE;
                parseMode = PARSE_MODE_OPEN;
                pData     = NULL;

                break;
            }

            case PARSE_MODE_ENUM_ACCOUNT_RIGHTS:
            {
                dwError = LwAllocateMemory(
                                sizeof(ACCOUNT_RIGHTS_TASK),
                                OUT_PPVOID(&pTask));
                BAIL_ON_LSA_ERROR(dwError);

                pTask->taskType = AccountRightsTask_Enumerate;
                pTask->pData    = NULL;

                dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LwAllocateString(pArg, &pszId);
                BAIL_ON_LSA_ERROR(dwError);

                Enumerate = FALSE;
                parseMode = PARSE_MODE_OPEN;
                pData     = NULL;

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

    if (Enumerate)
    {
        dwError = LwAllocateMemory(
                       sizeof(ACCOUNT_RIGHTS_TASK),
                       OUT_PPVOID(&pTask));
        BAIL_ON_LSA_ERROR(dwError);

        pTask->taskType = AccountRightsTask_Enumerate;
        pTask->pData    = NULL;

        dwError = LsaDLinkedListAppend(&pTaskList, pTask);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszId     = pszId;
    *ppTaskList = pTaskList;

cleanup:
    return dwError;

error:
    *ppTaskList = NULL;

    if (pTaskList)
    {
        LsaDLinkedListForEach(pTaskList, FreeTasksInList, NULL);
        LsaDLinkedListFree(pTaskList);
    }

    if (pTask)
    {
        FreeTask(pTask);
    }

    LW_SAFE_FREE_STRING(pszId);

    goto cleanup;
}

static
DWORD
GetStringListFromString(
    PSTR pszStr,
    CHAR Separator,
    PSTR **pppszList,
    PDWORD pNumElements
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR *ppszList = NULL;
    PSTR pszString = NULL;
    DWORD i = 0;
    size_t stringLen = 0;
    DWORD numElements = 1;
    PSTR pszElement = NULL;

    dwError = LwAllocateString(pszStr, &pszString);
    BAIL_ON_LSA_ERROR(dwError);

    stringLen = strlen(pszString);
    for (i = 0; i < stringLen; i++)
    {
        if (pszString[i] == Separator)
        {
            numElements++;
        }
    }

    dwError = LwAllocateMemory(sizeof(PSTR) * numElements,
                               OUT_PPVOID(&ppszList));
    BAIL_ON_LSA_ERROR(dwError);

    pszElement  = pszString;
    numElements = 0;

    for (i = 0; i < stringLen; i++)
    {
        if (pszString[i] != Separator)
        {
            continue;
        }

        pszString[i] = '\0';

        dwError = LwAllocateString(pszElement,
                                   &(ppszList[numElements++]));
        BAIL_ON_LSA_ERROR(dwError);

        pszElement = &(pszString[i + 1]);
    }

    // Copy the last element (it doesn't end with the separator)
    dwError = LwAllocateString(pszElement,
                               &(ppszList[numElements++]));
    BAIL_ON_LSA_ERROR(dwError);

    *pppszList    = ppszList;
    *pNumElements = numElements;

error:
    if (dwError)
    {
        for (i = 0; ppszList && i < numElements; i++)
        {
            LW_SAFE_FREE_MEMORY(ppszList[i]);
        }
        LW_SAFE_FREE_MEMORY(ppszList);

        *pppszList    = NULL;
        *pNumElements = 0;
    }

    LW_SAFE_FREE_MEMORY(pszString);

    return dwError;
}

VOID
FreeTasksInList(
    PVOID pTask,
    PVOID pGroupData
    )
{
    if (pTask)
    {
        FreeTask((PACCOUNT_RIGHTS_TASK)pTask);
    }
}

VOID
FreeTask(
    PACCOUNT_RIGHTS_TASK pTask
    )
{
    PACCOUNT_RIGHTS_DATA pData = pTask->pData;
    DWORD i = 0;

    if (pData)
    {
        for (i = 0; i < pData->NumAccountRights; i++)
        {
            LW_SAFE_FREE_MEMORY(pData->ppwszAccountRights[i]);
        }
        LW_SAFE_FREE_MEMORY(pData->ppwszAccountRights);
    }

    LW_SAFE_FREE_MEMORY(pTask);
}

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

VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s {modification options} ( account name | sid )\n\n", pszProgramName);

    fprintf(stdout, "\nModification options (default: --enum):\n");
    fprintf(stdout, "{ --help }\n");
    fprintf(stdout, "{ --add account_right[,account_right] }\n");
    fprintf(stdout, "{ --remove account_right[,account_right] }\n");
    fprintf(stdout, "{ --enum }\n");
}

static
DWORD
ProcessAccountRights(
    PCSTR pszId,
    PDLINKEDLIST pTaskList
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PSID pSid = NULL;
    PSID pAccountSid = NULL;
    LSA_FIND_FLAGS findFlags = 0;
    LSA_QUERY_LIST Query = {0};
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PSTR pszAccountSid = NULL;
    PDLINKEDLIST pListNode = NULL;
    DWORD i = 0;
    PWSTR *ppwszAccountRights = NULL;
    DWORD numAccountRights = 0;
    PSTR pszAccountRight = NULL;

    BAIL_ON_INVALID_STRING(pszId);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromCString(&pSid, pszId);
    if (ntStatus == STATUS_SUCCESS)
    {
        dwError = LwAllocateString(
                       pszId,
                       &pszAccountSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        Query.ppszStrings = &pszId;

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

        if (ppObjects[0])
        {
            dwError = LwAllocateString(
                           ppObjects[0]->pszObjectSid,
                           &pszAccountSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
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

            if (ppObjects[0])
            {
                dwError = LwAllocateString(
                               ppObjects[0]->pszObjectSid,
                               &pszAccountSid);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = ERROR_INVALID_ACCOUNT_NAME;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    if (!pszAccountSid)
    {
        fprintf(stderr, "Invalid account name %s\n", pszId);

        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    ntStatus = RtlAllocateSidFromCString(&pAccountSid, pszAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    for (pListNode = pTaskList; pListNode; pListNode = pListNode->pNext)
    {
        PACCOUNT_RIGHTS_TASK pTask = (PACCOUNT_RIGHTS_TASK)pListNode->pItem;

        switch (pTask->taskType)
        {
        case AccountRightsTask_Add:
        {
            PACCOUNT_RIGHTS_DATA pData = pTask->pData;

            dwError = LsaPrivsAddAccountRights(
                               hLsaConnection,
                               pAccountSid,
                               pData->ppwszAccountRights,
                               pData->NumAccountRights);
            BAIL_ON_LSA_ERROR(dwError);

            fprintf(stdout, "Account rights successfully added to %s\n", pszId);
            break;
        }

        case AccountRightsTask_Remove:
        {
            PACCOUNT_RIGHTS_DATA pData = pTask->pData;

            dwError = LsaPrivsRemoveAccountRights(
                               hLsaConnection,
                               pAccountSid,
                               pData->RemoveAll,
                               pData->ppwszAccountRights,
                               pData->NumAccountRights);
            BAIL_ON_LSA_ERROR(dwError);

            fprintf(stdout, "Account rights successfully removed from %s\n", pszId);
            break;
        }

        case AccountRightsTask_Enumerate:
        {
            if (pSid)
            {
                fprintf(stdout, "Account rights assigned to %s:\n", pszId);
            }
            else
            {
                fprintf(stdout,
                        "Account rights assigned to %s (%s):\n",
                        pszId,
                        pszAccountSid);
            }

            dwError = LsaPrivsEnumAccountRights(
                               hLsaConnection,
                               pAccountSid,
                               &ppwszAccountRights,
                               &numAccountRights);

            for (i = 0; i < numAccountRights; i++)
            {
                dwError = LwWc16sToMbs(ppwszAccountRights[i],
                                       &pszAccountRight);
                BAIL_ON_LSA_ERROR(dwError);

                fprintf(stdout, "%s\n", pszAccountRight);

                LW_SAFE_FREE_MEMORY(pszAccountRight);
                pszAccountRight = NULL;
            }
            break;
        }
        }
    }

cleanup:
    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(1, ppObjects);
    }

    RTL_FREE(&pSid);
    RTL_FREE(&pAccountSid);
    LW_SAFE_FREE_MEMORY(pszAccountSid);
    LW_SAFE_FREE_MEMORY(pszAccountRight);

    for (i = 0; ppwszAccountRights && i < numAccountRights; i++)
    {
        LW_SAFE_FREE_MEMORY(ppwszAccountRights[i]);
    }
    LW_SAFE_FREE_MEMORY(ppwszAccountRights);

    return dwError;

error:
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
