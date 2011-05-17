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
 * Module Name:
 *
 *        session2.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 session management
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

static
VOID
RdrSession2Unlink(
    PRDR_SESSION2 pSession
    );

static
VOID
RdrSession2Free(
    PRDR_SESSION2 pSession
    );

static
NTSTATUS
RdrTransceiveLogoff2(
    PRDR_OP_CONTEXT pContext,
    PRDR_SESSION2 pSession
    );

static
int
RdrSession2HashTreeCompareById(
    PCVOID vp1,
    PCVOID vp2
    )
{
    ULONG tid1 = *((ULONG *) vp1);
    ULONG tid2 = *((ULONG *) vp2);

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
RdrSession2HashTreeById(
    PCVOID vp
    )
{
    return (size_t) *(PULONG) vp;
}

NTSTATUS
RdrSession2Create(
    PRDR_SESSION2* ppSession
    )
{
    NTSTATUS status = 0;
    RDR_SESSION2 *pSession = NULL;
    BOOLEAN bDestroyMutex = FALSE;

    status = LwIoAllocateMemory(
                sizeof(RDR_SESSION2),
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
                &RdrSession2HashTreeCompareById,
                &RdrSession2HashTreeById,
                NULL,
                &pSession->pTreeHashById);
    BAIL_ON_NT_STATUS(status);

    /* Pre-allocate resources to send a logoff */
    status = RdrCreateContext(NULL, &pSession->pLogoffContext);
    BAIL_ON_NT_STATUS(status);

    status = RdrAllocateContextPacket(pSession->pLogoffContext, RDR_SMB2_STUB_SIZE);
    BAIL_ON_NT_STATUS(status);

    *ppSession = pSession;

cleanup:

    return status;

error:

    if (pSession)
    {
        SMBHashSafeFree(&pSession->pTreeHashById);

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
BOOLEAN
RdrLogoff2Complete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PRDR_SESSION2 pSession = pContext->State.TreeConnect.pSession2;

    RdrFreePacket(pPacket);
    RdrSession2Free(pSession);

    /* We don't explicitly free pContext because RdrSession2Free() does it */
    return FALSE;
}

static
VOID
RdrSession2Timeout(
    PLW_TASK pTask,
    LW_PVOID _pSession,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SESSION2 pSession = _pSession;
    BOOLEAN bLocked = FALSE;
    PRDR_OP_CONTEXT pContext = NULL;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
    }

    if ((WakeMask & LW_TASK_EVENT_TIME) ||
        ((WakeMask & LW_TASK_EVENT_EXPLICIT) && RdrIsShutdownSet()))
    {
        LWIO_LOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

        if (pSession->refCount == 0)
        {
            RdrSession2Unlink(pSession);

            pContext = pSession->pLogoffContext;
            pContext->Continue = RdrLogoff2Complete;
            pContext->State.TreeConnect.pSession2 = pSession;

            LWIO_UNLOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

            status = RdrTransceiveLogoff2(pContext, pSession);
            if (status != STATUS_PENDING)
            {
                /* Give up and free the session now */
                RdrSession2Free(pSession);
            }

            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = gRdrRuntime.config.usIdleTimeout * 1000000000ll;
        }
    }

    LWIO_UNLOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

    return;
}

static
VOID
RdrSession2Unlink(
    PRDR_SESSION2 pSession
    )
{
    if (pSession->bParentLink)
    {
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByPrincipal,
            &pSession->key);
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByUID,
            &pSession->ullSessionId);
        pSession->bParentLink = FALSE;
    }
}

VOID
RdrSession2Release(
    PRDR_SESSION2 pSession
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
            RdrSession2Unlink(pSession);
            LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
            RdrSession2Free(pSession);
        }
        else
        {
            LWIO_LOG_VERBOSE("Session %p is eligible for reaping", pSession);

            LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);

            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pSession->pTimeout,
                    gRdrRuntime.pSessionTimerGroup,
                    RdrSession2Timeout,
                    pSession) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pSession->pTimeout);
            }
            else
            {
                LWIO_LOG_ERROR("Could not create timer for session %p; logging off immediately");
                RdrSession2Timeout(NULL, pSession, LW_TASK_EVENT_TIME, &dummy, &llDummy);
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
RdrSession2Free(
    PRDR_SESSION2 pSession
    )
{
    assert(!pSession->refCount);

    SMBHashSafeFree(&pSession->pTreeHashByPath);
    SMBHashSafeFree(&pSession->pTreeHashById);

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
RdrSession2Invalidate(
    PRDR_SESSION2 pSession,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSocketLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    pSession->state = RDR_SESSION_STATE_ERROR;
    pSession->error = ntStatus;

    LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
    RdrSession2Unlink(pSession);
    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);

    RdrNotifyContextList(
        &pSession->StateWaiters,
        bInLock,
        &pSession->mutex,
        ntStatus,
        NULL);

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);
}

VOID
RdrSession2Revive(
    PRDR_SESSION2 pSession
    )
{
    if (pSession->pTimeout)
    {
        LwRtlCancelTask(pSession->pTimeout);
        LwRtlReleaseTask(&pSession->pTimeout);
    }
}

NTSTATUS
RdrSession2FindOrCreate(
    IN OUT PRDR_SOCKET* ppSocket,
    IN PIO_CREDS pCreds,
    uid_t uid,
    OUT PRDR_SESSION2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_SESSION2 pSession = NULL;
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

    /* This is the *UNIX* uid of the calling program */
    key.uid = uid;

    ntStatus = SMBHashGetValue(
        pSocket->pSessionHashByPrincipal,
        &key,
        OUT_PPVOID(&pSession));

    if (!ntStatus)
    {
        pSession->refCount++;
        RdrSession2Revive(pSession);
        RdrSocketRelease(pSocket);
        *ppSocket = NULL;
    }
    else
    {
        ntStatus = RdrSession2Create(&pSession);
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
        RdrSession2Release(pSession);
    }

    *ppSession = NULL;

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveLogoff2(
    PRDR_OP_CONTEXT pContext,
    PRDR_SESSION2 pSession
    )
{
     NTSTATUS status = 0;
     PRDR_SOCKET pSocket = pSession->pSocket;
     PBYTE pCursor = NULL;
     ULONG ulRemaining = 0;

     status = RdrSmb2BeginPacket(&pContext->Packet);
     BAIL_ON_NT_STATUS(status);

     status = RdrSmb2EncodeHeader(
         &pContext->Packet,
         COM2_LOGOFF,
         0, /* flags */
         gRdrRuntime.SysPid,
         0, /* tid */
         pSession->ullSessionId,
         &pCursor,
         &ulRemaining);
     BAIL_ON_NT_STATUS(status);

     status = RdrSmb2EncodeStubRequest(
         &pContext->Packet,
         &pCursor,
         &ulRemaining);
     BAIL_ON_NT_STATUS(status);

     status = RdrSmb2FinishCommand(&pContext->Packet, &pCursor, &ulRemaining);
     BAIL_ON_NT_STATUS(status);

     status = RdrSocketTransceive(pSocket, pContext);
     BAIL_ON_NT_STATUS(status);

 cleanup:

     return status;

 error:

     goto cleanup;
}
