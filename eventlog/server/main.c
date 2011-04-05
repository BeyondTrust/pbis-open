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


EVTSERVERINFO gServerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,  /* Lock              */
    0,                          /* Start as daemon   */
    FALSE,                      /* Log to syslog */
    LOG_LEVEL_ERROR,            /* Max Log Level     */
    "",                         /* Log file path     */
    "",                         /* Config file path  */
    "",                         /* Cache path        */
    "",                         /* Prefix path       */
    0,                          /* Process exit flag */
    0,                          /* Process exit code */
    0,                          /* Replace existing db flag */
    0,                          /* Max log size  */
    0,                          /* Max records */
    0,                          /* Remove records older than*/
    0,                          /* Purge records at interval*/
    0,                          /* Enable/disable Remove records a boolean value TRUE or FALSE*/
    TRUE,                       /* Register TCP/IP RPC endpoints*/
    NULL,
};

#define EVT_LOCK_SERVERINFO   pthread_mutex_lock(&gServerInfo.lock)
#define EVT_UNLOCK_SERVERINFO pthread_mutex_unlock(&gServerInfo.lock)


static
DWORD
EVTCreateAccessDescriptor(
    PCSTR pszAllowReadTo,
    PCSTR pszAllowWriteTo,
    PCSTR pszAllowDeleteTo,
    PSECURITY_DESCRIPTOR_ABSOLUTE* ppDescriptor,
    PBOOLEAN pbFullyResolved
    );

static
DWORD
EVTGetProcessExitCode(
    PDWORD pdwExitCode
    );

static
DWORD
EVTGetRegisterTcpIp(
    PBOOLEAN pbRegisterTcpIp
    );

static
void
EVTExitHandler(
    void
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = EVTGetCachePath(&pszCachePath);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/eventlogd.err", pszCachePath);

    dwError = LwCheckFileTypeExists(
                    szErrCodeFilePath,
                    LWFILE_REGULAR,
                    &bFileExists);
    BAIL_ON_EVT_ERROR(dwError);

    if (bFileExists) {
        dwError = LwRemoveFile(szErrCodeFilePath);
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = EVTGetProcessExitCode(&dwExitCode);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwExitCode) {
        fp = fopen(szErrCodeFilePath, "w");
        if (fp == NULL) {
            dwError = errno;
            BAIL_ON_EVT_ERROR(dwError);
        }
        fprintf(fp, "%d\n", dwExitCode);
    }

error:

    if (pszCachePath) {
        LwFreeString(pszCachePath);
    }

    if (fp != NULL) {
        fclose(fp);
    }
}

static
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
EVTProcessShouldExit()
{
    BOOLEAN bResult = 0;

    EVT_LOCK_SERVERINFO;

    bResult = gServerInfo.bProcessShouldExit;

    EVT_UNLOCK_SERVERINFO;

    return bResult;
}

void
EVTSetProcessShouldExit(
    BOOLEAN val
    )
{
    EVT_LOCK_SERVERINFO;

    gServerInfo.bProcessShouldExit = val;

    EVT_UNLOCK_SERVERINFO;
}

DWORD
EVTGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwExitCode = gServerInfo.dwExitCode;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

void
EVTSetProcessExitCode(
    DWORD dwExitCode
    )
{
    EVT_LOCK_SERVERINFO;

    gServerInfo.dwExitCode = dwExitCode;

    EVT_UNLOCK_SERVERINFO;
}

