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
 *        create.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"
#include "lwthreads.h"


static
VOID
ItCreateInternal(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    UNICODE_STRING path = pIrp->Args.Create.FileName.Name;
    PIT_CCB pCcb = NULL;

    status = ItpCreateCcb(&pCcb, &path);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoFileSetContext(pIrp->FileHandle, pCcb);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCcb = NULL;

cleanup:
    ItpDestroyCcb(&pCcb);

    pIrp->IoStatusBlock.Status = status;
}

static
VOID
ItpCreateContinueCallback(
    IN PIT_IRP_CONTEXT pIrpContext
    )
{
    ItCreateInternal(pIrpContext->pIrp);
    IoIrpComplete(pIrpContext->pIrp);
    ItDestroyIrpContext(&pIrpContext);
}

NTSTATUS
ItDispatchCreate(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    int EE = 0;
    UNICODE_STRING path = pIrp->Args.Create.FileName.Name;
    UNICODE_STRING allowPath = { 0 };
    UNICODE_STRING asyncPath = { 0 };
    UNICODE_STRING testSyncPath = { 0 };
    UNICODE_STRING testAsyncPath = { 0 };

    status = RtlUnicodeStringAllocateFromCString(&allowPath, IOTEST_INTERNAL_PATH_ALLOW);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlUnicodeStringAllocateFromCString(&asyncPath, IOTEST_INTERNAL_PATH_ASYNC);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlUnicodeStringAllocateFromCString(&testSyncPath, IOTEST_INTERNAL_PATH_TEST_SYNC);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlUnicodeStringAllocateFromCString(&testAsyncPath, IOTEST_INTERNAL_PATH_TEST_ASYNC);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Only succeed for certain paths.
    if (path.Length == 0)
    {
        // Ok
    }
    else if (RtlUnicodeStringIsEqual(&path, &allowPath, FALSE))
    {
        // Ok
    }
    else if (RtlUnicodeStringIsEqual(&path, &asyncPath, FALSE))
    {
        status = ItDispatchAsync(pIrp, 5, ItpCreateContinueCallback, NULL);
        GOTO_CLEANUP_EE(EE);
    }
    else if (RtlUnicodeStringIsEqual(&path, &testSyncPath, FALSE))
    {
        // do test first
        status = ItTestSyncCreate();
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else if (RtlUnicodeStringIsEqual(&path, &testAsyncPath, FALSE))
    {
        // do test first
        status = ItTestAsyncCreate(TRUE, TRUE);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        GOTO_CLEANUP_EE(EE);
    }

    ItCreateInternal(pIrp);
    status = pIrp->IoStatusBlock.Status;

cleanup:
    RtlUnicodeStringFree(&allowPath);
    RtlUnicodeStringFree(&asyncPath);
    RtlUnicodeStringFree(&testSyncPath);
    RtlUnicodeStringFree(&testAsyncPath);

    pIrp->IoStatusBlock.Status = status;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
ItDispatchClose(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIT_CCB pCcb = NULL;

    status = ItpGetCcb(&pCcb, pIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    ItpDestroyCcb(&pCcb);

cleanup:    
    pIrp->IoStatusBlock.Status = status;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
