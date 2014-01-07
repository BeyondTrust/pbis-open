/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

#include "iop.h"

typedef ULONG IRP_FLAGS;

#define IRP_FLAG_PENDING                    0x00000001
#define IRP_FLAG_COMPLETE                   0x00000002
#define IRP_FLAG_CANCEL_PENDING             0x00000004
#define IRP_FLAG_CANCELLED                  0x00000008
#define IRP_FLAG_DISPATCHED                 0x00000010

typedef struct _IRP_INTERNAL {
    IRP Irp;
    LONG ReferenceCount;
    IRP_FLAGS Flags;
    LW_LIST_LINKS FileObjectLinks;
    // CancelLinks are used to add to the list of IRPs to cancel
    // on rundown.  However, for allocated but unused IRPs,
    // these links are used to store the IRP in the file object's
    // ZctCompletionIrpList.
    LW_LIST_LINKS CancelLinks;
    struct {
        PIO_IRP_CALLBACK Callback;
        PVOID CallbackContext;
    } Cancel;
    struct {
        BOOLEAN IsAsyncCall;
        union {
            struct {
                PIO_ASYNC_COMPLETE_CALLBACK Callback;
                PVOID CallbackContext;
                PIO_STATUS_BLOCK pIoStatusBlock;
                // Per-operation output parameters
                union {
                    struct {
                        OUT PIO_FILE_HANDLE pFileHandle;
                    } Create;
                    struct {
                        OUT PVOID* pCompletionContext;
                        IN PIRP pCompletionIrp;
                    } PrepareZctReadWrite;
                } OpOut;
            } Async;
            struct {
                PLW_RTL_EVENT Event;
            } Sync;
        };
    } Completion;
} IRP_INTERNAL, *PIRP_INTERNAL;

// TODO -- make inline -- just want type-safe macro.
PIRP_INTERNAL
IopIrpGetInternal(
    IN PIRP pIrp
    )
{
    return (PIRP_INTERNAL) pIrp;
}

PIO_ASYNC_CANCEL_CONTEXT
IopIrpGetAsyncCancelContextFromIrp(
    IN PIRP Irp
    )
{
    return (PIO_ASYNC_CANCEL_CONTEXT) Irp;
}

PIRP
IopIrpGetIrpFromAsyncCancelContext(
    IN PIO_ASYNC_CANCEL_CONTEXT Context
    )
{
    return (PIRP) Context;
}

VOID
IopIrpAcquireCancelLock(
    IN PIRP pIrp
    )
{
    LwRtlLockMutex(&pIrp->FileHandle->pDevice->CancelMutex);
}

VOID
IopIrpReleaseCancelLock(
    IN PIRP pIrp
    )
{
    LwRtlUnlockMutex(&pIrp->FileHandle->pDevice->CancelMutex);
}

NTSTATUS
IopIrpCreateDetached(
    OUT PIRP* ppIrp
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    PIRP_INTERNAL irpInternal = NULL;

    // Note that we allocate enough space for the internal fields.
    status = IO_ALLOCATE(&pIrp, IRP, sizeof(IRP_INTERNAL));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    irpInternal = IopIrpGetInternal(pIrp);
    irpInternal->ReferenceCount = 1;

cleanup:
    if (status)
    {
        IopIrpDereference(&pIrp);
    }

    *ppIrp = pIrp;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IopIrpAttach(
    IN OUT PIRP pIrp,
    IN IRP_TYPE Type,
    IN PIO_FILE_OBJECT pFileObject
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    LWIO_ASSERT(!pIrp->FileHandle);
    LWIO_ASSERT(pIrp->Type == IRP_TYPE_UNINITIALIZED);
    LWIO_ASSERT(Type != IRP_TYPE_UNINITIALIZED);

    IopFileObjectLock(pFileObject);

    // TODO-Add FILE_OBJECT_FLAG_CLOSED
    if ((Type != IRP_TYPE_CLOSE) &&
        (IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_CANCELLED) ||
         IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN)))
    {
        status = STATUS_CANCELLED;
        GOTO_CLEANUP_EE(EE);
    }
    else
    {
        LwListInsertTail(&pFileObject->IrpList,
                         &irpInternal->FileObjectLinks);
    }

    // These are immutable from here until the IRP is freed.
    pIrp->Type = Type;
    pIrp->FileHandle = pFileObject;
    IopFileObjectReference(pFileObject);
    // The file object reference keeps an implicit reference to the
    // device and driver.
    pIrp->DeviceHandle = pFileObject->pDevice;
    pIrp->DriverHandle = pFileObject->pDevice->Driver;

