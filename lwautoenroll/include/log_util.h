/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef LWAUTOENROLL_LOG_UTIL_H
#define LWAUTOENROLL_LOG_UTIL_H

#include <popt.h>

#include <lw/attrs.h>
#include <lw/types.h>
#include <lw/rtllog.h>

extern struct poptOption LwAutoenrollLogOptions[];

LW_BEGIN_EXTERN_C

DWORD LwAutoenrollLogInit(void);
DWORD LwAutoenrollLogToSyslog(LW_BOOL force);
DWORD LwAutoenrollLogToFile(PCSTR path, LW_BOOL force);

LW_RTL_LOG_LEVEL LwAutoenrollLogGetLevel(void);
DWORD LwAutoenrollLogSetLevel(LW_RTL_LOG_LEVEL level, LW_BOOL force);
DWORD LwAutoenrollLogSetLevelFromString(PCSTR level, LW_BOOL force);

LW_END_EXTERN_C

#endif /* LWAUTOENROLL_LOG_UTIL_H */
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
