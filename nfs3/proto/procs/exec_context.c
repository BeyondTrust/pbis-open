/*
 * Copyright Likewise Software    2004-2010
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
 *        exec_context.c
 *
 * Abstract:
 *
 *        NFS3
 *
 *        Exec context implementation routines.
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */

#include "includes.h"


static
VOID
Nfs3FreeExecContext(
   PNFS3_EXEC_CONTEXT pContext
   );


VOID
Nfs3ReleaseExecContextHandle(
   HANDLE hExecContext
   )
{
    Nfs3ReleaseExecContext((PNFS3_EXEC_CONTEXT)hExecContext);
}

VOID
Nfs3ReleaseExecContext(
   PNFS3_EXEC_CONTEXT pContext
   )
{
    if (InterlockedDecrement(&pContext->refCount) == 0)
    {
        Nfs3FreeExecContext(pContext);
    }
}

BOOLEAN
Nfs3IsValidExecContext(
   PNFS3_EXEC_CONTEXT pExecContext
   )
{
    // TODO
    return (pExecContext != NULL/* &&
            pExecContext->pConnection &&
            pExecContext->pSmbRequest*/);
}

static
VOID
Nfs3FreeExecContext(
   IN PNFS3_EXEC_CONTEXT pContext
   )
{
    // TODO

    /*
    if (pContext->pSmbRequest)
    {
        SMBPacketRelease(
            pContext->pConnection->hPacketAllocator,
            pContext->pSmbRequest);
    }

    if (pContext->pSmbResponse)
    {
        SMBPacketRelease(
            pContext->pConnection->hPacketAllocator,
            pContext->pSmbResponse);
    }
    
    if (pContext->pConnection)
    {
        SrvConnectionRelease(pContext->pConnection);
    }
    
    if (pContext->pExecMutex)
    {
        pthread_mutex_destroy(&pContext->execMutex);
    }
    */

    Nfs3FreeMemory(pContext);
}

NTSTATUS
Nfs3ProtocolExecute(
    PNFS3_EXEC_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    NFS3_LOCK_MUTEX(bInLock, &pContext->execMutex);

    // Just to make it to compile meanwhile...
    BAIL_ON_NT_STATUS(ntStatus);

/*
    SrvMpxTrackerSetExecutingExecContext(pContext);

    ntStatus = SrvProtocolAddContext(pContext, FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    if ((pContext->pSmbRequest->pSMBHeader->command == COM_NEGOTIATE) &&
        (SrvConnectionGetState(pContext->pConnection) !=
                                        LWIO_SRV_CONN_STATE_INITIAL))
    {
        SrvConnectionSetInvalid(pContext->pConnection);

        ntStatus = STATUS_CONNECTION_RESET;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (pContext->pSmbRequest->protocolVer)
    {
        case SMB_PROTOCOL_VERSION_1:

            ntStatus = SrvProtocolExecute_SMB_V1_Filter(pContext);

            break;

        case SMB_PROTOCOL_VERSION_2:

            ntStatus = SrvProtocolExecute_SMB_V2(pContext);

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    // Remove mid before sending a response since the client can immediately
    // re-use it. Note that, even if we are not sending a response for
    // some reason, it is fine to remove the mid since we are done with the
    // request.  Also note that in a ZCT read file case, we already sent
    // the response and will not send it here, but we already removed
    // the mid from the request.

    SrvMpxTrackerRemoveExecContext(pContext);

    // Cleanup any protocol state before sending a response.
    if (pContext->pProtocolContext)
    {
        pContext->pfnFreeContext(pContext->pProtocolContext);
        pContext->pProtocolContext = NULL;
    }

    if (pContext->pSmbResponse && pContext->pSmbResponse->pNetBIOSHeader->len)
    {
        ntStatus = SrvProtocolTransportSendResponse(
                        pContext->pConnection,
                        pContext->pSmbResponse,
                        pContext->pStatInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }
*/
cleanup:

    NFS3_UNLOCK_MUTEX(bInLock, &pContext->execMutex);

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            ntStatus = STATUS_SUCCESS;

            break;

        default:

            break;
    }

    goto cleanup;
}