DWORD
EVTGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    dwError = LwAllocateString(gServerInfo.szCachePath, ppszPath);

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTCheckAllowed(
    PACCESS_TOKEN pUserToken,
    ACCESS_MASK dwAccessMask,
    BOOLEAN* pAllowed
    )
{
    DWORD dwError = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    // Do not free
    PSECURITY_DESCRIPTOR_ABSOLUTE pDescriptor = NULL;
    PSTR pszAllowReadTo = NULL;
    PSTR pszAllowWriteTo = NULL;
    PSTR pszAllowDeleteTo = NULL;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bFullyResolved = FALSE;
    GENERIC_MAPPING GenericMapping = {0};
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ACCESS_MASK dwGrantedAccess = 0;
    BOOLEAN allowed = FALSE;

    EVT_LOCK_SERVERINFO;
    bLocked = TRUE;

    if (!gServerInfo.pAccess)
    {
        dwError = LwAllocateString(
                        gServerInfo.pszAllowReadTo ?
                            gServerInfo.pszAllowReadTo : "",
                        &pszAllowReadTo);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateString(
                        gServerInfo.pszAllowWriteTo ?
                            gServerInfo.pszAllowWriteTo : "",
                        &pszAllowWriteTo);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateString(
                        gServerInfo.pszAllowDeleteTo ?
                            gServerInfo.pszAllowDeleteTo : "",
                        &pszAllowDeleteTo);
        BAIL_ON_EVT_ERROR(dwError);

        EVT_UNLOCK_SERVERINFO;
        bLocked = FALSE;

        dwError = EVTCreateAccessDescriptor(
            pszAllowReadTo,
            pszAllowWriteTo,
            pszAllowDeleteTo,
            &pAbsolute,
            &bFullyResolved);
        BAIL_ON_EVT_ERROR(dwError);

        EVT_LOCK_SERVERINFO;
        bLocked = TRUE;

        if (bFullyResolved && !gServerInfo.pAccess)
        {
            gServerInfo.pAccess = pAbsolute;
            pAbsolute = NULL;
        }
        else
        {
            pDescriptor = pAbsolute;
        }
    }
    if (pDescriptor == NULL)
    {
        pDescriptor = gServerInfo.pAccess;
    }

    if (!RtlAccessCheck(pDescriptor,
                        pUserToken,
                        dwAccessMask,
                        0,
                        &GenericMapping,
                        &dwGrantedAccess,
                        &ntStatus))
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
        if (dwError == ERROR_ACCESS_DENIED)
        {
            dwError = 0;
        }
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        allowed = TRUE;
    }

    *pAllowed = allowed;

cleanup:
    if (bLocked)
    {
        EVT_UNLOCK_SERVERINFO;
    }
    LW_SAFE_FREE_STRING(pszAllowReadTo);
    LW_SAFE_FREE_STRING(pszAllowWriteTo);
    LW_SAFE_FREE_STRING(pszAllowDeleteTo);
    EVTFreeSecurityDescriptor(pAbsolute);
    return (dwError);

error:
    goto cleanup;
}

