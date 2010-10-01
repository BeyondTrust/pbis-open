/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "rdr.h"

#define INVALID_TIME ((time_t) -1)

static
void*
RdrReaperThread(
    void* pData
    );

static
NTSTATUS
RdrReaperReapGlobal(
    PRDR_GLOBAL_RUNTIME pRuntime,
    time_t currentTime,
    time_t* pNextWakeupTime
    );

static
NTSTATUS
RdrReaperReapSocket(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SOCKET pSocket,
    time_t currentTime,
    time_t* pNextWakeupTime
    );

static
NTSTATUS
RdrReaperReapSession(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SESSION pSession,
    time_t currentTime,
    time_t* pNextWakeupTime
    );

NTSTATUS
RdrReaperInit(
    PRDR_GLOBAL_RUNTIME pRuntime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pthread_mutex_init(&pRuntime->reaperMutex, NULL);
    pthread_cond_init(&pRuntime->reaperEvent, NULL);

    pRuntime->expirationTime = 10;
    pRuntime->nextWakeupTime = INVALID_TIME;

    pthread_create(
        &pRuntime->reaperThread,
        NULL,
        RdrReaperThread,
        pRuntime);
    
    return ntStatus;
}

NTSTATUS
RdrReaperShutdown(
    PRDR_GLOBAL_RUNTIME pRuntime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pthread_mutex_lock(&pRuntime->reaperMutex);
    pRuntime->bShutdown = TRUE;
    pthread_cond_signal(&pRuntime->reaperEvent);
    pthread_mutex_unlock(&pRuntime->reaperMutex);
    pthread_join(pRuntime->reaperThread, NULL);

    pthread_cond_destroy(&pRuntime->reaperEvent);
    pthread_mutex_destroy(&pRuntime->reaperMutex);

    return ntStatus;
}

VOID
RdrReaperPoke(
    PRDR_GLOBAL_RUNTIME pRuntime,
    time_t lastActiveTime
    )
{
    time_t nextWakeupTime = lastActiveTime + pRuntime->expirationTime;

    pthread_mutex_lock(&pRuntime->reaperMutex);
    if (pRuntime->nextWakeupTime == INVALID_TIME ||
        pRuntime->nextWakeupTime > nextWakeupTime)
    {
        pRuntime->nextWakeupTime = nextWakeupTime;
        pthread_cond_signal(&pRuntime->reaperEvent);
    }
    pthread_mutex_unlock(&pRuntime->reaperMutex);
}

static
void*
RdrReaperThread(
    void* pData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PRDR_GLOBAL_RUNTIME pRuntime = pData;
    time_t currentTime = 0;
    BOOLEAN bInReaperLock = FALSE;
    struct timespec wakeTime = {0};
    time_t nextWakeupTime = INVALID_TIME;

    LWIO_LOCK_MUTEX(bInReaperLock, &pRuntime->reaperMutex);

    while (!pRuntime->bShutdown)
    {
        if (pRuntime->nextWakeupTime != INVALID_TIME)
        {
            /* If we have a pending wakeup time, sleep for that long */
            wakeTime.tv_sec = pRuntime->nextWakeupTime;
            wakeTime.tv_nsec = 0;
            
            pthread_cond_timedwait(
                &pRuntime->reaperEvent,
                &pRuntime->reaperMutex,
                &wakeTime
                );
        }
        else
        {
            /* Sleep until someone pokes us with a new wakeup time */
            pthread_cond_wait(&pRuntime->reaperEvent, &pRuntime->reaperMutex);
        }

        currentTime = time(NULL);
        
        /* Run reaping algorithm if we are scheduled for a wakeup and the wakeup has passed */
        if (!pRuntime->bShutdown && pRuntime->nextWakeupTime != INVALID_TIME && currentTime >= pRuntime->nextWakeupTime)
        {
            /* Clear wakeup time */
            pRuntime->nextWakeupTime = INVALID_TIME;

            /* Run reaping algorithm outside the reaper lock to avoid blocking pokes */
            LWIO_UNLOCK_MUTEX(bInReaperLock, &pRuntime->reaperMutex);

            nextWakeupTime = INVALID_TIME;
            
            ntStatus = RdrReaperReapGlobal(
                pRuntime,
                currentTime,
                &nextWakeupTime);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_LOCK_MUTEX(bInReaperLock, &pRuntime->reaperMutex);
            
            /* Someone may have poked us with a new wakeup time while we did not hold
               the reaper lock, so choose either it or the value we calculated ourselves
               as appropriate */
            if (nextWakeupTime != INVALID_TIME &&
                (pRuntime->nextWakeupTime == INVALID_TIME ||
                 pRuntime->nextWakeupTime > nextWakeupTime))
            {
                pRuntime->nextWakeupTime = nextWakeupTime;
            }
        }
    }

    /* Run RdrReaperReapGlobal() one more time now that bShutdown is set
     * to clean up all remaining objects
     */

    LWIO_UNLOCK_MUTEX(bInReaperLock, &pRuntime->reaperMutex);

    ntStatus = RdrReaperReapGlobal(
           pRuntime,
           currentTime,
           &nextWakeupTime);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LWIO_UNLOCK_MUTEX(bInReaperLock, &pRuntime->reaperMutex);

    return NULL;
}

