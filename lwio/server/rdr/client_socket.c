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
_FindOrCreateSocket(
    IN PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    );

NTSTATUS
RdrSocketInit(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    assert(!gRdrRuntime.pSocketHashByName);

    /* @todo: support case and normalization insensitive string comparisons */
    /* Once we have libidn we'll also need the ability do Unicode case and
       normalization insensitive comparisons of strings.  Otherwise, if the
       string isn't an exact match a redundant DNS lookup would occur. Any
       existing connections to the resolved IP will be caught by the IPv4
       hash, but in a multiple A record DNS setup one could end up with
       multiple connections to a multihomed DCs or set of load-balanced DCs.
       The load balancing case should really be handled by SRV records
       parsed by the client.*/
    ntStatus = SMBHashCreate(
                    19,
                    SMBHashCaselessWc16StringCompare,
                    SMBHashCaselessWc16String,
                    NULL,
                    &gRdrRuntime.pSocketHashByName);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

/* @todo: add support for libidn for internationalized UNC hostnames */
/* @todo: handle IPv6 addresses */
/* The socket is returned with a reference */
NTSTATUS
SMBSrvClientSocketCreate(
    IN PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    )
{
    return _FindOrCreateSocket(pwszHostname, ppSocket);
}


/* Must be called with the session mutex held */
NTSTATUS
SMBSrvClientSocketIsStale_inlock(
    PSMB_SOCKET pSocket,
    PBOOLEAN    pbIsStale
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bIsStale = FALSE;
    SMB_HASH_ITERATOR iterator;

    if (pSocket->refCount > 2)
    {
        goto done;
    }

    ntStatus = SMBHashGetIterator(
                    pSocket->pSessionHashByPrincipal,
                    &iterator);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SMBHashNext(&iterator))
    {
        goto done;
    }

    /* @todo: find a tick function which won't jump backward */
    /* @todo: make idle time configurable */
    if (difftime(time(NULL), pSocket->lastActiveTime) < 60*15)
    {
        goto done;
    }

    bIsStale = TRUE;

done:

    *pbIsStale = bIsStale;

error:

    return ntStatus;
}

static
NTSTATUS
_FindOrCreateSocket(
    IN PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;

    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
    
    ntStatus = SMBHashGetValue(
        gRdrRuntime.pSocketHashByName,
        pwszHostname,
        (PVOID *) &pSocket);
    
    if (!ntStatus)
    {
        pSocket->refCount++;
    }
    else
    {
        ntStatus = SMBSocketCreate(
            pwszHostname,
            gRdrRuntime.config.bSignMessagesIfSupported,
            &pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
        
        ntStatus = SMBHashSetValue(
            gRdrRuntime.pSocketHashByName,
            pSocket->pwszHostname,
            pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        pSocket->bParentLink = TRUE;
    }
    
    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    *ppSocket = pSocket;
    
cleanup:
    
    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
    
    return ntStatus;
    
error:

    *ppSocket = NULL;

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketAddSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashSetValue(
        pSocket->pSessionHashByPrincipal,
        &pSession->key,
        pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->bParentLink = TRUE;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketRemoveSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashRemoveKey(
                    pSocket->pSessionHashByPrincipal,
                    &pSession->key);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketAddSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* No need to check for a race here; the principal hash is always checked
       first */
    ntStatus = SMBHashSetValue(
                    pSocket->pSessionHashByUID,
                    &pSession->uid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->bParentLink = TRUE;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketRemoveSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashRemoveKey(
                    pSocket->pSessionHashByUID,
                    &pSession->uid);
    BAIL_ON_NT_STATUS(ntStatus);

    /* @todo: this need be set only when the hash is empty */
    SMBSocketUpdateLastActiveTime(pSocket);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
RdrSocketShutdown(
    VOID
    )
{
    SMBHashSafeFree(&gRdrRuntime.pSocketHashByName);

    return 0;
}
