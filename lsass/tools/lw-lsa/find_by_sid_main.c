/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        Tool to lookup objects in AD by SID
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define LSA_ENABLE_DEPRECATED

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszSID,
    PDWORD pdwInfoLevel
    );

static
VOID
ShowUsage();

static
DWORD
PrintUser(
    HANDLE hLsaConnection,
    PLSA_SECURITY_OBJECT pObject,
    DWORD  dwInfoLevel
    );

static
DWORD
PrintGroup(
    HANDLE hLsaConnection,
    PLSA_SECURITY_OBJECT pObject,
    DWORD  dwInfoLevel
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
find_by_sid_main(
    int argc,
    char* argv[]
    )
{
    DWORD   dwError = 0;
    PSTR    pszSID = NULL;
    DWORD   dwInfoLevel = 0;
    HANDLE  hLsaConnection = (HANDLE)NULL;
    size_t  dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    size_t  stSids = 1;
    LSA_QUERY_LIST QueryList;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = ParseArgs(argc, argv, &pszSID, &dwInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    QueryList.ppszStrings = (PCSTR*) &pszSID;

    dwError = LsaFindObjects(
        hLsaConnection,
        NULL,
        0,
        LSA_OBJECT_TYPE_UNDEFINED,
        LSA_QUERY_TYPE_BY_SID,
        (DWORD) stSids,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);
    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (ppObjects[0]->type)
    {
        case AccountType_Group:

            dwError = PrintGroup(
                            hLsaConnection,
                            ppObjects[0],
                            dwInfoLevel);

            break;

        case AccountType_User:

            dwError = PrintUser(
                            hLsaConnection,
                            ppObjects[0],
                            dwInfoLevel);

            break;

        case AccountType_NotFound:

            dwError = LW_ERROR_NO_SUCH_OBJECT;

            break;

        default:

            dwError = LW_ERROR_INTERNAL;

            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszSID);

    return (dwError);

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
                        "Failed to locate SID.  Error code %u (%s).\n%s\n",
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
                "Failed to locate SID.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszSID,
    PDWORD pdwInfoLevel
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszSID = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else if (!strcmp(pszArg, "--level")) {
                    parseMode = PARSE_MODE_LEVEL;
                }
                else
                {
                    dwError = LwAllocateString(pszArg, &pszSID);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;

            case PARSE_MODE_LEVEL:

                dwInfoLevel = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;
        }

    } while (iArg < argc);

    if (LW_IS_NULL_OR_EMPTY_STR(pszSID)) {
       fprintf(stderr, "Please specify a SID to query for.\n");
       ShowUsage();
       exit(1);
    }

    *ppszSID = pszSID;
    *pdwInfoLevel = dwInfoLevel;

cleanup:

    return dwError;

error:

    *ppszSID = NULL;
    *pdwInfoLevel = 0;

    LW_SAFE_FREE_STRING(pszSID);

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: find-by-sid {--level [0, 1, 2]} <SID>\n"
            "\n"
            "Note: level 2 is only valid for user sids\n");
}

