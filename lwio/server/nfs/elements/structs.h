/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _NFS_TIMER_REQUEST
{
    LONG                   refCount;

    LONG64                 llExpiry;
    PVOID                  pUserData;
    PFN_NFS_TIMER_CALLBACK pfnTimerExpiredCB;

    struct _NFS_TIMER_REQUEST* pNext;
    struct _NFS_TIMER_REQUEST* pPrev;

} NFS_TIMER_REQUEST;

typedef struct _NFS_TIMER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    pthread_cond_t   event;
    pthread_cond_t*  pEvent;

    PNFS_TIMER_REQUEST pRequests;

    BOOLEAN bStop;

} NFS_TIMER_CONTEXT, *PNFS_TIMER_CONTEXT;

typedef struct _NFS_TIMER
{
    pthread_t  timerThread;
    pthread_t* pTimerThread;

    NFS_TIMER_CONTEXT context;

} NFS_TIMER, *PNFS_TIMER;

typedef struct _NFS_ELEMENTS_GLOBALS
{
    pthread_mutex_t  mutex;

    NFS_TIMER timer;

    PBYTE pHintsBuffer;
    ULONG ulHintsLength;

    LONG64 llBootTime;
    BOOLEAN bShareNameEcpEnabled;

    GENERIC_MAPPING ShareGenericMapping;

    pthread_rwlock_t        statsLock;
    pthread_rwlock_t*       pStatsLock;

    NFS_ELEMENTS_STATISTICS stats;
	
} NFS_ELEMENTS_GLOBALS, *PNFS_ELEMENTS_GLOBALS;

#endif /* __STRUCTS_H__ */



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/