cleanup:
    IopFileObjectUnlock(pFileObject);

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IopIrpCreate(
    OUT PIRP* ppIrp,
    IN IRP_TYPE Type,
    IN PIO_FILE_OBJECT pFileObject
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;

    status = IopIrpCreateDetached(&pIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopIrpAttach(pIrp, Type, pFileObject);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        IopIrpDereference(&pIrp);
    }

    *ppIrp = pIrp;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

static
VOID
IopIrpFree(
    IN OUT PIRP* ppIrp
    )
{
    PIRP pIrp = *ppIrp;

    if (pIrp)
    {
        PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

        LWIO_ASSERT(0 == irpInternal->ReferenceCount);
        LWIO_ASSERT(STATUS_PENDING != pIrp->IoStatusBlock.Status);

        switch (pIrp->Type)
        {
            case IRP_TYPE_CREATE:
            case IRP_TYPE_CREATE_NAMED_PIPE:
                IoSecurityDereferenceSecurityContext(&pIrp->Args.Create.SecurityContext);
                RtlUnicodeStringFree(&pIrp->Args.Create.FileName.Name);
                break;
            case IRP_TYPE_QUERY_DIRECTORY:
                if (pIrp->Args.QueryDirectory.FileSpec)
                {
                    LwRtlUnicodeStringFree(&pIrp->Args.QueryDirectory.FileSpec->Pattern);
                    IO_FREE(&pIrp->Args.QueryDirectory.FileSpec);
                }
                break;
            default:
                break;
        }

        // Note that the parent (FO) lock is already held
        // by IopIrpDereference().

        // Might not be in the list if IRP creation failed.
        if (irpInternal->FileObjectLinks.Next)
        {
            LwListRemove(&irpInternal->FileObjectLinks);
        }
        IopFileObjectDereference(&pIrp->FileHandle);

        IoMemoryFree(pIrp);
        *ppIrp = NULL;
    }
}

VOID
IoIrpMarkPending(
    IN PIRP pIrp,
    IN PIO_IRP_CALLBACK CancelCallback,
    IN OPTIONAL PVOID CancelCallbackContext
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    LWIO_ASSERT(CancelCallback);

    IopIrpAcquireCancelLock(pIrp);

    LWIO_ASSERT(!irpInternal->Cancel.Callback);
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_PENDING));
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_COMPLETE));
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_CANCELLED));

    SetFlag(irpInternal->Flags, IRP_FLAG_PENDING);
    irpInternal->Cancel.Callback = CancelCallback;
    irpInternal->Cancel.CallbackContext = CancelCallbackContext;

    IopIrpReleaseCancelLock(pIrp);

    //
    // Take a reference that will be released by IoIrpComplete.
    //

    IopIrpReference(pIrp);
}

