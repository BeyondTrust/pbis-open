/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <bail.h>
#include <log_util.h>

#include <lwerror.h>
#include <lwstr.h>

static void LogToSyslog(LW_RTL_LOG_LEVEL level, PVOID pUserData,
        PCSTR message);
static void LogToFile(LW_RTL_LOG_LEVEL level, PVOID pUserData,
        PCSTR message);

typedef VOID (*PLW_AUTO_ENROLL_LOG_CALLBACK)(LW_RTL_LOG_LEVEL level, PVOID pUserData, PCSTR pszMessage);

static LW_RTL_LOG_LEVEL gMaxLogLevel = LW_RTL_LOG_LEVEL_ERROR;
static PLW_AUTO_ENROLL_LOG_CALLBACK gpLogCallback = LogToFile;
static PVOID            gpLogCallbackData = NULL;

static
VOID
LwAutoenroolLogCallback(
    LW_IN LW_OPTIONAL LW_PVOID Context,
    LW_IN LW_RTL_LOG_LEVEL Level,
    LW_IN LW_OPTIONAL LW_PCSTR ComponentName,
    LW_IN LW_PCSTR FunctionName,
    LW_IN LW_PCSTR FileName,
    LW_IN LW_ULONG LineNumber,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    )
{
    DWORD dwError = 0;
    PSTR formattedMessage = NULL;

    va_list argList;
    va_start(argList, Format);
    dwError = LwAllocateStringPrintfV(&formattedMessage, Format, argList);
    va_end(argList);

    if (!dwError)
    {
        gpLogCallback(Level, Context, formattedMessage);
    }

    LW_SAFE_FREE_STRING(formattedMessage);
}

static struct
{
    unsigned int    bLogLevelSpecified:1;
    unsigned int    bLogFileSpecified:1;
    unsigned int    bSyslogSpecified:1;
    unsigned int    bProcessingOptions:1;
}
gLogStatus;

static struct LwAutoenrollLogLevel {
    PCSTR       name;
    LW_RTL_LOG_LEVEL lwLogLevel;
    int         iSyslogLevel;
} LwAutoenrollLogLevels[] = {
    { "error",          LW_RTL_LOG_LEVEL_ERROR,   LOG_ERR, },
    { "warning",        LW_RTL_LOG_LEVEL_WARNING, LOG_WARNING, },
    { "info",           LW_RTL_LOG_LEVEL_INFO,    LOG_INFO, },
    { "verbose",        LW_RTL_LOG_LEVEL_VERBOSE, LOG_INFO, },
    { "debug",          LW_RTL_LOG_LEVEL_DEBUG,   LOG_INFO, },
    { "trace",          LW_RTL_LOG_LEVEL_TRACE,   LOG_INFO, },
    { NULL,             0,                    0, }
};

enum LwAutoenrollLogOptionType {
    LW_LOG_OPTION_FILE = 1,
    LW_LOG_OPTION_LEVEL,
    LW_LOG_OPTION_SYSLOG,
};

static void LwAutoenrollLogOptionCallback(
        poptContext poptContext,
        enum poptCallbackReason reason,
        const struct poptOption *option,
        const char *arg,
        void *data
    );

struct poptOption LwAutoenrollLogOptions[] = {
    {
        NULL,
        0,
        POPT_ARG_CALLBACK | POPT_CBFLAG_PRE | POPT_CBFLAG_POST,
        LwAutoenrollLogOptionCallback,
        0,
        0,
        NULL,
    },
    {
        "logfile",
        0,
        POPT_ARG_STRING,
        NULL,
        LW_LOG_OPTION_FILE,
        "Output log messages to a file (default: stdout).",
        "path",
    },
    {
        "syslog",
        0,
        POPT_ARG_NONE,
        NULL,
        LW_LOG_OPTION_SYSLOG,
        "Output log messages to syslog.",
        NULL,
    },
    {
        "loglevel",
        0,
        POPT_ARG_STRING,
        NULL,
        LW_LOG_OPTION_LEVEL,
        "Specify the maximum level of message to log."
            " {error, warning, info, verbose, debug, trace}",
        "level",
    },
    POPT_TABLEEND
};

static void
LogToSyslog(LW_RTL_LOG_LEVEL level, PVOID pUserData, PCSTR message)
{
    syslog(LwAutoenrollLogLevels[level - 1].iSyslogLevel, "%s", message);
}

static void
LogToFile(LW_RTL_LOG_LEVEL level, PVOID pUserData, PCSTR message)
{
    FILE *fpLog = pUserData;

    if (fpLog == NULL)
    {
        fpLog = stdout;
    }

    fprintf(fpLog, "%s\n", message);
    fflush(fpLog);
}

