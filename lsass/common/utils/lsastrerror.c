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
 *        lsastrerror.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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

