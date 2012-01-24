/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        poller.c
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *
 *        Poller thread
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#include "includes.h"

#define NANOSECS_PER_SECOND 1000000000

#define UMN_ILLSEQ_FAIL    1
#define UMN_ILLSEQ_SKIP    1
#define UMN_ILLSEQ_QMARK    1

/* UCS-2, little endian byte order
 * Note that UCS-2 without LE will
 * default to big endian on FreeBSD
 */
#if defined(WORDS_BIGENDIAN)
#define WINDOWS_ENCODING "UCS-2BE"
#else
#define WINDOWS_ENCODING "UCS-2LE"
#endif

DWORD
UmnSrvWc16sFromMbsWithRepl(
    OUT PWSTR* ppwszDest,
    IN PCSTR pszSrc
    )
{
    DWORD dwError = 0;
    iconv_t hIconv = iconv_open(WINDOWS_ENCODING, "UTF-8");
    size_t sSrcSize = pszSrc ? strlen(pszSrc) : 0;
    size_t sDestSize = (sSrcSize + 1) * 2;
    PWSTR pwszDest = NULL;
    char *pSrcPos = (char *)pszSrc;
    char *pDestPos = NULL;

    if(hIconv == (iconv_t)-1)
    {
        dwError = errno;
        BAIL_ON_UMN_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(
	          LW_RTL_ALLOCATE(
			  &pwszDest,
			  WCHAR,
			  sDestSize));
    BAIL_ON_UMN_ERROR(dwError);

    pDestPos = (char *)pwszDest;

    while (sSrcSize > 0)
    {
        // Ignore the result of iconv since the error code would just tell us
        // that iconv encountered an illegal sequence.
        iconv(
                hIconv,
                (ICONV_IN_TYPE)&pSrcPos,
                &sSrcSize,
                &pDestPos,
                &sDestSize);
        assert(sDestSize > 0);
        if (sSrcSize > 0)
        {
            // This the official unicode replacement character. It is drawn
            // as a black diamond with a white question mark in the middle.
            *(WCHAR*)pDestPos = 0xFFFD;
            pDestPos += 2;
            sDestSize--;
            pSrcPos++;
            sSrcSize--;
        }
    }
    *(WCHAR*)pDestPos = 0;

    *ppwszDest = pwszDest;

cleanup:
    if (hIconv != (iconv_t)-1)
    {
        iconv_close(hIconv);
    }
    return dwError;

error:
    LW_RTL_FREE(&pwszDest);
    if (ppwszDest)
    {
        *ppwszDest = NULL;
    }
    goto cleanup;
}

VOID
UmnSrvTimevalToTimespec(
    OUT struct timespec *pDest,
    IN struct timeval *pSrc
    )
{
    pDest->tv_sec = pSrc->tv_sec;
    pDest->tv_nsec = pSrc->tv_usec * 1000;
}

VOID
UmnSrvTimespecAdd(
    OUT struct timespec *pDest,
    IN struct timespec *pA,
    IN struct timespec *pB
    )
{
    long lTotalSecs = pA->tv_sec + pB->tv_sec;
    long lTotalNSecs = 0;

    // Just incase the source operands were in an overflowed format, normalize
    // them now.
    lTotalSecs += pA->tv_nsec / NANOSECS_PER_SECOND;
    lTotalSecs += pB->tv_nsec / NANOSECS_PER_SECOND;

    // The largest valid value for tv_nsec is (10^9 - 1). Two of these values
    // can be added without worrying about overflow because:
    //      (10^9 - 1) * 2 < (2^31 - 1)
    lTotalNSecs = pA->tv_nsec % NANOSECS_PER_SECOND +
                    pB->tv_nsec % NANOSECS_PER_SECOND;

    // Carry from nsecs to secs
    lTotalSecs += lTotalNSecs / NANOSECS_PER_SECOND;
    lTotalNSecs %= NANOSECS_PER_SECOND;

    pDest->tv_sec = lTotalSecs;
    pDest->tv_nsec = lTotalNSecs;
}

VOID
UmnSrvTimespecSubtract(
    OUT struct timespec *pDest,
    IN struct timespec *pA,
    IN struct timespec *pB
    )
{
    long lTotalSecs = pA->tv_sec - pB->tv_sec;
    long lTotalNSecs = 0;

    // Just incase the source operands were in an overflowed format, normalize
    // them now.
    lTotalSecs += pA->tv_nsec / NANOSECS_PER_SECOND;
    lTotalSecs -= pB->tv_nsec / NANOSECS_PER_SECOND;

    // The result of this computation is in the range:
    // -(10^9 - 1) < lTotalNSecs < 10^9 - 1
    // That range is representable in long (no overflow)
    lTotalNSecs = pA->tv_nsec % NANOSECS_PER_SECOND -
                    pB->tv_nsec % NANOSECS_PER_SECOND;

    // Carry from nsecs to secs
    if (lTotalNSecs < 0)
    {
        lTotalSecs--;
        lTotalNSecs += NANOSECS_PER_SECOND;
    }

    pDest->tv_sec = lTotalSecs;
    pDest->tv_nsec = lTotalNSecs;
}

DWORD
UmnConvertToCollectorRecords(
    DWORD dwCount,
    EVENT_LOG_RECORD *pEventRecords,
    PLWCOLLECTOR_RECORD *ppCollectorRecords
    )
{
    DWORD dwError = 0;
    PLWCOLLECTOR_RECORD pCollectorRecords = NULL;
    DWORD dwIndex = 0;

    dwError = RTL_ALLOCATE(
                    &pCollectorRecords,
                    LWCOLLECTOR_RECORD,
                    sizeof(*pCollectorRecords) * dwCount);
    BAIL_ON_UMN_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        pCollectorRecords[dwIndex].dwColDateTime = time(NULL);
        pCollectorRecords[dwIndex].event.qwEventRecordId = pEventRecords[dwIndex].dwEventRecordId;
        dwError = RtlWC16StringAllocateFromCString(
                        &pCollectorRecords[dwIndex].event.pwszLogname,
                        pEventRecords[dwIndex].pszEventTableCategoryId);
        BAIL_ON_UMN_ERROR(dwError);
        dwError = RtlWC16StringAllocateFromCString(
                        &pCollectorRecords[dwIndex].event.pwszEventType,
                        pEventRecords[dwIndex].pszEventType);
        BAIL_ON_UMN_ERROR(dwError);
        pCollectorRecords[dwIndex].event.dwEventDateTime = pEventRecords[dwIndex].dwEventDateTime;
        dwError = RtlWC16StringAllocateFromCString(
                        &pCollectorRecords[dwIndex].event.pwszEventSource,
                        pEventRecords[dwIndex].pszEventSource);
        BAIL_ON_UMN_ERROR(dwError);
        dwError = RtlWC16StringAllocateFromCString(
                        &pCollectorRecords[dwIndex].event.pwszEventCategory,
                        pEventRecords[dwIndex].pszEventCategory);
        BAIL_ON_UMN_ERROR(dwError);
        pCollectorRecords[dwIndex].event.dwEventSourceId = pEventRecords[dwIndex].dwEventSourceId;
        dwError = UmnSrvWc16sFromMbsWithRepl(
                        &pCollectorRecords[dwIndex].event.pwszUser,
                        pEventRecords[dwIndex].pszUser);
        BAIL_ON_UMN_ERROR(dwError);
        dwError = RtlWC16StringAllocateFromCString(
                        &pCollectorRecords[dwIndex].event.pwszComputer,
                        pEventRecords[dwIndex].pszComputer);
        BAIL_ON_UMN_ERROR(dwError);
        dwError = UmnSrvWc16sFromMbsWithRepl(
                        &pCollectorRecords[dwIndex].event.pwszDescription,
                        pEventRecords[dwIndex].pszDescription);
        BAIL_ON_UMN_ERROR(dwError);

        if (pEventRecords[dwIndex].pszData)
        {
            dwError = RtlCStringDuplicate(
                            (PSTR*)&pCollectorRecords[dwIndex].event.pvData,
                            pEventRecords[dwIndex].pszData);
            BAIL_ON_UMN_ERROR(dwError);

            pCollectorRecords[dwIndex].event.dwDataLen = strlen(
                    pEventRecords[dwIndex].pszData);
        }
    }

    *ppCollectorRecords = pCollectorRecords;

cleanup:
    return dwError;

error:
    *ppCollectorRecords = NULL;
    if (pCollectorRecords)
    {
        CltrFreeRecordList(dwCount, pCollectorRecords);
    }

    goto cleanup;
}

