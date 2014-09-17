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
 *        main.c
 *
 * Abstract:
 *
 *        Netlogon
 *
 *        Tool to set the netlogond log level.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lwnet-system.h>
#include <lwnet.h>
#include <lw/base.h>
#include <lwerror.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for geteuid()
#include <sys/types.h> // for geteuid()

#define MY_GOTO_CLEANUP_ON_ERROR(dwError, pszErrorVariable, StaticErrorMessage) \
    do { \
        if (dwError) \
        { \
            (pszErrorVariable) = (StaticErrorMessage); \
            goto cleanup; \
        } \
    } while (0)

#define HAVE_MORE_ARGS(Argc, LastArgIndex, ArgsNeeded) \
    (((Argc) - (LastArgIndex)) > (ArgsNeeded))

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
VOID
ShowUsage(
    IN PCSTR pszProgramName
    )
{
    printf("Usage: %s LOGLEVEL\n"
           "\n"
           "    Set the log level of netlogond.\n"
           "\n"
           "  where LOGLEVEL is one of:\n"
           "\n"
           "    error\n"
           "    warning\n"
           "    info\n"
           "    verbose\n"
           "    debug\n"
           "    trace\n"
           "\n", pszProgramName);
}

static
VOID
ParseArgs(
    IN int argc,
    IN const char* argv[],
    OUT PLWNET_LOG_LEVEL pLogLevel
    )
{
    PCSTR pszProgramName = NULL;
    DWORD iArg = 0;
    PCSTR pszArg = NULL;
    LWNET_LOG_LEVEL LogLevel = 0;

    pszProgramName = strrchr(argv[0], '/');
    if (pszProgramName)
    {
        pszProgramName++;
    }
    if (RtlCStringIsNullOrEmpty(pszProgramName))
    {
        pszProgramName = argv[0];
    }

    // parse options
    for (iArg = 1; iArg < argc; iArg++)
    {
        pszArg = argv[iArg];

        if (!strcasecmp(pszArg, "-h") ||
            !strcasecmp(pszArg, "--help"))
        {
            ShowUsage(pszProgramName);
            exit(0);
        }
        else if (!strncasecmp(pszArg, "-", 1))
        {
            fprintf(stderr, "Invalid option: %s\n", pszArg);
            ShowUsage(pszProgramName);
            exit(1);
        }
        else
        {
            // this is an agument, so push is back
            iArg--;
            break;
        }
    }

    // parse arguments
    if (!HAVE_MORE_ARGS(argc, iArg, 1))
    {
        fprintf(stderr, "Missing LOGLEVEL argument.\n");
        ShowUsage(pszProgramName);
        exit(1);
    }

    pszArg = argv[iArg + 1];
    iArg++;

    if (!strcasecmp(pszArg, "error"))
    {
        LogLevel = LWNET_LOG_LEVEL_ERROR;
    }
    else if (!strcasecmp(pszArg, "warning"))
    {
        LogLevel = LWNET_LOG_LEVEL_WARNING;
    }
    else if (!strcasecmp(pszArg, "info"))
    {
        LogLevel = LWNET_LOG_LEVEL_INFO;
    }
    else if (!strcasecmp(pszArg, "verbose"))
    {
        LogLevel = LWNET_LOG_LEVEL_VERBOSE;
    }
    else if (!strcasecmp(pszArg, "debug"))
    {
        LogLevel = LWNET_LOG_LEVEL_DEBUG;
    }
    else if (!strcasecmp(pszArg, "trace"))
    {
        LogLevel = LWNET_LOG_LEVEL_TRACE;
    }
    else
    {
        fprintf(stderr, "Invalid LOGLEVEL argument: %s\n", pszArg);
        ShowUsage(pszProgramName);
        exit(1);
    }

    if (HAVE_MORE_ARGS(argc, iArg, 1))
    {
        fprintf(stderr, "Too many arguments.\n");
        ShowUsage(pszProgramName);
        exit(1);
    }

    *pLogLevel = LogLevel;
}

