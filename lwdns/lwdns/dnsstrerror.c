/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dnsstrerror.c
 *
 * Abstract:
 *
 *        Likewise DNS Update
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

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__)

int
DNSStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    )
{
    // On Solaris, strerror is MT-Safe
    char *pszResult = strerror(errnum);
    size_t requiredLen = 0;

    if (pszResult == NULL)
        return errno;

    requiredLen = strlen(pszResult) + 1;
    if (buflen < requiredLen)
    {
        return ERANGE;
    }
    memcpy(pszBuf, pszResult, requiredLen);
    return 0;
}

#else

int
DNSStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    )
{
#ifdef STRERROR_R_CHAR_P
    char *pszResult = strerror_r(errnum, pszBuf, buflen);

    if (pszResult == NULL) {
        return errno ? errno : EINVAL;
    }

    if (pszResult != pszBuf)
    {
        // strerror_r returned a statically allocated buffer
        size_t requiredLen = strlen(pszResult) + 1;
        if (buflen < requiredLen)
        {
            return ERANGE;
        }
        memcpy(pszBuf, pszResult, requiredLen);
        return 0;
    }

    if (strlen(pszBuf) == buflen - 1)
    {
        // We can't tell if the error string exactly fit into the buffer, or
        // if the buffer is too small. We'll assume it's too small.
        return ERANGE;
    }

    return 0;
#else
    return strerror_r(errnum, pszBuf, buflen);
#endif
}

#endif

