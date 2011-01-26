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
 *     lsapstore-test.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Test Itility
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lsa/lsapstore-api.h>
#include "lwargvcursor.h"
#include <lw/rtllog.h>
#include <lwerror.h>
#include <lwstr.h>
#include <lwmem.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "config.h"
#if !defined(HAVE_STRTOLL) && defined(HAVE___STRTOLL)
#define strtoll __strtoll
#endif

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

#define BAIL_ON_LW_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            goto error; \
        } \
    } while (0)


PCSTR
Basename(
    PCSTR pszPath
    )
{
    PSTR pszSlash = strrchr(pszPath, '/');

    if (pszSlash)
    {
        return pszSlash + 1;
    }
    else
    {
        return pszPath;
    }
}

VOID
PrintErrorMessage(
    IN DWORD ErrorCode
    )
{
    PCSTR pszErrorName = LwWin32ExtErrorToName(ErrorCode);
    PSTR pszErrorMessage = NULL;
    DWORD size = LwGetErrorString(ErrorCode, NULL, 0);

    if (size > 0)
    {
        DWORD dwError = LwAllocateMemory(size, OUT_PPVOID(&pszErrorMessage));
        if (!dwError)
        {
            (void) LwGetErrorString(ErrorCode, pszErrorMessage, size);
        }
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszErrorMessage))
    {
        fprintf(stderr,
                "Error code %u (%s).\n%s\n",
                ErrorCode,
                LW_PRINTF_STRING(pszErrorName),
                pszErrorMessage);
    }
    else
    {
        fprintf(stderr,
                "Error code %u (%s).\n",
                ErrorCode,
                LW_PRINTF_STRING(pszErrorName));
    }

    LW_SAFE_FREE_STRING(pszErrorMessage);
}

