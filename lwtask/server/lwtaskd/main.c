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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Service Entry API
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
PSTR
LwTaskGetProgramName(
    PSTR pszFullProgramPath
    );

static
DWORD
LwTaskSrvParseArgs(
    int  argc,
    PSTR argv[]
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

#ifdef ENABLE_PIDFILE
VOID
LwTaskSrvCreatePIDFile(
    VOID
    );

static
pid_t
LwTaskSrvGetPidFromPidFile(
    VOID
    );

static
VOID
LwTaskSrvRemovePidFile(
    VOID
    );
#endif /* ENABLE_PIDFILE */

int
main(
    int   argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;

    pthread_rwlock_init(&gLwTaskSrvGlobals.mutex, NULL);
    gLwTaskSrvGlobals.pMutex = &gLwTaskSrvGlobals.mutex;

    dwError = LwTaskProdConsInitContents(
                    &gLwTaskSrvGlobals.workQueue,
                    gLwTaskSrvGlobals.dwMaxNumWorkItemsInQueue,
                    &LwTaskReleaseContextHandle);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvParseArgs(argc, argv);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskInitLogging_r(
                    LwTaskGetProgramName(argv[0]),
                    gLwTaskSrvGlobals.logTarget,
                    gLwTaskSrvGlobals.maxAllowedLogLevel,
                    gLwTaskSrvGlobals.szLogFilePath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    LW_TASK_LOG_VERBOSE("Logging started");

    if (atexit(LwTaskSrvExitHandler) < 0) {
       dwError = LwMapErrnoToLwError(errno);
       BAIL_ON_LW_TASK_ERROR(dwError);
    }

    if (LwTaskSrvShouldStartAsDaemon())
    {
       dwError = LwTaskSrvStartAsDaemon();
       BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskBlockSelectedSignals();
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskInitCacheFolders();
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskRepositoryInit();
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateInit();
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvInit();
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskSrvStartListenThread();
    BAIL_ON_LW_TASK_ERROR(dwError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        int ret = -1;

        notifyFd = atoi(pszSmNotify);

        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));

        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            dwError = LwMapErrnoToLwError(errno);

            LW_TASK_LOG_ERROR(
                    "Could not notify service manager: %s (%i)",
                    strerror(errno),
                    errno);

            BAIL_ON_LW_TASK_ERROR(dwError);
        }

        close(notifyFd);
    }

    // Handle signals, blocking until we are supposed to exit.
    dwError = LwTaskSrvHandleSignals();
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    LW_TASK_LOG_VERBOSE("Likewise Task Service main cleaning up");

    LwTaskSrvStopProcess();

    LwTaskSrvStopListenThread();

    LwTaskSrvShutdown();

    LwTaskMigrateShutdown();

    LwTaskRepositoryShutdown();

    LW_TASK_LOG_INFO("Likewise Task Service exiting...");

    LwTaskSrvSetProcessExitCode(dwError);

    LwTaskShutdownLogging_r();

    #ifdef ENABLE_PIDFILE
    LwTaskSrvRemovePidFile();
    #endif

    if (gLwTaskSrvGlobals.pMutex)
    {
        pthread_rwlock_destroy(&gLwTaskSrvGlobals.mutex);
    }

    return dwError;

error:

    LW_TASK_LOG_ERROR("Likewise Task Service exiting due to error [Code:%u]", dwError);

    goto cleanup;
}

