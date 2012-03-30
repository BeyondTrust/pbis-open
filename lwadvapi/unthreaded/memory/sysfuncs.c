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
 *        Likewise Security and Authentication Subsystem (LSASS)
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

long long int
LwStrtoll(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE_STRTOLL)
    return strtoll(nptr, endptr, base);
#elif defined(HAVE___STRTOLL)
    return __strtoll(nptr, endptr, base);
#elif SIZEOF_LONG_LONG_INT == SIZEOF_LONG_INT && defined(HAVE_STRTOL)
    return (long long) strtol(nptr, endptr, base);
#else
#error strtoll support is not available
#endif
}

unsigned long long int
LwStrtoull(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE_STRTOULL)
    return strtoull(nptr, endptr, base);
#elif defined(HAVE___STRTOULL)
    return __strtoull(nptr, endptr, base);
#elif SIZEOF_LONG_LONG_INT == SIZEOF_LONG_INT && defined(HAVE_STRTOUL)
    return (unsigned long long) strtoul(nptr, endptr, base); 
#else
#error strtoull support is not available
#endif
}

#if !defined(HAVE_RPL_MALLOC) && !HAVE_MALLOC
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

#if !defined(HAVE_RPL_REALLOC) && !HAVE_REALLOC
#undef realloc

void* realloc(void* buf, size_t n);

void*
rpl_realloc(void* buf, size_t n)
{
    return realloc(buf, n);
}

#endif /* ! HAVE_RPL_REALLOC */
