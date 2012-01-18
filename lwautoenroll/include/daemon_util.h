/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef LW_AUTOENROLL_DAEMON_UTIL_H
#define LW_AUTOENROLL_DAEMON_UTIL_H

#include <popt.h>

#include <lw/types.h>

extern struct poptOption LwAutoenrollDaemonOptions[];

DWORD
LwAutoenrollDaemonStart(
        VOID
        );

#endif /* LW_AUTOENROLL_DAEMON_UTIL_H */
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
