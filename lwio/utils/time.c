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
 *        time.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Time API
 *
 *        Various utilities to get system time
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
LwioGetSystemTimeString(
    PSTR   pszBuf,  /* IN OUT */
    size_t sBufLen  /* IN     */
    );

static
NTSTATUS
LwioGetSystemTime(
    struct timespec* pTimeSpec /* IN OUT */
    );

PSTR
_LwioLogGetTimeStampPrefix(
    VOID
    )
{
    // Logger lock must be held.
    LwioGetSystemTimeString(gszLwioLogTimeStampPrefix, sizeof(gszLwioLogTimeStampPrefix));
    return gszLwioLogTimeStampPrefix;
}

static
VOID
LwioGetSystemTimeString(
    PSTR   pszBuf,  /* IN OUT */
    size_t sBufLen  /* IN     */
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct timespec ts = {0};
    struct tm tmbuf = {0};
    size_t sWritten = 0;

    status = LwioGetSystemTime(&ts);
    if (status)
    {
        goto error;
    }

    localtime_r(&ts.tv_sec, &tmbuf);

    if (gbLwioLogDoNanoSecondTime)
    {
        sWritten = strftime(pszBuf, sBufLen, LWIO_LOG_TIME_FORMAT, &tmbuf);
    }
    else
    {
        sWritten = strftime(pszBuf, sBufLen, LWIO_LOG_TIME_FORMAT ":", &tmbuf);
    }
    if (sWritten == 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto error;
    }

    if (gbLwioLogDoNanoSecondTime)
    {
        size_t sRemaining = sBufLen - sWritten;

        if (!sRemaining)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto error;
        }

        sWritten = snprintf(pszBuf + sWritten, sRemaining, ".%09ld:", ts.tv_nsec);
        if (sWritten < 0)
        {
            status = STATUS_INTERNAL_ERROR;
        }
        else if (sWritten >= sRemaining)
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        if (status)
        {
            goto error;
        }
    }


cleanup:

    return;

error:

    switch (sBufLen)
    {
        case 0:
            break;
        case 1:
            pszBuf[0] = '0';
            break;
        default:
            pszBuf[0] = ':';
            pszBuf[1] = '0';
    }

    goto cleanup;
}

static
NTSTATUS
LwioGetSystemTime(
    struct timespec* pTimeSpec /* IN OUT */
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct timespec ts = {0};

#ifdef LW_SUPPORT_NANOSECOND_TIMESTAMP

#if defined(HAVE_CLOCK_GETTIME)

    if (gbLwioLogDoNanoSecondTime)
    {
        if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
        {
            status = LwErrnoToNtStatus(errno);
        }
    }
    else

#endif /* defined(HAVE_CLOCK_GETRES) && defined(HAVE_CLOCK_GETTIME) */

#endif /* LW_SUPPORT_NANOSECOND_TIMESTAMP */

    {
        ts.tv_sec  = time(NULL);
        ts.tv_nsec = 0;
    }

    *pTimeSpec = ts;

    return status;
}
