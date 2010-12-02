/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        FILENAME
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        AD Provider Get Machine Info
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include <lsa/ad.h>
#include "common.h"
#include <stdarg.h>
#include "lsautils.h"
#include <lwpwdinfo.h>
#include <stdio.h>
#include <string.h>

static
DWORD
ParseArgs(
    IN int argc,
    IN PCSTR argv[],
    OUT PCSTR* ppszDomainName,
    OUT PBOOLEAN PrintPassword
    );

static
VOID
ShowUsageError(
    IN PCSTR pszProgramName
    );

static
VOID
ShowUsageHelp(
    IN PCSTR pszProgramName
    );

int
ad_get_machine_main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = NULL;
    PCSTR pszDomainName = NULL;
    BOOLEAN printPassword = FALSE;
    PLWPS_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = ParseArgs(argc, argv, &pszDomainName, &printPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdGetMachinePassword(hLsaConnection, pszDomainName, &pPasswordInfo);
    if ((dwError == LW_ERROR_ACCESS_DENIED) ||
        (dwError == ERROR_ACCESS_DENIED))
    {
        if (geteuid() != 0)
        {
            fprintf(stderr, "You are not super-user.  Access was denied.\n");
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    printf("Machine Password Information:\n"
           "  DNS Domain Name: %s\n"
           "  NetBIOS Domain Name: %s\n"
           "  Domain SID: %s\n"
           "  SAM Account Name: %s\n"
           "  FQDN: %s.%s\n"
           "  Join Type: %u\n"
           "  Last Change Time: %d\n"
           "",
           pPasswordInfo->pszDnsDomainName,
           pPasswordInfo->pszDomainName,
           pPasswordInfo->pszSID,
           pPasswordInfo->pszMachineAccount,
           pPasswordInfo->pszHostname, pPasswordInfo->pszHostDnsDomain,
           pPasswordInfo->dwSchannelType,
           (int) pPasswordInfo->last_change_time);

    if (printPassword)
    {
        printf("  Password: %s\n", pPasswordInfo->pszMachinePassword);
    }

error:
    if (dwError)
    {
        PrintErrorMessage(dwError);
    }

    if (pPasswordInfo)
    {
        LsaAdFreeMachinePassword(pPasswordInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;
}

static
PCSTR
PopNextOption(
    IN OUT PLW_ARGV_CURSOR Cursor
    )
{
    PCSTR option = LwArgvCursorPeek(Cursor);

    if (option)
    {
        if (!strcmp("--", option))
        {
            option = NULL;
            LwArgvCursorPop(Cursor);
        }
        else if (strncmp("-", option, 1))
        {
            option = NULL;
        }
    }

    return option;
}

static
BOOLEAN
IsHelpOption(
    IN PCSTR Option
    )
{
    return (!strcmp(Option, "--help") ||
            !strcmp(Option, "-h") ||
            !strcmp(Option, "-?"));
}

BOOLEAN
IsHelpCommand(
    IN PCSTR Command
    )
{
    return !strcmp(Command, "help");
}

static
DWORD
ParseArgs(
    IN int argc,
    IN PCSTR argv[],
    OUT PCSTR* ppszDomainName,
    OUT PBOOLEAN PrintPassword
    )
{
    DWORD dwError = 0;
    PCSTR programName = NULL;
    PCSTR option = NULL;
    PCSTR command = NULL;
    PCSTR domainName = NULL;
    LW_ARGV_CURSOR cursor;
    BOOLEAN printPassword = FALSE;

    LwArgvCursorInit(&cursor, argc, argv);
    programName = LwArgvCursorPop(&cursor);

    // Process Options:

    for (;;)
    {
        option = PopNextOption(&cursor);
        if (!option)
        {
            break;
        }
        else if (IsHelpOption(option))
        {
            ShowUsageHelp(programName);
        }
        else
        {
            fprintf(stderr, "Unrecognized option: %s\n", option);
            ShowUsageError(programName);
        }
    }

    // Process Command:

    command = LwArgvCursorPop(&cursor);
    if (!command)
    {
        fprintf(stderr, "Missing command.\n");
        ShowUsageError(programName);
    }
    else if (IsHelpCommand(command))
    {
        ShowUsageHelp(programName);
    }
    else if (!strcmp(command, "account"))
    {
        printPassword = FALSE;
        domainName = LwArgvCursorPop(&cursor);
        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }
    }
    else if (!strcmp(command, "password"))
    {
        printPassword = TRUE;
        domainName = LwArgvCursorPop(&cursor);
        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }
    }
    else
    {
        fprintf(stderr, "Unrecognized command: %s\n", command);
        ShowUsageError(programName);
    }

    *ppszDomainName = domainName;
    *PrintPassword = printPassword;

    return dwError;
}

static
VOID
ShowUsage(
    IN PCSTR pszProgramName,
    IN int ExitCode
    )
{
    PCSTR pszUseProgramName = Basename(pszProgramName);
    printf("Usage: %s <account | password> [DNS-DOMAIN-NAME]\n", pszUseProgramName);
    exit(ExitCode);
}

static
VOID
ShowUsageError(
    IN PCSTR pszProgramName
    )
{
    ShowUsage(pszProgramName, 1);
}

static
VOID
ShowUsageHelp(
    IN PCSTR pszProgramName
    )
{
    ShowUsage(pszProgramName, 0);
}