static
NTSTATUS
RdrReaperSnapshotHashValues(
    PSMB_HASH_TABLE pHash,
    void*** pppSnapshot,
    PULONG pulSnapshotCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    void** ppSnapshot = NULL;
    void** ppNewSnapshot = NULL;
    SMB_HASH_ITERATOR iter = {0};
    SMB_HASH_ENTRY* pEntry = NULL;
    ULONG ulIndex = 0;
    ULONG ulCapacity = 0;

    ntStatus = SMBHashGetIterator(pHash, &iter);
    BAIL_ON_NT_STATUS(ntStatus);

    do 
    {
        pEntry = SMBHashNext(&iter);

        if (ulIndex >= ulCapacity)
        {
            if (ulCapacity == 0)
            {
                ulCapacity = 8;
            }
            else
            {
                ulCapacity *= 2;
            }
            
            ppNewSnapshot = LwRtlMemoryRealloc(ppSnapshot, ulCapacity * sizeof(*ppSnapshot));
            if (!ppNewSnapshot)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ppSnapshot = ppNewSnapshot;
        }

        if (pEntry)
        {
            ppSnapshot[ulIndex++] = pEntry->pValue;
        }
        else
        {
            ppSnapshot[ulIndex] = NULL;
        }
    } while (pEntry != NULL);

    *pppSnapshot = ppSnapshot;
    *pulSnapshotCount = ulIndex;

cleanup:

    return ntStatus;

error:

    *pppSnapshot = NULL;

    RTL_FREE(&ppSnapshot);
    
    goto cleanup;
}

static
BOOLEAN
RdrReaperIsExpired(
    time_t lastActivity,
    time_t currentTime,
    time_t expirationTime
    )
{
    return lastActivity + expirationTime <= currentTime;
}

static
VOID
RdrReaperUpdateNextWakeTime(
    time_t lastActivity,
    time_t expirationTime,
    time_t* pNextWakeupTime
    )
{
    if (*pNextWakeupTime == INVALID_TIME ||
        *pNextWakeupTime > lastActivity + expirationTime)
    {
        *pNextWakeupTime = lastActivity + expirationTime;
    }
}


