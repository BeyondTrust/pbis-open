/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lwerror.h>

#include <bail.h>
#include <daemon_util.h>
#include <log_util.h>

static BOOL startAsDaemon = 0;

struct poptOption LwAutoenrollDaemonOptions[] =
{
    {
        "start-as-daemon",
        0,
        POPT_ARG_NONE,
        &startAsDaemon,
        0,
        "Run in the background.",
        NULL,
    },
    POPT_TABLEEND
};

DWORD
LwAutoenrollDaemonStart(
        void
        )
{
    PCSTR pszSmNotify;
    DWORD error = LW_ERROR_SUCCESS;
    int ret;

    if (startAsDaemon)
    {
        pid_t pid;
        int fd;
        int devNull;

        /* Log to syslog unless otherwise specified. */
        LwAutoenrollLogToSyslog(LW_FALSE);

        if ((pid = fork()) != 0)
        {
            BAIL_ON_UNIX_ERROR(pid == -1, ": fork failed");
            return 0;
        }

        // Let the first child be a session leader
        setsid();

        // Spawn a second child
        if ((pid = fork()) != 0)
        {
            // Let the first child terminate
            // This will ensure that the second child cannot be a session leader
            // Therefore, the second child cannot hold a controlling terminal
            BAIL_ON_UNIX_ERROR(pid == -1, ": fork failed");
            return 0;
        }

        // This is the second child executing
        ret = chdir("/");
        BAIL_ON_UNIX_ERROR(ret == -1, ": chdir(\"/\") failed");

        // Make sure files we create are not world-writable.
        umask(022);

        devNull = open("/dev/null", O_RDWR, 0);
        BAIL_ON_UNIX_ERROR(
            devNull == -1,
            "open(\"/dev/null\", O_RDWR failed");
        for (fd = 0; fd < 3; fd++)
        {
            ret = dup2(devNull, fd);
            BAIL_ON_UNIX_ERROR(ret == -1);
        }

        close(devNull);
    }

    LW_RTL_LOG_INFO("starting");

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        /* Notify lwsmd that we've started. */
        int notifyFd = atoi(pszSmNotify);
        char notifyCode = 0;

        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while (ret != sizeof(notifyCode) && errno == EINTR);

        close(notifyFd);
        BAIL_ON_UNIX_ERROR(ret == -1);
    }

cleanup:
    return error;
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
