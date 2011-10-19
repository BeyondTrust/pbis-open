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

/* @todo: support internationalized principals */
NTSTATUS
SMBSrvClientSessionCreate(
    IN OUT PSMB_SOCKET* ppSocket,
    IN PIO_CREDS pCreds,
    uid_t uid,
    OUT PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;
    PSMB_SOCKET pSocket = *ppSocket;
    struct _RDR_SESSION_KEY key = {0};

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    switch (pCreds->type)
    {
    case IO_CREDS_TYPE_KRB5_TGT:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &key.pszPrincipal,
            pCreds->payload.krb5Tgt.pwszClientPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case IO_CREDS_TYPE_PLAIN:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &key.pszPrincipal,
            pCreds->payload.krb5Tgt.pwszClientPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    default:
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    key.uid = uid;
    
    ntStatus = SMBHashGetValue(
        pSocket->pSessionHashByPrincipal,
        &key,
        OUT_PPVOID(&pSession));
    
    if (!ntStatus)
    {
        pSession->refCount++;
        SMBSocketRelease(pSocket);
        *ppSocket = NULL;
    }
    else
    {
        ntStatus = SMBSessionCreate(&pSession);
        BAIL_ON_NT_STATUS(ntStatus);
        
        pSession->pSocket = pSocket;

        ntStatus = SMBStrndup(
            key.pszPrincipal,
            strlen(key.pszPrincipal) + 1,
            &pSession->key.pszPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        
        pSession->key.uid = key.uid;
        
        ntStatus = SMBHashSetValue(
            pSocket->pSessionHashByPrincipal,
            &pSession->key,
            pSession);
        BAIL_ON_NT_STATUS(ntStatus);
        
        pSession->bParentLink = TRUE;
        
        *ppSocket = NULL;
    }
    
    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
    
    *ppSession = pSession;
    
cleanup:

    LWIO_SAFE_FREE_STRING(key.pszPrincipal);
    
    return ntStatus;
    
error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
    
    if (pSession)
    {
        SMBSessionRelease(pSession);
    }

    *ppSession = NULL;

    goto cleanup;
}

/* Must be called with the session mutex held */
NTSTATUS
SMBSrvClientSessionIsStale_inlock(
    PSMB_SESSION pSession,
    PBOOLEAN     pbIsStale
    )
{
    NTSTATUS ntStatus = 0;
    SMB_HASH_ITERATOR iterator;
    BOOLEAN bIsStale = FALSE;

    if (pSession->refCount > 2)
    {
        goto done;
    }

    ntStatus = SMBHashGetIterator(
                    pSession->pTreeHashByPath,
                    &iterator);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SMBHashNext(&iterator))
    {
        goto done;
    }

    /* @todo: find a tick function which won't jump backward */
    /* @todo: make idle time configurable */
    if (difftime(time(NULL), pSession->lastActiveTime) < 60*15)
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
SMBSrvClientSessionAddTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    /* No need to check for a race here; the path hash is always checked
       first */
    ntStatus = SMBHashSetValue(
                    pSession->pTreeHashByTID,
                    &pTree->tid,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->bParentLink = TRUE;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSessionRemoveTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    ntStatus = SMBHashRemoveKey(
                    pSession->pTreeHashByTID,
                    &pTree->tid);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBSessionUpdateLastActiveTime(pSession);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSessionAddTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    /* @todo: check for race */
    ntStatus = SMBHashSetValue(
                    pSession->pTreeHashByPath,
                    pTree->pszPath,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->bParentLink = TRUE;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSessionRemoveTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    ntStatus = SMBHashRemoveKey(pSession->pTreeHashByPath, pTree->pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSessionRelease(
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;

    /** @todo: keep unused sockets around for a little while when daemonized.
     * To avoid writing frequently to shared cache lines, perhaps set a bit
     * when the hash transitions to non-empty, then periodically sweep for
     * empty hashes.  If a hash is empty after x number of timed sweeps, tear
     * down the parent.
     */

    /* @todo: verify that the tree hash is empty */
    ntStatus = Logoff(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMBSessionRelease(pSession);

    return ntStatus;

error:

    goto cleanup;
}
