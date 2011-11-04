/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        async.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"
#include "lwthreads.h"

static
VOID
ItpAsyncCompleteWorkCallback(
    IN PIOTEST_WORK_ITEM pWorkItem,
    IN PVOID pContext
    )
{
    PIT_IRP_CONTEXT pIrpContext = (PIT_IRP_CONTEXT) pContext;

    if (pIrpContext->IsCancelled)
    {
        pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;
        IoIrpComplete(pIrpContext->pIrp);
        ItDestroyIrpContext(&pIrpContext);
    }
    else
    {
        pIrpContext->ContinueCallback(pIrpContext);
    }
}

static
VOID
ItpCancelAsync(
    IN PIRP pIrp,
    IN PVOID CallbackContext
    )
{
    PIT_IRP_CONTEXT pIrpContext = (PIT_IRP_CONTEXT) CallbackContext;
    PIT_DRIVER_STATE pState = NULL;
    BOOLEAN wasInQueue = FALSE;

    pState = ItGetDriverState(pIrp);

    wasInQueue = ItRemoveWorkQueue(pState->pWorkQueue, pIrpContext->pWorkItem);

    if (wasInQueue)
    {
        NTSTATUS status = STATUS_SUCCESS;

        pIrpContext->IsCancelled = TRUE;

        status = ItAddWorkQueue(
            pState->pWorkQueue,
            pIrpContext->pWorkItem,
            pIrpContext,
            0,
            ItpAsyncCompleteWorkCallback);
        LWIO_ASSERT(!status);
    }
}

NTSTATUS
ItDispatchAsync(
    IN PIRP pIrp,
    IN ULONG WaitSeconds,
    IN IT_CONTINUE_CALLBACK ContinueCallback,
    IN PVOID ContinueContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIT_IRP_CONTEXT pIrpContext = NULL;
    PIT_DRIVER_STATE pState = NULL;

    status = ItCreateIrpContext(&pIrpContext, pIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrpContext->ContinueCallback = ContinueCallback;
    pIrpContext->ContinueContext = ContinueContext;

    pState = ItGetDriverState(pIrp);

    status = ItAddWorkQueue(
                    pState->pWorkQueue,
                    pIrpContext->pWorkItem,
                    pIrpContext,
                    WaitSeconds,
                    ItpAsyncCompleteWorkCallback);
    LWIO_ASSERT(!status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    IoIrpMarkPending(pIrp, ItpCancelAsync, pIrpContext);
    status = STATUS_PENDING;

cleanup:
    if (!NT_SUCCESS(status))
    {
        ItDestroyIrpContext(&pIrpContext);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

VOID
ItAsyncCompleteSetEvent(
    IN PVOID pCallbackContext
    )
{
    IO_LOG_ENTER("");
    LwRtlSetEvent((PLW_RTL_EVENT) pCallbackContext);
    IO_LOG_LEAVE("");
}

VOID
ItSimpleSuccessContinueCallback(
    IN PIT_IRP_CONTEXT pIrpContext
    )
{
    pIrpContext->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;
    IoIrpComplete(pIrpContext->pIrp);
    ItDestroyIrpContext(&pIrpContext);
}