static
VOID
ShowUsage(
    IN PCSTR pszProgramName,
    IN int ExitCode
    );

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
        else
        {
            LwArgvCursorPop(Cursor);
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

static
BOOLEAN
IsHelpCommand(
    IN PCSTR Command
    )
{
    return !strcmp(Command, "help");
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

static
inline
VOID
PrintSuperUserWarningOnError(
    IN DWORD dwError
    )
{
    if ((dwError == LW_ERROR_ACCESS_DENIED) ||
        (dwError == ERROR_ACCESS_DENIED))
    {
        if (geteuid() != 0)
        {
            fprintf(stderr, "You are not super-user.  Access was denied.\n");
        }
    }
}

static
VOID
PrintAccountInfo(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    printf(""
           "  DNS Domain Name: %s\n"
           "  NetBIOS Domain Name: %s\n"
           "  Domain SID: %s\n"
           "  SAM Account Name: %s\n"
           "  Account Flags: 0x%08x (%u)\n"
           "  Key Version: %u\n"
           "  FQDN: %s\n"
           "  Last Change Time: %lld\n"
           "",
           LW_PRINTF_STRING(pAccountInfo->DnsDomainName),
           LW_PRINTF_STRING(pAccountInfo->NetbiosDomainName),
           LW_PRINTF_STRING(pAccountInfo->DomainSid),
           LW_PRINTF_STRING(pAccountInfo->SamAccountName),
           pAccountInfo->AccountFlags, pAccountInfo->AccountFlags,
           pAccountInfo->KeyVersionNumber,
           LW_PRINTF_STRING(pAccountInfo->Fqdn),
           (long long int) pAccountInfo->LastChangeTime);
}

static
VOID
PrintPasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    PrintAccountInfo(&pPasswordInfo->Account);
    printf("  Password: %s\n", pPasswordInfo->Password);
}

static
DWORD
DoGetPasswordInfo(
    IN OPTIONAL PCSTR pszDnsDomainName
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = LsaPstoreGetPasswordInfoA(pszDnsDomainName, &pPasswordInfo);
    PrintSuperUserWarningOnError(dwError);
    BAIL_ON_LW_ERROR(dwError);

    printf("Machine Password Info:\n");
    PrintPasswordInfo(pPasswordInfo);

error:
    if (pPasswordInfo)
    {
        LsaPstoreFreePasswordInfoA(pPasswordInfo);
    }

    return dwError;
}

static
DWORD
DoSetPasswordInfo(
    IN PCSTR DnsDomainName,
    IN PCSTR NetbiosDomainName,
    IN PCSTR DomainSid,
    IN PCSTR SamAccountName,
    IN LSA_MACHINE_ACCOUNT_FLAGS AccountFlags,
    IN DWORD KeyVersionNumber,
    IN PCSTR Fqdn,
    IN LONG64 LastChangeTime,
    IN PCSTR Password
    )
{
    DWORD dwError = 0;
    LSA_MACHINE_PASSWORD_INFO_A passwordInfo = { { 0 } };

    passwordInfo.Account.DnsDomainName = (PSTR) DnsDomainName;
    passwordInfo.Account.NetbiosDomainName = (PSTR) NetbiosDomainName;
    passwordInfo.Account.DomainSid = (PSTR) DomainSid;
    passwordInfo.Account.SamAccountName = (PSTR) SamAccountName;
    passwordInfo.Account.AccountFlags = AccountFlags;
    passwordInfo.Account.KeyVersionNumber = KeyVersionNumber;
    passwordInfo.Account.Fqdn = (PSTR) Fqdn;
    passwordInfo.Account.LastChangeTime = LastChangeTime;
    passwordInfo.Password = (PSTR) Password;

    printf("Setting Machine Password Info:\n");
    PrintPasswordInfo(&passwordInfo);

    dwError = LsaPstoreSetPasswordInfoA(&passwordInfo);
    PrintSuperUserWarningOnError(dwError);
    BAIL_ON_LW_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
DoDeletePasswordInfo(
    IN OPTIONAL PCSTR pszDnsDomainName
    )
{
    DWORD dwError = 0;

    dwError = LsaPstoreDeletePasswordInfoA(pszDnsDomainName);
    PrintSuperUserWarningOnError(dwError);
    BAIL_ON_LW_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
DoGetDefaultDomain(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR defaultDomain = NULL;

    dwError = LsaPstoreGetDefaultDomainA(&defaultDomain);
    PrintSuperUserWarningOnError(dwError);
    BAIL_ON_LW_ERROR(dwError);

    printf("Default Domain: %s\n", LW_PRINTF_STRING(defaultDomain));

error:
    LSA_PSTORE_FREE(&defaultDomain);

    return dwError;
}

static
DWORD
DoSetDefaultDomain(
    IN PCSTR DefaultDomain
    )
{
    DWORD dwError = 0;

    printf("Setting Default Domain to: %s\n", LW_PRINTF_STRING(DefaultDomain));

    dwError = LsaPstoreSetDefaultDomainA(DefaultDomain);
    PrintSuperUserWarningOnError(dwError);
    BAIL_ON_LW_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
DoGetJoinedDomains(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR* joinedDomains = NULL;
    DWORD count = 0;
    DWORD i = 0;

    dwError = LsaPstoreGetJoinedDomainsA(&joinedDomains, &count);
    PrintSuperUserWarningOnError(dwError);
    BAIL_ON_LW_ERROR(dwError);

    printf("Joined %d domain(s):\n", count);
    for (i = 0; i < count; i++)
    {
        printf("%d - %s\n", i, joinedDomains[i]);
    }

error:
    if (joinedDomains)
    {
        LsaPstoreFreeStringArrayA(joinedDomains, count);
    }

    return dwError;
}

static
VOID
LogCallback(
    IN OPTIONAL LW_PVOID Context,
    IN LW_RTL_LOG_LEVEL Level,
    IN OPTIONAL PCSTR ComponentName,
    IN PCSTR FunctionName,
    IN PCSTR FileName,
    IN ULONG LineNumber,
    IN PCSTR Format,
    IN ...
    )
{
    DWORD dwError = 0;
    PSTR formattedMessage = NULL;
    va_list argList;
    PCSTR levelString = NULL;
    size_t messageLength = 0;
    PCSTR optionalNewLine = NULL;

    va_start(argList, Format);
    dwError = LwAllocateStringPrintfV(&formattedMessage, Format, argList);
    va_end(argList);
    if (dwError)
    {
        goto error;
    }

    switch (Level)
    {
        case LW_RTL_LOG_LEVEL_ALWAYS:
            levelString = "ALWAYS";
            break;
        case LW_RTL_LOG_LEVEL_ERROR:
            levelString = "ERROR";
            break;
        case LW_RTL_LOG_LEVEL_WARNING:
            levelString = "WARNING";
            break;
        case LW_RTL_LOG_LEVEL_INFO:
            levelString = "INFO";
            break;
        case LW_RTL_LOG_LEVEL_VERBOSE:
            levelString = "VERBOSE";
            break;
        case LW_RTL_LOG_LEVEL_DEBUG:
            levelString = "DEBUG";
            break;
        case LW_RTL_LOG_LEVEL_TRACE:
            levelString = "TRACE";
            break;
        default:
            levelString = NULL;
            break;
    }

    messageLength = strlen(formattedMessage);
    if (!messageLength || formattedMessage[messageLength-1] != '\n')
    {
        optionalNewLine = "\n";
    }

    printf("%s: [%s() %s:%d] %s%s",
           LW_RTL_LOG_SAFE_STRING(levelString),
           FunctionName,
           FileName,
           LineNumber,
           formattedMessage,
           optionalNewLine);

error:
    if (dwError)
    {
        printf("WARNING: Failed to format log message");
    }

    LW_SAFE_FREE_STRING(formattedMessage);
}

static
DWORD
ParseLONG64(
    IN PCSTR pszString,
    OUT PLONG64 pResult
    )
{
    DWORD dwError = 0;
    LONG64 result = 0;
    PSTR pszEnd = NULL;

    result = strtoll(pszString, &pszEnd, 10);
    if (!pszEnd || pszEnd == pszString || *pszEnd)
    {
        dwError = ERROR_INVALID_PARAMETER;
        result = 0;
    }

    *pResult = result;

    return dwError;
}

static
DWORD
ParseULONG(
    IN PCSTR pszString,
    OUT PULONG pResult
    )
{
    DWORD dwError = 0;
    LONG64 result = 0;

    dwError = ParseLONG64(pszString, &result);
    if (!dwError)
    {
        if (result < 0 || result > MAXULONG)
        {
            dwError = ERROR_INVALID_PARAMETER;
            result = 0;
        }
    }

    *pResult = (ULONG) result;

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
    printf("Usage: %s [OPTIONS] get-password-info [DNS-DOMAIN-NAME]\n"
           "       %s [OPTIONS] set-password-info <ARGS...>\n"
           "       %s [OPTIONS] delete-password-info [DNS-DOMAIN-NAME]\n"
           "       %s [OPTIONS] get-default-domain\n"
           "       %s [OPTIONS] set-default-domain DNS-DOMAIN-NAME\n"
           "       %s [OPTIONS] get-joined-domains\n"
           "\n"
           "  where OPTIONS are:\n"
           "\n"
           "    -d    -- enable debug output\n"
           "\n"
           "  where ARGS for set-password-info are:\n"
           "\n"
           "    DNS-DOMAIN-NAME\n"
           "    NETBIOS-DOMAIN-NAME\n"
           "    DOMAIN-SID\n"
           "    SAM-ACCOUNT-NAME (e.g., COMPUTER$)\n"
           "    ACCOUNT-FLAGS (1 for regular workstation join)\n"
           "    KEY-VERSION-NUMBER (e.g., 1)\n"
           "    FQDN (e.g., computer.ad.example.com)\n"
           "    LAST-CHANGE-TIME\n"
           "    PASSWORD\n"
           "\n"
           "  for example:\n"
           "\n"
           "    %s set-password-info AD.EXAMPLE.COM EXAMPLE S-1-5-123-456-789 COMPUTER$ 1 1 computer.example.com 0 SECRET\n"
           "",
           pszUseProgramName,
           pszUseProgramName,
           pszUseProgramName,
           pszUseProgramName,
           pszUseProgramName,
           pszUseProgramName,
           pszUseProgramName);
    exit(ExitCode);
}

int
main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError = 0;
    PCSTR programName = NULL;
    PCSTR option = NULL;
    PCSTR command = NULL;
    LW_ARGV_CURSOR cursor;
    BOOLEAN enableDebug = FALSE;

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
        else if (!strcmp(option, "-d"))
        {
            enableDebug = TRUE;
        }
        else
        {
            fprintf(stderr, "Unrecognized option: %s\n", option);
            ShowUsageError(programName);
        }
    }

    if (enableDebug)
    {
        LwRtlLogSetLevel(LW_RTL_LOG_LEVEL_DEBUG);
        LwRtlLogSetCallback(LogCallback, NULL);
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
    else if (!strcmp(command, "get-password-info"))
    {
        PCSTR domainName = LwArgvCursorPop(&cursor);
        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }
        dwError = DoGetPasswordInfo(domainName);
        BAIL_ON_LW_ERROR(dwError);
    }
    else if (!strcmp(command, "set-password-info"))
    {
        PCSTR dnsDomainName = LwArgvCursorPop(&cursor);
        PCSTR netbiosDomainName = LwArgvCursorPop(&cursor);
        PCSTR domainSid = LwArgvCursorPop(&cursor);
        PCSTR samAccountName = LwArgvCursorPop(&cursor);
        PCSTR accountFlagsString = LwArgvCursorPop(&cursor);
        LSA_MACHINE_ACCOUNT_FLAGS accountFlags = 0;
        PCSTR keyVersionNumberString = LwArgvCursorPop(&cursor);
        DWORD keyVersionNumber = 0;
        PCSTR fqdn = LwArgvCursorPop(&cursor);
        PCSTR lastChangeTimeString = LwArgvCursorPop(&cursor);
        LONG64 lastChangeTime = 0;
        PCSTR password = LwArgvCursorPop(&cursor);

        if (!dnsDomainName)
        {
            fprintf(stderr, "Missing DNS-DOMAIN-NAME argument.\n");
            ShowUsageError(programName);
        }
        if (!netbiosDomainName)
        {
            fprintf(stderr, "Missing NETBIOS-DOMAIN-NAME argument.\n");
            ShowUsageError(programName);
        }
        if (!domainSid)
        {
            fprintf(stderr, "Missing DOMAIN-SID argument.\n");
            ShowUsageError(programName);
        }
        if (!samAccountName)
        {
            fprintf(stderr, "Missing SAM-ACCOUNT-NAME argument.\n");
            ShowUsageError(programName);
        }
        if (!accountFlagsString)
        {
            fprintf(stderr, "Missing ACCOUNT-FLAGS argument.\n");
            ShowUsageError(programName);
        }
        if (ParseULONG(accountFlagsString, &accountFlags))
        {
            fprintf(stderr, "ACCOUNT-FLAGS must be a valid ULONG instead of %s.\n", accountFlagsString);
            ShowUsageError(programName);
        }
        if (!keyVersionNumberString)
        {
            fprintf(stderr, "Missing KEY-VERSION-NUMBER argument.\n");
            ShowUsageError(programName);
        }
        if (ParseULONG(keyVersionNumberString, &keyVersionNumber))
        {
            fprintf(stderr, "KEY-VERSION-NUMBER must be a valid ULONG instead of %s.\n", keyVersionNumberString);
            ShowUsageError(programName);
        }
        if (!fqdn)
        {
            fprintf(stderr, "Missing FQDN argument.\n");
            ShowUsageError(programName);
        }
        if (!lastChangeTimeString)
        {
            fprintf(stderr, "Missing LAST-CHANGE-TIME argument.\n");
            ShowUsageError(programName);
        }
        if (ParseLONG64(lastChangeTimeString, &lastChangeTime))
        {
            fprintf(stderr, "LAST-CHANGE-TIME must be a valid LONG64 instead of %s.\n", lastChangeTimeString);
            ShowUsageError(programName);
        }
        if (!password)
        {
            fprintf(stderr, "Missing PASSWORD argument.\n");
            ShowUsageError(programName);
        }

        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }

        dwError = DoSetPasswordInfo(
                        dnsDomainName,
                        netbiosDomainName,
                        domainSid,
                        samAccountName,
                        accountFlags,
                        keyVersionNumber,
                        fqdn,
                        lastChangeTime,
                        password);
        BAIL_ON_LW_ERROR(dwError);
    }
    else if (!strcmp(command, "delete-password-info"))
    {
        PCSTR domainName = LwArgvCursorPop(&cursor);
        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }
        dwError = DoDeletePasswordInfo(domainName);
        BAIL_ON_LW_ERROR(dwError);
    }
    else if (!strcmp(command, "get-default-domain"))
    {
        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }
        dwError = DoGetDefaultDomain();
        BAIL_ON_LW_ERROR(dwError);
    }
    else if (!strcmp(command, "set-default-domain"))
    {
        PCSTR domainName = LwArgvCursorPop(&cursor);
        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }
        dwError = DoSetDefaultDomain(domainName);
        BAIL_ON_LW_ERROR(dwError);
    }
    else if (!strcmp(command, "get-joined-domains"))
    {
        if (LwArgvCursorRemaining(&cursor))
        {
            fprintf(stderr, "Too many arguments.\n");
            ShowUsageError(programName);
        }
        dwError = DoGetJoinedDomains();
        BAIL_ON_LW_ERROR(dwError);
    }
    else
    {
        fprintf(stderr, "Unrecognized command: %s\n", command);
        ShowUsageError(programName);
    }

error:
    if (dwError)
    {
        PrintErrorMessage(dwError);
    }

    return dwError ? 1 : 0;
}
