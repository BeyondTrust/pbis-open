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
 *        Program to list groups for user
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
void
ShowUsage()
{
    printf("Usage: list-groups-for-user [options] <user name>\n"
           "   or: list-groups-for-user [options] --uid <uid>\n"
           "\n"
           "  where options are:\n"
           "\n"
           "    --show-sid  -- Show SIDs\n");
}

static
BOOLEAN
IsAllDigits(
    IN PCSTR pszString
    )
{
    const char* pChar = NULL;
    BOOLEAN bIsAllDigits = TRUE;

    for (pChar = pszString; *pChar; pChar++)
    {
        if (!isdigit((int)*pChar))
        {
            bIsAllDigits = FALSE;
            break;
        }
    }

    return bIsAllDigits;
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

static
DWORD
StringToId(
    IN PCSTR pszString,
    OUT PDWORD pValue
    )
{
    DWORD dwError = 0;
    long long int result = 0;

    errno = 0;
    result = LwStrtoll(pszString, NULL, 10);
    if (errno)
    {
        perror("Cannot convert to id");
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (result < 0 || result > MAXDWORD)
    {
        printf("Argument out of range\n");
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *pValue = (DWORD) result;
    return dwError;

error:
    result = 0;
    goto cleanup;
}

static
VOID
ParseArgs(
    IN int argc,
    IN char* argv[],
    OUT PCSTR* ppszUserName,
    OUT PDWORD pdwId,
    OUT PBOOLEAN pbShowSid
    )
{
    DWORD dwError = 0;
    int iArg = 0;
    PSTR pszArg = NULL;
    BOOLEAN bIsId = FALSE;
    PSTR pszUserName = NULL;
    DWORD dwId = 0;
    BOOLEAN bShowSid = FALSE;

    if (argc < 2)
    {
        ShowUsage();
        exit(1);
    }

    // First, get any options.
    for (iArg = 1; iArg < argc; iArg++)
    {
        pszArg = argv[iArg];

        if (!strcmp(pszArg, "--help") || !strcmp(pszArg, "-h"))
        {
            ShowUsage();
            exit(0);
        }
        else if (!strcmp(pszArg, "--uid") || !strcmp(pszArg, "-u"))
        {
            PCSTR pszUid = NULL;

            bIsId = TRUE;

            if ((iArg + 1) >= argc)
            {
                fprintf(stderr, "Missing argument for %s option.\n", pszArg);
                ShowUsage();
                exit(1);
            }

            pszUid = argv[iArg + 1];
            iArg++;

            if (!IsAllDigits(pszUid))
            {
                fprintf(stderr, "Non-numeric argument for %s option.\n", pszArg);
                ShowUsage();
                exit(1);
            }

            dwError = StringToId(pszUid, &dwId);
            if (dwError)
            {
                fprintf(stderr, "Invalid range for %s option.\n", pszArg);
                ShowUsage();
                exit(1);
            }

            // There can be no options following this one.
            iArg++;
            break;
        }
        else if (!strcmp(pszArg, "--show-sid"))
        {
            bShowSid = TRUE;
        }
        else
        {
            break;
        }
    }

    // Now get positional arguments.
    if ((argc - iArg) >= 1)
    {
        pszUserName = argv[iArg++];
        if (LW_IS_NULL_OR_EMPTY_STR(pszUserName))
        {
           fprintf(stderr, "Please specify a non-empty user name to query for.\n");
           ShowUsage();
           exit(1);
        }
    }

    // Now verify arguments.
    if (argc > iArg)
    {
        fprintf(stderr, "Too many arguments.\n");
        ShowUsage();
        exit(1);
    }

    if (bIsId && pszUserName)
    {
        fprintf(stderr, "Please specify either a uid or user name.\n");
        ShowUsage();
        exit(1);
    }

    *ppszUserName = pszUserName;
    *pdwId = dwId;
    *pbShowSid = bShowSid;
}

int
list_groups_for_user_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PCSTR pszUserName = NULL;
    DWORD  dwNumGroups = 0;
    DWORD  iGroup = 0;
    LSA_FIND_FLAGS FindFlags = 0;
    DWORD dwGroupInfoLevel = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD dwId = 0;
    BOOLEAN bShowSid = FALSE;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

    ParseArgs(argc, argv, &pszUserName, &dwId, &bShowSid);

    if (pszUserName)
    {
        dwError = LsaValidateUserName(pszUserName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszUserName)
    {
        dwError = LsaGetGroupsForUserByName(
                        hLsaConnection,
                        pszUserName,
                        FindFlags,
                        dwGroupInfoLevel,
                        &dwNumGroups,
                        &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaGetGroupsForUserById(
                        hLsaConnection,
                        dwId,
                        FindFlags,
                        dwGroupInfoLevel,
                        &dwNumGroups,
                        &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszUserName)
    {
        printf("Number of groups found for user '%s' : %u\n", pszUserName, dwNumGroups);
    }
    else
    {
        printf("Number of groups found for uid %u : %u\n", dwId, dwNumGroups);
    }

    switch(dwGroupInfoLevel)
    {
        case 0:

            for (iGroup = 0; iGroup < dwNumGroups; iGroup++)
            {
                PLSA_GROUP_INFO_0* pGroupInfoList = (PLSA_GROUP_INFO_0*)ppGroupInfoList;

                if (bShowSid)
                {
                    fprintf(stdout,
                            "Group[%u of %u] name = %s (gid = %u, sid = %s)\n",
                            iGroup+1,
                            dwNumGroups,
                            pGroupInfoList[iGroup]->pszName,
                            (unsigned int) pGroupInfoList[iGroup]->gid,
                            pGroupInfoList[iGroup]->pszSid);
                }
                else
                {
                    fprintf(stdout,
                            "Group[%u of %u] name = %s (gid = %u)\n",
                            iGroup+1,
                            dwNumGroups,
                            pGroupInfoList[iGroup]->pszName,
                            (unsigned int) pGroupInfoList[iGroup]->gid);
                }
            }

            break;

        default:
            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

cleanup:

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroups);
    }

    if (hLsaConnection)
    {
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
                if (pszUserName)
                {
                    fprintf(
                        stderr,
                        "Failed to find groups for user '%s'.  Error code %u (%s).\n%s\n",
                        pszUserName,
                        dwError,
                        LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                        pszErrorBuffer);
                }
                else
                {
                    fprintf(
                        stderr,
                        "Failed to find groups for uid %u.  Error code %u (%s).\n%s\n",
                        dwId,
                        dwError,
                        LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                        pszErrorBuffer);
                }
                bPrintOrigError = FALSE;
            }
        }

        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        if (pszUserName)
        {
            fprintf(
                stderr,
                "Failed to find groups for user '%s'.  Error code %u (%s).\n",
                pszUserName,
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
        }
        else
        {
            fprintf(
                stderr,
                "Failed to find groups for uid %u.  Error code %u (%s).\n",
                dwId,
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
        }
    }

    goto cleanup;
}
