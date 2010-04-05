/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise Server Service Services
 *
 *        System Functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

// consolelog.c

DWORD
NfsSvcOpenConsoleLog(
    NFSSVC_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE     phLog
    );

DWORD
NfsSvcCloseConsoleLog(
    HANDLE hLog
    );

VOID
NfsSvcLogToConsole(
    HANDLE      hLog,
    NFSSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

VOID
NfsSvcFreeConsoleLogInfo(
    PNFSSVC_CONSOLE_LOG pConsoleLog
    );

// filelog.c

DWORD
NfsSvcOpenFileLog(
    PCSTR            pszFilePath,
    NFSSVC_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE          phLog
    );

DWORD
NfsSvcGetFileLogInfo(
    HANDLE hLog,
    PNFSSVC_LOG_INFO* ppLogInfo
    );

DWORD
NfsSvcCloseFileLog(
    HANDLE hLog
    );

VOID
NfsSvcLogToFile(
    HANDLE      hLog,
    NFSSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

VOID
NfsSvcFreeFileLogInfo(
    PNFSSVC_FILE_LOG pFileLog
    );

// logger.c

DWORD
NfsSvcSetupLogging(
    HANDLE                 hLog,
    NFSSVC_LOG_LEVEL       maxAllowedLogLevel,
    PFN_NFSSVC_LOG_MESSAGE pfnLogger
    );

VOID
NfsSvcResetLogging(
    VOID
    );

// loginfo.c

VOID
NfsSvcFreeLogInfo(
    PNFSSVC_LOG_INFO pLogInfo
    );

// sysfuncs.c

#if !HAVE_DECL_ISBLANK
int isblank(int c);
#endif

void
NfsSvc_vsyslog(
    int priority,
    const char *format,
    va_list ap
    );

#if !defined(HAVE_RPL_MALLOC)

void*
rpl_malloc(
    size_t n
    );

#endif /* ! HAVE_RPL_MALLOC */

#if !defined(HAVE_RPL_REALLOC)

void*
rpl_realloc(
    void* buf,
    size_t n
    );

// syslog.c

DWORD
NfsSvcOpenSyslog(
    PCSTR       pszIdentifier,
    NFSSVC_LOG_LEVEL maxAllowedLogLevel,
    DWORD       dwOptions,
    DWORD       dwFacility,
    PHANDLE     phLog
    );

VOID
NfsSvcSetSyslogMask(
    NFSSVC_LOG_LEVEL logLevel
    );

VOID
NfsSvcLogToSyslog(
    HANDLE      hLog,
    NFSSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

DWORD
NfsSvcCloseSyslog(
    HANDLE hLog
    );

VOID
NfsSvcFreeSysLogInfo(
    PNFSSVC_SYS_LOG pSysLog
    );

#endif /* ! HAVE_RPL_REALLOC */


