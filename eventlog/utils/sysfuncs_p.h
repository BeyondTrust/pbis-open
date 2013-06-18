/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sysfuncs_p.h
 *
 * Abstract:
 *
 *        Likewise Eventlog Services
 *
 *        System Functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __SYSFUNCS_P_H__
#define __SYSFUNCS_P_H__

#if !HAVE_DECL_ISBLANK
int isblank(int c);
#endif

void
evt_sys_vsyslog(
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

#endif /* ! HAVE_RPL_REALLOC */

#endif /* __SYSFUNCS_P_H__ */