DWORD
LwAutoenrollLogInit(void)
{
    /* Don't set up logging until all options are processed. */
    if (!gLogStatus.bProcessingOptions)
    {
        LwRtlLogSetLevel(gMaxLogLevel);
        LwRtlLogSetCallback(LwAutoenroolLogCallback, gpLogCallbackData);
    }

    return LW_ERROR_SUCCESS;
}

DWORD
LwAutoenrollLogToSyslog(LW_BOOL force)
{
    DWORD error;

    if (gpLogCallback == LogToSyslog)
    {
        error = LW_ERROR_SUCCESS;
        goto cleanup;
    }

    if (!force && gLogStatus.bLogFileSpecified)
    {
        error = LW_ERROR_SUCCESS;
        goto cleanup;
    }

    openlog(NULL, LOG_PID, LOG_DAEMON);

    gpLogCallback = LogToSyslog;
    gpLogCallbackData = NULL;

    error = LwAutoenrollLogInit();
    BAIL_ON_LW_ERROR(error);

cleanup:
    return error;
}

DWORD
LwAutoenrollLogToFile(const char *path, LW_BOOL force)
{
    DWORD error;

    if (!force && gLogStatus.bSyslogSpecified)
    {
        error = LW_ERROR_SUCCESS;
        goto cleanup;
    }

    if (gpLogCallback == LogToFile
            && gpLogCallbackData != NULL
            && gpLogCallbackData != stdout)
    {
        fclose(gpLogCallbackData);
    }

    if (path[1] == '\0' && (path[0] == '-' || path[0] == '.'))
    {
        if (gpLogCallback == LogToFile && gpLogCallbackData == stdout)
        {
            error = LW_ERROR_SUCCESS;
            goto cleanup;
        }

        gpLogCallbackData = stdout;
    }
    else
    {
        FILE *fp = fopen(path, "a");
        BAIL_ON_UNIX_ERROR(
            fp == NULL,
            ": fopen(%s, \"a\") failed",
            path);
        gpLogCallbackData = fp;
    }

    gpLogCallback = LogToFile;

    error = LwAutoenrollLogInit();
    BAIL_ON_LW_ERROR(error);

cleanup:
    return error;
}

LW_RTL_LOG_LEVEL
LwAutoenrollLogGetLevel(void)
{
    return gMaxLogLevel;
}

DWORD
LwAutoenrollLogSetLevel(LW_RTL_LOG_LEVEL level, LW_BOOL force)
{
    DWORD error;

    if (!force && gLogStatus.bLogLevelSpecified)
    {
        error = LW_ERROR_SUCCESS;
        goto cleanup;
    }

    gMaxLogLevel = level;

    if (gpLogCallback == LogToSyslog)
    {
        setlogmask(LOG_UPTO(LwAutoenrollLogLevels[gMaxLogLevel - 1].iSyslogLevel));
    }

    error = LwAutoenrollLogInit();
    BAIL_ON_LW_ERROR(error);

cleanup:
    return error;
}

DWORD
LwAutoenrollLogSetLevelFromString(PCSTR logLevel, LW_BOOL force)
{
    const struct LwAutoenrollLogLevel *level;

    for (level = LwAutoenrollLogLevels; level->name != NULL; ++level)
    {
        if (!strcmp(level->name, logLevel))
        {
            if (gMaxLogLevel != level->lwLogLevel)
            {
                return LwAutoenrollLogSetLevel(level->lwLogLevel, force);
            }

            break;
        }
    }

    return (level->name == NULL);
}

static void
LwAutoenrollLogOptionCallback(
        poptContext poptContext,
        enum poptCallbackReason reason,
        const struct poptOption *option,
        const char *arg, void *data
    )
{
    DWORD error = 0;

    switch (reason)
    {
        case POPT_CALLBACK_REASON_PRE:
            gpLogCallback = LogToFile;
            gpLogCallbackData = stdout;
            gLogStatus.bProcessingOptions = 1;
            break;

        case POPT_CALLBACK_REASON_POST:
            gLogStatus.bProcessingOptions = 0;
            LwAutoenrollLogInit();
            break;

        case POPT_CALLBACK_REASON_OPTION:
            switch (option->val)
            {
                case LW_LOG_OPTION_FILE:
                    gLogStatus.bSyslogSpecified = 0;
                    LwAutoenrollLogToFile(arg, LW_TRUE);
                    gLogStatus.bLogFileSpecified = 1;
                    break;

                case LW_LOG_OPTION_SYSLOG:
                    gLogStatus.bLogFileSpecified = 0;
                    LwAutoenrollLogToSyslog(LW_TRUE);
                    gLogStatus.bSyslogSpecified = 1;
                    break;

                case LW_LOG_OPTION_LEVEL:
                    error = LwAutoenrollLogSetLevelFromString(arg, LW_TRUE);
                    if (error)
                    {
                        fprintf(stderr, "unknown level %s\n", arg);
                        exit(1);
                    }
                    gLogStatus.bLogLevelSpecified = 1;
                    break;
            }
            break;
    }
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
