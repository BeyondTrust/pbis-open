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
 *        lsastrerror.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        strerror_r wrapper
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "config.h"
#include <string.h>
#include <errno.h>
#include <lwerror.h>

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__)

int
LsaStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    )
{
    // On Solaris, strerror is MT-Safe
    char *pszResult = strerror(errnum);
    size_t requiredLen = 0;

    if (pszResult == NULL)
        return LwMapErrnoToLwError(errno);

    requiredLen = strlen(pszResult) + 1;
    if (buflen < requiredLen)
    {
        return LW_ERROR_ERRNO_ERANGE;
    }
    memcpy(pszBuf, pszResult, requiredLen);
    return 0;
}

#else

int
LsaStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    )
{
#ifdef STRERROR_R_CHAR_P
    errno = 0;
    char *pszResult = strerror_r(errnum, pszBuf, buflen);

    if (pszResult == NULL) {
        return errno ? LwMapErrnoToLwError(errno) : LW_ERROR_INVALID_PARAMETER;
    }

    if (pszResult != pszBuf)
    {
        // strerror_r returned a statically allocated buffer
        size_t requiredLen = strlen(pszResult) + 1;
        if (buflen < requiredLen)
        {
            return LW_ERROR_ERRNO_ERANGE;
        }
        memcpy(pszBuf, pszResult, requiredLen);
        return 0;
    }

    if (strlen(pszBuf) == buflen - 1)
    {
        // We can't tell if the error string exactly fit into the buffer, or
        // if the buffer is too small. We'll assume it's too small.
        return LW_ERROR_ERRNO_ERANGE;
    }

    return 0;
#else
    if (strerror_r(errnum, pszBuf, buflen) == 0)
    {
        return 0;
    }
    else
    {
        return LwMapErrnoToLwError(errno);
    }
#endif
}

#endif

