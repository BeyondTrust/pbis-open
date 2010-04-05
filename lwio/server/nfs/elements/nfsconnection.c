/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
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
 *        nfsconnection.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        Connection Object
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
NfsConnection2AcquireAsyncId_inlock(
   PLWIO_NFS_CONNECTION pConnection,
   PULONG64             pullAsyncId
   );

static
VOID
NfsConnectionFree(
    PLWIO_NFS_CONNECTION pConnection
    );

// Rules:
//
// Only one reader thread can read from this socket
// Multiple writers can write to the socket, in a synchronized manner per message
// Both readers and writers can set the connection state to be invalid
// The file descriptor is set to -1 only by the reader thread (not the writers)
// The file descriptor is closed only when the connection object is freed.

static
NTSTATUS
NfsConnectionAcquireSessionId_inlock(
   PLWIO_NFS_CONNECTION pConnection,
   PUSHORT             pUid
   );

static
VOID
NfsConnectionFreeContentsClientProperties(
    PNFS_CLIENT_PROPERTIES pProperties
    );

static
int
NfsConnectionSessionCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
NfsConnectionSessionRelease(
    PVOID pSession
    );

static
NTSTATUS
NfsConnection2AcquireSessionId_inlock(
   PLWIO_NFS_CONNECTION pConnection,
   PULONG64             pUid
   );

static
int
NfsConnection2SessionCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
NfsConnection2SessionRelease(
    PVOID pSession
    );

static
int
NfsConnection2AsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
NfsConnection2AsyncStateRelease(
    PVOID pAsyncState
    );

static
VOID
NfsConnectionRundown_inlock(
    PLWIO_NFS_CONNECTION pConnection
    );

static
NTSTATUS
NfsConnectionRundownSessionRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
NfsConnection2RundownSessionRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
NfsConnection2RundownAsyncStatesRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

