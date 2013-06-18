/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include "lwcertd.h"

#include <sys/wait.h>

#include <errno.h>
#include <popt.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <lwerror.h>

#include <bail.h>
#include <daemon_util.h>
#include <log_util.h>

static DWORD trustedCertPollInterval = 3600;

struct poptOption options[] = {
    {
        "poll-interval",
        'p',
        POPT_ARG_INT,
        &trustedCertPollInterval,
        0,
        "Poll interval for trusted certificates.",
        "seconds",
    },
    {
        .argInfo = POPT_ARG_INCLUDE_TABLE,
        .arg = LwAutoenrollDaemonOptions,
    },
    {
        .argInfo = POPT_ARG_INCLUDE_TABLE,
        .arg = LwAutoenrollLogOptions,
    },
    POPT_AUTOHELP
    POPT_TABLEEND
};

int
main(int argc, const char **argv)
{
    poptContext poptContext;
    time_t nextTrustedCertPollTime = 0;
    int ret;
    DWORD error;
    sigset_t signals;

    poptContext = poptGetContext(NULL, argc, argv, options, 0);
    while ((ret = poptGetNextOpt(poptContext)) >= 0)
    {
        /* All options are processed automatically. */
    }

    if (ret < -1)
    {
        fprintf(stderr, "%s: %s: %s\n", argv[0],
                poptBadOption(poptContext, POPT_BADOPTION_NOALIAS),
                poptStrerror(ret));
        error = 1;
        goto cleanup;
    }

    /* Start daemon processing, possibly in the background. */
    LwAutoenrollDaemonStart();

    /* Block signals so we can safely wait for them with sigtimedwait(). */
    sigemptyset(&signals);
    sigaddset(&signals, SIGHUP);
    ret = sigprocmask(SIG_BLOCK, &signals, NULL);
    BAIL_ON_UNIX_ERROR(ret == -1);

    while (1)
    {
        time_t now;

        while ((now = time(NULL)) < nextTrustedCertPollTime)
        {
            struct timespec timeout = { nextTrustedCertPollTime - now, 0 };

            switch (sigtimedwait(&signals, NULL, &timeout))
            {
                case SIGHUP:
                    /* Force the CA certificates to be downloaded. */
                    nextTrustedCertPollTime = 0;
                    break;
            }
        }

        GetTrustedCertificates();
        nextTrustedCertPollTime = now + trustedCertPollInterval;
    }

cleanup:
    /* Exit 1 on any error, 0 if OK. */
    return (error != 0);
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
