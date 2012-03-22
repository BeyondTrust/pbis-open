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
 *        session.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Session Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 */

#include "rdr.h"

static
VOID
RdrSessionFree(
    PRDR_SESSION pSession
    );

static
int
RdrSessionHashTreeCompareByTID(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
RdrSessionHashTreeByTID(
    PCVOID vp
    );

static
NTSTATUS
RdrTransceiveLogoff(
    PRDR_OP_CONTEXT pContext,
    PRDR_SESSION pSession
    );

NTSTATUS
RdrSessionCreate(
    PRDR_SESSION* ppSession
    )
{
    NTSTATUS status = 0;
    RDR_SESSION *pSession = NULL;
    BOOLEAN bDestroyMutex = FALSE;

    status = LwIoAllocateMemory(
                sizeof(RDR_SESSION),
                (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(status);

    LwListInit(&pSession->StateWaiters);

    pthread_mutex_init(&pSession->mutex, NULL);
    bDestroyMutex = TRUE;

    pSession->refCount = 1;

    status = SMBHashCreate(
                19,
                SMBHashCaselessWc16StringCompare,
                SMBHashCaselessWc16String,
                NULL,
                &pSession->pTreeHashByPath);
    BAIL_ON_NT_STATUS(status);

    status = SMBHashCreate(
                19,
                &RdrSessionHashTreeCompareByTID,
                &RdrSessionHashTreeByTID,
                NULL,
                &pSession->pTreeHashByTID);
    BAIL_ON_NT_STATUS(status);

    /* Pre-allocate resources to send a logoff */
    status = RdrCreateContext(NULL, &pSession->pLogoffContext);
    BAIL_ON_NT_STATUS(status);

    status = RdrAllocateContextPacket(pSession->pLogoffContext, 64 * 1024);
    BAIL_ON_NT_STATUS(status);

    *ppSession = pSession;

cleanup:

    return status;

error:

    if (pSession)
    {
        SMBHashSafeFree(&pSession->pTreeHashByTID);

        SMBHashSafeFree(&pSession->pTreeHashByPath);

        if (bDestroyMutex)
        {
            pthread_mutex_destroy(&pSession->mutex);
        }

        LwIoFreeMemory(pSession);
    }

    *ppSession = NULL;

    goto cleanup;
}

static
int
RdrSessionHashTreeCompareByTID(
    PCVOID vp1,
    PCVOID vp2
    )
{
    uint16_t tid1 = *((uint16_t *) vp1);
    uint16_t tid2 = *((uint16_t *) vp2);

    if (tid1 == tid2)
    {
        return 0;
    }
    else if (tid1 > tid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
RdrSessionHashTreeByTID(
    PCVOID vp
    )
{
    return *((uint16_t *) vp);
}

static
VOID
RdrSessionUnlink(
    PRDR_SESSION pSession
    )
{
    if (pSession->bParentLink)
    {
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByPrincipal,
            &pSession->key);
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByUID,
            &pSession->uid);
        pSession->bParentLink = FALSE;
    }
}

VOID
RdrSessionRevive(
    PRDR_SESSION pSession
    )
{
    if (pSession->pTimeout)
    {
        LwRtlCancelTask(pSession->pTimeout);
        LwRtlReleaseTask(&pSession->pTimeout);
    }
}

static
BOOLEAN
RdrLogoffComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PRDR_SESSION pSession = pContext->State.TreeConnect.pSession;

    RdrFreePacket(pPacket);
    RdrSessionFree(pSession);

    /* We don't explicitly free pContext because RdrSessionFree() does it */
    return FALSE;
}

static
VOID
RdrSessionTimeout(
    PLW_TASK pTask,
    LW_PVOID _pSession,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SESSION pSession = _pSession;
    BOOLEAN bLocked = FALSE;
    PRDR_OP_CONTEXT pContext = NULL;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
    }

    if ((WakeMask & LW_TASK_EVENT_TIME) ||
        ((WakeMask & LW_TASK_EVENT_CANCEL) && RdrIsShutdownSet()))
    {
        LWIO_LOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

        if (pSession->refCount == 0)
        {
            RdrSessionUnlink(pSession);
        
            pContext = pSession->pLogoffContext;
            pContext->Continue = RdrLogoffComplete;
            pContext->State.TreeConnect.pSession = pSession;

            LWIO_UNLOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

            status = RdrTransceiveLogoff(pContext, pSession);
            if (status != STATUS_PENDING)
            {
                /* Give up and free the session now */
                RdrSessionFree(pSession);
            }

            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
        }
    }
    else if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }

    LWIO_UNLOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

    return;
}

