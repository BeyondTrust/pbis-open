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
 *        lwnet-time.c
 *
 * Abstract:
 *
 *        Likewise Site Manager 
 *                    
 *        Time Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 * 
 */
#include "includes.h"

//Convert to seconds string of form ##s, ##m, ##h, or ##d
//where s,m,h,d = seconds, minutes, hours, days.
DWORD
LWNetParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    )
{
    DWORD  dwError = 0;
    DWORD  dwTimeInterval = 0;
    PSTR   pszTimeIntervalLocal = 0;
    DWORD  dwTimeIntervalLocalLen = 0;
    DWORD  dwUnitMultiplier = 0;
    PSTR   pszUnitCode = NULL;
    
    LwStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);

    BAIL_ON_INVALID_STRING(pszTimeInterval);
        
    dwError = LWNetAllocateString(
                    pszTimeInterval, 
                    &pszTimeIntervalLocal
                    );
    BAIL_ON_LWNET_ERROR(dwError);

    dwTimeIntervalLocalLen = strlen(pszTimeIntervalLocal);
    
    pszUnitCode = pszTimeIntervalLocal + dwTimeIntervalLocalLen - 1;

    if (isdigit((int)*pszUnitCode)) 
    {
        dwUnitMultiplier = 1;
    }

    else 
    {

        switch(*pszUnitCode) 
        {
            case 's':
            case 'S':
                dwUnitMultiplier = 1;
                break;
            
            case 'm':
            case 'M':
                dwUnitMultiplier = LWNET_SECONDS_IN_MINUTE;
                break;

            case 'h':
            case 'H':
                dwUnitMultiplier = LWNET_SECONDS_IN_HOUR;
                break;

            case 'd':
            case 'D':
                dwUnitMultiplier = LWNET_SECONDS_IN_DAY;
                break;

            default:
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_LWNET_ERROR(dwError);
                break;
        }

        *pszUnitCode = ' ';
    }
    
    LwStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);
    
    dwTimeInterval = (DWORD) atoi(pszTimeIntervalLocal) * dwUnitMultiplier;
    
    *pdwTimeInterval = dwTimeInterval;
    
cleanup:
    
    LWNET_SAFE_FREE_STRING(pszTimeIntervalLocal);
    
    return dwError;

error:
    
    goto cleanup;
}

