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
 *        netfile.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        File Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

static
DWORD
NetFileParseArguments(
    int    argc,
    char** argv,
    PNET_FILE_COMMAND_INFO* ppCommandInfo
    );

static
VOID
NetFileFreeCommandInfo(
    PNET_FILE_COMMAND_INFO pCommandInfo
    );

VOID
NetFileShowUsage(
    VOID
    )
{
    printf("Usage: lwnet file [<id> [close]]\n");
}

DWORD
NetFileInitialize(
    VOID
    )
{
    return NetApiInitialize();
}

DWORD
NetFile(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;
    PNET_FILE_COMMAND_INFO pCommandInfo = NULL;

    dwError = NetFileParseArguments(argc, argv, &pCommandInfo);
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = NetFileInitialize();
    BAIL_ON_LTNET_ERROR(dwError);

    if (pCommandInfo->bEnumerate)
    {
        dwError = NetExecFileEnum(pCommandInfo->pwszServerName);
    }
    else if (pCommandInfo->bQueryInfo)
    {
        dwError = NetExecFileQueryInfo(
                        pCommandInfo->pwszServerName,
                        pCommandInfo->dwFileId);
    }
    else if (pCommandInfo->bCloseFile)
    {
        dwError = NetExecFileClose(
                        pCommandInfo->pwszServerName,
                        pCommandInfo->dwFileId);
    }
    BAIL_ON_LTNET_ERROR(dwError);

cleanup:

    if (pCommandInfo)
    {
        NetFileFreeCommandInfo(pCommandInfo);
    }

    return dwError;

error:
    goto cleanup;
}

static
DWORD
NetFileParseArguments(
    int    argc,
    char** argv,
    PNET_FILE_COMMAND_INFO* ppCommandInfo
    )
{
    DWORD dwError = 0;
    PNET_FILE_COMMAND_INFO pCommandInfo = NULL;
    int   iArg = 1;
    enum NetFileParseState
    {
        NET_FILE_ARG_OPEN = 0,
        NET_FILE_ARG_FILE,
        NET_FILE_ARG_FILE_ID
    } parseState = NET_FILE_ARG_OPEN;

    dwError = LwNetAllocateMemory(
                    sizeof(*pCommandInfo),
                    (PVOID*)&pCommandInfo);
    BAIL_ON_LTNET_ERROR(dwError);

    for (iArg = 1; iArg < argc; iArg++)
    {
        PSTR  pszArg = argv[iArg];

        switch (parseState)
        {
            case NET_FILE_ARG_OPEN:

                if (!strcasecmp(pszArg, "-h") || !strcasecmp(pszArg, "help"))
                {
                    NetFileShowUsage();
                    goto done;
                }
                else if (!strcasecmp(pszArg, "file"))
                {
                    parseState = NET_FILE_ARG_FILE;

                    pCommandInfo->bEnumerate = TRUE;
                }
                else
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LTNET_ERROR(dwError);
                }

                break;

            case NET_FILE_ARG_FILE:

                if (LwNetCheckUnsignedInteger(pszArg))
                {
                    pCommandInfo->dwFileId = atoi(pszArg);

                    pCommandInfo->bEnumerate = FALSE;
                    pCommandInfo->bQueryInfo = TRUE;

                    parseState = NET_FILE_ARG_FILE_ID;
                }
                else if (!strcasecmp(pszArg, "-h") ||
                         !strcasecmp(pszArg, "--help") ||
                         !strcasecmp(pszArg, "help"))
                {
                    NetFileShowUsage();

                    pCommandInfo->bCloseFile = FALSE;
                    pCommandInfo->bEnumerate = FALSE;
                    pCommandInfo->bQueryInfo = FALSE;

                    goto done;
                }
                else
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LTNET_ERROR(dwError);
                }

                break;

            case NET_FILE_ARG_FILE_ID:

                if (!strcasecmp(pszArg, "close"))
                {
                    pCommandInfo->bQueryInfo = FALSE;
                    pCommandInfo->bCloseFile = TRUE;
                }
                else
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LTNET_ERROR(dwError);
                }

                parseState = NET_FILE_ARG_OPEN;

                break;

            default:

                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LTNET_ERROR(dwError);

                break;
        }
    }

    if (!pCommandInfo->bEnumerate &&
        !pCommandInfo->bQueryInfo &&
        !pCommandInfo->bCloseFile)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LTNET_ERROR(dwError);
    }

done:

    *ppCommandInfo = pCommandInfo;

cleanup:

    return dwError;

error:

    *ppCommandInfo = NULL;

    if (LW_ERROR_INVALID_PARAMETER == dwError)
    {
        NetFileShowUsage();
    }

    if (pCommandInfo)
    {
        LwNetFreeMemory(pCommandInfo);
    }

    goto cleanup;
}

DWORD
NetFileShutdown(
    VOID
    )
{
    return NetApiShutdown();
}

static
VOID
NetFileFreeCommandInfo(
    PNET_FILE_COMMAND_INFO pCommandInfo
    )
{
    LW_SAFE_FREE_MEMORY(pCommandInfo->pwszServerName);
    LW_SAFE_FREE_MEMORY(pCommandInfo);
}