static
VOID
IopIrpCompleteInternal(
    IN OUT PIRP pIrp,
    IN BOOLEAN IsAsyncCompletion
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    IopIrpAcquireCancelLock(pIrp);

    LWIO_ASSERT_MSG(IS_BOTH_OR_NEITHER(IsAsyncCompletion, IsSetFlag(irpInternal->Flags, IRP_FLAG_PENDING)), "IRP pending state is inconsistent.");
    LWIO_ASSERT_MSG(IsSetFlag(irpInternal->Flags, IRP_FLAG_DISPATCHED), "IRP cannot be completed unless it was properly dispatched.");
    LWIO_ASSERT_MSG(!IsSetFlag(irpInternal->Flags, IRP_FLAG_COMPLETE), "IRP cannot be completed more than once.");

    //
    // Note that the IRP may be CANCEL_PENDING or CANCELLED, but that
    // is ok.  In fact, completion may have been called in response
    // to cancellation.
    //

    SetFlag(irpInternal->Flags, IRP_FLAG_COMPLETE);

    IopIrpReleaseCancelLock(pIrp);

    IopFileObjectRemoveDispatched(pIrp->FileHandle, pIrp->Type);

    LWIO_ASSERT(IsValidStatusForIrpType(pIrp->IoStatusBlock.Status, pIrp->Type));

    switch (pIrp->Type)
    {
    case IRP_TYPE_CREATE:
    case IRP_TYPE_CREATE_NAMED_PIPE:
        if ((STATUS_SUCCESS == pIrp->IoStatusBlock.Status) ||
            (STATUS_OPLOCK_BREAK_IN_PROGRESS == pIrp->IoStatusBlock.Status))
        {
            // Handle special success processing having to do with file handle.
            // ISSUE-May not need lock since it should be only reference
            IopFileObjectLock(pIrp->FileHandle);
            SetFlag(pIrp->FileHandle->Flags, FILE_OBJECT_FLAG_CREATE_DONE);
            IopFileObjectUnlock(pIrp->FileHandle);

            IopFileObjectReference(pIrp->FileHandle);
            if (IsAsyncCompletion && irpInternal->Completion.IsAsyncCall)
            {
                *irpInternal->Completion.Async.OpOut.Create.pFileHandle = pIrp->FileHandle;
            }
        }
        break;

    case IRP_TYPE_CLOSE:
        if (STATUS_SUCCESS == pIrp->IoStatusBlock.Status)
        {
            PIO_FILE_OBJECT pFileObject = NULL;

            SetFlag(pIrp->FileHandle->Flags, FILE_OBJECT_FLAG_CLOSE_DONE);

            // Note that we must delete the reference from the create
            // w/o removing the file object value from the IRP (which
            // will be removed when the IRP is freed).

            pFileObject = pIrp->FileHandle;
            IopFileObjectDereference(&pFileObject);
        }
        else
        {
            LWIO_LOG_ERROR("Unable to close file object, status = 0x%08x",
                           pIrp->IoStatusBlock.Status);
        }
        break;

    case IRP_TYPE_READ:
    case IRP_TYPE_WRITE:
        if (IRP_ZCT_OPERATION_PREPARE == pIrp->Args.ReadWrite.ZctOperation)
        {
            if (STATUS_SUCCESS == pIrp->IoStatusBlock.Status)
            {
                LWIO_ASSERT(pIrp->Args.ReadWrite.ZctCompletionContext);

                if (IsAsyncCompletion && irpInternal->Completion.IsAsyncCall)
                {
                    PIRP pCompletionIrp = irpInternal->Completion.Async.OpOut.PrepareZctReadWrite.pCompletionIrp;
                    PVOID pCompletionContext = IopIrpSaveZctIrp(
                                                    pIrp->FileHandle,
                                                    pCompletionIrp,
                                                    pIrp->Args.ReadWrite.ZctCompletionContext);
                    *irpInternal->Completion.Async.OpOut.PrepareZctReadWrite.pCompletionContext = pCompletionContext;
                }
            }
            if (irpInternal->Completion.IsAsyncCall)
            {
                IopIrpDereference(&irpInternal->Completion.Async.OpOut.PrepareZctReadWrite.pCompletionIrp);
            }
        }
        break;
    }

    if (IsAsyncCompletion)
    {
        if (irpInternal->Completion.IsAsyncCall)
        {
            *irpInternal->Completion.Async.pIoStatusBlock = pIrp->IoStatusBlock;
            irpInternal->Completion.Async.Callback(
                    irpInternal->Completion.Async.CallbackContext);
        }
        else
        {
            LwRtlSetEvent(irpInternal->Completion.Sync.Event);
        }

        //
        // Release reference from IoIrpMarkPending().
        //

        IopIrpDereference(&pIrp);
    }
}

// must have set IO status block in IRP.
VOID
IoIrpComplete(
    IN OUT PIRP pIrp
    )
{
    IopIrpCompleteInternal(pIrp, TRUE);
}

VOID
IopIrpReference(
    IN PIRP pIrp
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
    InterlockedIncrement(&irpInternal->ReferenceCount);
}   

