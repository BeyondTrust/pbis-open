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

#include "includes.h"

NTSTATUS
WireGetCurrentNTTime(
    PLONG64 pllCurTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    struct timeval tv = {0};

    if (gettimeofday(&tv, NULL) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pllCurTime =
        ((tv.tv_sec + WIRE_NTTIME_EPOCH_DIFFERENCE_SECS) *
                    WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS) +
        tv.tv_usec * WIRE_FACTOR_MICROSECS_TO_HUNDREDS_OF_NANOSECS;

cleanup:

    return ntStatus;

error:

    *pllCurTime = 0LL;

    goto cleanup;
}

NTSTATUS
WireNTTimeToTimeSpec(
    LONG64 llCurTime,
    struct timespec* pTimeSpec
    )
{
    pTimeSpec->tv_sec =
            (llCurTime/WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS) -
                            WIRE_NTTIME_EPOCH_DIFFERENCE_SECS;

    pTimeSpec->tv_nsec =
            (llCurTime % WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS) * 100;

    return STATUS_SUCCESS;
}

NTSTATUS
WireNTTimeToSMBDateTime(
    LONG64    llNTTime,
    PSMB_DATE pSmbDate,
    PSMB_TIME pSmbTime
    )
{
    NTSTATUS ntStatus = 0;
    time_t   timeUnix = 0;
    struct tm stTime = {0};

    timeUnix = (llNTTime /  WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS) -
                    WIRE_NTTIME_EPOCH_DIFFERENCE_SECS;

    /* Adjust to local time zone */
    timeUnix -= (mktime(gmtime_r(&timeUnix, &stTime)) - timeUnix);

    gmtime_r(&timeUnix, &stTime);

    if ((stTime.tm_year + 1900) < 1980)
    {
        /* Minimum time for SMB format is Jan 1, 1980 00:00:00 */
        pSmbDate->usDay      = 1;
        pSmbDate->usMonth    = 1;
        pSmbDate->usYear     = 0;

        pSmbTime->TwoSeconds = 0;
        pSmbTime->Minutes    = 0;
        pSmbTime->Hours      = 0;
    } 
    else
    {
        pSmbDate->usDay = stTime.tm_mday;
        pSmbDate->usMonth = stTime.tm_mon + 1;
        pSmbDate->usYear = (stTime.tm_year + 1900) - 1980;

        pSmbTime->TwoSeconds = stTime.tm_sec / 2;
        pSmbTime->Minutes = stTime.tm_min;
        pSmbTime->Hours = stTime.tm_hour;
    }

    return ntStatus;
}

NTSTATUS
WireSMBDateTimeToNTTime(
    PSMB_DATE pSmbDate,
    PSMB_TIME pSmbTime,
    PLONG64   pllNTTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG64   llNTTime = 0LL;

    if (!pSmbDate || !pSmbTime)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((pSmbDate->usYear  > 0) &&
        (pSmbDate->usMonth > 0) &&
        (pSmbDate->usDay   > 0))
    {
        struct tm stTime   = {0};
        time_t    timeUnix = 0;

        stTime.tm_mday = pSmbDate->usDay;
        stTime.tm_mon  = pSmbDate->usMonth - 1;
        stTime.tm_year = (pSmbDate->usYear + 1980) - 1900;

        stTime.tm_sec  = pSmbTime->TwoSeconds * 2;
        stTime.tm_min  = pSmbTime->Minutes;
        stTime.tm_hour = pSmbTime->Hours;

        timeUnix = mktime(&stTime);

        llNTTime =
                (timeUnix + WIRE_NTTIME_EPOCH_DIFFERENCE_SECS) *
                            WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;
    }

    *pllNTTime = llNTTime;

cleanup:

    return ntStatus;

error:

    *pllNTTime = 0LL;

    goto cleanup;
}

NTSTATUS
WireNTTimeToSMBUTime(
    LONG64 llNTTime,
    PULONG pulSmbUTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    struct tm stTime = {0};
    time_t tSmbUTime = 0;

    /**
     * @todo - Handle overflow
     */
    tSmbUTime = (llNTTime / WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS) -
                                            WIRE_NTTIME_EPOCH_DIFFERENCE_SECS;

    /* Adjust the time zone */
    tSmbUTime += (tSmbUTime - mktime(gmtime_r(&tSmbUTime, &stTime)));

    *pulSmbUTime = (ULONG)tSmbUTime;

    return ntStatus;
}

NTSTATUS
WireSMBUTimetoNTTime(
    ULONG   ulSmbUTime,
    BOOLEAN bAdjustToLocalTimeZone,
    PLONG64 pllNTTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    struct tm stTime = {0};
    time_t tSmbUTime = ulSmbUTime;

    if (tSmbUTime == 0)
    {
        *pllNTTime = 0;
        goto cleanup;
    }

    if (bAdjustToLocalTimeZone == TRUE)
    {
        tSmbUTime -= (tSmbUTime - mktime(gmtime_r(&tSmbUTime, &stTime)));
    }

    /**
     * @todo - Handle overflow
     */
    *pllNTTime = (tSmbUTime + WIRE_NTTIME_EPOCH_DIFFERENCE_SECS) *
                                    WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;

 cleanup:
    return ntStatus;
}

static
ULONG64
MinutesInTimeFromYearZero(
    const struct tm* tmTime
    )
{
    ULONG64 ulMinutes = 0;
    ULONG ulLeapYears = 0;
    ULONG ulYear = tmTime->tm_year + 1900;  // tm_year uses 1900 as a base year
    
    // We do not have to include the current year in the computation of leap
    // years.  The current year is taken into account several lines below
    // when adding tm_yday to the number of days.
    ulLeapYears = (ulYear - 1) / 4 - 
                  (ulYear - 1) / 100 + 
                  (ulYear - 1) / 400 + 
                  1;    // The "0" leap year

    // ulMinutes variable is used to accumulate the number of minutes in the 
    // computation below.
    ulMinutes = ulYear * 365 + ulLeapYears + tmTime->tm_yday;
    ulMinutes = ulMinutes * 24 + tmTime->tm_hour;
    ulMinutes = ulMinutes * 60 + tmTime->tm_min;

    return ulMinutes;
}

// Returns minutes west from UTC
NTSTATUS
WireSMBUTimeToTimeZone(
    time_t utcTime,
    PSHORT psTimezone)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    struct tm localTime = { 0 };
    struct tm gmTime = { 0 };

    // Ignoring errors - will not fail if cannot get times from the system
    localtime_r(&utcTime, &localTime);
    gmtime_r(&utcTime, &gmTime);

    // localtime_r takes into account DST, while gmtime_r returns UTC, where
    // DST is not relevant.  Thus, the below calculation takes DST into account
    *psTimezone = MinutesInTimeFromYearZero(&gmTime) - 
                  MinutesInTimeFromYearZero(&localTime);

    return ntStatus;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