VOID
RdrSessionRelease(
    PRDR_SESSION pSession
    )
{
    BOOLEAN bInLock = FALSE;
    LW_TASK_EVENT_MASK dummy = 0;    
    LONG64 llDummy = 0;

    LWIO_LOCK_MUTEX(bInLock, &pSession->pSocket->mutex);

    assert(pSession->refCount > 0);

    if (--pSession->refCount == 0)
    {
        if (pSession->state != RDR_SESSION_STATE_READY ||
            pSession->pSocket->state != RDR_SOCKET_STATE_READY)
        {
            RdrSessionUnlink(pSession);
            LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
            RdrSessionFree(pSession);
        }
        else
        {
            LWIO_LOG_VERBOSE("Session %p is eligible for reaping", pSession);

            LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);

            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pSession->pTimeout,
                    gRdrRuntime.pSessionTimerGroup,
                    RdrSessionTimeout,
                    pSession) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pSession->pTimeout);
            }
            else
            {
                LWIO_LOG_ERROR("Could not create timer for session %p; logging off immediately");
                RdrSessionTimeout(NULL, pSession, LW_TASK_EVENT_TIME, &dummy, &llDummy);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
    }
}

static
VOID
RdrSessionFree(
    PRDR_SESSION pSession
    )
{
    assert(!pSession->refCount);

    SMBHashSafeFree(&pSession->pTreeHashByPath);
    SMBHashSafeFree(&pSession->pTreeHashByTID);

    pthread_mutex_destroy(&pSession->mutex);

    LWIO_SAFE_FREE_MEMORY(pSession->pSessionKey);
    LWIO_SAFE_FREE_MEMORY(pSession->key.pszPrincipal);
    LWIO_SAFE_FREE_MEMORY(pSession->key.pVerifier);

    if (pSession->pTimeout)
    {
        LwRtlCancelTask(pSession->pTimeout);
        LwRtlReleaseTask(&pSession->pTimeout);
    }

    if (pSession->pLogoffContext)
    {
        RdrFreeContext(pSession->pLogoffContext);
    }

    if (pSession->pSocket)
    {
        RdrSocketRelease(pSession->pSocket);
    }

    LwIoFreeMemory(pSession);
}

VOID
RdrSessionInvalidate(
    PRDR_SESSION   pSession,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSocketLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    pSession->state = RDR_SESSION_STATE_ERROR;
    pSession->error = ntStatus;

    LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
    RdrSessionUnlink(pSession);
    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);

    RdrNotifyContextList(
        &pSession->StateWaiters,
        bInLock,
        &pSession->mutex,
        ntStatus,
        NULL);

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);
}

NTSTATUS
RdrSessionFindOrCreate(
    IN OUT PRDR_SOCKET* ppSocket,
    IN PIO_CREDS pCreds,
    uid_t uid,
    OUT PRDR_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;
    PRDR_SOCKET pSocket = *ppSocket;
    struct _RDR_SESSION_KEY key = {0};
    PSTR pszUser = NULL;
    PSTR pszDomain = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    switch (pCreds->type)
    {
    case IO_CREDS_TYPE_KRB5_TGT:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &key.pszPrincipal,
            pCreds->payload.krb5Tgt.pwszClientPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        key.VerifierLength = pCreds->payload.krb5Tgt.ulTgtSize;
        ntStatus = RTL_ALLOCATE(&key.pVerifier, BYTE, key.VerifierLength);
        BAIL_ON_NT_STATUS(ntStatus);
        memcpy(key.pVerifier, pCreds->payload.krb5Tgt.pTgtData, key.VerifierLength);
        break;
    case IO_CREDS_TYPE_PLAIN:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &pszUser,
            pCreds->payload.plain.pwszUsername);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwRtlCStringAllocateFromWC16String(
            &pszDomain,
            pCreds->payload.plain.pwszDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwRtlCStringAllocatePrintf(
            &key.pszPrincipal,
            "%s\\%s",
            pszDomain,
            pszUser);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwRtlWC16StringDuplicate(
            (PWSTR*) (PVOID) &key.pVerifier,
            pCreds->payload.plain.pwszPassword);
        BAIL_ON_NT_STATUS(ntStatus);
        key.VerifierLength = LwRtlWC16StringNumChars(pCreds->payload.plain.pwszPassword) * 2;
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
        RdrSessionRevive(pSession);
        RdrSocketRelease(pSocket);
        *ppSocket = NULL;
    }
    else
    {
        ntStatus = RdrSessionCreate(&pSession);
        BAIL_ON_NT_STATUS(ntStatus);
        
        pSession->pSocket = pSocket;
        pSession->key = key;
        memset(&key, 0, sizeof(key));
        
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
    LWIO_SAFE_FREE_MEMORY(key.pVerifier);
    RTL_FREE(&pszUser);
    RTL_FREE(&pszDomain);
    
    return ntStatus;
    
error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
    
    if (pSession)
    {
        RdrSessionRelease(pSession);
    }

    *ppSession = NULL;

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveLogoff(
    PRDR_OP_CONTEXT pContext,
    PRDR_SESSION pSession
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SMBPacketMarshallHeader(
                pContext->Packet.pRawBuffer,
                pContext->Packet.bufferLen,
                COM_LOGOFF_ANDX,
                0,
                0,
                0,
                gRdrRuntime.SysPid,
                pSession->uid,
                0,
                TRUE,
                &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.pSMBHeader->wordCount = 2;

    pContext->Packet.pData = pContext->Packet.pParams;
    pContext->Packet.bufferUsed += sizeof(uint16_t); /* ByteCount */
    memset(pContext->Packet.pData, 0, sizeof(uint16_t));

    // no byte order conversions necessary (due to zeros)

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:
    goto cleanup;
}