static
PSTR
LwTaskGetProgramName(
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

static
DWORD
LwTaskSrvParseArgs(
    int  argc,
    PSTR argv[]
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;

    DWORD     dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int       iArg = 1;
    PSTR      pArg = NULL;
    BOOLEAN   bLogTargetSet = FALSE;

    do
    {
      pArg = argv[iArg++];

      if (pArg == NULL || *pArg == '\0')
      {
          break;
      }

      switch(parseMode)
      {
          case PARSE_MODE_OPEN:

              if (strcmp(pArg, "--logfile") == 0)
              {
                  parseMode = PARSE_MODE_LOGFILE;
              }
              else if ((strcmp(pArg, "--help") == 0) ||
                       (strcmp(pArg, "-h") == 0))
              {
                  ShowUsage(LwTaskGetProgramName(argv[0]));
                  exit(0);
              }
              else if (strcmp(pArg, "--start-as-daemon") == 0)
              {
                  gLwTaskSrvGlobals.dwStartAsDaemon = 1;

                  // If other arguments set before this set the log target
                  // don't over-ride that setting
                  if (!bLogTargetSet)
                  {
                      gLwTaskSrvGlobals.logTarget = LW_TASK_LOG_TARGET_SYSLOG;
                  }
              }
              else if (strcmp(pArg, "--syslog") == 0)
              {
                  bLogTargetSet = TRUE;
                  gLwTaskSrvGlobals.logTarget = LW_TASK_LOG_TARGET_SYSLOG;
              }
              else if (strcmp(pArg, "--loglevel") == 0)
              {
                  parseMode = PARSE_MODE_LOGLEVEL;
              }
              else
              {
                  LW_TASK_LOG_ERROR(
                        "Unrecognized command line option [%s]",
                        pArg);

                  ShowUsage(LwTaskGetProgramName(argv[0]));

                  exit(1);
              }

              break;

          case PARSE_MODE_LOGFILE:

              strcpy(gLwTaskSrvGlobals.szLogFilePath, pArg);

              LwStripWhitespace(gLwTaskSrvGlobals.szLogFilePath, TRUE, TRUE);

              if (!strcmp(gLwTaskSrvGlobals.szLogFilePath, "."))
              {
                  gLwTaskSrvGlobals.logTarget = LW_TASK_LOG_TARGET_CONSOLE;
              }
              else
              {
                  gLwTaskSrvGlobals.logTarget = LW_TASK_LOG_TARGET_FILE;
              }

              bLogTargetSet = TRUE;

              parseMode = PARSE_MODE_OPEN;

              break;

          case PARSE_MODE_LOGLEVEL:

              if (!strcasecmp(pArg, "error"))
              {
                  gLwTaskSrvGlobals.maxAllowedLogLevel = LW_TASK_LOG_LEVEL_ERROR;
              }
              else if (!strcasecmp(pArg, "warning"))
              {
                  gLwTaskSrvGlobals.maxAllowedLogLevel = LW_TASK_LOG_LEVEL_WARNING;
              }
              else if (!strcasecmp(pArg, "info"))
              {
                  gLwTaskSrvGlobals.maxAllowedLogLevel = LW_TASK_LOG_LEVEL_INFO;
              }
              else if (!strcasecmp(pArg, "verbose"))
              {
                  gLwTaskSrvGlobals.maxAllowedLogLevel = LW_TASK_LOG_LEVEL_VERBOSE;
              }
              else if (!strcasecmp(pArg, "debug"))
              {
                  gLwTaskSrvGlobals.maxAllowedLogLevel = LW_TASK_LOG_LEVEL_DEBUG;
              }
              else
              {
                  LW_TASK_LOG_ERROR("Error: Invalid log level [%s]", pArg);

                  ShowUsage(LwTaskGetProgramName(argv[0]));

                  exit(1);
              }

              parseMode = PARSE_MODE_OPEN;

              break;
      }

    } while (iArg < argc);

    if (gLwTaskSrvGlobals.dwStartAsDaemon)
    {
        if (gLwTaskSrvGlobals.logTarget == LW_TASK_LOG_TARGET_CONSOLE)
        {
            LW_TASK_LOG_ERROR("%s", "Error: Cannot log to console when executing as a daemon");

            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_TASK_ERROR(dwError);
        }
    }
    else
    {
        if (!bLogTargetSet)
        {
            gLwTaskSrvGlobals.logTarget = LW_TASK_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}

static
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
           "          [--logfile logFilePath]\n"
           "          [--syslog]\n"
           "          [--loglevel {error, warning, info, verbose, debug}]\n",
           pszProgramName);
}

VOID
LwTaskSrvExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    sprintf(szErrCodeFilePath, "%s/lwtaskd.err", CACHEDIR);

    dwError = LwCheckFileTypeExists(
                    szErrCodeFilePath,
                    LWFILE_REGULAR,
                    &bFileExists);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (bFileExists)
    {
        dwError = LwRemoveFile(szErrCodeFilePath);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = LwTaskSrvGetProcessExitCode(&dwExitCode);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (dwExitCode)
    {
       fp = fopen(szErrCodeFilePath, "w");
       if (fp == NULL)
       {
          dwError = LwMapErrnoToLwError(errno);
          BAIL_ON_LW_TASK_ERROR(dwError);
       }

       fprintf(fp, "%u\n", dwExitCode);
    }

error:

    if (fp != NULL)
    {
       fclose(fp);
    }
}

DWORD
LwTaskInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = CACHEDIR;
    BOOLEAN bExists = FALSE;

    dwError = LwCheckFileTypeExists(pszCachePath, LWFILE_DIRECTORY, &bExists);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = LwCreateDirectory(pszCachePath, cacheDirMode);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

BOOLEAN
LwTaskSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskSrvGlobals.mutex);

    bResult = (gLwTaskSrvGlobals.dwStartAsDaemon != 0);

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    return bResult;
}

DWORD
LwTaskSrvStartAsDaemon(
    VOID
    )
{
    DWORD dwError = 0;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    /* Use dcethread_fork() rather than fork() because we link with DCE/RPC */
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
    dwError = LwTaskSrvIgnoreSIGHUP();
    BAIL_ON_LW_TASK_ERROR(dwError);

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_LW_TASK_ERROR(dwError);

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

DWORD
LwTaskSrvGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskSrvGlobals.mutex);

    *pdwExitCode = gLwTaskSrvGlobals.dwExitCode;

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    return dwError;
}

VOID
LwTaskSrvSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskSrvGlobals.mutex);

    gLwTaskSrvGlobals.dwExitCode = dwExitCode;

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);
}

#ifdef ENABLE_PIDFILE
VOID
LwTaskSrvCreatePIDFile(
    VOID
    )
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = LwTaskSrvGetPidFromPidFile();
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
pid_t
LwTaskSrvGetPidFromPidFile(
    VOID
    )
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
VOID
LwTaskSrvRemovePidFile(
    VOID
    )
{
    if (LwTaskSrvGetPidFromPidFile() == getpid())
    {
        unlink(PID_FILE);
    }
}

#endif

DWORD
LwTaskBlockSelectedSignals(
    VOID
    )
{
    DWORD dwError = 0;
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    sigaddset(&default_signal_mask, SIGINT);
    sigaddset(&default_signal_mask, SIGTERM);
    sigaddset(&default_signal_mask, SIGHUP);
    sigaddset(&default_signal_mask, SIGQUIT);
    sigaddset(&default_signal_mask, SIGPIPE);

    dwError = pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
    BAIL_ON_LW_TASK_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

BOOLEAN
LwTaskSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, &gLwTaskSrvGlobals.mutex);

    bExit = gLwTaskSrvGlobals.bProcessShouldExit;

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);

    return bExit;
}

VOID
LwTaskSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;

    LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gLwTaskSrvGlobals.mutex);

    gLwTaskSrvGlobals.bProcessShouldExit = bExit;

    LW_TASK_UNLOCK_RWMUTEX(bInLock, &gLwTaskSrvGlobals.mutex);
}