NTSTATUS
NfsConnectionCreate(
    PLWIO_NFS_SOCKET                pSocket,
    HANDLE                          hPacketAllocator,
    HANDLE                          hGssContext,
    PLWIO_NFS_SHARE_ENTRY_LIST      pShareList,
    PNFS_PROPERTIES                 pServerProperties,
    PNFS_HOST_INFO                  pHostinfo,
    PNFS_CONNECTION_SOCKET_DISPATCH pSocketDispatch,
    PLWIO_NFS_CONNECTION*           ppConnection
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_CONNECTION pConnection = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(*pConnection),
                    (PVOID*)&pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->refCount = 1;

    pthread_rwlock_init(&pConnection->mutex, NULL);
    pConnection->pMutex = &pConnection->mutex;

    pthread_mutex_init(&pConnection->mutexGssNegotiate, NULL);
    pConnection->pMutexGssNegotiate = &pConnection->mutexGssNegotiate;

    ntStatus = LwRtlRBTreeCreate(
                    &NfsConnectionSessionCompare,
                    NULL,
                    &NfsConnectionSessionRelease,
                    &pConnection->pSessionCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeCreate(
                    &NfsConnection2AsyncStateCompare,
                    NULL,
                    &NfsConnection2AsyncStateRelease,
                    &pConnection->pAsyncStateCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAcquireHostInfo(
                    pHostinfo,
                    &pConnection->pHostinfo);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsGssAcquireContext(
                    pConnection->pHostinfo,
                    hGssContext,
                    &pConnection->hGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->ulSequence = 0;
    pConnection->hPacketAllocator = hPacketAllocator;
    pConnection->pShareList = pShareList;
    pConnection->state = LWIO_NFS_CONN_STATE_INITIAL;
    pConnection->pSocket = pSocket;
    pConnection->pSocketDispatch = pSocketDispatch;

    memcpy(&pConnection->serverProperties, pServerProperties, sizeof(*pServerProperties));
    uuid_copy(pConnection->serverProperties.GUID, pServerProperties->GUID);

    NFS_ELEMENTS_INCREMENT_CONNECTIONS;

    *ppConnection = pConnection;

cleanup:

    return ntStatus;

error:

    *ppConnection = NULL;

    if (pConnection)
    {
        NfsConnectionRelease(pConnection);
    }

    goto cleanup;
}

SMB_PROTOCOL_VERSION
NfsConnectionGetProtocolVersion(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    BOOLEAN bInLock = FALSE;
    SMB_PROTOCOL_VERSION protocolVersion = SMB_PROTOCOL_VERSION_UNKNOWN;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    protocolVersion = pConnection->protocolVer;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return protocolVersion;
}

NTSTATUS
NfsConnectionSetProtocolVersion(
    PLWIO_NFS_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = NfsConnectionSetProtocolVersion_inlock(pConnection, protoVer);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;
}

NTSTATUS
NfsConnectionSetProtocolVersion_inlock(
    PLWIO_NFS_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (protoVer != pConnection->protocolVer)
    {
        if (pConnection->pSessionCollection)
        {
            LwRtlRBTreeFree(pConnection->pSessionCollection);
            pConnection->pSessionCollection = NULL;
        }

        pConnection->protocolVer = protoVer;

        switch (protoVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                pConnection->ulSequence = 0;
                pConnection->usNextAvailableUid = 0;

                ntStatus = LwRtlRBTreeCreate(
                                &NfsConnectionSessionCompare,
                                NULL,
                                &NfsConnectionSessionRelease,
                                &pConnection->pSessionCollection);

                break;

            case SMB_PROTOCOL_VERSION_2:

                pConnection->ullNextAvailableUid = 0;

                ntStatus = LwRtlRBTreeCreate(
                                &NfsConnection2SessionCompare,
                                NULL,
                                &NfsConnection2SessionRelease,
                                &pConnection->pSessionCollection);

                break;

            default:

                ntStatus = STATUS_INVALID_PARAMETER_2;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

BOOLEAN
NfsConnectionIsInvalid(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    BOOLEAN bInvalid = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    bInvalid = pConnection->state == LWIO_NFS_CONN_STATE_INVALID;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return bInvalid;
}

VOID
NfsConnectionSetInvalid(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bDisconnect = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (pConnection->state != LWIO_NFS_CONN_STATE_INVALID)
    {
        bDisconnect = TRUE;
        pConnection->state = LWIO_NFS_CONN_STATE_INVALID;
        NfsConnectionRundown_inlock(pConnection);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    // Call disconnect w/o lock held.
    if (bDisconnect && pConnection->pSocket)
    {
        pConnection->pSocketDispatch->pfnDisconnect(pConnection->pSocket);
    }
}

LWIO_NFS_CONN_STATE
NfsConnectionGetState(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    LWIO_NFS_CONN_STATE connState = LWIO_NFS_CONN_STATE_INITIAL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    connState = pConnection->state;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return connState;
}

VOID
NfsConnectionSetState(
    PLWIO_NFS_CONNECTION pConnection,
    LWIO_NFS_CONN_STATE  connState
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    pConnection->state = connState;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);
}


NTSTATUS
NfsConnectionFindSession(
    PLWIO_NFS_CONNECTION pConnection,
    USHORT uid,
    PLWIO_NFS_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    pSession = pConnection->lruSession[ uid % NFS_LRU_CAPACITY ];
    if (!pSession || (pSession->uid != uid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &uid,
                        (PVOID*)&pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnection->lruSession[ uid % NFS_LRU_CAPACITY ] = pSession;
    }

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_INVALID_HANDLE;
    }

    *ppSession = NULL;

    goto cleanup;
}

NTSTATUS
NfsConnection2FindSession(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullUid,
    PLWIO_NFS_SESSION_2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_SESSION_2 pSession = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    pSession = pConnection->lruSession2[ ullUid % NFS_LRU_CAPACITY ];
    if (!pSession || (pSession->ullUid != ullUid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &ullUid,
                        (PVOID*)&pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnection->lruSession2[ ullUid % NFS_LRU_CAPACITY ] = pSession;
    }

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_USER_SESSION_DELETED;
    }

    *ppSession = NULL;

    goto cleanup;
}

NTSTATUS
NfsConnectionRemoveSession(
    PLWIO_NFS_CONNECTION pConnection,
    USHORT              uid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_NFS_SESSION pSession = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    pSession = pConnection->lruSession[ uid % NFS_LRU_CAPACITY ];
    if (pSession && (pSession->uid == uid))
    {
        pConnection->lruSession[ uid % NFS_LRU_CAPACITY ] = NULL;
    }

    ntStatus = LwRtlRBTreeRemove(
                    pConnection->pSessionCollection,
                    &uid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsConnection2RemoveSession(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullUid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_NFS_SESSION_2 pSession = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    pSession = pConnection->lruSession2[ ullUid % NFS_LRU_CAPACITY ];
    if (pSession && (pSession->ullUid == ullUid))
    {
        pConnection->lruSession2[ ullUid % NFS_LRU_CAPACITY ] = NULL;
    }

    ntStatus = LwRtlRBTreeRemove(
                    pConnection->pSessionCollection,
                    &ullUid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsConnectionCreateSession(
    PLWIO_NFS_CONNECTION pConnection,
    PLWIO_NFS_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;
    USHORT  uid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = NfsConnectionAcquireSessionId_inlock(
                    pConnection,
                    &uid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsSessionCreate(
                    uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pConnection->pSessionCollection,
                    &pSession->uid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        NfsSessionRelease(pSession);
    }

    goto cleanup;
}

NTSTATUS
NfsConnection2CreateSession(
    PLWIO_NFS_CONNECTION pConnection,
    PLWIO_NFS_SESSION_2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_NFS_SESSION_2 pSession = NULL;
    BOOLEAN bInLock = FALSE;
    ULONG64 ullUid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = NfsConnection2AcquireSessionId_inlock(
                    pConnection,
                    &ullUid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsSession2Create(
                    ullUid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pConnection->pSessionCollection,
                    &pSession->ullUid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        NfsSession2Release(pSession);
    }

    goto cleanup;
}

NTSTATUS
NfsConnection2CreateAsyncState(
    PLWIO_NFS_CONNECTION            pConnection,
    USHORT                          usCommand,
    PFN_LWIO_NFS_CANCEL_ASYNC_STATE pfnCancelAsyncState,
    PFN_LWIO_NFS_FREE_ASYNC_STATE   pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*              ppAsyncState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PLWIO_ASYNC_STATE pAsyncState = NULL;
    ULONG64           ullAsyncId  = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = NfsConnection2AcquireAsyncId_inlock(
                    pConnection,
                    &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAsyncStateCreate(
                    ullAsyncId,
                    usCommand,
                    NULL,
                    pfnCancelAsyncState,
                    pfnFreeAsyncState,
                    &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pConnection->pAsyncStateCollection,
                    &pAsyncState->ullAsyncId,
                    pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAsyncState = NfsAsyncStateAcquire(pAsyncState);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppAsyncState = NULL;

    if (pAsyncState)
    {
        NfsAsyncStateRelease(pAsyncState);
    }

    goto cleanup;
}

static
NTSTATUS
NfsConnection2AcquireAsyncId_inlock(
   PLWIO_NFS_CONNECTION pConnection,
   PULONG64             pullAsyncId
   )
{
    NTSTATUS ntStatus = 0;
    ULONG64  ullCandidateAsyncId = pConnection->ullNextAvailableAsyncId;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_ASYNC_STATE pAsyncState = NULL;

        /* 0 is never a valid async id */

        if ((ullCandidateAsyncId == 0) || (ullCandidateAsyncId == UINT64_MAX))
        {
            ullCandidateAsyncId = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pConnection->pAsyncStateCollection,
                        &ullCandidateAsyncId,
                        (PVOID*)&pAsyncState);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            ullCandidateAsyncId++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((ullCandidateAsyncId != pConnection->ullNextAvailableAsyncId) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_CONTEXT_IDS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pullAsyncId = ullCandidateAsyncId;

    /* Increment by 1 by make sure to deal with wrap around */

    ullCandidateAsyncId++;
    pConnection->ullNextAvailableAsyncId = ullCandidateAsyncId ? ullCandidateAsyncId : 1;

cleanup:

    return ntStatus;

error:

    *pullAsyncId = 0;

    goto cleanup;
}

NTSTATUS
NfsConnection2FindAsyncState(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullAsyncId,
    PLWIO_ASYNC_STATE*   ppAsyncState
    )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PLWIO_ASYNC_STATE pAsyncState = NULL;
    BOOLEAN           bInLock     = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pConnection->pAsyncStateCollection,
                    &ullAsyncId,
                    (PVOID*)&pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAsyncState = NfsAsyncStateAcquire(pAsyncState);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppAsyncState = NULL;

    goto cleanup;
}

NTSTATUS
NfsConnection2RemoveAsyncState(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullAsyncId
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pConnection->pAsyncStateCollection,
                    &ullAsyncId);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;
}

NTSTATUS
NfsConnectionGetNamedPipeClientAddress(
    PLWIO_NFS_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pAddr = NULL;
    ULONG ulAddrLength = 0;

    ntStatus = pConnection->pSocketDispatch->pfnGetAddressBytes(
                    pConnection->pSocket,
                    &pAddr,
                    &ulAddrLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoRtlEcpListInsert(pEcpList,
                                  IO_ECP_TYPE_PEER_ADDRESS,
                                  pAddr,
                                  ulAddrLength,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsConnectionGetNamedPipeSessionKey(
    PLWIO_NFS_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pSessionKey = pConnection->pSessionKey;
    ULONG ulSessionKeyLength = pConnection->ulSessionKeyLength;

    if (pSessionKey != NULL)
    {
        ntStatus = IoRtlEcpListInsert(pEcpList,
                                      IO_ECP_TYPE_SESSION_KEY,
                                      pSessionKey,
                                      ulSessionKeyLength,
                                      NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

PLWIO_NFS_CONNECTION
NfsConnectionAcquire(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    InterlockedIncrement(&pConnection->refCount);

    return pConnection;
}

VOID
NfsConnectionRelease(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    if (InterlockedDecrement(&pConnection->refCount) == 0)
    {
        NFS_ELEMENTS_DECREMENT_CONNECTIONS;

        NfsConnectionFree(pConnection);
    }
}

static
VOID
NfsConnectionFree(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    if (pConnection->readerState.pRequestPacket)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pConnection->readerState.pRequestPacket);
    }

    if (pConnection->pSessionKey)
    {
        NfsFreeMemory(pConnection->pSessionKey);
    }

    if (pConnection->hGssNegotiate)
    {
        NfsGssEndNegotiate(
            pConnection->hGssContext,
            pConnection->hGssNegotiate);
    }

    if (pConnection->hGssContext)
    {
        NfsGssReleaseContext(pConnection->hGssContext);
    }

    if (pConnection->pSocket)
    {
        pConnection->pSocketDispatch->pfnFree(pConnection->pSocket);
    }

    if (pConnection->pSessionCollection)
    {
        LwRtlRBTreeFree(pConnection->pSessionCollection);
    }

    if (pConnection->pAsyncStateCollection)
    {
        LwRtlRBTreeFree(pConnection->pAsyncStateCollection);
    }

    if (pConnection->pHostinfo)
    {
        NfsReleaseHostInfo(pConnection->pHostinfo);
    }

    if (pConnection->pMutex)
    {
        pthread_rwlock_destroy(&pConnection->mutex);
        pConnection->pMutex = NULL;
    }

    if (pConnection->pMutexGssNegotiate)
    {
        pthread_mutex_destroy(&pConnection->mutexGssNegotiate);;
        pConnection->pMutexGssNegotiate = NULL;
    }

    NfsConnectionFreeContentsClientProperties(&pConnection->clientProperties);

    NfsFreeMemory(pConnection);
}

static
NTSTATUS
NfsConnectionAcquireSessionId_inlock(
   PLWIO_NFS_CONNECTION pConnection,
   PUSHORT             pUid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateUid = pConnection->usNextAvailableUid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_NFS_SESSION pSession = NULL;

        /* 0 is never a valid session vuid */

        if ((candidateUid == 0) || (candidateUid == UINT16_MAX))
        {
            candidateUid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &candidateUid,
                        (PVOID*)&pSession);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            candidateUid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateUid != pConnection->usNextAvailableUid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_SESSIONS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pUid = candidateUid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    candidateUid++;
    pConnection->usNextAvailableUid = candidateUid ? candidateUid : 1;

cleanup:

    return ntStatus;

error:

    *pUid = 0;

    goto cleanup;
}

static
VOID
NfsConnectionFreeContentsClientProperties(
    PNFS_CLIENT_PROPERTIES pProperties
    )
{
    if (pProperties->pwszNativeLanMan)
    {
        NfsFreeMemory(pProperties->pwszNativeLanMan);
    }
    if (pProperties->pwszNativeOS)
    {
        NfsFreeMemory(pProperties->pwszNativeOS);
    }
    if (pProperties->pwszNativeDomain)
    {
        NfsFreeMemory(pProperties->pwszNativeDomain);
    }
}

static
int
NfsConnectionSessionCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pUid1 = (PUSHORT)pKey1;
    PUSHORT pUid2 = (PUSHORT)pKey2;

    assert (pUid1 != NULL);
    assert (pUid2 != NULL);

    if (*pUid1 > *pUid2)
    {
        return 1;
    }
    else if (*pUid1 < *pUid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
NfsConnectionSessionRelease(
    PVOID pSession
    )
{
    NfsSessionRelease((PLWIO_NFS_SESSION)pSession);
}

static
NTSTATUS
NfsConnection2AcquireSessionId_inlock(
   PLWIO_NFS_CONNECTION pConnection,
   PULONG64             pUid
   )
{
    NTSTATUS ntStatus = 0;
    ULONG64  candidateUid = pConnection->ullNextAvailableUid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_NFS_SESSION_2 pSession = NULL;

        /* 0 is never a valid session vuid */

        if ((candidateUid == 0) || (candidateUid == UINT64_MAX))
        {
            candidateUid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &candidateUid,
                        (PVOID*)&pSession);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            candidateUid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateUid != pConnection->ullNextAvailableUid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_SESSIONS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pUid = candidateUid;

    /* Increment by 1 by make sure to deal with wrap-around */

    candidateUid++;
    pConnection->ullNextAvailableUid = candidateUid ? candidateUid : 1;

cleanup:

    return ntStatus;

error:

    *pUid = 0;

    goto cleanup;
}

static
int
NfsConnection2SessionCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG64 pUid1 = (PULONG64)pKey1;
    PULONG64 pUid2 = (PULONG64)pKey2;

    assert (pUid1 != NULL);
    assert (pUid2 != NULL);

    if (*pUid1 > *pUid2)
    {
        return 1;
    }
    else if (*pUid1 < *pUid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
NfsConnection2SessionRelease(
    PVOID pSession
    )
{
    NfsSession2Release((PLWIO_NFS_SESSION_2)pSession);
}

static
int
NfsConnection2AsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG64 pAsyncId1 = (PULONG64)pKey1;
    PULONG64 pAsyncId2 = (PULONG64)pKey2;

    if (*pAsyncId1 > *pAsyncId2)
    {
        return 1;
    }
    else if (*pAsyncId1 < *pAsyncId2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
NfsConnection2AsyncStateRelease(
    PVOID pAsyncState
    )
{
    NfsAsyncStateRelease((PLWIO_ASYNC_STATE)pAsyncState);
}



static
VOID
NfsConnectionRundown_inlock(
    PLWIO_NFS_CONNECTION pConnection
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pConnection->pSessionCollection)
    {
        switch (pConnection->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = LwRtlRBTreeTraverse(
                                pConnection->pSessionCollection,
                                LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                                &NfsConnectionRundownSessionRbTreeVisit,
                                NULL);
                break;

            case SMB_PROTOCOL_VERSION_2:

                ntStatus = LwRtlRBTreeTraverse(
                                pConnection->pSessionCollection,
                                LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                                &NfsConnection2RundownSessionRbTreeVisit,
                                NULL);
                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pConnection->pAsyncStateCollection)
    {
        ntStatus = LwRtlRBTreeTraverse(
                        pConnection->pAsyncStateCollection,
                        LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                        &NfsConnection2RundownAsyncStatesRbTreeVisit,
                        NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return;

error:

    LWIO_LOG_ERROR("Connection run down failed [status: %s = 0x%08X (%d)]",
                   LwNtStatusToName(ntStatus),
                   ntStatus,
                   ntStatus);

    goto cleanup;
}

static
NTSTATUS
NfsConnectionRundownSessionRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_NFS_SESSION pSession = (PLWIO_NFS_SESSION)pData;

    if (pSession)
    {
        NfsSessionRundown(pSession);
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NfsConnection2RundownSessionRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_NFS_SESSION_2 pSession = (PLWIO_NFS_SESSION_2)pData;

    if (pSession)
    {
        NfsSession2Rundown(pSession);
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NfsConnection2RundownAsyncStatesRbTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_ASYNC_STATE pAsyncState = (PLWIO_ASYNC_STATE)pData;

    if (pAsyncState)
    {
        NfsAsyncStateCancel(pAsyncState);
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}