DWORD
UmnSrvPushEvents(
    IN OUT PDWORD pdwNextRecordId,
    IN FILE *pNextRecordFile,
    IN double dRatioPeriodUsed,
    OUT PDWORD pdwPeriodSecs
    )
{
    DWORD dwError = 0;
    DWORD dwNextRecordId = *pdwNextRecordId;
    DWORD dwBacklog = 0;
    DWORD dwRecordsPerPeriod = 0;
    DWORD dwRecordsPerBatch = 0;
    DWORD dwPeriodSecs = *pdwPeriodSecs;
    DWORD dwBatchCount = 0;
    DWORD dwBatchIndex = 0;
    DWORD dwPeriodCount = 0;
    HANDLE hEventLog = NULL;
    EVENT_LOG_RECORD *pEventRecords = NULL;
    PLWCOLLECTOR_RECORD pCollectorRecords = NULL;
    PSTR pszSPN = NULL;
    PWSTR pwszSPN = NULL;
    PSTR pszCollector = NULL;
    PWSTR pwszCollector = NULL;
    HANDLE hCollector = NULL;
    PSTR pszSqlFilter = NULL;
    PWSTR pwszSqlFilter = NULL;
    LW_PIO_CREDS pAccessToken = NULL;

    // Do push
    UMN_LOG_INFO("Copying events");

    dwError = RtlCStringAllocatePrintf(
                    &pszSqlFilter,
                    "EventRecordId >= %lu",
                    dwNextRecordId);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LwRtlWC16StringAllocateFromCString(
        &pwszSqlFilter,
        pszSqlFilter);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LWIOpenEventLog(
                    NULL,
                    &hEventLog);
    if (dwError == rpc_s_connect_rejected)
    {
        UMN_LOG_WARNING("Unable to connect to eventlog");
    }
    if (dwError == rpc_s_connection_closed)
    {
        UMN_LOG_WARNING("Eventlog closed connection");
    }
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LWICountEventLog(
                    hEventLog,
                    pwszSqlFilter,
                    &dwBacklog);
    BAIL_ON_UMN_ERROR(dwError);

    UMN_LOG_INFO("%lu events are waiting for the collector",
            dwBacklog);

    if (dwBacklog > 0)
    {
        LwRtlCStringFree(&pszCollector);
        dwError = UmnSrvGetCollectorAddress(NULL, &pszCollector);
        if (dwError == LW_STATUS_NOT_FOUND)
        {
            UMN_LOG_WARNING("The collector address is not set in the config file.");
            dwError = LW_STATUS_INVALID_ADDRESS;
        }
        BAIL_ON_UMN_ERROR(dwError);

        LwRtlWC16StringFree(&pwszCollector);
        dwError = RtlWC16StringAllocateFromCString(
                        &pwszCollector,
                        pszCollector);
        BAIL_ON_UMN_ERROR(dwError);

        LwRtlCStringFree(&pszSPN);
        dwError = UmnSrvGetCollectorServicePrincipal(NULL, &pszSPN);
        if (dwError == LW_STATUS_NOT_FOUND)
        {
            dwError = 0;
        }
        BAIL_ON_UMN_ERROR(dwError);

        LwRtlWC16StringFree(&pwszSPN);
        if (pszSPN != NULL)
        {
            dwError = RtlWC16StringAllocateFromCString(
                            &pwszSPN,
                            pszSPN);
            BAIL_ON_UMN_ERROR(dwError);
        }

        if (pAccessToken != NULL)
        {
            LwIoDeleteCreds(pAccessToken);
            pAccessToken = NULL;
        }

        dwError = UmnSrvSetupCredCache(&pAccessToken);
        if (dwError == NERR_SetupNotJoined)
        {
            UMN_LOG_WARNING("Could not find machine password. The computer is possibly not joined.", pszCollector);
        }
        BAIL_ON_UMN_ERROR(dwError);

        dwError = CltrOpenCollector(
                        pwszCollector,
                        pwszSPN,
                        pAccessToken,
                        &hCollector);
        if (dwError == LW_STATUS_CONNECTION_REFUSED ||
            dwError == ERROR_CANNOT_MAKE ||
            dwError == RPC_S_NO_CONTEXT_AVAILABLE)
        {
            UMN_LOG_WARNING("The collector [%s] is currently unreachable.", pszCollector);
        }
        if (dwError == LW_STATUS_RESOURCE_NAME_NOT_FOUND)
        {
            UMN_LOG_WARNING("Unable to get service ticket for collector. Check the collector-principal setting.");
        }
        if (dwError == RPC_S_INVALID_NET_ADDR)
        {
            UMN_LOG_WARNING("The collector hostname is not resolvable in DNS");
        }
        BAIL_ON_UMN_ERROR(dwError);

        dwError = CltrGetPushRate(
                        hCollector,
                        dwBacklog,
                        dRatioPeriodUsed,
                        &dwRecordsPerPeriod,
                        &dwRecordsPerBatch,
                        &dwPeriodSecs);
        BAIL_ON_UMN_ERROR(dwError);

        dwPeriodCount = LW_MIN(dwRecordsPerPeriod, dwBacklog);

        UMN_LOG_INFO("Pushing %lu events to collector %s during this period",
                dwPeriodCount,
                pszCollector);

        dwError = LWIReadEventLog(
                        hEventLog,
                        0,
                        dwPeriodCount,
                        pwszSqlFilter,
                        &dwPeriodCount,
                        &pEventRecords);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = UmnConvertToCollectorRecords(
                        dwPeriodCount,
                        pEventRecords,
                        &pCollectorRecords);
        BAIL_ON_UMN_ERROR(dwError);

        dwBatchIndex = 0;
        while (dwBatchIndex < dwPeriodCount && !gbPollerThreadShouldExit)
        {
            dwBatchCount = LW_MIN(dwRecordsPerBatch,
                            dwPeriodCount - dwBatchIndex);

            UMN_LOG_INFO("Pushing %lu events to collector in this batch",
                    dwBatchCount);

            dwError = CltrWriteRecords(
                            hCollector,
                            dwBatchCount,
                            pCollectorRecords + dwBatchIndex);
            if (dwError == LW_STATUS_ACCESS_DENIED)
            {
                UMN_LOG_ERROR("Insufficient access to write events to the collector.");
            }
            else if (dwError == LW_STATUS_INSUFFICIENT_RESOURCES)
            {
                UMN_LOG_ERROR("Unable to post events to collector. The collector ran out of disk space or ram.");
            }
            BAIL_ON_UMN_ERROR(dwError);

            dwNextRecordId = pEventRecords[dwBatchIndex + dwBatchCount - 1].
                                dwEventRecordId + 1;

            if (fseek(pNextRecordFile, 0L, SEEK_SET) < 0)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }
            if (ftruncate(fileno(pNextRecordFile), 0) < 0)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }
            if (fprintf(pNextRecordFile, "%d\n", dwNextRecordId) < 0)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }
            if (fflush(pNextRecordFile) < 0)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }

            dwBatchIndex += dwBatchCount;
        }

        dwError = CltrCloseCollector(hCollector);
        hCollector = NULL;
        if (dwError == ERROR_GRACEFUL_DISCONNECT)
        {
            UMN_LOG_WARNING("Connection to collector closed while trying to close handle");
            dwError = 0;
        }
        BAIL_ON_UMN_ERROR(dwError);

        LWIFreeEventRecordList(dwPeriodCount, pEventRecords);
        pEventRecords = NULL;

        CltrFreeRecordList(dwPeriodCount, pCollectorRecords);
        pCollectorRecords = NULL;
    }

    dwError = LWICloseEventLog(hEventLog);
    hEventLog = NULL;
    BAIL_ON_UMN_ERROR(dwError);

    *pdwNextRecordId = dwNextRecordId;
    *pdwPeriodSecs = dwPeriodSecs;
    
