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
 *        Test Program for exercising LsaEnumUsers
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"
#include "common.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")
// Why do we need YES/NO and TRUE/FALSE?
#define LW_PRINTF_YES_NO(x) ((x) ? "YES" : "NO")
#define LW_PRINTF_TRUE_FALSE(x) ((x) ? "TRUE" : "FALSE")

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwInfoLevel,
    PDWORD pdwBatchSize,
    PBOOLEAN pbCheckUserInList
    );

static
VOID
ShowUsage();

static
VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo,
    BOOLEAN bCheckUserInList,
    BOOLEAN bAllowedLogon
    );

static
VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo,
    BOOLEAN bCheckUserInList,
    BOOLEAN bAllowedLogon
    );

static
VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo,
    BOOLEAN bCheckUserInList,
    BOOLEAN bAllowedLogon
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
enum_users_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 0;
    DWORD dwBatchSize = 10;
    HANDLE hLsaConnection = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwNumUsersFound = 0;
    DWORD  dwTotalUsersFound = 0;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    BOOLEAN bCheckUserInList = FALSE;

    dwError = ParseArgs(argc, argv, &dwUserInfoLevel, &dwBatchSize, &bCheckUserInList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBeginEnumUsers(
                    hLsaConnection,
                    dwUserInfoLevel,
                    dwBatchSize,
                    0,
                    &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        DWORD iUser = 0;

        if (ppUserInfoList) {
           LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
           ppUserInfoList = NULL;
        }

        dwError = LsaEnumUsers(
                    hLsaConnection,
                    hResume,
                    &dwNumUsersFound,
                    &ppUserInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumUsersFound) {
            break;
        }

        dwTotalUsersFound+=dwNumUsersFound;

        for (iUser = 0; iUser < dwNumUsersFound; iUser++)
        {
            BOOLEAN bAllowedLogon = TRUE;
            PVOID pUserInfo = *(ppUserInfoList + iUser);

            if (bCheckUserInList)
            {
                dwError = LsaCheckUserInList(
                                          hLsaConnection,
                                          ((PLSA_USER_INFO_0)pUserInfo)->pszName,
                                          NULL);
                if (dwError)
                {
                    bAllowedLogon = FALSE;
                }
            }

            switch(dwUserInfoLevel)
            {
                case 0:
                    PrintUserInfo_0((PLSA_USER_INFO_0)pUserInfo,
                                    bCheckUserInList,
                                    bAllowedLogon);
                    break;
                case 1:
                    PrintUserInfo_1((PLSA_USER_INFO_1)pUserInfo,
                                    bCheckUserInList,
                                    bAllowedLogon);
                    break;
                case 2:
                    PrintUserInfo_2((PLSA_USER_INFO_2)pUserInfo,
                                    bCheckUserInList,
                                    bAllowedLogon);
                    break;
                default:

                    fprintf(stderr,
                            "Error: Invalid user info level %u\n",
                            dwUserInfoLevel);
                    break;
            }
        }
    } while (dwNumUsersFound);

    fprintf(stdout, "TotalNumUsersFound: %u\n", dwTotalUsersFound);

cleanup:

    if (ppUserInfoList) {
       LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }

    if ((hResume != (HANDLE)NULL) &&
        (hLsaConnection != (HANDLE)NULL)) {
        LsaEndEnumUsers(hLsaConnection, hResume);
    }

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

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
                fprintf(stderr, "Failed to enumerate users.  Error code %u (%s).\n"
                        "%s\n",
                        dwError, LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                        pszErrorBuffer);
                bPrintOrigError = FALSE;
            }

            if (dwError == ERROR_INVALID_DATA)
            {
                fprintf(stderr, "The users list has changed while enumerating. "
                        "Try again.\n");
            }
        }

        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to enumerate users.  Error code %u (%s).\n",
                dwError, LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwInfoLevel,
    PDWORD pdwBatchSize,
    PBOOLEAN pbCheckUserInList
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL,
            PARSE_MODE_BATCHSIZE,
            PARSE_MODE_DONE
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;
    DWORD dwBatchSize = 10;
    BOOLEAN bCheckUserInList = FALSE;

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
                else if (!strcmp(pszArg, "--batchsize")) {
                    parseMode = PARSE_MODE_BATCHSIZE;
                }
                else if (!strcmp(pszArg, "--checkUserinList") || !strcmp(pszArg, "-c") || !strcmp(pszArg, "-C")) {
                    bCheckUserInList = TRUE;
                }
                else
                {
                    ShowUsage();
                    exit(1);
                }
                break;

            case PARSE_MODE_LEVEL:

                if (!IsUnsignedInteger(pszArg))
                {
                    ShowUsage();
                    exit(1);
                }

                dwInfoLevel = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_BATCHSIZE:

                if (!IsUnsignedInteger(pszArg))
                {
                    ShowUsage();
                    exit(1);
                }

                dwBatchSize = atoi(pszArg);
		if ((dwBatchSize == 0) ||
                    (dwBatchSize > 1000)) {
                    ShowUsage();
                    exit(1);
                }
                parseMode = PARSE_MODE_DONE;

                break;

            case PARSE_MODE_DONE:
                ShowUsage();
                exit(1);
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage();
        exit(1);
    }

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage();
        exit(1);
    }

    *pdwInfoLevel = dwInfoLevel;
    *pdwBatchSize = dwBatchSize;
    *pbCheckUserInList = bCheckUserInList;

    return dwError;
}