DWORD
PrintUser(
    HANDLE hLsaConnection,
    PLSA_SECURITY_OBJECT pObject,
    DWORD  dwInfoLevel
    )
{
    DWORD dwError = 0;
    struct timeval current_tv;
    UINT64 u64current_NTtime = 0;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }
    u64current_NTtime = (current_tv.tv_sec + 11644473600LL) * 10000000LL;

    if (dwInfoLevel > 2)
    {
        dwError = LW_ERROR_INVALID_USER_INFO_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (pObject->type != LSA_OBJECT_TYPE_USER)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!pObject->enabled)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    fprintf(stdout, "User info (Level-%d):\n", dwInfoLevel);

    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
            LW_PRINTF_STRING(pObject->userInfo.pszUnixName));
    fprintf(stdout, "SID:      %s\n",
            LW_PRINTF_STRING(pObject->pszObjectSid));
    fprintf(stdout, "Uid:      %lu\n",
            (unsigned long)pObject->userInfo.uid);

    if (dwInfoLevel >= 1)
    {
        fprintf(stdout, "UPN:           %s\n",
                LW_PRINTF_STRING(pObject->userInfo.pszUPN));
        fprintf(stdout, "Generated UPN: %s\n",
                pObject->userInfo.bIsGeneratedUPN ? "YES" : "NO");
    }

    fprintf(stdout, "Gid:      %lu\n",
            (unsigned long)pObject->userInfo.gid);
    fprintf(stdout, "Gecos:    %s\n",
            LW_PRINTF_STRING(pObject->userInfo.pszGecos));
    fprintf(stdout, "Shell:    %s\n",
            LW_PRINTF_STRING(pObject->userInfo.pszShell));
    fprintf(stdout, "Home dir: %s\n",
            LW_PRINTF_STRING(pObject->userInfo.pszHomedir));

    if (dwInfoLevel >= 1)
    {
        fprintf(stdout, "LMHash length: %u\n", pObject->userInfo.dwLmHashLen);
        fprintf(stdout, "NTHash length: %u\n", pObject->userInfo.dwNtHashLen);
        fprintf(stdout, "Local User:    %s\n", pObject->bIsLocal ? "YES" : "NO");
    }

    if (dwInfoLevel >= 2)
    {
        fprintf(stdout, "Account disabled (or locked): %s\n",
                pObject->userInfo.bAccountDisabled ? "TRUE" : "FALSE");
        fprintf(stdout, "Account Expired:              %s\n",
                pObject->userInfo.bAccountExpired ? "TRUE" : "FALSE");
        fprintf(stdout, "Password never expires:       %s\n",
                pObject->userInfo.bPasswordNeverExpires ? "TRUE" : "FALSE");
        fprintf(stdout, "Password Expired:             %s\n",
                pObject->userInfo.bPasswordExpired ? "TRUE" : "FALSE");
        fprintf(stdout, "Prompt for password change:   %s\n",
                pObject->userInfo.bPromptPasswordChange ? "YES" : "NO");
        fprintf(stdout, "User can change password:     %s\n",
                pObject->userInfo.bUserCanChangePassword ? "YES" : "NO");
        if (pObject->userInfo.bIsAccountInfoKnown)
        {
            fprintf(stdout, "Days till password expires:   %llu\n",
                    (unsigned long long)(pObject->userInfo.qwPwdExpires -
                        u64current_NTtime) /
                    (10000000LL * 24*60*60));
        }
        else
        {
            fprintf(stdout, "Days till password expires:   unknown\n");
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
PrintGroup(
    HANDLE hLsaConnection,
    PLSA_SECURITY_OBJECT pObject,
    DWORD  dwInfoLevel
    )
{
    DWORD dwError = 0;
    DWORD dwMemberCount = 0;
    PLSA_SECURITY_OBJECT* ppMembers = NULL;
    DWORD iMember = 0;

    if (dwInfoLevel > 1)
    {
        dwError = LW_ERROR_INVALID_GROUP_INFO_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (pObject->type != LSA_OBJECT_TYPE_GROUP)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!pObject->enabled)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    fprintf(stdout, "Group info (Level-%d):\n", dwInfoLevel);
    if (dwInfoLevel > 1)
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
            LW_PRINTF_STRING(pObject->groupInfo.pszUnixName));
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pObject->groupInfo.gid);
    fprintf(stdout, "SID:      %s\n",
            LW_PRINTF_STRING(pObject->pszObjectSid));

    if (dwInfoLevel >= 1)
    {
        fprintf(stdout, "Members:\n");

        dwError = LsaQueryExpandedGroupMembers(
            hLsaConnection,
            NULL,
            0,
            LSA_OBJECT_TYPE_USER,
            pObject->pszObjectSid,
            &dwMemberCount,
            &ppMembers);
        BAIL_ON_LSA_ERROR(dwError);

        for (iMember = 0; iMember < dwMemberCount; iMember++)
        {
            if (ppMembers[iMember]->type == LSA_OBJECT_TYPE_USER &&
                    ppMembers[iMember]->enabled)
            {
              if (iMember)
              {
                 fprintf(stdout, "\n%s", LW_PRINTF_STRING(ppMembers[iMember]->userInfo.pszUnixName));
              }
              else
              {
                 fprintf(stdout, "%s", LW_PRINTF_STRING(ppMembers[iMember]->userInfo.pszUnixName));
              }
          }
        }
        fprintf(stdout, "\n");
    }

cleanup:
    if (ppMembers)
    {
        LsaUtilFreeSecurityObjectList(dwMemberCount, ppMembers);
    }
    return dwError;

error:
    goto cleanup;
}

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
