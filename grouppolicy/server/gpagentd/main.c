#include "includes.h"

static
void delete_pid_file();

static
CENTERROR
GetProcessExitCode(
    PDWORD pdwExitCode
    );

VOID
LogProcessStartedEvent(
    VOID
    );

VOID
LogProcessStoppedEvent(
    DWORD dwExitCode
    );

VOID
LogProcessFailureEvent(
    DWORD dwErrCode
    );

VOID
LogConfigReloadEvent(
    VOID
    );

static
void
GPOExitHandler(void)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    sprintf(szErrCodeFilePath, "%s/gpagentd.err", CACHEDIR);

    ceError = GPACheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        ceError = LwRemoveFile(szErrCodeFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GetProcessExitCode(&dwExitCode);
    BAIL_ON_CENTERIS_ERROR(ceError);

    LogProcessStoppedEvent(dwExitCode);

    if (dwExitCode) {
        fp = fopen(szErrCodeFilePath, "w");
        if (fp == NULL) {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        fprintf(fp, "%d\n", dwExitCode);
    }

error:

    if (fp != NULL) {
        fclose(fp);
    }

    delete_pid_file();
}

static
PSTR
get_program_name(
    PSTR pszFullProgramPath
    )
{
    PSTR pszNameStart;

    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

BOOLEAN
ProcessShouldExit()
{
    BOOLEAN bResult = 0;

    GPA_LOCK_SERVERINFO;

    bResult = gServerInfo.bProcessShouldExit;

    GPA_UNLOCK_SERVERINFO;

    return bResult;
}

void
SetProcessShouldExit(
    BOOLEAN val
    )
{
    GPA_LOCK_SERVERINFO;

    gServerInfo.bProcessShouldExit = val;

    GPA_UNLOCK_SERVERINFO;
}

BOOLEAN                 
IsGroupPolicyDisabled(
    VOID                
    )                   
{                       
    BOOLEAN bResult = FALSE;
                        
    GPA_LOCK_SERVERINFO;
            
    bResult = gServerInfo.bDisableGP;
    
    GPA_UNLOCK_SERVERINFO;
    
    return bResult;
}

static
CENTERROR
GetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    *pdwExitCode = gServerInfo.dwExitCode;

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

static
void
SetProcessExitCode(
    DWORD dwExitCode
    )
{
    GPA_LOCK_SERVERINFO;

    gServerInfo.dwExitCode = dwExitCode;

    GPA_UNLOCK_SERVERINFO;
}

CENTERROR
GetConfigPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    ceError = LwAllocateString(gServerInfo.pszConfigFilePath, ppszPath);

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetPolicySettingsPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    ceError = LwAllocateString(gServerInfo.pszPolicySettingsFilePath, ppszPath);

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetExtensionLibPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    ceError = LwAllocateString(gServerInfo.pszExtensionLibPath, ppszPath);

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetPrefixPath(
    PSTR* ppszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    ceError = LwAllocateString(gServerInfo.pszPrefixPath, ppszPath);

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetComputerPollingInterval(
    PDWORD pdwPollingInterval
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    *pdwPollingInterval = gServerInfo.dwComputerPollingInterval;

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetUserPollingInterval(
    PDWORD pdwPollingInterval
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    *pdwPollingInterval = gServerInfo.dwUserPollingInterval;

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetEnableEventLog(
    PBOOLEAN pbEnableEventLog
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    *pbEnableEventLog = gServerInfo.bEnableEventLog;

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetLoopbackProcessingMode(
    LoopbackProcessingMode * plpmUserPolicyMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    *plpmUserPolicyMode = gServerInfo.lpmUserPolicyMode;

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetDistroName(
    PSTR* ppszDistroName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    ceError = LwAllocateString(gServerInfo.pszDistroName, ppszDistroName);

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

CENTERROR
GetDistroVersion(
    PSTR* ppszDistroVersion
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOCK_SERVERINFO;

    ceError = LwAllocateString(gServerInfo.pszDistroVersion, ppszDistroVersion);

    GPA_UNLOCK_SERVERINFO;

    return (ceError);
}

void
ShowUsage(
    const PSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
           "          [--logfile logFilePath]\n"
           "          [--loglevel {none, always, error, warning, info, verbose, debug}]\n"
           "          [--configfile configfilepath]\n",
           pszProgramName);
}

static
CENTERROR
ParseArgs(
    int argc,
    PSTR argv[],
    PGP_ARGS pParsedArgs
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

    memset(pParsedArgs, 0, sizeof(*pParsedArgs));
    pParsedArgs->dwLogLevel = LOG_LEVEL_ERROR;

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
            if (strcmp(pArg, "--logfile") == 0) {
                parseMode = PARSE_MODE_LOGFILE;
            }
            else if ((strcmp(pArg, "--help") == 0) ||
                     (strcmp(pArg, "-h") == 0)) {
                ShowUsage(get_program_name(argv[0]));
                exit(0);
            }
            else if (strcmp(pArg, "--start-as-daemon") == 0) {
                pParsedArgs->dwStartAsDaemon = 1;
            }
            else if (strcmp(pArg, "--configfile") == 0) {
                parseMode = PARSE_MODE_CONFIGFILE;
            }
            else if (strcmp(pArg, "--loglevel") == 0) {
                parseMode = PARSE_MODE_LOGLEVEL;
            } else if (strcmp(pArg, "--donothing") == 0) {
                pParsedArgs->dwDoNothing = 1;
            } else {
                fprintf(stderr, "Error: Unrecognized command line option [%s]\n", pArg);
                ShowUsage(get_program_name(argv[0]));
                exit(1);
            }
        }
        break;
        case PARSE_MODE_LOGFILE:
        {
            pParsedArgs->pszLogFilePath = pArg;
            parseMode = PARSE_MODE_OPEN;
        }
        break;
        case PARSE_MODE_LOGLEVEL:
        {
            if (!strcasecmp(pArg, "none"))
            {
                pParsedArgs->dwLogLevel = LOG_LEVEL_NOTHING;
            }
            else if (!strcasecmp(pArg, "always"))
            {
                pParsedArgs->dwLogLevel = LOG_LEVEL_ALWAYS;
            }
            else if (!strcasecmp(pArg, "error"))
            {
                pParsedArgs->dwLogLevel = LOG_LEVEL_ERROR;
            }
            else if (!strcasecmp(pArg, "warning"))
            {
                pParsedArgs->dwLogLevel = LOG_LEVEL_WARNING;
            }
            else if (!strcasecmp(pArg, "info"))
            {
                pParsedArgs->dwLogLevel = LOG_LEVEL_INFO;
            }
            else if (!strcasecmp(pArg, "verbose"))
            {
                pParsedArgs->dwLogLevel = LOG_LEVEL_VERBOSE;
            }
            else if (!strcasecmp(pArg, "debug"))
            {
                pParsedArgs->dwLogLevel = LOG_LEVEL_LOCKS;
            }
            else
            {
                fprintf(stderr, "Error: Invalid log level [%s]\n", pArg);
                ShowUsage(get_program_name(argv[0]));
                exit(1);
            }
 
            parseMode = PARSE_MODE_OPEN;
        }
        break;
        case PARSE_MODE_CONFIGFILE:
        {
            pParsedArgs->pszConfigFilePath = pArg;
            parseMode = PARSE_MODE_OPEN;
        }
        break;
        }
    } while (iArg < argc);

    return CENTERROR_SUCCESS;
}

static
CENTERROR
start_as_daemon()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    if ((pid = fork()) != 0) {
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
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    ceError = chdir("/");
    BAIL_ON_CENTERIS_ERROR(ceError);

    // Clear our file mode creation mask
    umask(0);

    for (iFd = 0; iFd < 3; iFd++)
        close(iFd);

    for (iFd = 0; iFd < 3; iFd++) {

        fd = open("/dev/null", O_RDWR, 0);
        if (fd < 0) {
            fd = open("/dev/null", O_WRONLY, 0);
        }
        if (fd < 0) {
            GPA_LOG_ALWAYS("Failed to open fd %d: %d", iFd, errno);
            exit(1);
        }
        if (fd != iFd) {
            GPA_LOG_ALWAYS("Mismatched file descriptors: got %d instead of %d", fd, iFd);
            exit(1);
        }
    }

error:

    return ceError;
}

#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

static
pid_t pid_from_pid_file(BOOLEAN bStartUp)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[PID_FILE_CONTENTS_SIZE];

    fd = open(GP_AGENT_PID_FILE, O_RDONLY, 0644);
    if (fd < 0) {
        goto error;
    }

    result = read(fd, contents, sizeof(contents)-1);
    if (result < 0) {
        goto error;
    } else if (result == 0) {
        unlink(GP_AGENT_PID_FILE);
        goto error;
    }
    contents[result-1] = 0;

    result = atoi(contents);
    if (result < 0) {
        result = -1;
        goto error;
    } else if (result == 0) {
        unlink(GP_AGENT_PID_FILE);
        goto error;
    }

    pid = (pid_t) result;
    result = kill(pid, 0);
    if (result != 0 || errno == ESRCH) {
        unlink(GP_AGENT_PID_FILE);
        pid = 0;
    } else {
        if (bStartUp)
        {
            // Verify that the peer process is a gpagent, we can only do this in daemon startup.
        //TODO:commenting temporarily,
            //ceError = CTMatchProgramToPID(GP_AGENT_DAEMON_NAME, pid);
            if (CENTERROR_NO_SUCH_PROCESS == ceError) {
                unlink(GP_AGENT_PID_FILE);
                pid = 0;
            }
        }
    }

error:
    if (fd != -1) {
        close(fd);
    }

    return pid;
}

static
void delete_pid_file()
{
    pid_t pid;

    pid = pid_from_pid_file(FALSE);
    if (pid == getpid()) {
        LwRemoveFile(GP_AGENT_PID_FILE);
    }
}

static
void create_pid_file()
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = pid_from_pid_file(TRUE);
    if (pid > 0) {
        GPA_LOG_ERROR("Daemon already running as %d", (int) pid);
        result = -1;
        goto error;
    }

    fd = open(GP_AGENT_PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0) {
        GPA_LOG_ERROR("Could not create pid file: %s", strerror(errno));
        result = 1;
        goto error;
    }

    pid = getpid();
    snprintf(contents, sizeof(contents)-1, "%d\n", (int) pid);
    contents[sizeof(contents)-1] = 0;
    len = strlen(contents);

    result = (int) write(fd, contents, len);
    if ( result != (int) len ) {
        GPA_LOG_ERROR("Could not write to pid file: %s", strerror(errno));
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
CENTERROR
handleSignal(
    const siginfo_t* pSignalInfo,
    PBOOLEAN pbExitProcess
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    *pbExitProcess = 0;

    if (pSignalInfo != NULL) {

        switch(pSignalInfo->si_signo)
        {
        case SIGTERM:
        case SIGINT:
        {
            *pbExitProcess = 1;
        }
        break;
        case SIGHUP:
        {
            ceError = LoadClientSideExtensions();
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LoadGroupPolicyConfigurationSettings(TRUE);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        break;
        default:
        {
            GPA_LOG_INFO("Received signal [%d]", pSignalInfo->si_signo);
        }
        break;
        }
    }

    return ceError;

error:

    *pbExitProcess = 1;

    return ceError;
}

static
CENTERROR
InitServerState(
    PGP_ARGS pArgs
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct utsname info;

    pthread_mutex_init(&gServerInfo.lock, NULL);

    /* The following settings are now configured with the policy settings conf file, which
       is also managed via Group Policy features */
    gServerInfo.dwComputerPollingInterval = GPO_DEFAULT_POLL_TIMEOUT_SECS;
    gServerInfo.dwUserPollingInterval = GPO_DEFAULT_POLL_TIMEOUT_SECS;
    gServerInfo.lpmUserPolicyMode = LOOPBACK_PROCESSING_MODE_USER_ONLY;
    gServerInfo.bEnableEventLog = FALSE;
    gServerInfo.bMonitorSudoers = FALSE;
    gServerInfo.bDisableGP = FALSE;

    ceError = LwAllocateString(CONFIGDIR PATH_SEPARATOR_STR POLICY_SETTINGS_FILE_NAME, &gServerInfo.pszPolicySettingsFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pArgs->pszConfigFilePath)
    {
        ceError = LwAllocateString(pArgs->pszConfigFilePath, &gServerInfo.pszConfigFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = LwAllocateString(CONFIGDIR PATH_SEPARATOR_STR DEFAULT_CONFIG_FILE_NAME, &gServerInfo.pszConfigFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString(EXTNLIBDIR, &gServerInfo.pszExtensionLibPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString(PREFIXDIR, &gServerInfo.pszPrefixPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* Determine OS Distro and Version info */
    if (uname(&info))
    {
#if defined(__LWI_DARWIN__)
        ceError = LwAllocateString("Mac OSX", &gServerInfo.pszDistroName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPACaptureOutput("sw_vers -productVersion", &gServerInfo.pszDistroVersion);
        BAIL_ON_CENTERIS_ERROR(ceError);
#else
        ceError = LwAllocateString("Unspecified", &gServerInfo.pszDistroName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateString("Unknown", &gServerInfo.pszDistroVersion);
        BAIL_ON_CENTERIS_ERROR(ceError);
#endif
    }
    else
    {
        ceError = LwAllocateString(info.sysname, &gServerInfo.pszDistroName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateString(info.release, &gServerInfo.pszDistroVersion);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateStringPrintf(&gServerInfo.pszKrb5CredCacheEnvSetting,
                                     "%s=%s/krb5cc_gpagentd",
                                     KRB5CCENVVAR,
                                     CACHEDIR);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /*
     * putenv adds the variable to the env, and uses the buffer we provide, so
     * we cannot use an automatic variable or free the memory until we are done.
     */
    if (putenv(gServerInfo.pszKrb5CredCacheEnvSetting) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwCreateDirectory( CACHEDIR,
                                 S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwCreateDirectory( BASEFILESDIR,
                                 S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH );
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return (ceError);
}

static
void
CleanupServerState()
{
    LW_SAFE_FREE_STRING(gServerInfo.pszConfigFilePath);
    LW_SAFE_FREE_STRING(gServerInfo.pszExtensionLibPath);
    LW_SAFE_FREE_STRING(gServerInfo.pszPrefixPath);
    LW_SAFE_FREE_STRING(gServerInfo.pszDistroName);
    LW_SAFE_FREE_STRING(gServerInfo.pszDistroVersion);
    if (gServerInfo.pszKrb5CredCacheEnvSetting) {
        putenv(KRB5CCENVVAR"=");
    }
    LW_SAFE_FREE_STRING(gServerInfo.pszKrb5CredCacheEnvSetting);
}

static
CENTERROR
StartListener()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = pthread_create(&gServerThread, NULL, gpa_listener_main, NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pgServerThread = &gServerThread;

    return (ceError);

error:

    pgServerThread = NULL;

    return (ceError);
}

#if !IS_ACCEPT_A_CANCELLATION_POINT
static
void
FakeClientConnection()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR  szClientPath[PATH_MAX+1];
    DWORD dwFd = -1;
    BOOLEAN bFileExists = FALSE;
    struct sockaddr_un unixaddr;

    if ((dwFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf(szClientPath, "/var/tmp/.gpclient_%05ld", (long)getpid());

    ceError = GPACheckSockExists(szClientPath, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bFileExists) {
        ceError = LwRemoveFile(szClientPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bFileExists = FALSE;

    memset(&unixaddr, 0, sizeof(unixaddr));
    unixaddr.sun_family = AF_UNIX;
    strcpy(unixaddr.sun_path, szClientPath);
    if (bind(dwFd, (struct sockaddr*)&unixaddr, sizeof(unixaddr)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bFileExists = TRUE;

    ceError = LwChangePermissions(unixaddr.sun_path, S_IRWXU);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memset(&unixaddr, 0, sizeof(unixaddr));
    unixaddr.sun_family = AF_UNIX;
    sprintf(unixaddr.sun_path, "%s/.gpagentd", CACHEDIR);

    if (connect(dwFd, (struct sockaddr*)&unixaddr, sizeof(unixaddr)) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (dwFd >= 0) {
        close(dwFd);
    }

    if (bFileExists)
    {
        LwRemoveFile(szClientPath);
    }
}
#endif

static
CENTERROR
StopListener()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (pgServerThread) {
        /* On some platforms (e.g., Darwin), accept is not a cancellation point.
         * Setting the cancellation type to async (instead of deferred)
         * might cause problems with locked resources.  So, unblock the listener
         * thread's accept call by opening a dummy connection.
         */
        FakeClientConnection();
        pthread_join(gServerThread, NULL);
        pgServerThread = NULL;
    }

    return (ceError);
}

static
CENTERROR
InitializeLogging(
    DWORD dwLogLevel,
    PSTR pszSyslogIdentifier,
    PSTR pszLogFilePath,
    BOOLEAN isStartAsDaemon
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (isStartAsDaemon && IsNullOrEmptyString(pszLogFilePath))
    {
        ceError = gpa_init_logging_to_syslog(dwLogLevel,
                                             pszSyslogIdentifier,
                                             LOG_PID,
                                             LOG_DAEMON);
    }
    else
    {
        ceError = gpa_init_logging_to_file(dwLogLevel,
                                           pszLogFilePath);
    }

    return ceError;
}

static
VOID
InitGlobals()
{
    InitCSEGlobals();
}

static GPA_CONFIG_TABLE gConfigDescription[] =
{
    {
        "ComputerPolicyRefreshInterval",
        TRUE,
        GPATypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwComputerPollingInterval)
    },
    {
        "UserPolicyRefreshInterval",
        TRUE,
        GPATypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwUserPollingInterval)
    },
    {
        "UserPolicyLoopbackProcessingMode",
        TRUE,
        GPATypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.lpmUserPolicyMode)
    },
    {
        "EnableEventLog",
        TRUE,
        GPATypeBoolean,
        0,
        -1,
        NULL,
        &(gServerInfo.bEnableEventLog)
    },
    {
        "MonitorSudoers",
        TRUE,
        GPATypeBoolean,
        0,
        -1,
        NULL,
        &(gServerInfo.bMonitorSudoers)
    }
};

int
main(
    int argc,
    char* argv[]
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    siginfo_t sig_info;
#if HAVE_SIGTIMEDWAIT
    struct timespec timeout;
#endif /* HAVE_SIGTIMEDWAIT */
    BOOLEAN bAbort = FALSE;
    GP_ARGS args;
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;
    int ret = 0;

    setlocale(LC_ALL, "");
    InitGlobals();

    ceError = InitializeLogging(LOG_LEVEL_VERBOSE,
                                get_program_name(argv[0]),
                                NULL,
                                FALSE);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = ParseArgs(argc, argv, &args);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = InitializeLogging(args.dwLogLevel,
                                get_program_name(argv[0]),
                                args.pszLogFilePath,
                                args.dwStartAsDaemon);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (args.dwStartAsDaemon) {

        ceError = start_as_daemon();
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    create_pid_file();

    GPAInitializeEvents();

    if (atexit(GPOExitHandler) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    /*
     * This is a test stub
     * Turn this on, to run the gpagent in a safe mode
     * to test the remaining components in the system
     */
    if (args.dwDoNothing) {
        GPA_LOG_ALWAYS("Running under gpagent-do-nothing mode...");
        for(;;) {
            sleep(5 * 1000);
        }
    }

    ceError = GPAInitAuthService();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAEnsureAuthIsRunning();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = InitServerState(&args);
    BAIL_ON_CENTERIS_ERROR(ceError);

    // We don't want all our threads to get async signals
    // So, we will handle all the signals in our main thread
    sigemptyset(&group_policy_signal_set);
    sigaddset(&group_policy_signal_set, SIGTERM);
    sigaddset(&group_policy_signal_set, SIGINT);
    sigaddset(&group_policy_signal_set, SIGHUP);
    sigaddset(&group_policy_signal_set, SIGPIPE);

    /* Block signals in the initial thread */
    pthread_sigmask(SIG_BLOCK, &group_policy_signal_set, NULL);

    ceError = LwDsCacheAddPidException(getpid());
    if (ceError == LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK)
    {
        GPA_LOG_ERROR("Could not register process pid (%d) with Mac DirectoryService Cache plugin", (int) getpid());
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Read in group policy settings from grouppolicy-settings.conf */
    ceError = LoadGroupPolicyConfigurationSettings(FALSE);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = StartListener();
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);
        
        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            GPA_LOG_ERROR("Could not notify service manager: %s (%i)", strerror(errno), errno);
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        close(notifyFd);
    }

    if (IsGroupPolicyDisabled())
    {
        GPA_LOG_ERROR("Group policy feature disabled due to runtime configuration restriction.");
        GPA_LOG_ERROR("Process will run but not perform group policy processing.");
    }
    else
    {
        ceError = GPAStartMachinePolicyThread();
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAStartUserPolicyThread();
        BAIL_ON_CENTERIS_ERROR(ceError);

        LogProcessStartedEvent();
    }

    // Some Operating systems don't have sigtimedwait
    // Mac OS X is an example.
#if HAVE_SIGTIMEDWAIT
    timeout.tv_sec = 60;
    timeout.tv_nsec = 0;

    bAbort = FALSE;
    do {
        memset(&sig_info, 0, sizeof(siginfo_t));
        if (sigtimedwait(&group_policy_signal_set, &sig_info, &timeout) < 0)
        {
            if (errno == EAGAIN) {

                // timed out

            } else if (errno == EINTR) {

                continue;

            } else {

                GPA_LOG_ERROR("%s at %s:%d", strerror(errno), __FILE__, __LINE__);
                bAbort = 1;

            }
        }
        else
        {
            ceError = handleSignal(&sig_info, &bAbort);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } while (!bAbort);
#else
    bAbort = FALSE;
    do {
        if (sigwait(&group_policy_signal_set, &sig_info.si_signo) == 0) {
            ceError = handleSignal(&sig_info, &bAbort);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    } while (!bAbort);
#endif

error:

    GPA_LOG_INFO("Group Policy Service exiting...");

    LogProcessFailureEvent(ceError);

    /*
     * Indicate that the process is exiting
     */
    SetProcessShouldExit(GPO_TRUE);

    LwDsCacheRemovePidException(getpid());

    StopListener();
    GPAStopMachinePolicyThread();
    GPAStopUserPolicyThread();

    gpa_close_log();

    GPAShutdownEvents();

    SetProcessExitCode(ceError);

    CleanupServerState();

    return ceError;
}

/* HACK - This is to satisfy the less than modular test code */
CENTERROR
GPOSetServerDefaults()
{
    GP_ARGS args = { 0 };
    return InitServerState(&args);
}

#if defined(ENABLE_SLED_ONLY)
#define SUSE_RELEASE_FILE           "/etc/SuSE-release"

static
CENTERROR
IsSuSELinux(
    PBOOLEAN pbValue
    )
{   
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExists = FALSE;
            
    ceError = GPACheckFileExists( SUSE_RELEASE_FILE,
                                 &bExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pbValue = bExists;
    
error:

    return ceError;
} 

static
CENTERROR
IsSuSESLED(
    PBOOLEAN pbValue
    )
{   
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szVersion[256] = { 0 };
    BOOLEAN bSLED = FALSE;
    FILE * fp = NULL;
            
    ceError = GPAOpenFile(SUSE_RELEASE_FILE, "r", &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);
            
    if (!fp)
    {
        *pbValue = FALSE;
        goto error;
    }

    if ( NULL == fgets( szVersion,
                        sizeof(szVersion),
                        fp) )
    {
        if (feof(fp))
        {
            *pbValue = FALSE;
            goto error;
        }
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    LwStripWhitespace(szVersion);    

    if (strstr(szVersion, "SUSE Linux Enterprise Desktop 10") == szVersion ||
        strstr(szVersion, "SUSE Linux Enterprise Desktop 11") == szVersion)
    {
        bSLED = TRUE;
    }

    *pbValue = bSLED;
    
error:

    if (fp)
    {
        GPACloseFile(fp);
    }

    return ceError;
} 
#endif

CENTERROR
LoadGroupPolicyConfigurationSettings(
    BOOLEAN bRefresh)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LoopbackProcessingMode lpmUserPolicyMode = LOOPBACK_PROCESSING_MODE_USER_ONLY;
    DWORD dwComputerPollingInterval = GPO_DEFAULT_POLL_TIMEOUT_SECS;
    DWORD dwUserPollingInterval = GPO_DEFAULT_POLL_TIMEOUT_SECS;
    BOOLEAN bEnableEventLog = FALSE;
    BOOLEAN bMonitorSudoers = FALSE;
    BOOLEAN bDisableGP = FALSE;
#if defined(ENABLE_SLED_ONLY)
    BOOLEAN bIsSuSE = FALSE;
    BOOLEAN bIsSLED = FALSE;
#endif
    BOOLEAN bSettingsChanged = FALSE;

    if (bRefresh == FALSE)
    {
        bSettingsChanged = TRUE;
    }

    GPA_LOG_INFO("Loading Group Policy configuration settings");

    GPA_LOCK_SERVERINFO;

    //Read from registry
    ceError = GPAProcessConfig(
                "Services\\gpagent\\Parameters",
                "Policy\\Services\\gpagent\\Parameters",
                gConfigDescription,
                sizeof(gConfigDescription)/sizeof(gConfigDescription[0]));

    GPA_UNLOCK_SERVERINFO;

    BAIL_ON_CENTERIS_ERROR(ceError);

#if defined(ENABLE_SLED_ONLY)
    ceError = IsSuSELinux(&bIsSuSE);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bIsSuSE)
    {
        ceError = IsSuSESLED(&bIsSLED);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bIsSLED)
        {
            /* Runtime check to see if this version of Likewise intended for SuSE linux is actually running on a SuSE system */
            bDisableGP = TRUE;
        }
    }
    else
    {
        /* Runtime check to see if this version of Likewise intended for SuSE linux is actually running on a SuSE system */
        bDisableGP = TRUE;
    }
#endif

    GPA_LOCK_SERVERINFO;

    gServerInfo.bDisableGP = bDisableGP;

    /* Determine if an actual configuration change was made recently */
    if (gServerInfo.dwComputerPollingInterval != dwComputerPollingInterval)
    {
        bSettingsChanged = TRUE;
    }

    if (gServerInfo.dwUserPollingInterval != dwUserPollingInterval)
    {
        bSettingsChanged = TRUE;
    }

    if (gServerInfo.lpmUserPolicyMode != lpmUserPolicyMode)
    {
        bSettingsChanged = TRUE;
    }

    if (gServerInfo.bEnableEventLog != bEnableEventLog)
    {
        bSettingsChanged = TRUE;
    }

    if (gServerInfo.bMonitorSudoers != bMonitorSudoers)
    {
        bSettingsChanged = TRUE;
    }

    GPA_UNLOCK_SERVERINFO;

    if (bSettingsChanged)
    {
        LogConfigReloadEvent();
    }

    return (ceError);
error:

    return (ceError);
}

CENTERROR
FlushDirectoryServiceCache(
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int i;
    const char* cacheUtils[] = {
        "/usr/sbin/lookupd", /* Before Mac OS X 10.5 */
        "/usr/bin/dscacheutil" /* On Mac OS X 10.5 */
    };

    GPA_LOG_VERBOSE("Going to flush the Mac DirectoryService cache ...");

    for (i = 0; i < (sizeof(cacheUtils) / sizeof(cacheUtils[0])); i++)
    {
        const char* command = cacheUtils[i];
        BOOLEAN exists;

        /* Sanity check */
        if (!command)
        {
            continue;
        }

        ceError = GPACheckFileExists(command, &exists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!exists)
        {
            continue;
        }

        ceError = GPAShell("%command -flushcache",
                          GPASHELL_STRING(command, command));
        /* Bail regardless */
        goto error;
    }

    GPA_LOG_ERROR("Could not locate cache flush utility");
    ceError = CENTERROR_FILE_NOT_FOUND;

error:

    return ceError;
}

VOID
LogProcessStartedEvent(
    VOID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDescription = NULL;

    ceError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise group policy service was started.");
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_INFO_EVENT(GPAGENT_EVENT_INFO_SERVICE_STARTED, pszDescription);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LogProcessStoppedEvent(
    DWORD dwExitCode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDescription = NULL;

    ceError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise group policy service was stopped");
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (dwExitCode)
    {
        GPA_ERROR_EVENT(GPAGENT_EVENT_ERROR_SERVICE_STOPPED, pszDescription, dwExitCode);
    }
    else
    {
        GPA_INFO_EVENT(GPAGENT_EVENT_INFO_SERVICE_STOPPED, pszDescription);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LogProcessFailureEvent(
    DWORD dwErrCode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDescription = NULL;

    ceError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Likewise group policy service stopped running due to an error");
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_ERROR_EVENT(GPAGENT_EVENT_ERROR_SERVICE_START_FAILURE, pszDescription, dwErrCode);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LogConfigReloadEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;


    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Likewise group policy service configuration settings have been reloaded.\r\n\r\n" \
                 "     Current settings are...\r\n" \
                 "     Computer policy refresh interval:  %d\r\n" \
                 "     User policy refresh interval:      %d\r\n" \
                 "     User policy mode:                  %s\r\n" \
                 "     Enable event log:                  %s\r\n" \
                 "     Monitor Sudoers:                   %s\r\n",
                 gServerInfo.dwComputerPollingInterval,
                 gServerInfo.dwUserPollingInterval,
                 gServerInfo.lpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_USER_ONLY ? "User GPOs only" :
                 gServerInfo.lpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_REPLACE_USER ? "Replace user GPOs with computer GPOs" :
                 gServerInfo.lpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_MERGED ? "Merge user and computer GPOs" : "<Unrecognized mode>",
                 gServerInfo.bEnableEventLog ? "true" : "false",
                 gServerInfo.bMonitorSudoers ? "true" : "false");

    BAIL_ON_CENTERIS_ERROR(dwError);

    GPA_INFO_EVENT(GPAGENT_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED, pszDescription);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