static
NTSTATUS
RdrReaperReapGlobal(
    PRDR_GLOBAL_RUNTIME pRuntime,
    time_t currentTime,
    time_t* pNextWakeupTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SOCKET* pSocketList = NULL;
    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInHashLock = FALSE;
    time_t expirationTime = pRuntime->expirationTime;
    ULONG ulSocketCount = 0;
    ULONG ulIndex = 0;

    /* Within lock, obtain a snapshot of all sockets and take a reference to each */
    LWIO_LOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

    ntStatus = RdrReaperSnapshotHashValues(
        pRuntime->pSocketHashByName,
        (void***) (void*) &pSocketList,
        &ulSocketCount);
    BAIL_ON_NT_STATUS(ntStatus);

    for (ulIndex = 0; ulIndex < ulSocketCount; ulIndex++)
    {
        pSocket = pSocketList[ulIndex];
        pSocket->refCount++;
    }

    LWIO_UNLOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

    /* Reap sessions within each socket */
    for (ulIndex = 0; ulIndex < ulSocketCount; ulIndex++)
    {
        pSocket = pSocketList[ulIndex];
        
        ntStatus = RdrReaperReapSocket(
            pRuntime,
            pSocket,
            currentTime,
            pNextWakeupTime);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pSocketList)
    {
        /* Within lock, remove reference to each socket.  If the
           socket is expired, destroy it.  Otherwise, update the
           time we should next wake at */
        LWIO_LOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

        for (ulIndex = 0; ulIndex < ulSocketCount; ulIndex++)
        {
            pSocket = pSocketList[ulIndex];
        
            if (--pSocket->refCount == 0)
            {
                if (pRuntime->bShutdown ||
                    RdrReaperIsExpired(pSocket->lastActiveTime,
                        currentTime,
                        expirationTime))
                {
                    /* Ref count is 0 and socket has expired,
                       so remove it from the hash and leave it
                       in list to be freed */

                    LWIO_LOG_VERBOSE("Socket %p is expired, reaping", pSocket);
                    SMBHashRemoveKey(
                        pRuntime->pSocketHashByName,
                        pSocket->pwszHostname
                        );
                }
                else
                {
                    LWIO_LOG_VERBOSE("Socket %p is not expired, waiting", pSocket);
                    /* Ref count is 0 but the socket has not expired,
                       so take it out of the list so it is not freed
                       and calculate when it will expire */
                    pSocketList[ulIndex] = NULL;
                    
                    RdrReaperUpdateNextWakeTime(
                        pSocket->lastActiveTime,
                        expirationTime,
                        pNextWakeupTime);
                
                }
            }
            else
            {
                /* Ref count is not 0, so take it out of list
                   so it it not freed */
                pSocketList[ulIndex] = NULL;
            }
        }

        LWIO_UNLOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);
    }

    /* Free everything that was left in the list */
    for (ulIndex = 0; ulIndex < ulSocketCount; ulIndex++)
    {
        pSocket = pSocketList[ulIndex];
        if (pSocket)
        {
            SMBSocketFree(pSocket);
        }
    }

    RTL_FREE(&pSocketList);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

    goto cleanup;
}
    
static
NTSTATUS
RdrReaperReapSocket(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SOCKET pSocket,
    time_t currentTime,
    time_t* pNextWakeupTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SESSION* pSessionList = NULL;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInSocketLock = FALSE;
    time_t expirationTime = pRuntime->expirationTime;
    ULONG ulSessionCount = 0;
    ULONG ulIndex = 0;

    /* Within lock, obtain a snapshot of all sessions and take a reference to each */
    LWIO_LOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    ntStatus = RdrReaperSnapshotHashValues(
        pSocket->pSessionHashByUID,
        (void***) (void*) &pSessionList,
        &ulSessionCount);
    BAIL_ON_NT_STATUS(ntStatus);

    for (ulIndex = 0; ulIndex < ulSessionCount; ulIndex++)
    {
        pSession = pSessionList[ulIndex];
         pSession->refCount++;
    }

    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    /* Reap trees within each session */
    for (ulIndex = 0; ulIndex < ulSessionCount; ulIndex++)
    {
        pSession = pSessionList[ulIndex];

        ntStatus = RdrReaperReapSession(
            pRuntime,
            pSession,
            currentTime,
            pNextWakeupTime);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pSessionList)
    {
        /* Within lock, remove reference to each session.  If the
           session is expired, destroy it.  Otherwise, update the
           time we should next wake at */
        LWIO_LOCK_MUTEX(bInSocketLock, &pSocket->mutex);

        for (ulIndex = 0; ulIndex < ulSessionCount; ulIndex++)
        {
            pSession = pSessionList[ulIndex];
        
            if (--pSession->refCount == 0)
            {
                if (pRuntime->bShutdown ||
                    RdrReaperIsExpired(pSession->lastActiveTime,
                        currentTime,
                        expirationTime))
                {
                    LWIO_LOG_VERBOSE("Session %p is expired, reaping", pSession);
                    
                    SMBHashRemoveKey(
                        pSocket->pSessionHashByUID,
                        &pSession->uid
                        );
                    SMBHashRemoveKey(
                        pSocket->pSessionHashByPrincipal,
                        &pSession->key
                    );
                }
                else
                {
                    LWIO_LOG_VERBOSE("Session %p is not expired, waiting", pSession);

                    pSessionList[ulIndex] = NULL;
                    
                    RdrReaperUpdateNextWakeTime(
                        pSession->lastActiveTime,
                        expirationTime,
                        pNextWakeupTime);
                }
            }
            else
            {
                pSessionList[ulIndex] = NULL;
            }
        }

        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);
    }

    for (ulIndex = 0; ulIndex < ulSessionCount; ulIndex++)
    {
        pSession = pSessionList[ulIndex];
        if (pSession)
        {
            /* Attempt to log off session, ignoring errors */
            Logoff(pSession);
            SMBSessionFree(pSession);
        }
    }

    RTL_FREE(&pSessionList);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    goto cleanup;
}

