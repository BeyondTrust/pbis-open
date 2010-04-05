/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server (Process Utilities)
 *
 */
#ifndef __LWMGMTPCNTL_H__
#define __LWMGMTPCNTL_H__

/* This structure captures the arguments that must be
 * sent to the Group Policy Service
 */
typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
    /* Should start as daemon */
    DWORD dwStartAsDaemon;
    /* How much logging do you want? */
    DWORD dwLogLevel;
    /* log file path */
    char szLogFilePath[PATH_MAX + 1];
    /* config file path */
    char szConfigFilePath[PATH_MAX + 1];
    /* Cache path */
    char szCachePath[PATH_MAX+1];
    /* Prefix path */
    char szPrefixPath[PATH_MAX+1];
    /* Process termination flag */
    BOOLEAN  bProcessShouldExit;
    /* Process Exit Code */
    DWORD dwExitCode;
} LWMGMTSERVERINFO, *PLWMGMTSERVERINFO;

void
LWMGMTExitHandler(
    void
    );

PSTR
get_program_name(
    PSTR pszFullProgramPath
    );

BOOLEAN
LWMGMTProcessShouldExit();

VOID
LWMGMTSetProcessShouldExit(
    BOOLEAN val
    );

DWORD
LWMGMTGetProcessExitCode(
    PDWORD pdwExitCode
    );

void
LWMGMTSetProcessExitCode(
    DWORD dwExitCode
    );

DWORD
LWMGMTGetCachePath(
    PSTR* ppszPath
    );

DWORD
LWMGMTGetConfigPath(
    PSTR* ppszPath
    );

DWORD
LWMGMTGetPrefixPath(
    PSTR* ppszPath
    );

void
ShowUsage(
    const PSTR pszProgramName
    );

void
get_server_info_r(
    PLWMGMTSERVERINFO pServerInfo
    );

DWORD
LWMGMTParseArgs(
    int argc,
    PSTR argv[],
    PLWMGMTSERVERINFO pLWMGMTServerInfo
    );

DWORD
LWMGMTStartAsDaemon();

pid_t
pid_from_pid_file();

void
LWMGMTCreatePIDFile();

DWORD
LWMGMTSetServerDefaults();

DWORD
LWMGMTInitLogging(
    PSTR pszProgramName
    );

void
LWMGMTBlockAllSignals();

DWORD
LWMGMTStopSignalHandler();

DWORD
LWMGMTReadConfigSettings();

DWORD
LWMGMTConfigStartSection(
    PCSTR    pszSectionName,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

DWORD
LWMGMTConfigComment(
    PCSTR    pszComment,
    PBOOLEAN pbContinue
    );

DWORD
LWMGMTConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PBOOLEAN pbContinue
    );

DWORD
LWMGMTConfigEndSection(
    PCSTR pszSectionName,
    PBOOLEAN pbContinue
    );

DWORD
LWMGMTStartSignalHandler();

PVOID
LWMGMTHandleSignals(
    PVOID pArg
    );

DWORD
LWMGMTListenForRPC();

#endif /* __LWMGMTPCNTL_H__ */
