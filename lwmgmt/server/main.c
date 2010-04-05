/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Service (Process Utilities)
 *
 */

#include "includes.h"

int
main(
    int argc,
    char* argv[])
{
    int dwError = 0;
    rpc_binding_vector_p_t pLsaMgrBinding = NULL;
    rpc_binding_vector_p_t pKrbMgrBinding = NULL;

    dwError = LWMGMTSetServerDefaults();
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTParseArgs(
                    argc,
                    argv,
                    &gServerInfo
                 );
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (gServerInfo.dwStartAsDaemon) {

        dwError = LWMGMTStartAsDaemon();
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    if (atexit(LWMGMTExitHandler) < 0) {
        dwError = errno;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    LWMGMTCreatePIDFile();

    dwError = LWMGMTInitLogging(get_program_name(argv[0]));
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = KrbMgrRegisterForRPC(
                    "Likewise Keytab Management Service",
                    &pKrbMgrBinding);
    BAIL_ON_LWMGMT_ERROR(dwError);
    
    dwError = LsaServerRegisterForRPC(
                    "Likewise LSASS Management Service",
                    &pLsaMgrBinding);
    BAIL_ON_LWMGMT_ERROR(dwError);

    LWMGMTBlockAllSignals();

    dwError = LWMGMTReadConfigSettings();
    if (dwError != 0)
    {
        LWMGMT_LOG_ERROR("Failed to read config file.  Error code: [%u]\n", dwError);
        dwError = 0;
    }

    dwError = LWMGMTStartSignalHandler();
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTListenForRPC();
    BAIL_ON_LWMGMT_ERROR(dwError);

    LWMGMT_LOG_INFO("LWMGMT Service exiting...");

 cleanup:

    /*
     * Indicate that the process is exiting
     */
    LWMGMTSetProcessShouldExit(TRUE);

    LWMGMTStopSignalHandler();

    if (pKrbMgrBinding) {
        KrbMgrUnregisterForRPC(pKrbMgrBinding);
    }
    
    if (pLsaMgrBinding) {
        LsaServerUnregisterForRPC(pLsaMgrBinding);
    }

    LWMGMTCloseLog();

    LWMGMTSetProcessExitCode(dwError);

    return (dwError);

error:

    LWMGMT_LOG_ERROR("Likewise Management Service exiting due to error [code:%d]", dwError);

    goto cleanup;
}

void
LWMGMTExitHandler(
    void
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = LWMGMTGetCachePath(&pszCachePath);
    BAIL_ON_LWMGMT_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/lwmgmtd.err", pszCachePath);

    dwError = LWMGMTCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (bFileExists) {
        dwError = LWMGMTRemoveFile(szErrCodeFilePath);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    dwError = LWMGMTGetProcessExitCode(&dwExitCode);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (dwExitCode) {
        fp = fopen(szErrCodeFilePath, "w");
        if (fp == NULL) {
            dwError = errno;
            BAIL_ON_LWMGMT_ERROR(dwError);
        }
        fprintf(fp, "%d\n", dwExitCode);
    }

error:

    if (pszCachePath) {
        LWMGMTFreeString(pszCachePath);
    }

    if (fp != NULL) {
        fclose(fp);
    }
}

PSTR
get_program_name(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

BOOLEAN
LWMGMTProcessShouldExit()
{
    BOOLEAN bResult = 0;

    LWMGMT_LOCK_SERVERINFO;

    bResult = gServerInfo.bProcessShouldExit;

    LWMGMT_UNLOCK_SERVERINFO;

    return bResult;
}

VOID
LWMGMTSetProcessShouldExit(
    BOOLEAN val
    )
{
    LWMGMT_LOCK_SERVERINFO;

    gServerInfo.bProcessShouldExit = val;

    LWMGMT_UNLOCK_SERVERINFO;
}

DWORD
LWMGMTGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;

    LWMGMT_LOCK_SERVERINFO;

    *pdwExitCode = gServerInfo.dwExitCode;

    LWMGMT_UNLOCK_SERVERINFO;

    return (dwError);
}

void
LWMGMTSetProcessExitCode(
    DWORD dwExitCode
    )
{
    LWMGMT_LOCK_SERVERINFO;

    gServerInfo.dwExitCode = dwExitCode;

    LWMGMT_UNLOCK_SERVERINFO;
}

DWORD
LWMGMTGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    LWMGMT_LOCK_SERVERINFO;

    dwError = LWMGMTAllocateString(gServerInfo.szCachePath, ppszPath);

    LWMGMT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
LWMGMTGetConfigPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    LWMGMT_LOCK_SERVERINFO;

    dwError = LWMGMTAllocateString(gServerInfo.szConfigFilePath, ppszPath);

    LWMGMT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
LWMGMTGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    LWMGMT_LOCK_SERVERINFO;

    dwError = LWMGMTAllocateString(gServerInfo.szPrefixPath, ppszPath);

    LWMGMT_UNLOCK_SERVERINFO;

    return (dwError);
}

void
ShowUsage(
    const PSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
            "          [--logfile logFilePath]\n"
            "          [--loglevel {0, 1, 2, 3, 4, 5}]\n"
            "          [--configfile configfilepath]\n", pszProgramName);
}

void
get_server_info_r(
    PLWMGMTSERVERINFO pServerInfo
    )
{
    if (pServerInfo == NULL) {
        return;
    }

    LWMGMT_LOCK_SERVERINFO;

    memcpy(pServerInfo, &gServerInfo, sizeof(LWMGMTSERVERINFO));

    pthread_mutex_init(&pServerInfo->lock, NULL);

    LWMGMT_UNLOCK_SERVERINFO;
}

DWORD
LWMGMTParseArgs(
    int argc,
    PSTR argv[],
    PLWMGMTSERVERINFO pLWMGMTServerInfo
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_CONFIGFILE,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    DWORD dwLogLevel = 0;

    do
    {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
            break;
        }

        switch(parseMode)
        {
            case PARSE_MODE_OPEN:
            {
                if (strcmp(pArg, "--logfile") == 0)    {
                    parseMode = PARSE_MODE_LOGFILE;
                }
                else if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0)) {
                    ShowUsage(get_program_name(argv[0]));
                    exit(0);
                }
                else if (strcmp(pArg, "--start-as-daemon") == 0) {
                    pLWMGMTServerInfo->dwStartAsDaemon = 1;
                }
                else if (strcmp(pArg, "--configfile") == 0) {
                    parseMode = PARSE_MODE_CONFIGFILE;
                }
                else if (strcmp(pArg, "--loglevel") == 0) {
                    parseMode = PARSE_MODE_LOGLEVEL;
                }
                else {
                    LWMGMT_LOG_ERROR("Unrecognized command line option [%s]",
                                    pArg);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }
            }
            break;
            case PARSE_MODE_LOGFILE:
            {
                strncpy(pLWMGMTServerInfo->szLogFilePath, pArg, PATH_MAX);
                *(pLWMGMTServerInfo->szLogFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_LOGLEVEL:
            {
                dwLogLevel = atoi(pArg);

                if (dwLogLevel < LOG_LEVEL_ALWAYS || dwLogLevel > LOG_LEVEL_DEBUG) {

                    LWMGMT_LOG_ERROR("Error: Invalid log level [%d]", dwLogLevel);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }

                pLWMGMTServerInfo->dwLogLevel = dwLogLevel;

                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_CONFIGFILE:
            {
                strncpy(pLWMGMTServerInfo->szConfigFilePath, pArg, PATH_MAX);
                *(pLWMGMTServerInfo->szConfigFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;
            }
            break;
        }
    } while (iArg < argc);

    return 0;
}

DWORD
LWMGMTStartAsDaemon()
{
    DWORD dwError = 0;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    /* Use dcethread_fork() rather than fork() because we link with DCE/RPC */
    if ((pid = dcethread_fork()) != 0) {
        // Parent terminates
        exit (0);
    }

    // Let the first child be a session leader
    setsid();

    // Ignore SIGHUP, because when the first child terminates
    // it would be a session leader, and thus all processes in
    // its session would receive the SIGHUP signal. By ignoring
    // this signal, we are ensuring that our second child will
    // ignore this signal and will continue execution.
    if (signal(SIGHUP, SIG_IGN) < 0) {
        dwError = errno;
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_LWMGMT_ERROR(dwError);

    // Clear our file mode creation mask
    umask(0);

    for (iFd = 0; iFd < 3; iFd++)
        close(iFd);

    for (iFd = 0; iFd < 3; iFd++)    {

        fd = open("/dev/null", O_RDWR, 0);
        if (fd < 0) {
            fd = open("/dev/null", O_WRONLY, 0);
        }
        if (fd < 0) {
            exit(1);
        }
        if (fd != iFd) {
            exit(1);
        }
    }

    return (dwError);

 error:

    return (dwError);
}

pid_t
pid_from_pid_file()
{
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[PID_FILE_CONTENTS_SIZE];

    fd = open(PID_FILE, O_RDONLY, 0644);
    if (fd < 0) {
        goto error;
    }

    result = read(fd, contents, sizeof(contents)-1);
    if (result <= 0) {
        goto error;
    }
    contents[result-1] = 0;

    result = atoi(contents);
    if (result <= 0) {
        result = -1;
        goto error;
    }

    pid = (pid_t) result;
    result = kill(pid, 0);
    if (result != 0 || errno == ESRCH) {
        unlink(PID_FILE);
        pid = 0;
    }

 error:
    if (fd != -1) {
        close(fd);
    }

    return pid;
}

void
LWMGMTCreatePIDFile()
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = pid_from_pid_file();
    if (pid > 0) {
        fprintf(stderr, "Daemon already running as %d\n", (int) pid);
        result = -1;
        goto error;
    }

    fd = open(PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0) {
        fprintf(stderr, "Could not create pid file: %s\n", strerror(errno));
        result = 1;
        goto error;
    }

    pid = getpid();
    snprintf(contents, sizeof(contents)-1, "%d\n", (int) pid);
    contents[sizeof(contents)-1] = 0;
    len = strlen(contents);

    result = (int) write(fd, contents, len);
    if ( result != (int) len ) {
        fprintf(stderr, "Could not write to pid file: %s\n", strerror(errno));
        result = -1;
        goto error;
    }

    result = 0;

 error:
    if (fd != -1) {
        close(fd);
    }

    if (result < 0) {
        exit(1);
    }
}

DWORD
LWMGMTSetServerDefaults()
{
    DWORD dwError = 0;

    LWMGMT_LOCK_SERVERINFO;

    gServerInfo.dwLogLevel = LOG_LEVEL_ERROR;

    *(gServerInfo.szLogFilePath) = '\0';

    memset(gServerInfo.szConfigFilePath, 0, PATH_MAX+1);
    strncpy(gServerInfo.szConfigFilePath, DEFAULT_CONFIG_FILE_PATH, PATH_MAX);
    strcpy(gServerInfo.szCachePath, CACHEDIR);
    strcpy(gServerInfo.szPrefixPath, PREFIXDIR);

    LWMGMT_UNLOCK_SERVERINFO;

    return dwError;
}

DWORD
LWMGMTInitLogging(
    PSTR pszProgramName
    )
{
    DWORD dwError = 0;

    if (gServerInfo.dwStartAsDaemon) {

        dwError = LWMGMTInitLoggingToSyslog(gServerInfo.dwLogLevel,
                                             pszProgramName,
                                             LOG_PID,
                                             LOG_DAEMON);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }
    else
    {
        dwError = LWMGMTInitLoggingToFile(gServerInfo.dwLogLevel,
                                            gServerInfo.szLogFilePath);
        BAIL_ON_LWMGMT_ERROR(dwError);
    }

error:

    return dwError;
}

void
LWMGMTBlockAllSignals()
{
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
}

DWORD
LWMGMTStopSignalHandler()
{
    DWORD dwError = 0;
    unsigned32 status = 0;

    rpc_mgmt_stop_server_listening(NULL, &status);

    if (pgSignalHandlerThread && !pthread_cancel(gSignalHandlerThread)) {
        pthread_join(gSignalHandlerThread, NULL);
        pgSignalHandlerThread = NULL;
    }

    return (dwError);
}

DWORD
LWMGMTReadConfigSettings()
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;

    LWMGMT_LOG_INFO("Reading LWMGMT configuration settings");

    dwError = LWMGMTGetConfigPath(&pszConfigFilePath);
    BAIL_ON_LWMGMT_ERROR(dwError);

    dwError = LWMGMTParseConfigFile(
                pszConfigFilePath,
                &LWMGMTConfigStartSection,
                &LWMGMTConfigComment,
                &LWMGMTConfigNameValuePair,
                &LWMGMTConfigEndSection);
    BAIL_ON_LWMGMT_ERROR(dwError);

    if (pszConfigFilePath) {
        LWMGMTFreeString(pszConfigFilePath);
        pszConfigFilePath = NULL;
    }

cleanup:

    return dwError;

error:

    if (pszConfigFilePath) {
        LWMGMTFreeString(pszConfigFilePath);
    }

    goto cleanup;

}

/* call back functions to get the values from config file */
DWORD
LWMGMTConfigStartSection(
    PCSTR    pszSectionName,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{

    //This callback may not be required,retaining it for future
    LWMGMT_LOG_VERBOSE("LWMGMTConfigStartSection: SECTION Name=%s", pszSectionName);

    *pbSkipSection = FALSE;
    *pbContinue = TRUE;

    return 0;
}

DWORD
LWMGMTConfigComment(
    PCSTR    pszComment,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    LWMGMT_LOG_VERBOSE("LWMGMTConfigComment: %s",
        (IsNullOrEmptyString(pszComment) ? "" : pszComment));

    *pbContinue = TRUE;

    return 0;
}

DWORD
LWMGMTConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PBOOLEAN pbContinue
    )
{

    //strip the white spaces
    LWMGMTStripWhitespace((PSTR)pszName,1,1);
    LWMGMTStripWhitespace((PSTR)pszValue,1,1);

    LWMGMT_LOG_INFO("LWMGMTConfigNameValuePair: NAME=%s, VALUE=%s",
        (IsNullOrEmptyString(pszName) ? "" : pszName),
        (IsNullOrEmptyString(pszValue) ? "" : pszValue));

    *pbContinue = TRUE;

    return 0;
}

DWORD
LWMGMTConfigEndSection(
    PCSTR pszSectionName,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    LWMGMT_LOG_VERBOSE("LWMGMTConfigEndSection: SECTION Name=%s", pszSectionName);

    *pbContinue = TRUE;

    return 0;
}

/*
 * Set up the process environment to properly deal with signals.
 * By default, we isolate all threads from receiving asynchronous
 * signals. We create a thread that handles all async signals.
 * The signal handling actions are handled in the handler thread.
 *
 * For AIX, we cant use a thread that sigwaits() on a specific signal,
 * we use a plain old, lame old Unix signal handler.
 *
 */
DWORD
LWMGMTStartSignalHandler()
{
    DWORD dwError = 0;

    dwError = pthread_create(&gSignalHandlerThread,
                             NULL,
                             LWMGMTHandleSignals,
                             NULL);
    BAIL_ON_LWMGMT_ERROR(dwError);

    pgSignalHandlerThread = &gSignalHandlerThread;

cleanup:

    return (dwError);

error:

    pgSignalHandlerThread = NULL;

    goto cleanup;
}

PVOID
LWMGMTHandleSignals(
    PVOID pArg
    )
{
    DWORD dwError = 0;
    sigset_t catch_signal_mask;
    sigset_t old_signal_mask;
    int which_signal;
    unsigned32 status;

    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    pthread_sigmask(SIG_BLOCK,  &catch_signal_mask, &old_signal_mask);

    while (1)
    {
        /* Wait for a signal to arrive */
        sigwait(&catch_signal_mask, &which_signal);

        switch (which_signal)
        {
            case SIGINT:
            case SIGQUIT:
            {
                rpc_mgmt_stop_server_listening(NULL, &status);
                LWMGMTSetProcessShouldExit(TRUE);
                break;
            }
            case SIGHUP:
            {
                dwError = LWMGMTReadConfigSettings();
                BAIL_ON_LWMGMT_ERROR(dwError);
            }

        }
    }

error:
    return NULL;
}

DWORD
LWMGMTListenForRPC()
{
    DWORD dwError = 0;
    unsigned32 status = 0;

    TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, &status);
        if (status != 0)
        {
           dwError = LWMGMT_ERROR_RPC_EXCEPTION_UPON_LISTEN;
           BAIL_ON_LWMGMT_ERROR(dwError);
        }
    }
    CATCH_ALL
    {
        dwError = dcethread_exc_getstatus (THIS_CATCH);
    if(!dwError)
    {
            dwError = LWMGMT_ERROR_RPC_EXCEPTION_UPON_LISTEN;
    }
    }
    ENDTRY
    BAIL_ON_LWMGMT_ERROR(dwError);

cleanup:
    return dwError;

error:
    LWMGMT_LOG_ERROR("Failed to begin RPC listening.  Error code [%d]\n", dwError);
    goto cleanup;
}