cleanup:
    LwRtlCStringFree(&pszSqlFilter);
    LwRtlWC16StringFree(&pwszSqlFilter);
    LwRtlCStringFree(&pszCollector);
    LwRtlWC16StringFree(&pwszCollector);
    LwRtlCStringFree(&pszSPN);
    LwRtlWC16StringFree(&pwszSPN);
    if (hEventLog != NULL)
    {
        LWICloseEventLog(hEventLog);
    }
    if (hCollector != NULL)
    {
        CltrCloseCollector(hCollector);
    }
    if (pEventRecords != NULL)
    {
        LWIFreeEventRecordList(dwPeriodCount, pEventRecords);
    }
    if (pCollectorRecords != NULL)
    {
        CltrFreeRecordList(dwPeriodCount, pCollectorRecords);
    }
    if (pAccessToken != NULL)
    {
        LwIoDeleteCreds(pAccessToken);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
UmnSrvPollerRefresh(
    VOID
    )
{
    DWORD dwError = 0;

    if (gbPollerThreadRunning)
    {
        gbPollerRefresh = TRUE;
        dwError = pthread_cond_signal(
                        &gSignalPoller);
        BAIL_ON_UMN_ERROR(dwError);
    }
    else
    {
        dwError = ESRCH;
        BAIL_ON_UMN_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

PVOID
UmnSrvPollerThreadRoutine(
    IN PVOID pUnused
    )
{
    DWORD dwError = 0;
    struct timeval now;
    struct timespec periodStart, periodUsed, nowSpec, nextWake, pushWait = {0};
    BOOLEAN bMutexLocked = FALSE;
    DWORD dwNextRecordId = 0;
    DWORD dwPeriodSecs = 60;
    double dRatioPeriodUsed = 0;
    FILE *pNextRecordFile = NULL;
    int iNextRecordFileFd = -1;

    UMN_LOG_INFO("Event poller thread started");

    iNextRecordFileFd = open(UMN_NEXT_RECORD_DB, O_RDWR, 0);
    if (iNextRecordFileFd < 0)
    {
        if (errno == ENOENT)
        {
            // The file does not exist, so create it but don't read from it
            iNextRecordFileFd = creat(UMN_NEXT_RECORD_DB, S_IRUSR | S_IWUSR);
            if (iNextRecordFileFd < 0)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }
            pNextRecordFile = fdopen(iNextRecordFileFd, "w");
            if (!pNextRecordFile)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }
            iNextRecordFileFd = -1;
            if (fprintf(pNextRecordFile, "%d\n", dwNextRecordId) < 0)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }
            if (fflush(pNextRecordFile) < 0)
            {
                dwError = errno;
                BAIL_ON_UMN_ERROR(dwError);
            }
        }
        else
        {
            dwError = errno;
            BAIL_ON_UMN_ERROR(dwError);
        }
    }
    else
    {
        // The file does exists, so read the current position from it.
        pNextRecordFile = fdopen(iNextRecordFileFd, "r+");
        if (!pNextRecordFile)
        {
            dwError = errno;
            BAIL_ON_UMN_ERROR(dwError);
        }
        iNextRecordFileFd = -1;

        if (fscanf(pNextRecordFile, "%u", &dwNextRecordId) < 1)
        {
            dwError = errno;
            BAIL_ON_UMN_ERROR(dwError);
        }
    }

    UMN_LOG_INFO("Reading records greater than or equal to %d", dwNextRecordId);

    dwError = pthread_mutex_lock(&gSignalPollerMutex);
    BAIL_ON_UMN_ERROR(dwError);
    bMutexLocked = TRUE;

    while (!gbPollerThreadShouldExit)
    {
        dwError = gettimeofday(&now, NULL);
        BAIL_ON_UMN_ERROR(dwError);

        UmnSrvTimevalToTimespec(
            &periodStart,
            &now);

        dwError = gettimeofday(&now, NULL);
        BAIL_ON_UMN_ERROR(dwError);

        // periodUsed = now - periodStart
        UmnSrvTimevalToTimespec(
            &periodUsed,
            &now);

        UmnSrvTimespecSubtract(
            &periodUsed,
            &periodUsed,
            &periodStart);

        if (dwPeriodSecs)
        {
            dRatioPeriodUsed = (periodUsed.tv_sec +
                    periodUsed.tv_nsec / (double)NANOSECS_PER_SECOND) /
                    dwPeriodSecs;
        }
        else
        {
            dRatioPeriodUsed = 0;
        }

        dwError = UmnSrvPushEvents(
            &dwNextRecordId,
            pNextRecordFile,
            dRatioPeriodUsed,
            &dwPeriodSecs);
        if (dwError == ERROR_CONTEXT_EXPIRED) //service ticket expired
        {
            UMN_LOG_WARNING("Service ticket expired. Retrying");
            dwPeriodSecs = 0;
            dwError = 0;
        }
        if (dwError == LW_STATUS_CONNECTION_REFUSED ||//can't contact collector
            dwError == ERROR_CANNOT_MAKE || //can't contact eventlog
            dwError == RPC_S_NO_CONTEXT_AVAILABLE || //can't contact eventlog
            dwError == ERROR_GRACEFUL_DISCONNECT || //can't contact eventlog
            dwError == rpc_s_connection_closed || //can't contact eventlog
            dwError == rpc_s_connect_rejected || //can't contact eventlog
            dwError == LW_STATUS_RESOURCE_NAME_NOT_FOUND || //bad SPN
            dwError == RPC_S_INVALID_NET_ADDR || //bad dns hostname
            dwError == NERR_SetupNotJoined || //not joined
            dwError == LW_STATUS_INVALID_ADDRESS || //bad IP
            dwError == LW_STATUS_ACCESS_DENIED || //no access to collector
            dwError == LW_STATUS_INSUFFICIENT_RESOURCES) //collector out of disk space
        {
            // Try again later
            if (dwPeriodSecs < 60)
            {
                dwPeriodSecs = 60;
            }
            dwError = 0;
        }
        if (dwError)
        {
            UMN_LOG_ERROR("Unknown error %d occurred while pushing events", dwError);
            dwError = 0;
        }

        pushWait.tv_sec = dwPeriodSecs;

        UmnSrvTimespecAdd(
            &nextWake,
            &periodStart,
            &pushWait);

        UMN_LOG_INFO("Event poller sleeping for %f seconds",
                pushWait.tv_sec + pushWait.tv_nsec /
                (double)NANOSECS_PER_SECOND);

        while (!gbPollerThreadShouldExit && !gbPollerRefresh)
        {
            dwError = pthread_cond_timedwait(
                        &gSignalPoller,
                        &gSignalPollerMutex,
                        &nextWake);
            if (dwError == ETIMEDOUT)
            {
                dwError = 0;
                break;
            }
            else if (dwError == EINTR)
            {
                // Try again
                continue;
            }
            else if (dwError == 0)
            {
                // Check that this wasn't a spurious wakeup
                dwError = gettimeofday(&now, NULL);
                BAIL_ON_UMN_ERROR(dwError);

                UmnSrvTimevalToTimespec(
                    &nowSpec,
                    &now);

                if (nowSpec.tv_sec > nextWake.tv_sec ||
                        (nowSpec.tv_sec == nextWake.tv_sec &&
                            nowSpec.tv_nsec >= nextWake.tv_nsec))
                {
                    break;
                }
            }
            else
            {
                BAIL_ON_UMN_ERROR(dwError);
            }
        }
        gbPollerRefresh = FALSE;
    }

    UMN_LOG_INFO("Event poller thread stopped");

cleanup:
    if (bMutexLocked)
    {
        pthread_mutex_unlock(&gSignalPollerMutex);
    }

    if (dwError != 0)
    {
        UMN_LOG_ERROR(
                "User monitor polling thread exiting with code %d",
                dwError);
        kill(getpid(), SIGTERM);
    }
    if (pNextRecordFile)
    {
        fclose(pNextRecordFile);
    }
    if (iNextRecordFileFd >= 0)
    {
        close(iNextRecordFileFd);
    }

    return NULL;

error:
    goto cleanup;
}

