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
 *        poller_p.h
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 * 
 *        Poller thread
 *
 *        Private functions for the poller thread
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 * 
 */
#ifndef __POLLER_P_H__
#define __POLLER_P_H__

DWORD
UmnSrvWc16sFromMbsWithRepl(
    OUT PWSTR* ppwszDest,
    IN PCSTR pszSrc
    );

VOID
UmnSrvTimevalToTimespec(
    OUT struct timespec *pDest,
    IN struct timeval *pSrc
    );

VOID
UmnSrvTimespecAdd(
    OUT struct timespec *pDest,
    IN struct timespec *pA,
    IN struct timespec *pB
    );

DWORD
UmnSrvPushEvents(
    IN OUT PDWORD pdwNextRecordId,
    IN FILE *pNextRecordFile,
    IN double dRatioPeriodUsed,
    OUT PDWORD pdwPeriodSecs
    );

PVOID
UmnSrvPollerThreadRoutine(
    IN PVOID pUnused
    );

DWORD
UmnConvertToCollectorRecords(
    DWORD dwCount,
    EVENT_LOG_RECORD *pEventRecords,
    PLWCOLLECTOR_RECORD *ppCollectorRecords
    );

#endif /* __POLLER_P_H__ */