static
NTSTATUS
RdrReaperReapSession(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SESSION pSession,
    time_t currentTime,
    time_t* pNextWakeupTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_TREE* pTreeList = NULL;
    PSMB_TREE pTree = NULL;
    BOOLEAN bInSessionLock = FALSE;
    time_t expirationTime = pRuntime->expirationTime;
    ULONG ulTreeCount = 0;
    ULONG ulIndex = 0;

    LWIO_LOCK_MUTEX(bInSessionLock, &pSession->mutex);

    ntStatus = RdrReaperSnapshotHashValues(
        pSession->pTreeHashByPath,
        (void***) (void*) &pTreeList,
        &ulTreeCount);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pTreeList)
    {
        for (ulIndex = 0; ulIndex < ulTreeCount; ulIndex++)
        {
            pTree = pTreeList[ulIndex];
        
            if (pTree->refCount == 0)
            {
                if (pRuntime->bShutdown ||
                    RdrReaperIsExpired(pTree->lastActiveTime,
                        currentTime,
                        expirationTime))
                {
                    LWIO_LOG_VERBOSE("Tree %p is expired, reaping", pTree);

                    /* We can't remove the tree from
                       the TID hash because the receiver
                       thread will need it to deliver the
                       tree disconnect response.  It will
                       be removed in a separate pass.  We
                       can still remove it from the path
                       hash to prevent anyone else trying
                       to use it for new requests */
                    SMBHashRemoveKey(
                        pSession->pTreeHashByPath,
                        pTree->pszPath
                        );
                }
                else
                {
                    LWIO_LOG_VERBOSE("Tree %p is not expired, waiting", pTree);

                    pTreeList[ulIndex] = NULL;
                    
                    RdrReaperUpdateNextWakeTime(
                        pTree->lastActiveTime,
                        expirationTime,
                        pNextWakeupTime);
                }
            }
            else
            {
                pTreeList[ulIndex] = NULL;
            }
        }
    }

    LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);
    
    /* Pass one -- disconnect each tree outside the lock.
       We can't free or remove the tree from the session
       TID hash until we have received the disconnect reply */
    for (ulIndex = 0; ulIndex < ulTreeCount; ulIndex++)
    {
        pTree = pTreeList[ulIndex];
        if (pTree)
        {
            TreeDisconnect(pTree);
        }
    }

    LWIO_LOCK_MUTEX(bInSessionLock, &pSession->mutex);

    /* Pass two -- remove and free each tree now that they
       are all disconnected */
    for (ulIndex = 0; ulIndex < ulTreeCount; ulIndex++)
    {
        pTree = pTreeList[ulIndex];
        if (pTree)
        {           
            SMBHashRemoveKey(
                pSession->pTreeHashByTID,
                &pTree->tid
                );
            
            SMBTreeFree(pTree);
        }
    }
            
    LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

    RTL_FREE(&pTreeList);

    return ntStatus;

error:

    goto cleanup;
}