void
ShowUsage()
{
    printf("Usage: enum-users {--level [0, 1, 2]} {--batchsize [1..1000]} {--checkUserinList || -c} \n");
}

VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo,
    BOOLEAN bCheckUserInList,
    BOOLEAN bAllowedLogon
    )
{
    fprintf(stdout, "User info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:              %s\n", LW_PRINTF_STRING(pUserInfo->pszName));
    fprintf(stdout, "Uid:               %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:               %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:             %s\n", LW_PRINTF_STRING(pUserInfo->pszGecos));
    fprintf(stdout, "Shell:             %s\n", LW_PRINTF_STRING(pUserInfo->pszShell));
    fprintf(stdout, "Home dir:          %s\n", LW_PRINTF_STRING(pUserInfo->pszHomedir));
    if (bCheckUserInList)
    {
        fprintf(stdout, "Logon restriction: %s\n", LW_PRINTF_YES_NO(bAllowedLogon));
    }
    fprintf(stdout, "\n");
}

VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo,
    BOOLEAN bCheckUserInList,
    BOOLEAN bAllowedLogon
    )
{
    fprintf(stdout, "User info (Level-1):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:              %s\n", LW_PRINTF_STRING(pUserInfo->pszName));
    fprintf(stdout, "UPN:               %s\n", LW_PRINTF_STRING(pUserInfo->pszUPN));
    fprintf(stdout, "Generated UPN:     %s\n", LW_PRINTF_YES_NO(pUserInfo->bIsGeneratedUPN));
    fprintf(stdout, "Uid:               %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:               %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:             %s\n", LW_PRINTF_STRING(pUserInfo->pszGecos));
    fprintf(stdout, "Shell:             %s\n", LW_PRINTF_STRING(pUserInfo->pszShell));
    fprintf(stdout, "Home dir:          %s\n", LW_PRINTF_STRING(pUserInfo->pszHomedir));
    fprintf(stdout, "LMHash length:     %u\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length:     %u\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:        %s\n", LW_PRINTF_YES_NO(pUserInfo->bIsLocalUser));
    if (bCheckUserInList)
    {
        fprintf(stdout, "Logon restriction: %s\n", LW_PRINTF_YES_NO(bAllowedLogon));
    }
    fprintf(stdout, "\n");
}

VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo,
    BOOLEAN bCheckUserInList,
    BOOLEAN bAllowedLogon
    )
{
    fprintf(stdout, "User info (Level-2):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:                         %s\n", LW_PRINTF_STRING(pUserInfo->pszName));
    fprintf(stdout, "UPN:                          %s\n", LW_PRINTF_STRING(pUserInfo->pszUPN));
    fprintf(stdout, "Generated UPN:                %s\n", LW_PRINTF_YES_NO(pUserInfo->bIsGeneratedUPN));
    fprintf(stdout, "DN:                           %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszDN) ? "<null>" : pUserInfo->pszDN);
    fprintf(stdout, "Uid:                          %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:                          %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:                        %s\n", LW_PRINTF_STRING(pUserInfo->pszGecos));
    fprintf(stdout, "Shell:                        %s\n", LW_PRINTF_STRING(pUserInfo->pszShell));
    fprintf(stdout, "Home dir:                     %s\n", LW_PRINTF_STRING(pUserInfo->pszHomedir));
    fprintf(stdout, "LMHash length:                %u\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length:                %u\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:                   %s\n", LW_PRINTF_YES_NO(pUserInfo->bIsLocalUser));
    fprintf(stdout, "Account disabled (or locked): %s\n", LW_PRINTF_TRUE_FALSE(pUserInfo->bAccountDisabled));
    fprintf(stdout, "Account Expired:              %s\n", LW_PRINTF_TRUE_FALSE(pUserInfo->bAccountExpired));
    fprintf(stdout, "Password never expires:       %s\n", LW_PRINTF_TRUE_FALSE(pUserInfo->bPasswordNeverExpires));
    fprintf(stdout, "Password Expired:             %s\n", LW_PRINTF_TRUE_FALSE(pUserInfo->bPasswordExpired));
    fprintf(stdout, "Prompt for password change:   %s\n", LW_PRINTF_YES_NO(pUserInfo->bPromptPasswordChange));
    fprintf(stdout, "User can change password:     %s\n", LW_PRINTF_YES_NO(pUserInfo->bUserCanChangePassword));
    fprintf(stdout, "Days till password expires:   %u\n", pUserInfo->dwDaysToPasswordExpiry);
    if (bCheckUserInList)
    {
        fprintf(stdout, "Logon restriction:            %s\n", LW_PRINTF_YES_NO(bAllowedLogon));
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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