VOID
IopIrpDereference(
    IN OUT PIRP* ppIrp
    )
{
    PIRP pIrp = *ppIrp;
    if (pIrp)
    {
        PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
        LONG count = 0;
        PIO_FILE_OBJECT pFileObject = pIrp->FileHandle;

        if (pFileObject)
        {
            // Take a reference in case we free the IRP.
            IopFileObjectReference(pFileObject);
            // Lock since we may free and manipulate the FO IRP list.
            IopFileObjectLock(pFileObject);
        }

        count = InterlockedDecrement(&irpInternal->ReferenceCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            IopIrpFree(ppIrp);
        }

        // Remove our reference.
        if (pFileObject)
        {
            IopFileObjectUnlock(pFileObject);
            IopFileObjectDereference(&pFileObject);
        }
        *ppIrp = NULL;
    }
}

BOOLEAN
IopIrpCancel(
    IN PIRP pIrp
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
    BOOLEAN isCancellable = FALSE;
    BOOLEAN isAcquired = FALSE;

    if (!pIrp)
    {
        GOTO_CLEANUP();
    }

    IopIrpReference(pIrp);

    IopIrpAcquireCancelLock(pIrp);
    isAcquired = TRUE;

    if (!IsSetFlag(irpInternal->Flags, IRP_FLAG_CANCELLED | IRP_FLAG_COMPLETE))
    {
        if (irpInternal->Cancel.Callback)
        {
            ClearFlag(irpInternal->Flags, IRP_FLAG_CANCEL_PENDING);
            SetFlag(irpInternal->Flags, IRP_FLAG_CANCELLED);
            isCancellable = TRUE;
            irpInternal->Cancel.Callback(
                    pIrp,
                    irpInternal->Cancel.CallbackContext);
        }
        else
        {
            SetFlag(irpInternal->Flags, IRP_FLAG_CANCEL_PENDING);
        }
    }
    else
    {
        // If already cancelled or complete, we consider it as cancellable.
        isCancellable = TRUE;
    }

cleanup:

    if (isAcquired)
    {
        IopIrpReleaseCancelLock(pIrp);
    }

    if (pIrp)
    {
        IopIrpDereference(&pIrp);
    }

    return isCancellable;
}

static
inline
BOOLEAN
IopIrpIsCreate(
    IN PIRP pIrp
    )
{
    return ((IRP_TYPE_CREATE == pIrp->Type) ||
            (IRP_TYPE_CREATE_NAMED_PIPE == pIrp->Type));
}

static
inline
BOOLEAN
IopIrpIsPrepareZctReadWrite(
    IN PIRP pIrp
    )
{
    return (((IRP_TYPE_READ == pIrp->Type) ||
             (IRP_TYPE_WRITE == pIrp->Type)) &&
            (pIrp->Args.ReadWrite.ZctOperation == IRP_ZCT_OPERATION_PREPARE));
}

VOID
IopIrpSetOutputCreate(
    IN OUT PIRP pIrp,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    IN PIO_FILE_HANDLE pCreateFileHandle
    )
{
    LWIO_ASSERT(IopIrpIsCreate(pIrp));
    LWIO_ASSERT(pCreateFileHandle);

    if (AsyncControlBlock)
    {
        PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
        irpInternal->Completion.IsAsyncCall = TRUE;
        irpInternal->Completion.Async.OpOut.Create.pFileHandle = pCreateFileHandle;
    }
}

VOID
IopIrpSetOutputPrepareZctReadWrite(
    IN OUT PIRP pIrp,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    IN PVOID* pCompletionContext,
    IN PIRP pCompletionIrp
    )
{
    LWIO_ASSERT(IopIrpIsPrepareZctReadWrite(pIrp));
    LWIO_ASSERT(pCompletionContext);
    LWIO_ASSERT(pCompletionIrp);

    if (AsyncControlBlock)
    {
        PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
        irpInternal->Completion.IsAsyncCall = TRUE;
        irpInternal->Completion.Async.OpOut.PrepareZctReadWrite.pCompletionContext = pCompletionContext;
        irpInternal->Completion.Async.OpOut.PrepareZctReadWrite.pCompletionIrp = pCompletionIrp;
        IopIrpReference(pCompletionIrp);
    }
}

