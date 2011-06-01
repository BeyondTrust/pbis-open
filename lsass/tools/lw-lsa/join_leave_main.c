/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2010
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

#include <lw/rtlgoto.h>
#include <lw/errno.h>
#include <lsa/ad.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/utsname.h>
#include <errno.h>
#include <lwstr.h>
#include <lwmem.h>
#include "common.h"
#include <assert.h>

//
// Common parse/usage stuff
//

static
VOID
ShowJoinUsage(
    IN PCSTR pszProgramName,
    IN int ExitCode
    );

static
VOID
ShowLeaveUsage(
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

typedef VOID (*SHOW_USAGE_CALLBACK)(IN PCSTR pszProgramName);

static
VOID
ShowJoinUsageError(
    IN PCSTR pszProgramName
    )
{
    ShowJoinUsage(pszProgramName, 1);
}

static
VOID
ShowJoinUsageHelp(
    IN PCSTR pszProgramName
    )
{
    ShowJoinUsage(pszProgramName, 0);
}

static
VOID
ShowLeaveUsageError(
    IN PCSTR pszProgramName
    )
{
    ShowLeaveUsage(pszProgramName, 1);
}

static
VOID
ShowLeaveUsageHelp(
    IN PCSTR pszProgramName
    )
{
    ShowLeaveUsage(pszProgramName, 0);
}

//
// Specfic to this utility
//

typedef struct _JOIN_ARGS {
    PSTR pszDomain;
    PSTR pszUsername;
    PSTR pszPassword;
    PSTR pszMachineName;
    PSTR pszDnsSuffix;
    PSTR pszOu;
    PSTR pszOsName;
    PSTR pszOsVersion;
    PSTR pszOsServicePack;
    LSA_NET_JOIN_FLAGS JoinFlags;
} JOIN_ARGS, *PJOIN_ARGS;

typedef struct _LEAVE_ARGS {
    PSTR pszDomain;
    PSTR pszUsername;
    PSTR pszPassword;
    LSA_NET_JOIN_FLAGS JoinFlags;
} LEAVE_ARGS, *PLEAVE_ARGS;

static
VOID
FreeJoinArgsContents(
    IN OUT PJOIN_ARGS pArgs
    )
{
    LW_SAFE_FREE_MEMORY(pArgs->pszDomain);
    LW_SAFE_FREE_MEMORY(pArgs->pszUsername);
    LW_SAFE_FREE_MEMORY(pArgs->pszPassword);
    LW_SAFE_FREE_MEMORY(pArgs->pszMachineName);
    LW_SAFE_FREE_MEMORY(pArgs->pszDnsSuffix);
    LW_SAFE_FREE_MEMORY(pArgs->pszOu);
    LW_SAFE_FREE_MEMORY(pArgs->pszOsName);
    LW_SAFE_FREE_MEMORY(pArgs->pszOsVersion);
    LW_SAFE_FREE_MEMORY(pArgs->pszOsServicePack);
    pArgs->JoinFlags = 0;
}

static
VOID
FreeLeaveArgsContents(
    IN OUT PLEAVE_ARGS pArgs
    )
{
    LW_SAFE_FREE_MEMORY(pArgs->pszDomain);
    LW_SAFE_FREE_MEMORY(pArgs->pszUsername);
    LW_SAFE_FREE_MEMORY(pArgs->pszPassword);
    pArgs->JoinFlags = 0;
}

static
PCSTR
GetErrorString(
    IN DWORD dwError
    )
{
    PCSTR pszError = LwWin32ExtErrorToDescription(dwError);
    if (!pszError)
    {
        pszError = LwWin32ExtErrorToName(dwError);
    }
    if (!pszError)
    {
        pszError = "UNKNOWN";
    }
    return pszError;
}

static
VOID
PrintError(
    IN DWORD dwError
    )
{
    PCSTR pszError = GetErrorString(dwError);
    fprintf(stderr, "ERROR: %s (%d) (0x%08x)\n",
            pszError, dwError, dwError);
}

static
DWORD
GetCurrentDomain(
    OUT PSTR* ppszDnsDomainName
    )
{
    DWORD dwError = 0;
    HANDLE hLsa = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;
    PSTR pszDnsDomainName = NULL;

    dwError = LsaOpenServer(&hLsa);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    dwError = LsaAdGetMachineAccountInfo(hLsa, NULL, &pAccountInfo);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    dwError = LwAllocateString(
                    pAccountInfo->DnsDomainName,
                    &pszDnsDomainName);
    GOTO_CLEANUP_ON_WINERROR(dwError);

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    }

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    if (pAccountInfo)
    {
        LsaAdFreeMachineAccountInfo(pAccountInfo);
    }

    *ppszDnsDomainName = pszDnsDomainName;

    return dwError;
}