DWORD
EVTGetMaxRecords(
    DWORD* pdwMaxRecords
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxRecords = gServerInfo.dwMaxRecords;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetMaxAge(
    DWORD* pdwMaxAge
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxAge = gServerInfo.dwMaxAge;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetMaxLogSize(
    DWORD* pdwMaxLogSize
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxLogSize = gServerInfo.dwMaxLogSize;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetDBPurgeInterval(
    PDWORD pdwPurgeInterval
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwPurgeInterval = gServerInfo.dwPurgeInterval;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetRemoveAsNeeded(
    PBOOLEAN pbRemoveAsNeeded
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pbRemoveAsNeeded = gServerInfo.bRemoveAsNeeded;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

static
DWORD
EVTGetRegisterTcpIp(
    PBOOLEAN pbRegisterTcpIp
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pbRegisterTcpIp = gServerInfo.bRegisterTcpIp;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    dwError = LwAllocateString(gServerInfo.szPrefixPath, ppszPath);

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

void
EVTUnlockServerInfo()
{
    EVT_UNLOCK_SERVERINFO;
}

static
void
ShowUsage(
    const PSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
            "          [--syslog]\n"
            "          [--logfile logFilePath]\n"
            "          [--replacedb]\n"
            "          [--loglevel {error, warning, info, verbose, debug}]\n"
            "          [--configfile configfilepath]\n", pszProgramName);
}

void
get_server_info_r(
    PEVTSERVERINFO pServerInfo
    )
{
    if (pServerInfo == NULL) {
        return;
    }

    EVT_LOCK_SERVERINFO;

    memcpy(pServerInfo, &gServerInfo, sizeof(EVTSERVERINFO));

    pthread_mutex_init(&pServerInfo->lock, NULL);

    EVT_UNLOCK_SERVERINFO;
}

static
DWORD
EVTParseArgs(
    int argc,
    PSTR argv[],
    PEVTSERVERINFO pEVTServerInfo
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
                    pEVTServerInfo->dwStartAsDaemon = 1;
                }
                else if (strcmp(pArg, "--configfile") == 0) {
                    parseMode = PARSE_MODE_CONFIGFILE;
                }
                else if (strcmp(pArg, "--syslog") == 0)
                {
                    pEVTServerInfo->bLogToSyslog = TRUE;
                }
                else if (strcmp(pArg, "--loglevel") == 0) {
                    parseMode = PARSE_MODE_LOGLEVEL;
                }
                else if (strcmp(pArg, "--replacedb") == 0) {
                    pEVTServerInfo->bReplaceDB = TRUE;
                } else {
                    EVT_LOG_ERROR("Unrecognized command line option [%s]",
                                    pArg);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }
            }
            break;
            case PARSE_MODE_LOGFILE:
            {
                strncpy(pEVTServerInfo->szLogFilePath, pArg, PATH_MAX);
                *(pEVTServerInfo->szLogFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_LOGLEVEL:
            {
                if (!strcasecmp(pArg, "always"))
                {
                    dwLogLevel = LOG_LEVEL_ALWAYS;
                }
                else if (!strcasecmp(pArg, "error"))
                {
                    dwLogLevel = LOG_LEVEL_ERROR;
                }
                else if (!strcasecmp(pArg, "warning"))
                {
                    dwLogLevel = LOG_LEVEL_WARNING;
                }
                else if (!strcasecmp(pArg, "info"))
                {
                    dwLogLevel = LOG_LEVEL_INFO;
                }
                else if (!strcasecmp(pArg, "verbose"))
                {
                    dwLogLevel = LOG_LEVEL_VERBOSE;
                }
                else if (!strcasecmp(pArg, "debug"))
                {
                    dwLogLevel = LOG_LEVEL_DEBUG;
                }
                else
                {
                    dwLogLevel = atoi(pArg);

                    if (dwLogLevel < LOG_LEVEL_ALWAYS ||
                        dwLogLevel > LOG_LEVEL_DEBUG)
                    {

                        EVT_LOG_ERROR(
                                "Error: Invalid log level [%d]",
                                dwLogLevel);
                        ShowUsage(get_program_name(argv[0]));
                        exit(1);
                    }
                }

                pEVTServerInfo->dwLogLevel = dwLogLevel;

                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_CONFIGFILE:
            {
                strncpy(pEVTServerInfo->szConfigFilePath, pArg, PATH_MAX);
                *(pEVTServerInfo->szConfigFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;

            }
            break;
        }
    } while (iArg < argc);

    return 0;
}

static
void
EVTSrvNOPHandler(
    int unused
    )
{
}

DWORD
EVTSrvIgnoreSIGHUP(
    VOID
    )
{
    DWORD dwError = 0;

    // Instead of ignoring the signal by passing SIG_IGN, we install a nop
    // signal handler. This way if we later decide to catch it with sigwait,
    // the signal will still get delivered to the process.
    if (signal(SIGHUP, EVTSrvNOPHandler) < 0) {
        dwError = errno;
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
EVTStartAsDaemon()
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
    dwError = EVTSrvIgnoreSIGHUP();
    BAIL_ON_EVT_ERROR(dwError);

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_EVT_ERROR(dwError);

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

#define DAEMON_NAME "eventlogd"
#define PID_DIR "/var/run"
#define PID_FILE PID_DIR "/" DAEMON_NAME ".pid"

#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

static
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

static
void
EVTCreatePIDFile()
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


static
DWORD
EVTSetConfigDefaults()
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    gServerInfo.dwMaxLogSize = EVT_DEFAULT_MAX_LOG_SIZE;
    gServerInfo.dwMaxRecords =  EVT_DEFAULT_MAX_RECORDS;
    gServerInfo.dwMaxAge = EVT_DEFAULT_MAX_AGE;
    gServerInfo.dwPurgeInterval = EVT_DEFAULT_PURGE_INTERVAL;

    EVTFreeSecurityDescriptor(gServerInfo.pAccess);
    gServerInfo.pAccess = NULL;

    LW_SAFE_FREE_STRING(gServerInfo.pszAllowReadTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowWriteTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowDeleteTo);

    EVT_UNLOCK_SERVERINFO;

    return dwError;
}

static
DWORD
EVTSetServerDefaults()
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    gServerInfo.dwLogLevel = LOG_LEVEL_ERROR;

    *(gServerInfo.szLogFilePath) = '\0';

    memset(gServerInfo.szConfigFilePath, 0, PATH_MAX+1);
    strncpy(gServerInfo.szConfigFilePath, DEFAULT_CONFIG_FILE_PATH, PATH_MAX);
    strcpy(gServerInfo.szCachePath, CACHEDIR);
    strcpy(gServerInfo.szPrefixPath, PREFIXDIR);

    EVT_UNLOCK_SERVERINFO;

    dwError = EVTSetConfigDefaults();

    return dwError;
}

static
void
EVTBlockSelectedSignals()
{
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    sigaddset(&default_signal_mask, SIGINT);
    sigaddset(&default_signal_mask, SIGTERM);
    sigaddset(&default_signal_mask, SIGHUP);
    sigaddset(&default_signal_mask, SIGQUIT);
    sigaddset(&default_signal_mask, SIGPIPE);

    pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
}

static
DWORD
EVTInitLogging(
    PSTR pszProgramName
    )
{
    if ((gServerInfo.dwStartAsDaemon &&
            gServerInfo.szLogFilePath[0] == '\0') ||
            gServerInfo.bLogToSyslog)
    {

        return EVTInitLoggingToSyslog(gServerInfo.dwLogLevel,
                                      pszProgramName,
                                      LOG_PID,
                                      LOG_DAEMON);
    }
    else
    {
        return EVTInitLoggingToFile(gServerInfo.dwLogLevel,
                                    gServerInfo.szLogFilePath);
    }
}

static PSTR gpszAllowReadTo;
static PSTR gpszAllowWriteTo;
static PSTR gpszAllowDeleteTo;
static EVT_CONFIG_TABLE gConfigDescription[] =
{
    {
        "MaxDiskUsage",
        TRUE,
        EVTTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwMaxLogSize)
    },
    {
        "MaxNumEvents",
        TRUE,
        EVTTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwMaxRecords)
    },
    {
        "MaxEventLifespan",
        TRUE,
        EVTTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwMaxAge)
    },
    {
        "EventDbPurgeInterval",
        TRUE,
        EVTTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwPurgeInterval)
    },
    {
        "RemoveEventsAsNeeded",
        TRUE,
        EVTTypeBoolean,
        0,
        -1,
        NULL,
        &(gServerInfo.bRemoveAsNeeded)
    },
    {
        "RegisterTcpIp",
        TRUE,
        EVTTypeBoolean,
        0,
        -1,
        NULL,
        &(gServerInfo.bRegisterTcpIp)
    },
    {
        "AllowReadTo",
        TRUE,
        EVTTypeString,
        0,
        -1,
        NULL,
        &gpszAllowReadTo
    },
    {
        "AllowWriteTo",
        TRUE,
        EVTTypeString,
        0,
        -1,
        NULL,
        &gpszAllowWriteTo
    },
    {
        "AllowDeleteTo",
        TRUE,
        EVTTypeString,
        0,
        -1,
        NULL,
        &gpszAllowDeleteTo
    }
};

VOID
EVTLogConfigReload(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "     Current config settings are...\r\n" \
                 "     Max Disk Usage :                 %d\r\n" \
                 "     Max Number Of Events:            %d\r\n" \
                 "     Max Event Lifespan:              %d\r\n" \
                 "     Remove Events As Needed:         %s\r\n" \
                 "     Register TCP/IP RPC endpoints:   %s\r\n" \
                 "     Allow Read   To :                %s\r\n" \
                 "     Allow Write  To :                %s\r\n" \
                 "     Allow Delete To :                %s\r\n",
                 gServerInfo.dwMaxLogSize,
                 gServerInfo.dwMaxRecords,
                 gServerInfo.dwMaxAge,
                 gServerInfo.bRemoveAsNeeded? "true" : "false",
                 gServerInfo.bRegisterTcpIp ? "true" : "false",
                 gServerInfo.pszAllowReadTo ?
                    gServerInfo.pszAllowReadTo: "",
                 gServerInfo.pszAllowWriteTo ?
                    gServerInfo.pszAllowWriteTo: "",
                 gServerInfo.pszAllowDeleteTo ?
                    gServerInfo.pszAllowDeleteTo: "");

    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_INFO("%s", pszDescription);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

static
DWORD
EVTStringSplit(
    PCSTR   pszInput,
    PDWORD  pdwCount,
    PSTR**  pppszArray
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PCSTR  pszStart = NULL;
    PCSTR  pszEnd = NULL;
    PSTR* ppszArray = NULL;
    PSTR pszAdd = NULL;

    for (pszStart = pszInput; *pszStart !=  0; pszStart++)
    {
        if (*pszStart == ',') dwCount++;
    }
    dwCount++;

    dwError = LwAllocateMemory(
                  (dwCount+1)*sizeof(PCSTR),
                  (PVOID *)&ppszArray);

    dwCount = 0;
    pszStart = pszInput;
    while (TRUE)
    {
        pszEnd = strchr(pszStart, ',');
        if ( pszEnd )
        {
            dwError = LwStrndup(
                         pszStart,
                         pszEnd - pszStart,
                         &pszAdd);
            BAIL_ON_EVT_ERROR(dwError);
        }
        else
        {
            dwError = LwAllocateString(
                        pszStart,
                        &pszAdd);
            BAIL_ON_EVT_ERROR(dwError);
        }
        LwStripWhitespace(pszAdd, TRUE, TRUE);
        if (pszAdd[0])
        {
            ppszArray[dwCount++] = pszAdd;
            pszAdd = NULL;
        }
        else
        {
            LW_SAFE_FREE_STRING(pszAdd);
        }

        if (pszEnd)
        {
            pszStart = pszEnd + 1;
        }
        else
        {
            break;
        }
    }

    *pppszArray = ppszArray;
    *pdwCount = dwCount;

cleanup:
    LW_SAFE_FREE_STRING(pszAdd);
    return dwError;

error:
    LwFreeStringArray(
        ppszArray,
        dwCount);
    goto cleanup;
}

VOID
EVTFreeSidArray(
    PLW_MAP_SECURITY_CONTEXT pContext,
    DWORD dwCount,
    PSID* ppSidArray
    )
{
    DWORD dwInputIndex = 0;

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        LwMapSecurityFreeSid(pContext, &ppSidArray[dwInputIndex]);
    }
    LW_SAFE_FREE_MEMORY(ppSidArray);
}