DWORD
UmnSrvStopPollerThread(
    VOID
    )
{
    DWORD dwError = 0;

    gbPollerThreadShouldExit = TRUE;
    if (gbPollerThreadRunning)
    {
        dwError = pthread_cond_signal(
                        &gSignalPoller);
        BAIL_ON_UMN_ERROR(dwError);

        dwError = pthread_join(
                        gPollerThread,
                        NULL);
        BAIL_ON_UMN_ERROR(dwError);

        gbPollerThreadRunning = FALSE;
    }

    if (gbSignalPollerCreated)
    {
        dwError = pthread_cond_destroy(&gSignalPoller);
        BAIL_ON_UMN_ERROR(dwError);
        gbSignalPollerCreated = FALSE;
    }
                    
    if (gbSignalPollerMutexCreated)
    {
        dwError = pthread_mutex_destroy(&gSignalPollerMutex);
        BAIL_ON_UMN_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
UmnSrvStartPollerThread(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bDestroyOnError = FALSE;

    if (gbPollerThreadRunning)
    {
        dwError = EEXIST;
        BAIL_ON_UMN_ERROR(dwError);
    }

    bDestroyOnError = TRUE;

    dwError = pthread_cond_init(
                    &gSignalPoller,
                    NULL);
    BAIL_ON_UMN_ERROR(dwError);
    gbSignalPollerCreated = TRUE;
                    
    dwError = pthread_mutex_init(
                    &gSignalPollerMutex,
                    NULL);
    BAIL_ON_UMN_ERROR(dwError);
    gbSignalPollerMutexCreated = TRUE;

    gbPollerThreadShouldExit = FALSE;

    dwError = pthread_create(
                    &gPollerThread,
                    NULL,
                    UmnSrvPollerThreadRoutine,
                    NULL);
    BAIL_ON_UMN_ERROR(dwError);
    gbPollerThreadRunning = TRUE;

cleanup:
    return dwError;

error:
    if (bDestroyOnError)
    {
        UmnSrvStopPollerThread();
    }
    goto cleanup;
}