NTSTATUS
IopIrpDispatch(
    IN PIRP pIrp,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK pIoStatusBlock
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE ATTRIBUTE_UNUSED = 0;
    BOOLEAN isAsyncCall = FALSE;
    LW_RTL_EVENT event = LW_RTL_EVENT_ZERO_INITIALIZER;
    PIRP pExtraIrpReference = NULL;
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
    BOOLEAN needCancel = FALSE;
    IRP_TYPE irpType = pIrp->Type;

    LWIO_ASSERT(pIoStatusBlock);

    isAsyncCall = AsyncControlBlock ? TRUE : FALSE;
    if (isAsyncCall)
    {
        LWIO_ASSERT(!AsyncControlBlock->AsyncCancelContext);
        LWIO_ASSERT(AsyncControlBlock->Callback);

        irpInternal->Completion.Async.Callback = AsyncControlBlock->Callback;
        irpInternal->Completion.Async.CallbackContext = AsyncControlBlock->CallbackContext;
        irpInternal->Completion.Async.pIoStatusBlock = pIoStatusBlock;

        // Assert that caller has set required out params via IopIrpSetOutput*().
        LWIO_ASSERT(!IopIrpIsCreate(pIrp) || irpInternal->Completion.Async.OpOut.Create.pFileHandle);
        LWIO_ASSERT(!IopIrpIsPrepareZctReadWrite(pIrp) || irpInternal->Completion.Async.OpOut.PrepareZctReadWrite.pCompletionContext);

        // Reference IRP since we may need to return an an async cancel context.
        IopIrpReference(pIrp);
        pExtraIrpReference = pIrp;
    }
    else
    {
        // Since sync, assert IopIrpSetOutput*() has not actually set anything.
        LWIO_ASSERT(!irpInternal->Completion.IsAsyncCall);

        status = LwRtlInitializeEvent(&event);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        irpInternal->Completion.Sync.Event = &event;
    }

    irpInternal->Completion.IsAsyncCall = isAsyncCall;

    // We have to dispatch once we add the IRP as "dipatched"
    // and we have to call IopIrpCompleteInternal() so that
    // it gets subtracted.

    status = IopFileObjectAddDispatched(pIrp->FileHandle, pIrp->Type);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    SetFlag(irpInternal->Flags, IRP_FLAG_DISPATCHED);

    status = IopDeviceCallDriver(pIrp->DeviceHandle, pIrp);
    // Handle synchronous completion
    if (STATUS_PENDING != status)
    {
        IopIrpCompleteInternal(pIrp, FALSE);
    }
    // Handle asynchronous dispatch
    else
    {
        IopIrpAcquireCancelLock(pIrp);

        LWIO_ASSERT(IsSetFlag(irpInternal->Flags, IRP_FLAG_PENDING));
        LWIO_ASSERT(irpInternal->Cancel.Callback);

        needCancel = IsSetFlag(irpInternal->Flags, IRP_FLAG_CANCEL_PENDING);

        IopIrpReleaseCancelLock(pIrp);

        if (needCancel)
        {
            IopIrpCancel(pIrp);
        }

        // Handle waiting for asynchronous completion for synchronous caller
        if (!isAsyncCall)
        {
            LwRtlWaitEvent(&event, NULL);

            LWIO_ASSERT(pIrp->IoStatusBlock.Status != STATUS_PENDING);
            status = pIrp->IoStatusBlock.Status;
        }
    }

    //
    // At this point, we are either complete or this is
    // an async call that returned STATUS_PENDING.
    //

    LWIO_ASSERT((STATUS_PENDING == status) || (pIrp->IoStatusBlock.Status == status));

cleanup:
    LwRtlCleanupEvent(&event);

    if (STATUS_PENDING == status)
    {
        LWIO_ASSERT(isAsyncCall);

        AsyncControlBlock->AsyncCancelContext = IopIrpGetAsyncCancelContextFromIrp(pIrp);
    }
    else
    {
        if (isAsyncCall)
        {
            //
            // Remove async cancel context reference added earlier since we
            // are returning synchronously w/o an async cancel context.
            //

            IopIrpDereference(&pExtraIrpReference);
        }

        pIrp->IoStatusBlock.Status = status;
        *pIoStatusBlock = pIrp->IoStatusBlock;
    }

    LWIO_ASSERT(IS_BOTH_OR_NEITHER(pExtraIrpReference, (STATUS_PENDING == status)));
    LWIO_ASSERT((STATUS_PENDING != status) || isAsyncCall);
    LWIO_ASSERT(IsValidStatusForIrpType(status, irpType));
    return status;
}