static
DWORD
EVTNamesToSids(
    PLW_MAP_SECURITY_CONTEXT pContext,
    DWORD dwCount,
    PSTR* ppszArray,
    PDWORD pdwSidCount,
    PSID** pppSidArray
    )
{
    DWORD dwError = 0;
    PSID pSid = NULL;
    DWORD dwInputIndex = 0;
    DWORD dwOutputIndex = 0;
    PSID* ppSidArray = NULL;

    dwError = LwAllocateMemory(
                sizeof(PSID) * dwCount,
                (PVOID)&ppSidArray);
    BAIL_ON_EVT_ERROR(dwError);

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        dwError = LwNtStatusToWin32Error(
            LwMapSecurityGetSidFromName(
                pContext,
                &pSid,
                TRUE,
                ppszArray[dwInputIndex]));
        if (dwError == LW_ERROR_NO_SUCH_USER || dwError == ERROR_NOT_FOUND)
        {
            dwError = LwNtStatusToWin32Error(
                LwMapSecurityGetSidFromName(
                    pContext,
                    &pSid,
                    FALSE,
                    ppszArray[dwInputIndex]));
        }
        if (dwError == LW_ERROR_NO_SUCH_GROUP || dwError == ERROR_NOT_FOUND)
        {
            dwError = 0;
        }
        BAIL_ON_EVT_ERROR(dwError);
        if (pSid)
        {
            ppSidArray[dwOutputIndex] = pSid;
            dwOutputIndex++;
        }
    }

    *pppSidArray = ppSidArray;
    *pdwSidCount = dwOutputIndex;