DWORD
LWNetSetSystemTime(
    IN LWNET_UNIX_TIME_T Time
    )
{
    DWORD dwError = 0;
    BOOLEAN bTimeset = FALSE;
    time_t ttCurTime = (time_t) Time;

#if !defined(HAVE_CLOCK_SETTIME) && !defined(HAVE_SETTIMEOFDAY)
#error Either clock_settime or settimeofday is needed
#endif

#ifdef HAVE_CLOCK_SETTIME
    struct timespec systemspec;
#endif
#if HAVE_SETTIMEOFDAY
    struct timeval systemval;
#endif
    long long readTime = -1;

#ifdef HAVE_CLOCK_SETTIME
    memset(&systemspec, 0, sizeof(systemspec));
    systemspec.tv_sec = ttCurTime;
#endif
#if HAVE_SETTIMEOFDAY
    memset(&systemval, 0, sizeof(systemval));
    systemval.tv_sec = ttCurTime;
#endif

#ifdef HAVE_CLOCK_SETTIME
    if (!bTimeset)
    {
        if (clock_settime(CLOCK_REALTIME, &systemspec) == -1)
        {
            LWNET_LOG_VERBOSE("Setting time with clock_settime failed %d", errno);
        }
        else
        {
            LWNET_LOG_VERBOSE("Setting time with clock_settime worked");
            bTimeset = TRUE;
        }
    }
#endif
    
#ifdef HAVE_SETTIMEOFDAY
    if (!bTimeset)
    {
        if (settimeofday(&systemval, NULL) == -1)
        {
            LWNET_LOG_VERBOSE("Setting time with settimeofday failed %d", errno);
        }
        else
        {
            LWNET_LOG_VERBOSE("Setting time with settimeofday worked");
            bTimeset = TRUE;
        }
    }
#endif
    
    if (!bTimeset)
    {
        dwError = ERROR_INVALID_TIME;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    //Verify the clock got set
    bTimeset = FALSE;
#ifdef HAVE_CLOCK_GETTIME
    if (!bTimeset && clock_gettime(CLOCK_REALTIME, &systemspec) >= 0)
    {
        bTimeset = TRUE;
        readTime = systemspec.tv_sec;
    }
#endif
    
#ifdef HAVE_GETTIMEOFDAY
    if (!bTimeset && gettimeofday(&systemval, NULL) >= 0)
    {
        bTimeset = TRUE;
        readTime = systemval.tv_sec;
    }
#endif
    
    if (!bTimeset) {
        dwError = ERROR_INVALID_TIME;
        BAIL_ON_LWNET_ERROR(dwError);
    }
        
    //Make sure the time is now within 5 seconds of what we set
    if (labs(readTime - ttCurTime) > 5)
    {
        LWNET_LOG_ERROR("Attempted to set time to %ld, but it is now %ld.", ttCurTime, readTime);
        dwError = ERROR_INVALID_TIME;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    return dwError;      
    
error:

    goto cleanup;
}

DWORD
LWNetGetSystemTime(
    OUT PLWNET_UNIX_TIME_T pTime
    )
{
    DWORD dwError = 0;
    time_t now = time(NULL);

    if (now == (time_t) -1)
    {
        // ISSUE-2008/07/01-dalmeida -- erro code conversion
        dwError = LwMapErrnoToLwError(errno);
        now = 0;
        BAIL_ON_LWNET_ERROR(dwError);
    }
error:
    *pTime = now;
    return dwError;
}

DWORD
LWNetGetSystemTimeInMs(
    OUT PLWNET_UNIX_MS_TIME_T pTime
    )
{
    DWORD dwError = 0;
    struct timeval now = { 0 };
    
    // ISSUE-2008/07/01-dalmeida -- erro code conversion
    if (gettimeofday(&now, NULL) < 0)
    {
        // ISSUE-2008/07/01-dalmeida -- erro code conversion
        dwError = LwMapErrnoToLwError(errno);
        now.tv_sec = 0;
        now.tv_usec = 0;
        BAIL_ON_LWNET_ERROR(dwError);
    }
error:
    *pTime = ((LWNET_UNIX_MS_TIME_T)now.tv_sec) * LWNET_MILLISECONDS_IN_SECOND + (LWNET_UNIX_MS_TIME_T) now.tv_usec / LWNET_MICROSECONDS_IN_MILLISECOND;
    return dwError;
}

DWORD
LWNetTimeInMsToTimespec(
    IN LWNET_UNIX_MS_TIME_T Time,
    OUT struct timespec* Timespec
    )
{
    struct timespec result = { 0, 0 };
    
    result.tv_sec = (time_t) (Time / LWNET_MILLISECONDS_IN_SECOND);
    result.tv_nsec = (long) (Time - ((LWNET_UNIX_MS_TIME_T) result.tv_sec * LWNET_MILLISECONDS_IN_SECOND));

    *Timespec = result;
    return 0;
}

DWORD
LWNetSleepInMs(
    IN LWNET_UNIX_MS_TIME_T Time
    )
{
    DWORD dwError = ERROR_SUCCESS;
    pthread_mutex_t retryLock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t retryCondition = PTHREAD_COND_INITIALIZER;
    LWNET_UNIX_MS_TIME_T now = 0;
    struct timespec timeout = { 0, 0 };

    dwError = LWNetGetSystemTimeInMs(&now);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetTimeInMsToTimespec(now + Time, &timeout);
    BAIL_ON_LWNET_ERROR(dwError);

    pthread_mutex_lock(&retryLock);
    dwError = pthread_cond_timedwait(&retryCondition,
                                     &retryLock,
                                     &timeout);
    pthread_mutex_unlock(&retryLock);        
    if (dwError == ETIMEDOUT)
    {
        dwError = 0;
    }
    BAIL_ON_LWNET_ERROR(dwError);
    
error:
    return dwError;
}

DWORD
LWNetCrackLdapTime(
    IN PCSTR pszStrTime,
    OUT struct tm* pTm
    )
{
    DWORD dwError = 0;
    PSTR pszCrackTime = NULL;
    PSTR pszYear;
    PSTR pszMonth;
    PSTR pszDay;
    PSTR pszHour;
    PSTR pszMinute;
    PSTR pszSecond;
    PSTR pszJunk;

    if (pszStrTime == NULL || strlen(pszStrTime) < 14)
    {
        dwError = EINVAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateString(pszStrTime, &pszCrackTime);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pszYear = pszCrackTime + 0;
    pszMonth = pszYear + 4;
    pszDay = pszMonth + 2;
    pszHour = pszDay + 2;
    pszMinute = pszHour + 2;
    pszSecond = pszMinute + 2;
    pszJunk = pszSecond + 2;

    // Note that we always expect the time to be in "zulu" format (GMT),
    // so pszJunk should always be .0Z or something like that
    // (see http://support.microsoft.com/kb/219005).

    memset(pTm, 0, sizeof(*pTm));

    *pszJunk = '\0';
    pTm->tm_sec = atoi(pszSecond);
    *pszSecond = '\0';
    pTm->tm_min = atoi(pszMinute);
    *pszMinute = '\0';
    pTm->tm_hour = atoi(pszHour);
    *pszHour = '\0';
    pTm->tm_mday = atoi(pszDay);
    *pszDay = '\0';
    pTm->tm_mon = atoi(pszMonth) - 1;
    *pszMonth = '\0';
    pTm->tm_year = atoi(pszYear) - 1900;

    if ((pTm->tm_sec < 0 || pTm->tm_sec > 60) ||
        (pTm->tm_min < 0 || pTm->tm_min > 59) ||
        (pTm->tm_hour < 0 || pTm->tm_hour > 23) ||
        (pTm->tm_mday < 1 || pTm->tm_mday > 31) ||
        (pTm->tm_mon < 0 || pTm->tm_mon > 11))
    {
        dwError = EINVAL;
        memset(pTm, 0, sizeof(*pTm));
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    
    LWNET_SAFE_FREE_STRING(pszCrackTime);

    return dwError;
}
