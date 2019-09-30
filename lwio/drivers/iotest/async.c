/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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

