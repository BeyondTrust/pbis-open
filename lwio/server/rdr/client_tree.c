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

#include "rdr.h"

static
NTSTATUS
SMBSrvClientTreeCreate(
    IN OUT PSMB_SESSION* ppSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    );

static
NTSTATUS
RdrAcquireNegotiatedSocket(
    PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    );

static
NTSTATUS
RdrAcquireEstablishedSession(
    IN OUT PSMB_SOCKET* ppSocket,
    PIO_CREDS pCreds,
    uid_t uid,
    OUT PSMB_SESSION* ppSession
    );

static
NTSTATUS
RdrAcquireConnectedTree(
    IN OUT PSMB_SESSION* ppSession,
    IN PCSTR pszSharename,
    OUT PSMB_TREE* ppTree
    );

NTSTATUS
SMBSrvClientTreeOpen(
    PCWSTR pwszHostname,
    PCSTR pszSharename,
    PIO_CREDS pCreds,
    uid_t uid,
    PSMB_TREE* ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SOCKET pSocket = NULL;
    PSMB_SESSION pSession = NULL;
    PSMB_TREE pTree = NULL;

    ntStatus = RdrAcquireNegotiatedSocket(
        pwszHostname,
        &pSocket);
    BAIL_ON_NT_STATUS(ntStatus);
    
    ntStatus = RdrAcquireEstablishedSession(
        &pSocket,
        pCreds,
        uid,
        &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RdrAcquireConnectedTree(
        &pSession,
        pszSharename,
        &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    if (pSession)
    {
        SMBSrvClientSessionRelease(pSession);
    }

    if (pSocket)
    {
        SMBSocketRelease(pSocket);
    }

    goto cleanup;
}

static
NTSTATUS
RdrAcquireNegotiatedSocket(
    IN PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInSocketLock = FALSE;
    
    ntStatus = SMBSrvClientSocketCreate(
        pwszHostname,
        &pSocket);
    BAIL_ON_NT_STATUS(ntStatus);
    
    LWIO_LOCK_MUTEX(bInSocketLock, &pSocket->mutex);
    
    if (pSocket->state == RDR_SOCKET_STATE_NOT_READY)
    {
        /* We're the first thread to use this socket.
           Go through the connect/negotiate procedure */
        pSocket->state = RDR_SOCKET_STATE_CONNECTING;
        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);
        
        ntStatus = SMBSocketConnect(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
        
        ntStatus = Negotiate(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {     
        ntStatus = SMBSocketWaitReady(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);
    }

    *ppSocket = pSocket;

cleanup:

    return ntStatus;
    
error:

    *ppSocket = NULL;

    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    if (pSocket)
    {
        SMBSocketRelease(pSocket);
    }

    goto cleanup;
}

static
NTSTATUS
RdrAcquireEstablishedSession(
    PSMB_SOCKET* ppSocket,
    PIO_CREDS pCreds,
    uid_t uid,
    OUT PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInSessionLock = FALSE;
    BOOLEAN bInSocketLock = FALSE;
    BOOLEAN bSessionSetupInProgress = FALSE;

    ntStatus = SMBSrvClientSessionCreate(
        ppSocket,
        pCreds,
        uid,
        &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInSessionLock, &pSession->mutex);

    if (pSession->state == RDR_SESSION_STATE_NOT_READY)
    {
        /* Begin initializing session */
        pSession->state = RDR_SESSION_STATE_INITIALIZING;
        LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

        /* Exclude other session setups on this socket */
        LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
        ntStatus = SMBSocketWaitSessionSetup(pSession->pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
        pSession->pSocket->bSessionSetupInProgress = bSessionSetupInProgress = TRUE;
        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);

        ntStatus = SessionSetup(
                    pSession->pSocket,
                    pCreds,
                    &pSession->uid,
                    &pSession->pSessionKey,
                    &pSession->dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pSession->pSocket->pSessionKey && pSession->pSessionKey)
        {
            ntStatus = LwIoAllocateMemory(
                pSession->dwSessionKeyLength,
                (PVOID*)&pSession->pSocket->pSessionKey);
            BAIL_ON_NT_STATUS(ntStatus);
            
            memcpy(pSession->pSocket->pSessionKey, pSession->pSessionKey, pSession->dwSessionKeyLength);
            
            pSession->pSocket->dwSessionKeyLength = pSession->dwSessionKeyLength;
        }

        ntStatus = SMBSrvClientSocketAddSessionByUID(
            pSession->pSocket,
            pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Wake up anyone waiting for session to be ready */
        LWIO_LOCK_MUTEX(bInSessionLock, &pSession->mutex);
        pSession->state = RDR_SESSION_STATE_READY;
        pthread_cond_broadcast(&pSession->event);
        LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

        /* Wake up anyone waiting for session setup exclusion */
        LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
        pSession->pSocket->bSessionSetupInProgress = bSessionSetupInProgress = FALSE;
        pthread_cond_broadcast(&pSession->pSocket->event);
        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
    }
    else
    {
        /* Wait for session to be ready */
        ntStatus = SMBSessionWaitReady(pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);
    }
    
    *ppSession = pSession;

cleanup:

    return ntStatus;
    
error:

    *ppSession = NULL;

    if (bSessionSetupInProgress)
    {
        LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
        pSession->pSocket->bSessionSetupInProgress = FALSE;
        pthread_cond_broadcast(&pSession->pSocket->event);
    }

    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
    LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

    if (pSession)
    {
        SMBSessionRelease(pSession);
    }

    goto cleanup;
}

static
NTSTATUS
RdrAcquireConnectedTree(
    IN OUT PSMB_SESSION* ppSession,
    IN PCSTR pszSharename,
    OUT PSMB_TREE* ppTree
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_TREE pTree = NULL;
    BOOLEAN bInTreeLock = FALSE;
    BOOLEAN bInSessionLock = FALSE;
    PWSTR pwszPath = NULL;
    BOOLEAN bTreeConnectInProgress = TRUE;

    ntStatus = SMBSrvClientTreeCreate(
        ppSession,
        pszSharename,
        &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInTreeLock, &pTree->mutex);

    if (pTree->state == RDR_TREE_STATE_NOT_READY)
    {
        pTree->state = RDR_TREE_STATE_INITIALIZING;
        LWIO_UNLOCK_MUTEX(bInTreeLock, &pTree->mutex);

        /* Exclude other tree connects in this session */
        LWIO_LOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
        ntStatus = SMBSessionWaitTreeConnect(pTree->pSession);
        BAIL_ON_NT_STATUS(ntStatus);
        pTree->pSession->bTreeConnectInProgress = bTreeConnectInProgress = TRUE;
        LWIO_UNLOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);

        ntStatus = SMBMbsToWc16s(pTree->pszPath, &pwszPath);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = TreeConnect(pTree->pSession, pwszPath, &pTree->tid);
        BAIL_ON_NT_STATUS(ntStatus);
        
        ntStatus = SMBSrvClientSessionAddTreeById(pTree->pSession, pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_LOCK_MUTEX(bInTreeLock, &pTree->mutex);
        pTree->state = RDR_TREE_STATE_READY;
        pthread_cond_broadcast(&pTree->event);
        LWIO_UNLOCK_MUTEX(bInTreeLock, &pTree->mutex);

        /* Wake up anyone waiting for tree connect exclusion */
        LWIO_LOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
        pTree->pSession->bTreeConnectInProgress = bTreeConnectInProgress = FALSE;
        pthread_cond_broadcast(&pTree->pSession->event);
        LWIO_UNLOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
    }
    else
    {
        ntStatus = SMBTreeWaitReady(pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bInTreeLock, &pTree->mutex);
    }

    *ppTree = pTree;

cleanup:

    RTL_FREE(&pwszPath);

    return ntStatus;
    
error:

    *ppTree = NULL;

    if (bTreeConnectInProgress)
    {
        LWIO_LOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
        pTree->pSession->bTreeConnectInProgress = bTreeConnectInProgress = FALSE;
        pthread_cond_broadcast(&pTree->pSession->event);
    }

    LWIO_UNLOCK_MUTEX(bInTreeLock, &pTree->mutex);
    LWIO_UNLOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    goto cleanup;
}

static
NTSTATUS
SMBSrvClientTreeCreate(
    IN OUT PSMB_SESSION* ppSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    )
{
    NTSTATUS  ntStatus = 0;
    PSMB_TREE pTree = NULL;
    BOOLEAN   bInLock = FALSE;
    PSMB_SESSION pSession = *ppSession;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    ntStatus = SMBHashGetValue(
                pSession->pTreeHashByPath,
                pszPath,
                (PVOID *) &pTree);

    if (!ntStatus)
    {
        pTree->refCount++;
        SMBSessionRelease(pSession);
        *ppSession = NULL;
    }
    else
    {
        ntStatus = SMBTreeCreate(&pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->pSession = pSession;

        ntStatus = SMBStrndup(
            pszPath,
            strlen(pszPath) + 1,
            &pTree->pszPath);
        BAIL_ON_NT_STATUS(ntStatus);
        
        ntStatus = SMBHashSetValue(
            pSession->pTreeHashByPath,
            pTree->pszPath,
            pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->bParentLink = TRUE;

        *ppSession = NULL;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    *ppTree = NULL;

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    goto cleanup;
}

NTSTATUS
SMBSrvClientTreeAddResponse(
    PSMB_TREE     pTree,
    PSMB_RESPONSE pResponse
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);

    /* @todo: if we allocate the MID outside of this function, we need to
       check for a conflict here */
    ntStatus = SMBHashSetValue(
                    pTree->pResponseHash,
                    &pResponse->mid,
                    pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pResponse->pTree)
    {
        SMBTreeRelease(pResponse->pTree);
    }
    SMBTreeAddReference(pTree);

    pResponse->pTree = pTree;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
}

/* Must be called with the tree mutex held */
NTSTATUS
SMBSrvClientTreeIsStale_inlock(
    PSMB_TREE pTree,
    PBOOLEAN  pbIsStale
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bIsStale = FALSE;
    SMB_HASH_ITERATOR iterator;

    if (pTree->refCount > 2)
    {
        goto done;
    }

    ntStatus = SMBHashGetIterator(
                    pTree->pResponseHash,
                    &iterator);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SMBHashNext(&iterator))
    {
        goto done;
    }

    /* @todo: find a tick function which won't jump backward */
    /* @todo: make idle time configurable */
    if (difftime(time(NULL), pTree->lastActiveTime) < 60*15)
    {
        goto done;
    }

    bIsStale = TRUE;

done:

    *pbIsStale = bIsStale;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientTreeClose(
    PSMB_TREE pTree
    )
{
    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    return 0;
}
