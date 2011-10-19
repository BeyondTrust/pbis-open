/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sysfuncs.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Utilities
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
lsmb_vsyslog(
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

    dwError = SMBAllocateStringPrintfV(&buffer, format, ap);
    if (!dwError)
    {
        syslog(priority, "%s", buffer);
    }

    LWIO_SAFE_FREE_STRING(buffer);
#endif /* ! HAVE_VSYSLOG */
}

#if defined(__LWI_AIX__) || defined(__LWI_HP_UX__)

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

#endif /* defined(__LWI_AIX__) || defined(__LWI_HP_UX__) */

//
// WARNING about AIX stpncpy:
//
// Normal stpncpy(dest, src, n) effectively does this:
//
//   count = min(strlen(src), n)
//   memcpy(dest, src, count)
//   return dest + count
//
// However, AIX (at least 5.3) returns this: dest + max(0, n-1).
//
// So, on AIX, we should replace stpncpy.  However, if we use
// n = strlen(src) + 1, we would get the same answer back from
// normal and AIX.
//

// TODO: Change the way we check whether to replace stpncpy.  See GNU gettext.

char*
lsmb_stpncpy(
    char *dest,
    const char* src,
    size_t n
    )
{
    size_t iCh = 0;
    char* end = NULL;

    for (; src && *src && (iCh < n); iCh++)
    {
        *dest++ = *src++;
    }

    end = dest;

    for (; iCh < n; iCh++)
    {
        *dest++ = '\0';
    }

    return end;
}

#if !defined(HAVE_STRNLEN)

size_t
strnlen(
    const char *s,
    size_t maxlen
    )
{
    size_t len = 0;

    while (s && *s && (len < maxlen))
    {
        s++;
        len++;
    }

    return len;
}


#endif /* !defined(HAVE_STRNLEN) */


