/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        sysfuncs.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        System Functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
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
reg_vsyslog(
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

    dwError = LwRtlCStringAllocatePrintfV(&buffer, format, ap);
    if (!dwError)
    {
        syslog(priority, "%s", buffer);
    }

    LWREG_SAFE_FREE_STRING(buffer);
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

