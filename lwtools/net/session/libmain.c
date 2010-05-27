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
 *        netsession.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        Session Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

static
DWORD
NetSessionParseArguments(
    int argc,
    char ** argv,
    PNET_SESSION_COMMAND_INFO* ppCommandInfo
    );

static
VOID
NetSessionFreeCommandInfo(
    PNET_SESSION_COMMAND_INFO pCommandInfo
    );

VOID
NetSessionShowUsage(
    VOID
    )
{
    printf("Usage: lwnet session [\\\\computername] [del | delete]\n");
}


DWORD
NetSessionInitialize(
    VOID
    )
{
    return NetApiInitialize();
}

DWORD
NetSession(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;
    PNET_SESSION_COMMAND_INFO pCommandInfo = NULL;

    dwError = NetSessionParseArguments(
                    argc,
                    argv,
                    &pCommandInfo
                    );
    BAIL_ON_LTNET_ERROR(dwError);

    dwError = NetSessionInitialize();
    BAIL_ON_LTNET_ERROR(dwError);

    if (pCommandInfo->bEnumerate)
    {
        dwError = NetExecSessionEnum(
                        pCommandInfo->pwszServername,
                        pCommandInfo->pwszClientname);
    }
    else if (pCommandInfo->bLogoff)
    {
        dwError = NetExecSessionLogoff(
                        pCommandInfo->pwszServername,
                        pCommandInfo->pwszClientname);
    }
    BAIL_ON_LTNET_ERROR(dwError);

cleanup:

    if (pCommandInfo)
    {
        NetSessionFreeCommandInfo(pCommandInfo);
    }

    return dwError;

error:

    fprintf(stderr,
            "Error [%u][%s] %s\n",
            dwError,
            LwWin32ExtErrorToName(dwError),
            LwWin32ExtErrorToDescription(dwError));

    goto cleanup;
}

DWORD
NetSessionShutdown(
    VOID
    )
{
    return NetApiShutdown();
}

static
DWORD
NetSessionParseArguments(
    int argc,
    char ** argv,
    PNET_SESSION_COMMAND_INFO* ppCommandInfo
    )
{
    DWORD dwError = 0;
    PNET_SESSION_COMMAND_INFO pCommandInfo = NULL;
    int   iArg = 1;
    enum NetSessionParseState
    {
        NET_SESSION_ARG_OPEN = 0,
        NET_SESSION_ARG_SESSION,
        NET_SESSION_ARG_NAME
    } parseState = NET_SESSION_ARG_OPEN;

    dwError = LwNetAllocateMemory(
                    sizeof(*pCommandInfo),
                    (PVOID*)&pCommandInfo);
    BAIL_ON_LTNET_ERROR(dwError);

    for (iArg = 1; iArg < argc; iArg++)
    {
        PSTR  pszArg = argv[iArg];

        switch (parseState)
        {
            case NET_SESSION_ARG_OPEN:

                if (!strcasecmp(pszArg, "-h") || !strcasecmp(pszArg, "--help"))
                {
                    NetSessionShowUsage();
                    goto done;
                }
                else if (!strcasecmp(pszArg, "session"))
                {
                    parseState = NET_SESSION_ARG_SESSION;

                    pCommandInfo->bEnumerate = TRUE;
                }
                else
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LTNET_ERROR(dwError);
                }

                break;

            case NET_SESSION_ARG_SESSION:

                if (!strncmp(pszArg, "\\\\", sizeof("\\\\")-1))
                {
                    dwError = LwMbsToWc16s(pszArg, &pCommandInfo->pwszClientname);
                    BAIL_ON_LTNET_ERROR(dwError);

                    parseState = NET_SESSION_ARG_NAME;
                }
                else if (!strncmp(pszArg, "//", sizeof("//")-1))
                {
                    wchar16_t wszPrefix[] = {'\\', '\\', 0};

                    dwError = LwMbsToWc16s(pszArg, &pCommandInfo->pwszClientname);
                    BAIL_ON_LTNET_ERROR(dwError);

                    memcpy( (PBYTE)pCommandInfo->pwszClientname,
                            (PBYTE)&wszPrefix[0],
                            (sizeof(wszPrefix)/sizeof(wszPrefix[0])-1) * sizeof(wszPrefix[0]));

                    parseState = NET_SESSION_ARG_NAME;
                }
                else if (!strcasecmp(pszArg, "-h") ||
                         !strcasecmp(pszArg, "--help") ||
                         !strcasecmp(pszArg, "help"))
                {
                    NetSessionShowUsage();

                    pCommandInfo->bEnumerate = FALSE;
                    pCommandInfo->bLogoff    = FALSE;

                    goto done;
                }
                else
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LTNET_ERROR(dwError);
                }

                break;

            case NET_SESSION_ARG_NAME:

                if (!strcasecmp(pszArg, "del") || !strcasecmp(pszArg, "delete"))
                {
                    pCommandInfo->bEnumerate = FALSE;
                    pCommandInfo->bLogoff = TRUE;
                }
                else
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LTNET_ERROR(dwError);
                }

                parseState = NET_SESSION_ARG_OPEN;

                break;

            default:

                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LTNET_ERROR(dwError);

                break;
        }
    }

    if (!pCommandInfo->bEnumerate && !pCommandInfo->bLogoff)
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
        NetSessionShowUsage();
    }

    if (pCommandInfo)
    {
        LwNetFreeMemory(pCommandInfo);
    }

    goto cleanup;
}

static
VOID
NetSessionFreeCommandInfo(
    PNET_SESSION_COMMAND_INFO pCommandInfo
    )
{
    LW_SAFE_FREE_MEMORY(pCommandInfo->pwszServername);
    LW_SAFE_FREE_MEMORY(pCommandInfo->pwszClientname);
    LW_SAFE_FREE_MEMORY(pCommandInfo);
}
