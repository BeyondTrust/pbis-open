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
 *        Tool to Manage AD Join/Leave/Query
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "includes.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
ParseArgs(
    int                     argc,
    char*                   argv[],
    PLW_DOMAIN_INFO_REQUEST pDomainInfoRequest
    );

static
VOID
ShowUsage();

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
lw_domain_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    LW_DOMAIN_INFO_REQUEST domainInfoRequest = {0};
    size_t  dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR    pszComputerName = NULL;
    PSTR    pszDnsDomainName = NULL;
    PSTR    pszComputerDN = NULL;

    dwError = ParseArgs(
                    argc,
                    argv,
                    &domainInfoRequest);
    BAIL_ON_LSA_ERROR(dwError);

    switch (domainInfoRequest.taskType)
    {
        case LW_DOMAIN_TASK_TYPE_JOIN:

            // TODO: Canonicalize the OU

            dwError = LwDomainJoin(
                            domainInfoRequest.args.joinArgs.pszDomainName,
                            domainInfoRequest.args.joinArgs.pszOU,
                            domainInfoRequest.args.joinArgs.pszUsername,
                            domainInfoRequest.args.joinArgs.pszPassword
                            );
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case LW_DOMAIN_TASK_TYPE_LEAVE:

            dwError = LwDomainLeave(
                            domainInfoRequest.args.joinArgs.pszUsername,
                            domainInfoRequest.args.joinArgs.pszPassword
                            );
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case LW_DOMAIN_TASK_TYPE_QUERY:

            dwError = LwDomainQuery(
                            &pszComputerName,
                            &pszDnsDomainName,
                            &pszComputerDN);
            BAIL_ON_LSA_ERROR(dwError);

            fprintf(stdout,
                    "Name = %s\n",
                    LSA_SAFE_LOG_STRING(pszComputerName));
            fprintf(stdout,
                    "Domain = %s\n",
                    LSA_SAFE_LOG_STRING(pszDnsDomainName));
            fprintf(stdout,
                    "Distinguished Name = %s\n",
                    LSA_SAFE_LOG_STRING(pszComputerDN));

            break;

        default:

            dwError = LW_ERROR_INVALID_PARAMETER;
    }

cleanup:

    LwFreeDomainInfoRequest(&domainInfoRequest);

    LW_SAFE_FREE_STRING(pszComputerName);
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszComputerDN);

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
                        "Failed to enumerate maps.  Error code %u (%s).\n%s",
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
                "Failed to enumerate maps.  Error code %u (%s).\n",
                dwError,
                LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
    }

    dwError = 1;

    goto cleanup;
}

static
DWORD
ParseArgs(
    int                     argc,
    char*                   argv[],
    PLW_DOMAIN_INFO_REQUEST pDomainInfoRequest
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_JOIN_OU
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    LW_DOMAIN_INFO_REQUEST domainInfoRequest = {0};

    do
    {
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
                else if (!strcmp(pszArg, "--join"))
                {
                    if (domainInfoRequest.taskType == LW_DOMAIN_TASK_TYPE_UNKNOWN)
                    {
                        domainInfoRequest.taskType = LW_DOMAIN_TASK_TYPE_JOIN;
                    }
                    else
                    {
                        ShowUsage();
                        exit(1);
                    }
                }
                else if (!strcmp(pszArg, "--leave"))
                {
                    if (domainInfoRequest.taskType == LW_DOMAIN_TASK_TYPE_UNKNOWN)
                    {
                        domainInfoRequest.taskType = LW_DOMAIN_TASK_TYPE_LEAVE;
                    }
                    else
                    {
                        ShowUsage();
                        exit(1);
                    }
                }
                else if (!strcmp(pszArg, "--query"))
                {
                    if (domainInfoRequest.taskType == LW_DOMAIN_TASK_TYPE_UNKNOWN)
                    {
                        domainInfoRequest.taskType = LW_DOMAIN_TASK_TYPE_QUERY;
                    }
                    else
                    {
                        ShowUsage();
                        exit(1);
                    }
                }
                else if (!strcmp(pszArg, "--ou"))
                {
                    if (domainInfoRequest.taskType == LW_DOMAIN_TASK_TYPE_JOIN)
                    {
                        ShowUsage();
                        exit(1);
                    }

                    parseMode = PARSE_MODE_JOIN_OU;
                }
                else
                {
                    if (domainInfoRequest.taskType == LW_DOMAIN_TASK_TYPE_JOIN)
                    {
                        if (LW_IS_NULL_OR_EMPTY_STR(domainInfoRequest.args.joinArgs.pszDomainName))
                        {
                            dwError = LwAllocateString(
                                            pszArg,
                                            &domainInfoRequest.args.joinArgs.pszDomainName);
                            BAIL_ON_LSA_ERROR(dwError);
                        }
                        else if (LW_IS_NULL_OR_EMPTY_STR(domainInfoRequest.args.joinArgs.pszUsername))
                        {
                            dwError = LwAllocateString(
                                            pszArg,
                                            &domainInfoRequest.args.joinArgs.pszUsername);
                            BAIL_ON_LSA_ERROR(dwError);
                        }
                        else if (LW_IS_NULL_OR_EMPTY_STR(domainInfoRequest.args.joinArgs.pszPassword))
                        {
                            dwError = LwAllocateString(
                                            pszArg,
                                            &domainInfoRequest.args.joinArgs.pszPassword);
                            BAIL_ON_LSA_ERROR(dwError);
                        }
                        else
                        {
                            ShowUsage();
                            exit(1);
                        }
                    }
                    else if (domainInfoRequest.taskType == LW_DOMAIN_TASK_TYPE_LEAVE)
                    {
                        if (LW_IS_NULL_OR_EMPTY_STR(domainInfoRequest.args.leaveArgs.pszUsername))
                        {
                            dwError = LwAllocateString(
                                            pszArg,
                                            &domainInfoRequest.args.leaveArgs.pszUsername);
                            BAIL_ON_LSA_ERROR(dwError);
                        }
                        else if (LW_IS_NULL_OR_EMPTY_STR(domainInfoRequest.args.leaveArgs.pszPassword))
                        {
                            dwError = LwAllocateString(
                                            pszArg,
                                            &domainInfoRequest.args.leaveArgs.pszPassword);
                            BAIL_ON_LSA_ERROR(dwError);
                        }
                        else
                        {
                            ShowUsage();
                            exit(1);
                        }
                    }
                    else
                    {
                        ShowUsage();
                        exit(1);
                    }
                }

                break;

            case PARSE_MODE_JOIN_OU:

                if (domainInfoRequest.taskType != LW_DOMAIN_TASK_TYPE_JOIN)
                {
                    ShowUsage();
                    exit(1);
                }

                LW_SAFE_FREE_STRING(domainInfoRequest.args.joinArgs.pszOU);

                dwError = LwAllocateString(
                              pszArg,
                              &domainInfoRequest.args.joinArgs.pszOU);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;
        }

    } while (iArg < argc);

    *pDomainInfoRequest = domainInfoRequest;

cleanup:

    return dwError;

error:

    LwFreeDomainInfoRequest(&domainInfoRequest);

    goto cleanup;
}

static
void
ShowUsage(
    VOID
    )
{
    printf("Usage: lw-domain { --join <options> | --leave <options> | --query }\n\n"
           "Join  options: --ou <organizational unit path> <username> <password>\n"
           "Leave options: {<username> <password>}\n"
          );
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