cleanup:
    return dwError;

error:
    EVTFreeSidArray(
        pContext,
        dwCount,
        ppSidArray);
    goto cleanup;
}

DWORD
EVTGetAllowAcesSize(
    DWORD dwCount,
    PSID* ppSidArray
    )
{
    DWORD dwInputIndex = 0;
    DWORD dwSize = 0;

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        dwSize += RtlLengthAccessAllowedAce(ppSidArray[dwInputIndex]);
    }
    return dwSize;
}

static
DWORD
EVTAddAllowAces(
    PACL pDacl,
    DWORD dwCount,
    PSID* ppSidArray,
    ACCESS_MASK dwAccessMask
    )
{
    DWORD dwInputIndex = 0;
    DWORD dwError = 0;

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        dwError = LwNtStatusToWin32Error(
            RtlAddAccessAllowedAceEx(pDacl,
                                            ACL_REVISION,
                                            0,
                                            dwAccessMask,
                                            ppSidArray[dwInputIndex]));
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
EVTFreeSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    PSID pOwnerSid = NULL;
    BOOLEAN bOwnerDefaulted = FALSE;
    PSID pPrimaryGroupSid = NULL;
    BOOLEAN bPrimaryGroupDefaulted = FALSE;
    PACL pDacl = NULL;
    BOOLEAN bDaclPresent = FALSE;
    BOOLEAN bDaclDefaulted = FALSE;
    PACL pSacl = NULL;
    BOOLEAN bSaclPresent = FALSE;
    BOOLEAN bSaclDefaulted = FALSE;

    if (pSecDesc)
    {
        RtlGetOwnerSecurityDescriptor(pSecDesc,
                                                 &pOwnerSid,
                                                 &bOwnerDefaulted);
        LW_SAFE_FREE_MEMORY(pOwnerSid);

        RtlGetGroupSecurityDescriptor(pSecDesc,
                                                 &pPrimaryGroupSid,
                                                 &bPrimaryGroupDefaulted);
        LW_SAFE_FREE_MEMORY(pPrimaryGroupSid);

        RtlGetDaclSecurityDescriptor(pSecDesc,
                                                &bDaclPresent,
                                                &pDacl,
                                                &bDaclDefaulted);
        LW_SAFE_FREE_MEMORY(pDacl);

        RtlGetSaclSecurityDescriptor(pSecDesc,
                                                &bSaclPresent,
                                                &pSacl,
                                                &bSaclDefaulted);
        LW_SAFE_FREE_MEMORY(pSacl);

        LW_SAFE_FREE_MEMORY(pSecDesc);
    }
}