static
DWORD
GetHostname(
    OUT PSTR* ppszHostname
    )
{
    DWORD dwError = 0;
    CHAR buffer[1024] = { 0 };
    PSTR dot = NULL;
    PSTR pszHostname = NULL;

    if (gethostname(buffer, sizeof(buffer) - 1) == -1)
    {
        dwError = LwErrnoToWin32Error(errno);
        assert(dwError);
        GOTO_CLEANUP();
    }

    dot = strchr(buffer, '.');
    if (dot)
    {
        *dot = 0;
    }

    dwError = LwAllocateString(buffer, &pszHostname);
    GOTO_CLEANUP_ON_WINERROR(dwError);

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszHostname);
    }

    *ppszHostname = pszHostname;

    return dwError;
}

static
DWORD
GetOsInfo(
    OUT OPTIONAL PSTR* ppszOsName,
    OUT OPTIONAL PSTR* ppszOsVersion,
    OUT OPTIONAL PSTR* ppszOsServicePack
    )
{
    DWORD dwError = 0;
    struct utsname utsBuffer = { { 0 } };
    PSTR pszOsName = NULL;
    PSTR pszOsVersion = NULL;
    PSTR pszOsServicePack = NULL;
    
    if (uname(&utsBuffer) == -1)
    {
        dwError = LwErrnoToWin32Error(errno);
        assert(dwError);
        GOTO_CLEANUP();
    }

    if (ppszOsName)
    {
        dwError = LwAllocateString(utsBuffer.sysname, &pszOsName);
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

    if (ppszOsVersion)
    {
        if (!strcmp(utsBuffer.sysname, "AIX"))
        {
            dwError = LwAllocateStringPrintf(
                            &pszOsVersion,
                            "%s.%s",
                            utsBuffer.version, utsBuffer.release);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else
        {
            dwError = LwAllocateString(utsBuffer.release, &pszOsVersion);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
    }

    if (ppszOsServicePack)
    {
        if (!strcmp(utsBuffer.sysname, "AIX"))
        {
            dwError = LwAllocateString("", &pszOsServicePack);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else
        {
            dwError = LwAllocateString(utsBuffer.version, &pszOsServicePack);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
    }

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszOsName);
        LW_SAFE_FREE_STRING(pszOsVersion);
        LW_SAFE_FREE_STRING(pszOsServicePack);
    }

    if (ppszOsName)
    {
        *ppszOsName = pszOsName;
    }
    if (ppszOsVersion)
    {
        *ppszOsVersion = pszOsVersion;
    }
    if (ppszOsServicePack)
    {
        *ppszOsServicePack = pszOsServicePack;
    }

    return dwError;
}

static
DWORD
DoJoinDomain(
    IN PCSTR pszDomain,
    IN PCSTR pszUsername,
    IN PCSTR pszPassword,
    IN PCSTR pszMachineName,
    IN PCSTR pszDnsSuffix,
    IN OPTIONAL PCSTR pszOu,
    IN PCSTR pszOsName,
    IN PCSTR pszOsVersion,
    IN PCSTR pszOsServicePack,
    IN LSA_NET_JOIN_FLAGS JoinFlags
    )
{
    DWORD dwError = 0;
    HANDLE hLsa = NULL;
    PSTR pszCurrentDomain = NULL;

    assert(pszDomain);
    assert(pszUsername);
    assert(pszPassword);
    assert(pszMachineName);
    assert(pszDnsSuffix);
    assert(pszOsName);
    assert(pszOsVersion);
    assert(pszOsServicePack);

    printf("Joining to AD Domain: %s\n"
           "With Computer DNS Name: %s.%s\n\n",
           pszDomain,
           pszMachineName,
           pszDnsSuffix);

    dwError = LsaOpenServer(&hLsa);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    dwError = LsaAdJoinDomain(
                    hLsa,
                    pszMachineName,
                    pszDnsSuffix,
                    pszDomain,
                    pszOu,
                    pszUsername,
                    pszPassword,
                    pszOsName,
                    pszOsVersion,
                    pszOsServicePack,
                    JoinFlags);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    printf("SUCCESS!\n");

    dwError = GetCurrentDomain(&pszCurrentDomain);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    printf("Your computer is now joined to '%s'\n", pszCurrentDomain);

cleanup:
    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    LW_SAFE_FREE_MEMORY(pszCurrentDomain);

    return dwError;
}

static
DWORD
DoLeaveDomain(
    IN PCSTR pszDomain,
    IN OPTIONAL PCSTR pszUsername,
    IN OPTIONAL PCSTR pszPassword,
    IN LSA_NET_JOIN_FLAGS JoinFlags
    )
{
    HANDLE hLsa = NULL;
    DWORD dwError = 0;
    PCSTR pszUseDomain = IsSetFlag(JoinFlags, LSA_NET_JOIN_DOMAIN_MULTIPLE) ? pszDomain : NULL;

    assert(pszDomain);

    printf("Leaving AD Domain: %s\n", pszDomain);

    dwError = LsaOpenServer(&hLsa);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    dwError = LsaAdLeaveDomain2(hLsa, pszUsername, pszPassword, pszUseDomain, JoinFlags);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    printf("SUCCESS\n");

cleanup:
    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    return dwError;
}


static
DWORD
CleanupUsername(
    IN PCSTR pszDomain,
    IN OUT PSTR* ppszUsername
    )
{
    DWORD dwError = 0;
    PSTR pszOldUsername = *ppszUsername;
    PSTR pszNewUsername = NULL;

    // Fix up username and prompt for password, if needed.
    if (pszOldUsername)
    {
        PSTR at = NULL;

        if (strchr(pszOldUsername, '\\'))
        {
            fprintf(stderr,
                    "The USERNAME (%s) is not allowed to contain a backslash.\n",
                    pszOldUsername);
            exit(1);
        }

        at = strchr(pszOldUsername, '@');
        if (at)
        {
            LwStrToUpper(at);
        }
        else
        {
            dwError = LwAllocateStringPrintf(
                            &pszNewUsername,
                            "%s@%s",
                            pszOldUsername,
                            pszDomain);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
    }

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(pszNewUsername);
    }

    if (pszNewUsername)
    {
        LW_SAFE_FREE_MEMORY(*ppszUsername);
        *ppszUsername = pszNewUsername;
    }

    return dwError;
}

static
DWORD
GetPassword(
    OUT PSTR* ppszPassword
    )
{
    DWORD dwError = 0;
    CHAR buffer[129] = { 0 };
    int i = 0;
    BOOLEAN needReset = FALSE;
    struct termios oldAttributes;
    struct termios newAttributes;
    PSTR pszPassword = NULL;

    if (tcgetattr(0, &oldAttributes) == -1)
    {
        dwError = LwErrnoToWin32Error(errno);
        assert(dwError);
        GOTO_CLEANUP();
    }

    newAttributes = oldAttributes;
    ClearFlag(newAttributes.c_lflag, ECHO);

    if (tcsetattr(0, TCSANOW, &newAttributes) == -1)
    {
        dwError = LwErrnoToWin32Error(errno);
        assert(dwError);
        GOTO_CLEANUP();
    }

    needReset = TRUE;

    for (i = 0; i < (sizeof(buffer) - 1); i++)
    {
        if (read(0, &buffer[i], 1))
        {
            if (buffer[i] == '\n')
            {
                buffer[i] = 0;
                break;
            }
        }
        else
        {
            dwError = LwErrnoToWin32Error(errno);
            assert(dwError);
            GOTO_CLEANUP();
        }
    }

    if (i == (sizeof(buffer) - 1))
    {
        dwError = LwErrnoToWin32Error(ENOBUFS);
        assert(dwError);
        GOTO_CLEANUP();
    }

    dwError = LwAllocateString(buffer, &pszPassword);
    GOTO_CLEANUP_ON_WINERROR(dwError);

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(pszPassword);
    }

    if (needReset)
    {
        if (tcsetattr(0, TCSANOW, &oldAttributes) == -1)
        {
            assert(0);
        }
    }

    *ppszPassword = pszPassword;

    return dwError;
}

static
DWORD
PromptPassword(
    IN PCSTR pszUsername,
    OUT PSTR* ppszPassword
    )
{
    DWORD dwError = 0;
    fprintf(stdout, "%s's password: ", pszUsername);
    fflush(stdout);
    dwError = GetPassword(ppszPassword);
    fprintf(stdout, "\n");
    return dwError;
}

static
DWORD
GetOptionValue(
    IN PCSTR pszProgramName,
    IN SHOW_USAGE_CALLBACK ShowUsageError,
    IN OUT PLW_ARGV_CURSOR pCursor,
    IN PCSTR pszOptionName,
    OUT PSTR* ppszOptionValue
    )
{
    PCSTR value = LwArgvCursorPop(pCursor);
    if (!value)
    {
        fprintf(stderr, "Missing argument for option: %s\n", pszOptionName);
        ShowUsageError(pszProgramName);
    }
    return LwAllocateString(value, ppszOptionValue);
}

static
DWORD
GetArgumentValue(
    IN PCSTR pszProgramName,
    IN SHOW_USAGE_CALLBACK ShowUsageError,
    IN OUT PLW_ARGV_CURSOR pCursor,
    IN OPTIONAL PCSTR pszArgumentName,
    OUT PSTR* ppszArgumentValue
    )
{
    DWORD dwError = 0;
    PSTR result = NULL;
    PCSTR value = LwArgvCursorPop(pCursor);

    if (!value)
    {
        if (pszArgumentName)
        {
            fprintf(stderr, "Missing %s argument.\n", pszArgumentName);
            ShowUsageError(pszProgramName);
        }
    }
    else
    {
        dwError = LwAllocateString(value, &result);
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(result);
    }

    *ppszArgumentValue = result;

    return dwError;
}

static
DWORD
ParseJoinArgs(
    IN int argc,
    IN const char *argv[],
    OUT PJOIN_ARGS pArgs
    )
{
    DWORD dwError = 0;
    PCSTR programName = NULL;
    PCSTR option = NULL;
    LW_ARGV_CURSOR cursor;
    SHOW_USAGE_CALLBACK ShowUsageHelp = ShowJoinUsageHelp;
    SHOW_USAGE_CALLBACK ShowUsageError = ShowJoinUsageError;

    memset(pArgs, 0, sizeof(*pArgs));

    LwArgvCursorInit(&cursor, argc, argv);
    programName = LwArgvCursorPop(&cursor);

    // Process options:
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
        else if (!strcmp("--name", option))
        {
            dwError = GetOptionValue(programName, ShowUsageError, &cursor, option,
                                     &pArgs->pszMachineName);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else if (!strcmp("--dnssuffix", option))
        {
            dwError = GetOptionValue(programName, ShowUsageError, &cursor, option,
                                     &pArgs->pszDnsSuffix);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else if (!strcmp("--ou", option))
        {
            dwError = GetOptionValue(programName, ShowUsageError, &cursor, option,
                                     &pArgs->pszOu);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else if (!strcmp("--osname", option))
        {
            dwError = GetOptionValue(programName, ShowUsageError, &cursor, option,
                                     &pArgs->pszOsName);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else if (!strcmp("--osversion", option))
        {
            dwError = GetOptionValue(programName, ShowUsageError, &cursor, option,
                                     &pArgs->pszOsVersion);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else if (!strcmp("--osservicepack", option))
        {
            dwError = GetOptionValue(programName, ShowUsageError, &cursor, option,
                                     &pArgs->pszOsServicePack);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
        else if (!strcmp("--notimesync", option))
        {
            SetFlag(pArgs->JoinFlags, LSA_NET_JOIN_DOMAIN_NOTIMESYNC);
        }
        else if (!strcmp("--multiple", option))
        {
            SetFlag(pArgs->JoinFlags, LSA_NET_JOIN_DOMAIN_NOTIMESYNC);
            SetFlag(pArgs->JoinFlags, LSA_NET_JOIN_DOMAIN_MULTIPLE);
        }
        else
        {
            fprintf(stderr, "Unrecognized option: %s\n", option);
            ShowUsageError(programName);
        }
    }

    // Process arguments:
    dwError = GetArgumentValue(programName, ShowUsageError, &cursor, "DOMAIN",
                               &pArgs->pszDomain);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    LwStrToUpper(pArgs->pszDomain);

    dwError = GetArgumentValue(programName, ShowUsageError, &cursor, "USERNAME",
                               &pArgs->pszUsername);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    // Optional argument
    dwError = GetArgumentValue(programName, ShowUsageError, &cursor, NULL,
                               &pArgs->pszPassword);
    assert(!dwError);

    if (LwArgvCursorRemaining(&cursor))
    {
        fprintf(stderr, "Too many arguments.\n");
        ShowUsageError(programName);
    }

    // Initialize missing options as needed

    if (!pArgs->pszDnsSuffix)
    {
        dwError = LwAllocateString(pArgs->pszDomain, &pArgs->pszDnsSuffix);
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }
    LwStrToLower(pArgs->pszDnsSuffix);

    if (!pArgs->pszMachineName)
    {
        dwError = GetHostname(&pArgs->pszMachineName);
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

    if (!pArgs->pszOsName ||
        !pArgs->pszOsVersion ||
        !pArgs->pszOsServicePack)
    {
        dwError = GetOsInfo(
                        pArgs->pszOsName ? NULL : &pArgs->pszOsName,
                        pArgs->pszOsVersion ? NULL : &pArgs->pszOsVersion,
                        pArgs->pszOsServicePack ? NULL : &pArgs->pszOsServicePack);
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

    dwError = CleanupUsername(pArgs->pszDomain, &pArgs->pszUsername);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    if (!pArgs->pszPassword)
    {
        dwError = PromptPassword(pArgs->pszUsername, &pArgs->pszPassword);
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

cleanup:
    if (dwError)
    {
        FreeJoinArgsContents(pArgs);
    }

    return dwError;
}

static
DWORD
ParseLeaveArgs(
    IN int argc,
    IN const char *argv[],
    OUT PLEAVE_ARGS pArgs
    )
{
    DWORD dwError = 0;
    PCSTR programName = NULL;
    PCSTR option = NULL;
    LW_ARGV_CURSOR cursor;
    SHOW_USAGE_CALLBACK ShowUsageHelp = ShowLeaveUsageHelp;
    SHOW_USAGE_CALLBACK ShowUsageError = ShowLeaveUsageError;

    memset(pArgs, 0, sizeof(*pArgs));

    LwArgvCursorInit(&cursor, argc, argv);
    programName = LwArgvCursorPop(&cursor);

    // Process options:
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
        else if (!strcmp("--multiple", option))
        {
            SetFlag(pArgs->JoinFlags, LSA_NET_JOIN_DOMAIN_MULTIPLE);

            dwError = GetOptionValue(programName, ShowUsageError, &cursor, option,
                                     &pArgs->pszDomain);
            GOTO_CLEANUP_ON_WINERROR(dwError);

            LwStrToUpper(pArgs->pszDomain);
        }
        else
        {
            fprintf(stderr, "Unrecognized option: %s\n", option);
            ShowUsageError(programName);
        }
    }

    // Optional arguments
    dwError = GetArgumentValue(programName, ShowUsageError, &cursor, NULL,
                               &pArgs->pszUsername);
    assert(!dwError);
    dwError = GetArgumentValue(programName, ShowUsageError, &cursor, NULL,
                               &pArgs->pszPassword);
    assert(!dwError);

    if (LwArgvCursorRemaining(&cursor))
    {
        fprintf(stderr, "Too many arguments.\n");
        ShowUsageError(programName);
    }

    // Initialize implicit domain argument.
    if (!pArgs->pszDomain)
    {
        dwError = GetCurrentDomain(&pArgs->pszDomain);
        if (NERR_SetupNotJoined == dwError)
        {
            fprintf(stderr, "The computer is not joined to a domain.\n");
            exit(1);
        }
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

    // If USERNAME was specified, clean it up and get password
    // as needed.
    if (pArgs->pszUsername)
    {
        dwError = CleanupUsername(pArgs->pszDomain, &pArgs->pszUsername);
        GOTO_CLEANUP_ON_WINERROR(dwError);

        if (!pArgs->pszPassword)
        {
            dwError = PromptPassword(pArgs->pszUsername, &pArgs->pszPassword);
            GOTO_CLEANUP_ON_WINERROR(dwError);
        }
    }

cleanup:
    if (dwError)
    {
        FreeLeaveArgsContents(pArgs);
    }

    return dwError;
}

static
VOID
ShowJoinUsage(
    IN PCSTR pszProgramName,
    IN int ExitCode
    )
{
    FILE* file = ExitCode ? stderr : stdout;

    fprintf(file,
            "Usage: %s [options] DOMAIN USERNAME [PASSWORD]\n"
            "\n"
            "  options:\n"
            "\n"
            "    --name MACHINE     -- Machine name to join as.  Default is hostname.\n"
            "    --dnssuffix SUFFIX -- DNS suffix to set for machine in AD.  Default is\n"
            "                          AD domain.\n"
            "    --ou OU            -- OU to join.\n"
            "    --osname STRING    -- OS name to set in AD.\n"
            "    --osversion STRING -- OS version to set in AD .\n"
            "    --osservicepack STRING -- OS service pack to set in AD.\n"
            "    --notimesync       -- Do not synchronize time to domain.\n"
            "    --multiple         -- Do multi-domain join.\n"
            "\n"
            "  notes:\n"
            "\n"
            "    If any OS option is not specified, its value from from uname.  If the\n"
            "    empty string is passed in for an OS option, the value in AD for the\n"
            "    attribute is preserved.\n"
            "",
            pszProgramName);

    exit(ExitCode);
}

static
VOID
ShowLeaveUsage(
    IN PCSTR pszProgramName,
    IN int ExitCode
    )
{
    FILE* file = ExitCode ? stderr : stdout;

    fprintf(file,
            "Usage: %s [options] [USERNAME [PASSWORD]]\n"
            "\n"
            "  options:\n"
            "\n"
            "    --multiple DOMAIN  -- Leave a specific domain (multi-domain mode).\n"
            "",
            pszProgramName);

    exit(ExitCode);
}

int
JoinMain(
    int argc,
    const char** argv
    )
{
    DWORD dwError = 0;
    JOIN_ARGS args = { 0 };

    dwError = ParseJoinArgs(argc, argv, &args);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    dwError = DoJoinDomain(
                    args.pszDomain,
                    args.pszUsername,
                    args.pszPassword,
                    args.pszMachineName,
                    args.pszDnsSuffix,
                    args.pszOu,
                    args.pszOsName,
                    args.pszOsVersion,
                    args.pszOsServicePack,
                    args.JoinFlags);
    GOTO_CLEANUP_ON_WINERROR(dwError);

cleanup:
    if (dwError)
    {
        PrintError(dwError);
    }

    FreeJoinArgsContents(&args);

    return dwError ? 1 : 0;
}

int
LeaveMain(
    int argc,
    const char** argv
    )
{
    DWORD dwError = 0;
    LEAVE_ARGS args = { 0 };

    dwError = ParseLeaveArgs(argc, argv, &args);
    GOTO_CLEANUP_ON_WINERROR(dwError);

    dwError = DoLeaveDomain(
                    args.pszDomain,
                    args.pszUsername,
                    args.pszPassword,
                    args.JoinFlags);
    GOTO_CLEANUP_ON_WINERROR(dwError);

cleanup:
    if (dwError)
    {
        PrintError(dwError);
    }

    FreeLeaveArgsContents(&args);

    return dwError ? 1 : 0;
}
