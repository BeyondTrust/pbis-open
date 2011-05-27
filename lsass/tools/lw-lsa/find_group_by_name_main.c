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
 *        Test Program for exercising LsaFindGroupByName
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
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

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszGroupId,
    PLSA_FIND_FLAGS pFindFlags,
    PDWORD pdwInfoLevel,
    PBOOLEAN pbCountOnly
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
    PLSA_GROUP_INFO_1 pGroupInfo,
    BOOLEAN bCountOnly
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
find_group_by_name_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR  pszGroupId = NULL;
    DWORD dwInfoLevel = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    BOOLEAN bCountOnly = FALSE;
    LSA_FIND_FLAGS FindFlags = 0;

    dwError = ParseArgs(argc, argv, &pszGroupId, &FindFlags, &dwInfoLevel, &bCountOnly);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszGroupId,
                    FindFlags,
                    dwInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch(dwInfoLevel)
    {
        case 0:
            PrintGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
            break;
        case 1:
            PrintGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo, bCountOnly);
            break;
        default:

            fprintf(stderr, "Error: Invalid group info level [%u]\n", dwInfoLevel);
            break;
    }

cleanup:

    if (pGroupInfo) {
       LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszGroupId);

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
                        "Failed to locate group.  Error code %u (%s).\n%s\n",
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
                "Failed to locate group.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszGroupId,
    PLSA_FIND_FLAGS pFindFlags,
    PDWORD pdwInfoLevel,
    PBOOLEAN pbCountOnly
    )
{
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszGroupId = NULL;
    LSA_FIND_FLAGS FindFlags = 0;
    DWORD dwInfoLevel = 0;
    BOOLEAN bCountOnly = FALSE;

    for (iArg = 1; iArg < argc; iArg++)
    {
        pszArg = argv[iArg];
        if (!strcmp(pszArg, "--help") ||
            !strcmp(pszArg, "-h"))
        {
            ShowUsage();
            exit(0);
        }
        else if (!strcmp(pszArg, "--count"))
        {
            bCountOnly = TRUE;
        }
        else if (!strcmp(pszArg, "--level"))
        {
            PCSTR pszValue;
            if (iArg + 1 >= argc)
            {
                fprintf(stderr, "Missing argument for %s option.\n", pszArg);
                ShowUsage();
                exit(1);
            }
            pszValue = argv[++iArg];
            if (!IsUnsignedInteger(pszValue))
            {
                fprintf(stderr, "Please enter an info level which is an unsigned integer.\n");
                ShowUsage();
                exit(1);
            }
            dwInfoLevel = atoi(pszValue);
        }
        else if (!strcmp(pszArg, "--flags"))
        {
            PCSTR pszValue;
            if (iArg + 1 >= argc)
            {
                fprintf(stderr, "Missing argument for %s option.\n", pszArg);
                ShowUsage();
                exit(1);
            }
            pszValue = argv[++iArg];
            if (!IsUnsignedInteger(pszValue))
            {
                fprintf(stderr, "Please enter a flags value which is an unsigned integer.\n");
                ShowUsage();
                exit(1);
            }
            FindFlags = atoi(pszValue);
        }
        else if (pszArg[0] == '-')
        {
            fprintf(stderr, "Invalid option '%s'.\n", pszArg);
            ShowUsage();
            exit(1);
        }
        else
        {
            break;
        }
    }

    if ((argc - iArg) < 1)
    {
        fprintf(stderr, "Missing required group name argument.\n");
        ShowUsage();
        exit(1);
    }
    dwError = LwAllocateString(argv[iArg++], &pszGroupId);
    BAIL_ON_LSA_ERROR(dwError);

    if ((argc - iArg) > 0)
    {
        fprintf(stderr, "Too many arguments.\n");
        ShowUsage();
        exit(1);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszGroupId))
    {
        fprintf(stderr, "Please specify a non-empty group name to query for.\n");
        ShowUsage();
        exit(1);
    }

    *ppszGroupId = pszGroupId;
    *pFindFlags = FindFlags;
    *pdwInfoLevel = dwInfoLevel;
    *pbCountOnly = bCountOnly;

cleanup:

    return dwError;

error:

    *ppszGroupId = NULL;
    *pFindFlags = 0;
    *pdwInfoLevel = 0;
    *pbCountOnly = FALSE;

    LW_SAFE_FREE_STRING(pszGroupId);

    goto cleanup;
}

void
ShowUsage()
{
    PCSTR pszProgramName = "lw-find-group-by-name";
    printf("Usage: %s [OPTIONS] <GROUP_NAME>\n"
           "\n"
           "    Lookup a group by name.\n"
           "\n"
           "  Options:\n"
           "\n"
           "    --level LEVEL   - Output level can be 0 or 1.\n"
           "                      0 does not include membership info.\n"
           "                      1 include group membership info.\n"
           "\n"
           "    --count         - If used with level 1, shows membership count only\n"
           "                      instead of listing group members.\n"
           "\n"
           "    --flags FLAGS   - Find flags can be 0 or 1.\n"
           "\n"
           "  Examples:\n"
           "\n"
           "    %s DOMAIN\\\\groupname\n"
           "    %s --level 1 groupalias\n"
           "\n",
           pszProgramName,
           pszProgramName,
           pszProgramName);
}

VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    printf("Group info (Level 0):\n"
           "====================\n"
           "Name: %s\n"
           "Gid:  %u\n"
           "SID:  %s\n",
           LSA_SAFE_LOG_STRING(pGroupInfo->pszName),
           (unsigned int)pGroupInfo->gid,
           LSA_SAFE_LOG_STRING(pGroupInfo->pszSid));
}

VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo,
    BOOLEAN bCountOnly
    )
{
    PSTR* ppszMembers = NULL;
    DWORD iMember = 0;

    printf("Group info (Level 1):\n"
           "====================\n"
           "Name: %s\n"
           "Gid:  %u\n"
           "SID:  %s\n",
           LSA_SAFE_LOG_STRING(pGroupInfo->pszName),
           (unsigned int)pGroupInfo->gid,
           LSA_SAFE_LOG_STRING(pGroupInfo->pszSid));

    if (!bCountOnly)
    {
        printf("Members:\n");
    }

    ppszMembers = pGroupInfo->ppszMembers;
    if (ppszMembers)
    {
        while (!LW_IS_NULL_OR_EMPTY_STR(ppszMembers[iMember]))
        {
            if (!bCountOnly)
            {
                printf("%s\n", ppszMembers[iMember]);
            }
            iMember++;
        }
    }

    printf("Members Count: %u\n", iMember);
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