static
DWORD
EVTCreateAccessDescriptor(
    PCSTR pszAllowReadTo,
    PCSTR pszAllowWriteTo,
    PCSTR pszAllowDeleteTo,
    PSECURITY_DESCRIPTOR_ABSOLUTE* ppDescriptor,
    PBOOLEAN pbFullyResolved
    )
{
    PSECURITY_DESCRIPTOR_ABSOLUTE pDescriptor = NULL;
    DWORD dwCount = 0;
    PSTR* ppszArray = NULL;
    DWORD dwReadCount = 0;
    DWORD dwWriteCount = 0;
    DWORD dwDeleteCount = 0;
    PSID* ppReadList = NULL;
    PSID* ppWriteList = NULL;
    PSID* ppDeleteList = NULL;
    PACL pDacl = NULL;
    DWORD dwDaclSize = 0;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;
    DWORD dwError = 0;
    PSID pLocalSystem = NULL;
    PSID pAdministrators = NULL;
    BOOLEAN bFullyResolved = TRUE;

    dwError = LwAllocateWellKnownSid(
                    WinLocalSystemSid,
                    NULL,
                    &pLocalSystem,
                    NULL);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(
                    WinBuiltinAdministratorsSid,
                    NULL,
                    &pAdministrators,
                    NULL);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        LwMapSecurityCreateContext(
                &pContext));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwAllocateMemory(
                SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                (PVOID*)&pDescriptor);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateSecurityDescriptorAbsolute(
                pDescriptor,
                SECURITY_DESCRIPTOR_REVISION));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTStringSplit(
                pszAllowReadTo,
                &dwCount,
                &ppszArray);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTNamesToSids(
                pContext,
                dwCount,
                ppszArray,
                &dwReadCount,
                &ppReadList);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwReadCount < dwCount)
    {
        bFullyResolved = FALSE;
    }

    LwFreeStringArray(
        ppszArray,
        dwCount);
    ppszArray = NULL;

    dwError = EVTStringSplit(
                pszAllowWriteTo,
                &dwCount,
                &ppszArray);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTNamesToSids(
                pContext,
                dwCount,
                ppszArray,
                &dwWriteCount,
                &ppWriteList);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwWriteCount < dwCount)
    {
        bFullyResolved = FALSE;
    }

    LwFreeStringArray(
        ppszArray,
        dwCount);
    ppszArray = NULL;

    dwError = EVTStringSplit(
                pszAllowDeleteTo,
                &dwCount,
                &ppszArray);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTNamesToSids(
                pContext,
                dwCount,
                ppszArray,
                &dwDeleteCount,
                &ppDeleteList);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwDeleteCount < dwCount)
    {
        bFullyResolved = FALSE;
    }

    LwFreeStringArray(
        ppszArray,
        dwCount);
    ppszArray = NULL;

    dwDaclSize = ACL_HEADER_SIZE +
        EVTGetAllowAcesSize(dwReadCount, ppReadList) +
        EVTGetAllowAcesSize(dwWriteCount, ppWriteList) +
        EVTGetAllowAcesSize(dwDeleteCount, ppDeleteList);

    dwError = LwAllocateMemory(
        dwDaclSize,
        OUT_PPVOID(&pDacl));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAddAllowAces(
                pDacl,
                dwReadCount,
                ppReadList,
                EVENTLOG_READ_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAddAllowAces(
                pDacl,
                dwWriteCount,
                ppWriteList,
                EVENTLOG_WRITE_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAddAllowAces(
                pDacl,
                dwWriteCount,
                ppWriteList,
                EVENTLOG_DELETE_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetDaclSecurityDescriptor(
            pDescriptor,
            TRUE,
            pDacl,
            FALSE));
    BAIL_ON_EVT_ERROR(dwError);
    pDacl = NULL;

    dwError = LwNtStatusToWin32Error(
        RtlSetOwnerSecurityDescriptor(
            pDescriptor,
            pLocalSystem,
            FALSE));
    BAIL_ON_EVT_ERROR(dwError);
    pLocalSystem = NULL;

    dwError = LwNtStatusToWin32Error(
        RtlSetGroupSecurityDescriptor(
            pDescriptor,
            pAdministrators,
            FALSE));
    BAIL_ON_EVT_ERROR(dwError);
    pAdministrators = NULL;

    *ppDescriptor = pDescriptor;
    if (pbFullyResolved)
    {
        *pbFullyResolved = bFullyResolved;
    }

cleanup:
    if (ppszArray)
    {
        LwFreeStringArray(
            ppszArray,
            dwCount);
    }
    EVTFreeSidArray(
        pContext,
        dwReadCount,
        ppReadList);
    EVTFreeSidArray(
        pContext,
        dwWriteCount,
        ppWriteList);
    EVTFreeSidArray(
        pContext,
        dwDeleteCount,
        ppDeleteList);
    LwMapSecurityFreeContext(&pContext);
    return dwError;

error:
    EVTFreeSecurityDescriptor(pDescriptor);
    *ppDescriptor = NULL;
    if (pbFullyResolved)
    {
        *pbFullyResolved = FALSE;
    }
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pLocalSystem);
    LW_SAFE_FREE_MEMORY(pAdministrators);
    goto cleanup;
}