static
VOID
PrintLogInfo(
    IN PCSTR pszPrefix,
    IN LWNET_LOG_LEVEL LogLevel,
    IN LWNET_LOG_TARGET LogTarget,
    IN PCSTR pszLogPath
    )
{
    PCSTR pszLogLevel = NULL;
    PCSTR pszLogTarget = NULL;

    switch (LogLevel)
    {
    case LWNET_LOG_LEVEL_ALWAYS:
        pszLogLevel = "always";
        break;
    case LWNET_LOG_LEVEL_ERROR:
        pszLogLevel = "error";
        break;
    case LWNET_LOG_LEVEL_WARNING:
        pszLogLevel = "warning";
        break;
    case LWNET_LOG_LEVEL_INFO:
        pszLogLevel = "info";
        break;
    case LWNET_LOG_LEVEL_VERBOSE:
        pszLogLevel = "verbose";
        break;
    case LWNET_LOG_LEVEL_DEBUG:
        pszLogLevel = "debug";
        break;
    case LWNET_LOG_LEVEL_TRACE:
        pszLogLevel = "trace";
        break;
    default:
        pszLogLevel = "<UNKNOWN>";
        break;
    }

    switch (LogTarget)
    {
    case LWNET_LOG_TARGET_DISABLED:
        pszLogTarget = "disabled";
        break;
    case LWNET_LOG_TARGET_CONSOLE:
        pszLogTarget = "console";
        break;
    case LWNET_LOG_TARGET_FILE:
        pszLogTarget = "file";
        break;
    case LWNET_LOG_TARGET_SYSLOG:
        pszLogTarget = "syslog";
        break;
    default:
        pszLogTarget = "<unknown>";
        break;
    }

    fprintf(stdout,
            "%s log settings:\n"
            "\n"
            "log level = %s\n"
            "log target = %s\n"
            "log path = %s\n",
            pszPrefix,
            pszLogLevel,
            pszLogTarget,
            pszLogPath ? pszLogPath : "n/a");
}

static
VOID
PrintError(
    IN OPTIONAL PCSTR pszErrorPrefix,
    IN DWORD dwError
    )
{
    PCSTR pszUseErrorPrefix = NULL;
    size_t size = 0;
    PSTR pszErrorString = NULL;

    if (dwError)
    {
        pszUseErrorPrefix = pszErrorPrefix;
        if (!pszUseErrorPrefix)
        {
            pszUseErrorPrefix = "Failed communication with the LWNET Agent";
        }

        size = LwGetErrorString(dwError, NULL, 0);
        if (size)
        {
            pszErrorString = malloc(size);
            if (pszErrorString)
            {
                LwGetErrorString(dwError, pszErrorString, size);
            }
        }
        if (RtlCStringIsNullOrEmpty(pszErrorString))
        {
            fprintf(stderr,
                    "%s.  Error code %u (%s).\n",
                    pszUseErrorPrefix,
                    dwError,
                    LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)));
        }
        else
        {
            fprintf(stderr,
                    "%s.  Error code %u (%s).\n%s\n",
                    pszUseErrorPrefix,
                    dwError,
                    LW_PRINTF_STRING(LwWin32ExtErrorToName(dwError)),
                    pszErrorString);
        }
    }
}

int
main(
    int argc,
    const char* argv[]
    )
{
    DWORD dwError = 0;
    PCSTR pszErrorPrefix = NULL;
    LWNET_LOG_LEVEL LogLevel = 0;
    LWNET_LOG_TARGET LogTarget = 0;
    PSTR pszLogPath = NULL;

    if (geteuid() != 0)
    {
        fprintf(stderr, "This program requires super-user privileges.\n");
        exit(1);
    }

    ParseArgs(argc, argv, &LogLevel);

    dwError = LWNetSetLogLevel(LogLevel);
    MY_GOTO_CLEANUP_ON_ERROR(dwError, pszErrorPrefix, "Failed to set log level");

    fprintf(stdout, "The log level was set successfully\n\n");

    dwError = LWNetGetLogInfo(&LogLevel, &LogTarget, &pszLogPath);
    MY_GOTO_CLEANUP_ON_ERROR(dwError, pszErrorPrefix, "Failed to get log info");

    PrintLogInfo("Updated", LogLevel, LogTarget, pszLogPath);

cleanup:
    if (pszLogPath)
    {
        LWNetFreeString(pszLogPath);
    }

    if (dwError)
    {
        PrintError(pszErrorPrefix, dwError);
    }

    return dwError ? 1 : 0;
}
