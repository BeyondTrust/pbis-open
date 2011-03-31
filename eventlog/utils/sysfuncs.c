/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sysfuncs.c
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

#include "includes.h"

#if !HAVE_DECL_ISBLANK
int isblank(int c)
{
    return c == '\t' || c == ' ';
}
#endif

#if !defined(HAVE_STRTOLL)

long long int
strtoll(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE___STRTOLL)
    return __strtoll(nptr, endptr, base);
#elif SIZEOF_LONG_LONG_INT == SIZEOF_LONG_INT && defined(HAVE_STRTOL)
    return (long long) strtol(nptr, endptr, base);
#else
#error strtoll support is not available
#endif
}

#endif /* defined(HAVE_STRTOLL) */

#if !defined(HAVE_STRTOULL)

unsigned long long int
strtoull(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE___STRTOULL)
    return __strtoull(nptr, endptr, base);
#elif SIZEOF_LONG_LONG_INT == SIZEOF_LONG_INT && defined(HAVE_STRTOUL)
    return (unsigned long long) strtoul(nptr, endptr, base);
#else
#error strtoull support is not available
#endif
}

#endif /* defined(HAVE_STRTOULL) */

void
evt_sys_vsyslog(
    int priority,
    const char *format,
    va_list ap
    )
{
#if defined(HAVE_VSYSLOG)
    vsyslog(priority, format, ap);
#else
    DWORD dwError;
    PSTR buffer = NULL;

    dwError = LwAllocateStringPrintfV(&buffer, format, ap);
    if (!dwError)
    {
        syslog(priority, "%s", buffer);
    }

    LW_SAFE_FREE_STRING(buffer);
#endif /* ! HAVE_VSYSLOG */
}

#if !defined(HAVE_RPL_MALLOC)
#undef malloc

void* malloc(size_t n);

//See http://wiki.buici.com/wiki/Autoconf_and_RPL_MALLOC
void*
rpl_malloc(size_t n)
{
    if (n == 0)
        n = 1;
    return malloc(n);
}

#endif /* ! HAVE_RPL_MALLOC */

#if !defined(HAVE_RPL_REALLOC)
#undef realloc

void* realloc(void* buf, size_t n);

void*
rpl_realloc(void* buf, size_t n)
{
    return realloc(buf, n);
}

#endif /* ! HAVE_RPL_REALLOC */