static
DWORD
EVTReadEventLogConfigSettings()
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    EVT_LOG_INFO("Read Eventlog configuration settings");

    dwError = EVTSetConfigDefaults();
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOCK_SERVERINFO;
    bLocked = TRUE;

    dwError = EVTProcessConfig(
                "Services\\eventlog\\Parameters",
                "Policy\\Services\\eventlog\\Parameters",
                gConfigDescription,
                sizeof(gConfigDescription)/sizeof(gConfigDescription[0]));

    LW_SAFE_FREE_STRING(gServerInfo.pszAllowReadTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowWriteTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowDeleteTo);
    gServerInfo.pszAllowReadTo = gpszAllowReadTo;
    gServerInfo.pszAllowWriteTo = gpszAllowWriteTo;
    gServerInfo.pszAllowDeleteTo = gpszAllowDeleteTo;
    gpszAllowReadTo = NULL;
    gpszAllowWriteTo = NULL;
    gpszAllowDeleteTo = NULL;

    EVTFreeSecurityDescriptor(gServerInfo.pAccess);
    gServerInfo.pAccess = NULL;

    EVT_UNLOCK_SERVERINFO;
    bLocked = FALSE;

    EVTLogConfigReload();

cleanup:

    LW_SAFE_FREE_STRING(gpszAllowReadTo);
    LW_SAFE_FREE_STRING(gpszAllowWriteTo);
    LW_SAFE_FREE_STRING(gpszAllowDeleteTo);

    if (bLocked)
    {
        EVT_UNLOCK_SERVERINFO;
    }

    return dwError;

error:
    goto cleanup;
}

static
VOID
EVTInterruptHandler(
    int Signal
    )
{
    if (Signal == SIGINT)
    {
        raise(SIGTERM);
    }
}

static
DWORD
EVTHandleSignals(
    void
    )
{
    DWORD dwError = 0;
    struct sigaction action;
    sigset_t catch_signal_mask;
    int which_signal = 0;
    int sysRet = 0;

    // After starting up threads, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = EVTInterruptHandler;

    sysRet = sigaction(SIGINT, &action, NULL);
    dwError = (sysRet != 0) ? errno : 0;
    BAIL_ON_EVT_ERROR(dwError);

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    dwError = pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL);
    BAIL_ON_EVT_ERROR(dwError);

    // These should already be blocked...
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGTERM);
    sigaddset(&catch_signal_mask, SIGQUIT);
    sigaddset(&catch_signal_mask, SIGHUP);
    sigaddset(&catch_signal_mask, SIGPIPE);

    while (1)
    {
        /* Wait for a signal to arrive */
        sigwait(&catch_signal_mask, &which_signal);

        switch (which_signal)
        {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
            {
                goto error;
            }
            case SIGPIPE:
            {
                EVT_LOG_DEBUG("Handled SIGPIPE");

                break;
            } 
            case SIGHUP:
            {
                dwError = EVTReadEventLogConfigSettings();
                BAIL_ON_EVT_ERROR(dwError);

                break;
            }
        }
    }

