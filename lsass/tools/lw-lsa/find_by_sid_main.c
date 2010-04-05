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

#define _POSIX_PTHREAD_SEMANTICS 1
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
LookupUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszDomain,
    PCSTR  pszSamAccountName,
    DWORD  dwInfoLevel
    );

static
VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo
    );

static
VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo
    );

static
VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo
    );

static
DWORD
LookupGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszDomain,
    PCSTR  pszSamAccountName,
    DWORD  dwInfoLevel
    );

static
VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    );

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
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
    PLSA_SID_INFO pSidInfoList = NULL;
    CHAR  chDomainSeparator = 0;

    dwError = ParseArgs(argc, argv, &pszSID, &dwInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetNamesBySidList(
                    hLsaConnection,
                    stSids,
                    &pszSID,
                    &pSidInfoList,
                    &chDomainSeparator);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSetDomainSeparator(
                chDomainSeparator);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pSidInfoList[0].accountType)
    {
        case AccountType_Group:

            dwError = LookupGroupByName(
                            hLsaConnection,
                            pSidInfoList[0].pszDomainName,
                            pSidInfoList[0].pszSamAccountName,
                            dwInfoLevel);

            break;

        case AccountType_User:

            dwError = LookupUserByName(
                            hLsaConnection,
                            pSidInfoList[0].pszDomainName,
                            pSidInfoList[0].pszSamAccountName,
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

    if (pSidInfoList)
    {
        LsaFreeSIDInfoList(pSidInfoList, stSids);
    }

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
    printf("Usage: lw-find-by-sid {--level [0, 1, 2]} <SID>\n"
            "\n"
            "Note: level 2 is only valid for user sids\n");
}

DWORD
LookupUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszDomain,
    PCSTR  pszSamAccountName,
    DWORD  dwInfoLevel
    )
{
    DWORD dwError = 0;
    PSTR  pszUsername = NULL;
    PVOID pUserInfo = NULL;

    dwError = LwAllocateStringPrintf(
                    &pszUsername,
                    "%s%c%s",
                    pszDomain,
                    LsaGetDomainSeparator(),
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszUsername,
                    dwInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch(dwInfoLevel)
    {
        case 0:

            PrintUserInfo_0((PLSA_USER_INFO_0)pUserInfo);
            break;

        case 1:

            PrintUserInfo_1((PLSA_USER_INFO_1)pUserInfo);
            break;

        case 2:

            PrintUserInfo_2((PLSA_USER_INFO_2)pUserInfo);
            break;

        default:

            dwError = LW_ERROR_INVALID_USER_INFO_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);

            break;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszUsername);

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    fprintf(stdout, "User info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "SID:      %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszSid) ? "<null>" : pUserInfo->pszSid);
    fprintf(stdout, "Uid:      %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:    %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:    %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir: %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
}

VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    fprintf(stdout, "User info (Level-1):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:          %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "SID:           %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszSid) ? "<null>" : pUserInfo->pszSid);
    fprintf(stdout, "UPN:           %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN) ? "<null>" : pUserInfo->pszUPN);
    fprintf(stdout, "Generated UPN: %s\n", pUserInfo->bIsGeneratedUPN ? "YES" : "NO");
    fprintf(stdout, "Uid:           %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:           %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:         %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:         %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir:      %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "LMHash length: %d\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length: %d\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:    %s\n", pUserInfo->bIsLocalUser ? "YES" : "NO");
}

VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo
    )
{
    fprintf(stdout, "User info (Level-2):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:                       %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "SID:                        %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszSid) ? "<null>" : pUserInfo->pszSid);
    fprintf(stdout, "UPN:                        %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN) ? "<null>" : pUserInfo->pszUPN);
    fprintf(stdout, "Generated UPN:              %s\n", pUserInfo->bIsGeneratedUPN ? "YES" : "NO");
    fprintf(stdout, "Uid:                        %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:                        %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:                      %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:                      %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir:                   %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "LMHash length:              %d\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length:              %d\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:                 %s\n", pUserInfo->bIsLocalUser ? "YES" : "NO");
    fprintf(stdout, "Account disabled:           %s\n",
            pUserInfo->bAccountDisabled ? "TRUE" : "FALSE");
    fprintf(stdout, "Account Expired:            %s\n",
            pUserInfo->bAccountExpired ? "TRUE" : "FALSE");
    fprintf(stdout, "Account Locked:             %s\n",
            pUserInfo->bAccountLocked ? "TRUE" : "FALSE");
    fprintf(stdout, "Password never expires:     %s\n",
            pUserInfo->bPasswordNeverExpires ? "TRUE" : "FALSE");
    fprintf(stdout, "Password Expired:           %s\n",
            pUserInfo->bPasswordExpired ? "TRUE" : "FALSE");
    fprintf(stdout, "Prompt for password change: %s\n",
            pUserInfo->bPromptPasswordChange ? "YES" : "NO");
    fprintf(stdout, "User can change password:   %s\n",
            pUserInfo->bUserCanChangePassword ? "YES" : "NO");
    fprintf(stdout, "Days till password expires: %d\n",
            pUserInfo->dwDaysToPasswordExpiry);
}

DWORD
LookupGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszDomain,
    PCSTR  pszSamAccountName,
    DWORD  dwInfoLevel
    )
{
    DWORD dwError = 0;
    PSTR  pszGroupname = NULL;
    PVOID pGroupInfo = NULL;

    dwError = LwAllocateStringPrintf(
                    &pszGroupname,
                    "%s%c%s",
                    pszDomain,
                    LsaGetDomainSeparator(),
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszGroupname,
                    0,
                    dwInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch(dwInfoLevel)
    {
        case 0:

            PrintGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
            break;

        case 1:

            PrintGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo);
            break;

        default:

            dwError = LW_ERROR_INVALID_GROUP_INFO_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);

            break;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszGroupname);

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    goto cleanup;
}

VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    fprintf(stdout, "Group info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszName) ? "<null>" : pGroupInfo->pszName);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:     %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszSid) ? "<null>" : pGroupInfo->pszSid);
}

VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    PSTR* ppszMembers = NULL;
    DWORD iMember = 0;

    fprintf(stdout, "Group info (Level-1):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszName) ? "<null>" : pGroupInfo->pszName);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:     %s\n",
                        LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszSid) ? "<null>" : pGroupInfo->pszSid);

    fprintf(stdout, "Members:\n");

    ppszMembers = pGroupInfo->ppszMembers;

    if (ppszMembers){
    while (!LW_IS_NULL_OR_EMPTY_STR(*ppszMembers)) {
          if (iMember) {
             fprintf(stdout, "\n%s", *ppszMembers);
          } else {
             fprintf(stdout, "%s", *ppszMembers);
          }
          iMember++;
          ppszMembers++;
       }
    }
    fprintf(stdout, "\n");
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
