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
 *        Test Program for exercising LsaEnumGroups
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"
#include "common.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwInfoLevel,
    PDWORD pdwBatchSize,
    PBOOLEAN pbCheckGroupMembersOnline
    );

static
VOID
ShowUsage();

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
enum_groups_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwBatchSize = 10;
    HANDLE hLsaConnection = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwTotalGroupsFound = 0;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = FALSE;
    BOOLEAN bCheckGroupMembersOnline = FALSE;

    dwError = ParseArgs(argc, argv, &dwGroupInfoLevel, &dwBatchSize, &bCheckGroupMembersOnline);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBeginEnumGroupsWithCheckOnlineOption(
                    hLsaConnection,
                    dwGroupInfoLevel,
                    dwBatchSize,
                    bCheckGroupMembersOnline,
                    0,
                    &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        DWORD iGroup = 0;

        if (ppGroupInfoList) {
           LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
           ppGroupInfoList = NULL;
        }

        dwError = LsaEnumGroups(
                    hLsaConnection,
                    hResume,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumGroupsFound) {
            break;
        }

        dwTotalGroupsFound+=dwNumGroupsFound;

        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            PVOID pGroupInfo = *(ppGroupInfoList + iGroup);

            switch(dwGroupInfoLevel)
            {
                case 0:
                    PrintGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
                    break;
                case 1:
                    PrintGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo);
                    break;
                default:

                    fprintf(stderr,
                            "Error: Invalid Group info level %u\n",
                            dwGroupInfoLevel);
                    break;
            }
        }
    } while (dwNumGroupsFound);

    fprintf(stdout, "TotalNumGroupsFound: %u\n", dwTotalGroupsFound);

cleanup:

    if (ppGroupInfoList) {
       LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    if ((hResume != (HANDLE)NULL) &&
        (hLsaConnection != (HANDLE)NULL)) {
        LsaEndEnumGroups(hLsaConnection, hResume);
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
                fprintf(stderr, "Failed to enumerate groups.  Error code %u (%s).\n"
                        "%s\n",
                        dwError, LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                        pszErrorBuffer);
                bPrintOrigError = FALSE;
            }

            if (dwError == ERROR_INVALID_DATA)
            {
                fprintf(stderr, "The groups list has changed while enumerating. "
                        "Try again.\n");
            }
        }

        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to enumerate groups.  Error code %u (%s).\n",
                dwError, LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwInfoLevel,
    PDWORD pdwBatchSize,
    PBOOLEAN pbCheckGroupMembersOnline
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL,
            PARSE_MODE_BATCHSIZE
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;
    DWORD dwBatchSize = 10;
    BOOLEAN bCheckGroupMembersOnline = FALSE;

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
                else if (!strcmp(pszArg, "--check-group-members-online") ||
                        !strcmp(pszArg, "-c"))
               {
                   bCheckGroupMembersOnline = TRUE;
               }
                else
                {
                    ShowUsage();
                    exit(0);
                }
                break;

            case PARSE_MODE_LEVEL:

                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please use an info level which is an unsigned integer.\n");
                    ShowUsage();
                    exit(1);
                }

                dwInfoLevel = atoi(pszArg);
                if (dwInfoLevel > 1) {
                    ShowUsage();
                    exit(1);
                }

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_BATCHSIZE:

                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please use a batchsize which is an unsigned integer.\n");
                    ShowUsage();
                    exit(1);
                }

                dwBatchSize = atoi(pszArg);
                if ((dwBatchSize == 0) ||
                    (dwBatchSize > 1000)) {
                    ShowUsage();
                    exit(1);
                }
                parseMode = PARSE_MODE_OPEN;

                break;
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN)
    {
        ShowUsage();
        exit(1);
    }

    *pdwInfoLevel = dwInfoLevel;
    *pdwBatchSize = dwBatchSize;
    *pbCheckGroupMembersOnline = bCheckGroupMembersOnline;

    return dwError;
}

static
void
ShowUsage()
{
    printf("Usage: lw-enum-groups {--level [0, 1]} {--batchsize [1..1000]} {--check-group-members-online | -c}\n");
}

static
VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    fprintf(stdout, "Group info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name: %s\n", LW_PRINTF_STRING(pGroupInfo->pszName));
    fprintf(stdout, "Gid:  %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:  %s\n", LW_PRINTF_STRING(pGroupInfo->pszSid));
    fprintf(stdout, "\n");
}

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    PSTR* ppszMembers = NULL;

    fprintf(stdout, "Group info (Level-1):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name: %s\n", LW_PRINTF_STRING(pGroupInfo->pszName));
    fprintf(stdout, "Gid:  %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:  %s\n", LW_PRINTF_STRING(pGroupInfo->pszSid));
    fprintf(stdout, "Members:\n");

    ppszMembers = pGroupInfo->ppszMembers;
    if (ppszMembers)
    {
        while (!LW_IS_NULL_OR_EMPTY_STR(*ppszMembers))
        {
            fprintf(stdout, "  %s\n", *ppszMembers);
            ppszMembers++;
        }
    }
    fprintf(stdout, "\n");
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