error:

    return dwError;
}

static
void*
EVTNetworkThread(
    void* pArg
    )
{
    DWORD dwError = 0;
    DWORD index = 0;
    static ENDPOINT endpoints[] =
    {
        {"ncacn_ip_tcp", NULL, TRUE},
        {NULL, NULL}
    };
    struct timespec delay = {5, 0};
    BOOLEAN *pbExitNow = (BOOLEAN *)pArg;
 
    while (endpoints[index].protocol && !*pbExitNow)
    {
        dwError = EVTRegisterEndpoint(
            "Likewise Eventlog Service",
            &endpoints[index]
            );
        
        if (dwError)
        {
            dwError = 0;
            dcethread_delay(&delay);
        }
        else
        {
            if (endpoints[index].endpoint)
            {
                EVT_LOG_VERBOSE("Listening on %s:[%s]",
                                endpoints[index].protocol,
                                endpoints[index].endpoint);
            }
            else
            {
                EVT_LOG_VERBOSE("Listening on %s",
                                endpoints[index].protocol,
                                endpoints[index].endpoint);
            }
                                
            index++;
        }
    }

    dwError = EVTListen();

    if (dwError)
    {
        raise(SIGTERM);
    }
    return NULL;
}

int
main(
    int argc,
    char* argv[])
{
    DWORD dwError = 0;
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;
    int ret = 0;
    dcethread* networkThread = NULL;
    BOOLEAN bExitNow = FALSE;
    BOOLEAN bRegisterTcpIp = TRUE;

    dwError = EVTSetServerDefaults();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTParseArgs(
                    argc,
                    argv,
                    &gServerInfo
                 );
    BAIL_ON_EVT_ERROR(dwError);


    dwError = LwEvtDbCreateDB(gServerInfo.bReplaceDB);
    BAIL_ON_EVT_ERROR(dwError);

    if (gServerInfo.bReplaceDB) {
        goto cleanup;
    }

    if (gServerInfo.dwStartAsDaemon) {

        dwError = EVTStartAsDaemon();
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (atexit(EVTExitHandler) < 0) {
        dwError = errno;
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVTCreatePIDFile();

    dwError = EVTInitLogging(get_program_name(argv[0]));
    BAIL_ON_EVT_ERROR(dwError);

    EVTBlockSelectedSignals();

    dwError = EVTReadEventLogConfigSettings();
    if (dwError != 0)
    {
        EVT_LOG_ERROR("Failed to read eventlog configuration.  Error code: [%u]\n", dwError);
        dwError = 0;
    }

    dwError = EVTGetRegisterTcpIp(&bRegisterTcpIp);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbInitEventDatabase();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwmEvtSrvStartListenThread();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTRegisterInterface();
    BAIL_ON_EVT_ERROR(dwError);

    if (bRegisterTcpIp)
    {
        dwError = LwMapErrnoToLwError(dcethread_create(
                                      &networkThread,
                                      NULL,
                                      EVTNetworkThread,
                                      &bExitNow));
        BAIL_ON_EVT_ERROR(dwError);
    }

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);
        
        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            EVT_LOG_ERROR("Could not notify service manager: %s (%i)", strerror(errno), errno);
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_EVT_ERROR(dwError);
        }

        close(notifyFd);
    }

    dwError = EVTHandleSignals();
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_INFO("Eventlog Service exiting...");

    bExitNow = TRUE;

    dwError = EVTUnregisterAllEndpoints();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTStopListen();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwmEvtSrvStopListenThread();
    BAIL_ON_EVT_ERROR(dwError);
    
    if (bRegisterTcpIp)
    {
        dwError = LwMapErrnoToLwError(dcethread_interrupt(networkThread));
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (bRegisterTcpIp)
    {
        dwError = LwMapErrnoToLwError(dcethread_join(networkThread, NULL));
        BAIL_ON_EVT_ERROR(dwError);
    }

 cleanup:

    /*
     * Indicate that the process is exiting
     */
    EVTSetProcessShouldExit(TRUE);

    EVTCloseLog();

    LwEvtDbShutdownEventDatabase();

    EVTSetConfigDefaults();
    EVTFreeSecurityDescriptor(gServerInfo.pAccess);
    gServerInfo.pAccess = NULL;

    EVTSetProcessExitCode(dwError);

    exit (dwError);

error:

    EVT_LOG_ERROR("Eventlog exiting due to error [code:%d]", dwError);

    goto cleanup;
}