VOID
IopIrpCancelFileObject(
    IN PIO_FILE_OBJECT pFileObject,
    IN BOOLEAN IsForRundown
    )
{
    BOOLEAN isLocked = FALSE;
    PLW_LIST_LINKS pLinks = NULL;
    PIRP_INTERNAL irpInternal = NULL;
    LW_LIST_LINKS cancelList = { 0 };
    PIRP pIrp = NULL;

    LwListInit(&cancelList);

    // Gather IRPs we want to cancel while holding FO lock.
    IopFileObjectLock(pFileObject);
    isLocked = TRUE;

    if (IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_CANCELLED))
    {
        GOTO_CLEANUP();
    }

    if (IsForRundown)
    {
        SetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_CANCELLED);
    }

    // gather list of IRPs
    for (pLinks = pFileObject->IrpList.Next;
         pLinks != &pFileObject->IrpList;
         pLinks = pLinks->Next)
    {
        irpInternal = LW_STRUCT_FROM_FIELD(pLinks, IRP_INTERNAL, FileObjectLinks);

        LWIO_ASSERT(irpInternal->Irp.FileHandle == pFileObject);

        // Verify that this IRP is not already being cancelled.
        if (!irpInternal->CancelLinks.Next)
        {
            IopIrpReference(&irpInternal->Irp);
            LwListInsertTail(&cancelList, &irpInternal->CancelLinks);
        }
    }
    IopFileObjectUnlock(pFileObject);
    isLocked = FALSE;

    // Iterate over list, calling IopIrpCancel as appropriate.
    while (!LwListIsEmpty(&cancelList))
    {
        pLinks = LwListRemoveHead(&cancelList);
        irpInternal = LW_STRUCT_FROM_FIELD(pLinks, IRP_INTERNAL, CancelLinks);
        pIrp = &irpInternal->Irp;

        IopIrpCancel(pIrp);
        IopIrpDereference(&pIrp);
    }

cleanup:
    if (isLocked)
    {
        IopFileObjectUnlock(pFileObject);
    }
}

VOID
IopIrpFreeZctIrpList(
    IN OUT PIO_FILE_OBJECT pFileObject
    )
{
    PLW_LIST_LINKS pLinks = NULL;;
    PIRP_INTERNAL irpInternal = NULL;
    PIRP pIrp = NULL;

    while (!LwListIsEmpty(&pFileObject->ZctCompletionIrpList))
    {
        pLinks = LwListRemoveHead(&pFileObject->ZctCompletionIrpList);
        irpInternal = LW_STRUCT_FROM_FIELD(pLinks, IRP_INTERNAL, CancelLinks);
        pIrp = &irpInternal->Irp;

        LWIO_ASSERT(1 == irpInternal->ReferenceCount);
        LWIO_ASSERT(!pIrp->FileHandle);

        IopIrpDereference(&pIrp);
    }
}

PVOID
IopIrpSaveZctIrp(
    IN OUT PIO_FILE_OBJECT pFileObject,
    IN PIRP pIrp,
    IN PVOID pCompletionContext
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    LWIO_ASSERT(pCompletionContext);

    IopIrpReference(pIrp);

    pIrp->Args.ReadWrite.ZctCompletionContext = pCompletionContext;

    IopFileObjectLock(pFileObject);
    LwListInsertTail(&pFileObject->ZctCompletionIrpList, &irpInternal->CancelLinks);
    IopFileObjectUnlock(pFileObject);

    return pIrp;
}

PIRP
IopIrpLoadZctIrp(
    IN OUT PIO_FILE_OBJECT pFileObject,
    IN PVOID pCompletionContext
    )
{
    PIRP pIrp = (PIRP) pCompletionContext;
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    LWIO_ASSERT(pIrp->Args.ReadWrite.ZctCompletionContext);
    LWIO_ASSERT(irpInternal->CancelLinks.Next && irpInternal->CancelLinks.Prev);

    IopFileObjectLock(pFileObject);
    LwListRemove(&irpInternal->CancelLinks);
    IopFileObjectUnlock(pFileObject);

    RtlZeroMemory(&irpInternal->CancelLinks, sizeof(irpInternal->CancelLinks));

    return pIrp;
}
