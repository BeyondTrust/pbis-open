/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        share.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        Share Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

VOID
NetShareShowUsage(
	VOID
	)
{
    printf(
        "Usage: lwnet share help\n"
        "       lwnet share [ <options> ... ]\n"
        "       lwnet share add [ <options> ... ] <name=path>\n"
        "       lwnet share del [ <options> ... ] <name>\n");

    printf("\n"
           "Options:\n"
           "\n"
           "  --server <server>       Specify target server (default: local machine)\n"
           "\n");
}

static
VOID
NetShareFreeCommandInfo(
    PNET_SHARE_COMMAND_INFO pCommandInfo
    );

static
DWORD
AppendStringArray(
    PDWORD pdwCount,
    PWSTR** pppwszArray,
    PWSTR pwszString
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszNewArray = NULL;

    dwError = LwReallocMemory(
        *pppwszArray,
        OUT_PPVOID(&ppwszNewArray),
        sizeof(*ppwszNewArray) * (*pdwCount + 1));
    BAIL_ON_LWUTIL_ERROR(dwError);

    ppwszNewArray[(*pdwCount)++] = pwszString;

    *pppwszArray = ppwszNewArray;

error:

    return dwError;
}


static
DWORD
ParseShareAddOptionArgs(
    IN int argc,
    IN int indexStart,
    IN char** argv,
    IN OUT NET_SHARE_ADD_INFO_PARAMS ShareAddParams
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PWSTR pwszArg = NULL;

    for (dwIndex = indexStart; dwIndex < argc; dwIndex++)
    {
        if (!strcmp(argv[dwIndex], "--allow"))
        {
            dwError = LwMbsToWc16s(argv[++dwIndex], &pwszArg);
            BAIL_ON_LWUTIL_ERROR(dwError);

            dwError = AppendStringArray(
                &ShareAddParams.dwAllowUserCount,
                &ShareAddParams.ppwszAllowUsers,
                pwszArg);
            BAIL_ON_LWUTIL_ERROR(dwError);

            pwszArg = NULL;
        }
        else if (!strcmp(argv[dwIndex], "--deny"))
        {
            dwError = LwMbsToWc16s(argv[++dwIndex], &pwszArg);
            BAIL_ON_LWUTIL_ERROR(dwError);

            dwError = AppendStringArray(
                &ShareAddParams.dwDenyUserCount,
                &ShareAddParams.ppwszDenyUsers,
                pwszArg);
            BAIL_ON_LWUTIL_ERROR(dwError);

            pwszArg = NULL;
        }
        else if (!strcmp(argv[dwIndex], "--comment"))
        {
            dwError = LwMbsToWc16s(argv[++dwIndex], &ShareAddParams.pwszComment);
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
        else if (!strcmp(argv[dwIndex], "--read-only"))
        {
        	ShareAddParams.bReadOnly = TRUE;
        }
        else if (!strcmp(argv[dwIndex], "--read-write"))
        {
        	ShareAddParams.bReadWrite = TRUE;
        }
        else if (!strcmp(argv[dwIndex], "--clear-allow"))
        {
        	ShareAddParams.bClearAllow = TRUE;
        }
        else if (!strcmp(argv[dwIndex], "--clear-deny"))
        {
        	ShareAddParams.bClearDeny = TRUE;
        }
        else
        {
            dwError = LwMbsToWc16s(argv[dwIndex], &ShareAddParams.pwszTarget);
            BAIL_ON_LWUTIL_ERROR(dwError);
            break;
        }
    }

error:

    LWUTIL_SAFE_FREE_MEMORY(pwszArg);

    return dwError;
}


static
DWORD
NetShareAddParseArguments(
	int argc,
	char** argv,
	IN OUT PNET_SHARE_COMMAND_INFO pCommandInfo
	)
{
	DWORD dwError = 0;
	PCSTR pszPath =  NULL;
	size_t sShareNameLen = 0;
	int indexShareAddArg = 3;
	// Do not free
	PCSTR pszShareAddShareInfo = NULL;
	PSTR pszShareName = NULL;

	if (!argv[indexShareAddArg])
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

	if (!strcasecmp(argv[indexShareAddArg], "--server"))
	{
	    dwError = LwMbsToWc16s(argv[++indexShareAddArg], &pCommandInfo->ShareAddInfo.pwszServerName);
	    BAIL_ON_LWUTIL_ERROR(dwError);

	    indexShareAddArg++;
	}

	if (indexShareAddArg > argc-1)
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

	pszShareAddShareInfo = argv[indexShareAddArg];

	pszPath = strchr(pszShareAddShareInfo, '=');
	if (LWUTIL_IS_NULL_OR_EMPTY_STR(pszPath))
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

    dwError = LwMbsToWc16s(pszPath+1, &pCommandInfo->ShareAddInfo.pwszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    sShareNameLen = strlen(pszShareAddShareInfo)-strlen(pszPath);

    if (!sShareNameLen)
    {
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LwNetAllocateMemory(sShareNameLen+1,
    		                  (PVOID*)&pszShareName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    memcpy(pszShareName, pszShareAddShareInfo, sShareNameLen);

    dwError = LwMbsToWc16s(pszShareName, &pCommandInfo->ShareAddInfo.pwszShareName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    // Process add options
    dwError = ParseShareAddOptionArgs(argc, ++indexShareAddArg, argv, pCommandInfo->ShareAddInfo);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:
    LWUTIL_SAFE_FREE_STRING(pszShareName);

    return dwError;

error:

    NetShareFreeCommandInfo(pCommandInfo);

    goto cleanup;
}

static
DWORD
NetShareDelParseArguments(
	int argc,
	char** argv,
	IN OUT PNET_SHARE_COMMAND_INFO pCommandInfo
	)
{
	DWORD dwError = 0;
	int indexShareDelArg = 3;

	if (!argv[indexShareDelArg])
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

	if (!strcasecmp(argv[indexShareDelArg], "--server"))
	{
	    dwError = LwMbsToWc16s(argv[++indexShareDelArg], &pCommandInfo->ShareDelInfo.pwszServerName);
	    BAIL_ON_LWUTIL_ERROR(dwError);

	    indexShareDelArg++;
	}

	if (indexShareDelArg > argc-1)
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

    dwError = LwMbsToWc16s(argv[indexShareDelArg], &pCommandInfo->ShareDelInfo.pwszShareName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:

    NetShareFreeCommandInfo(pCommandInfo);

    goto cleanup;
}

static
DWORD
NetShareEnumParseArguments(
	int argc,
    char ** argv,
	IN OUT PNET_SHARE_COMMAND_INFO pCommandInfo
	)
{
	DWORD dwError = 0;

	if (!argv[3])
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

    dwError = LwMbsToWc16s(argv[3], &pCommandInfo->ShareEnumInfo.pwszServerName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:

    NetShareFreeCommandInfo(pCommandInfo);

    goto cleanup;
}


static
DWORD
NetShareParseArguments(
    int argc,
	char ** argv,
	PNET_SHARE_COMMAND_INFO* ppCommandInfo
	)
{
	DWORD dwError = 0;
	PNET_SHARE_COMMAND_INFO pCommandInfo = NULL;

	if (argc < 2)
	{
		dwError = LW_ERROR_INTERNAL;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

    dwError = LwNetAllocateMemory(sizeof(*pCommandInfo),
    		                       (PVOID*)&pCommandInfo);
    BAIL_ON_LWUTIL_ERROR(dwError);


    if (!argv[2])
    {
    	pCommandInfo->dwControlCode = NET_SHARE_ENUM;
    	goto cleanup;
    }

    if (!strcasecmp(argv[2], NET_SHARE_COMMAND_HELP))
    {
    	NetShareShowUsage();
    	goto cleanup;
    }
    else if (!strcasecmp(argv[2], NET_SHARE_COMMAND_ADD))
	{
		pCommandInfo->dwControlCode = NET_SHARE_ADD;

		if (!argv[3])
		{
			dwError = LW_ERROR_INVALID_PARAMETER;
			BAIL_ON_LWUTIL_ERROR(dwError);
		}

		dwError = NetShareAddParseArguments(argc, argv, pCommandInfo);
		BAIL_ON_LWUTIL_ERROR(dwError);
	}
	else if (!strcasecmp(argv[2], NET_SHARE_COMMAND_DEL))
	{
		pCommandInfo->dwControlCode = NET_SHARE_DEL;

		dwError = NetShareDelParseArguments(argc, argv, pCommandInfo);
		BAIL_ON_LWUTIL_ERROR(dwError);
	}
	else if (!strcasecmp(argv[2], "--server"))
    {
		pCommandInfo->dwControlCode = NET_SHARE_ENUM;

		dwError = NetShareEnumParseArguments(argc, argv, pCommandInfo);
		BAIL_ON_LWUTIL_ERROR(dwError);
    }
	else
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

cleanup:

    *ppCommandInfo = pCommandInfo;

    return dwError;

error:
    if (LW_ERROR_INVALID_PARAMETER == dwError)
    {
    	NetShareShowUsage();
    }

    LWUTIL_SAFE_FREE_MEMORY(pCommandInfo);
    pCommandInfo = NULL;

    goto cleanup;
}

DWORD
NetShareInitialize(
    VOID
    )
{
    return NetApiInitialize();
}

DWORD
NetShare(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;
    PNET_SHARE_COMMAND_INFO pCommandInfo = NULL;

    dwError = NetShareParseArguments(
                    argc,
                    argv,
                    &pCommandInfo
                    );
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = NetShareInitialize();
    BAIL_ON_LWUTIL_ERROR(dwError);

    switch (pCommandInfo->dwControlCode)
    {

        case NET_SHARE_ADD:

        	dwError = LwUtilNetShareAdd(pCommandInfo->ShareAddInfo);
        	BAIL_ON_LWUTIL_ERROR(dwError);
        	break;


        case NET_SHARE_DEL:
            dwError = LwUtilNetShareDel(pCommandInfo->ShareDelInfo);
        	BAIL_ON_LWUTIL_ERROR(dwError);
        	break;

        case NET_SHARE_ENUM:

            dwError = LwUtilNetShareEnum(pCommandInfo->ShareEnumInfo);
        	BAIL_ON_LWUTIL_ERROR(dwError);
        	break;

        default:
        	break;
    }

cleanup:
    NetShareFreeCommandInfo(pCommandInfo);
    LWUTIL_SAFE_FREE_MEMORY(pCommandInfo);
    pCommandInfo = NULL;

    return dwError;

error:
    goto cleanup;
}

DWORD
NetShareShutdown(
    VOID
    )
{
    return NetApiShutdown();
}

static
VOID
NetShareFreeCommandInfo(
	PNET_SHARE_COMMAND_INFO pCommandInfo
	)
{
	if (!pCommandInfo)
		return;

	switch (pCommandInfo->dwControlCode)
	{
	    case NET_SHARE_ADD:

	    	LWUTIL_SAFE_FREE_MEMORY(pCommandInfo->ShareAddInfo.pwszServerName);
	    	LWUTIL_SAFE_FREE_MEMORY(pCommandInfo->ShareAddInfo.pwszPath);
	    	LWUTIL_SAFE_FREE_MEMORY(pCommandInfo->ShareAddInfo.pwszShareName);
	        break;

	    case NET_SHARE_DEL:

	    	LWUTIL_SAFE_FREE_MEMORY(pCommandInfo->ShareDelInfo.pwszServerName);
	    	LWUTIL_SAFE_FREE_MEMORY(pCommandInfo->ShareDelInfo.pwszShareName);
	        break;

	    case NET_SHARE_ENUM:

	    	LWUTIL_SAFE_FREE_MEMORY(pCommandInfo->ShareEnumInfo.pwszServerName);

	        break;

	     default:
	        break;
	}

	return;
}
