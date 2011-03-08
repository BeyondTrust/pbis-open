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

#include "iop.h"

VOID
IopFileObjectLock(
    IN PIO_FILE_OBJECT pFileObject
    )
{
    LwRtlLockMutex(&pFileObject->Mutex);
}

VOID
IopFileObjectUnlock(
    IN PIO_FILE_OBJECT pFileObject
    )
{
    LwRtlUnlockMutex(&pFileObject->Mutex);
}

VOID
IopFileObjectReference(
    IN PIO_FILE_OBJECT pFileObject
    )
{
    LONG count = InterlockedIncrement(&pFileObject->ReferenceCount);
    LWIO_ASSERT(count > 1);
}

VOID
IopFileObjectDereference(
    IN OUT PIO_FILE_OBJECT* ppFileObject
    )
{
    PIO_FILE_OBJECT pFileObject = *ppFileObject;

    if (pFileObject)
    {
        // Note that we do not have to acquire the device
        // lock here since it is impossible to resolve
        // a file object from a device.
        LONG count = InterlockedDecrement(&pFileObject->ReferenceCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            IopFileObjectFree(&pFileObject);
        }
        *ppFileObject = NULL;
    }
}

NTSTATUS
IopFileObjectAllocate(
    OUT PIO_FILE_OBJECT* ppFileObject,
    IN PIO_DEVICE_OBJECT pDevice,
    IN PIO_FILE_NAME FileName
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_FILE_OBJECT pFileObject = NULL;

    status = IO_ALLOCATE(&pFileObject, IO_FILE_OBJECT, sizeof(*pFileObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pFileObject->ReferenceCount = 1;
    pFileObject->pDevice = pDevice;
    IopDeviceReference(pDevice);

    LwListInit(&pFileObject->IrpList);
    LwListInit(&pFileObject->DeviceLinks);
    LwListInit(&pFileObject->RundownLinks);
    LwListInit(&pFileObject->ZctCompletionIrpList);

    // Pre-allocate IRP for close.
    status = IopIrpCreateDetached(&pFileObject->pCloseIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (FileName->RootFileHandle)
    {
        SetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RELATIVE);
    }

    status = LwRtlUnicodeStringDuplicate(
                    &pFileObject->FileName,
                    &FileName->Name);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeMutex(&pFileObject->Mutex, TRUE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeConditionVariable(&pFileObject->Rundown.Condition);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    IopDeviceLock(pDevice);
    if (IsSetFlag(pDevice->Flags, IO_DEVICE_OBJECT_FLAG_RUNDOWN) ||
        IsSetFlag(pDevice->Flags, IO_DEVICE_OBJECT_FLAG_RUNDOWN_DRIVER))
    {
        // TODO: Find "correct" error code.
        status = STATUS_INVALID_HANDLE;
    }
    else
    {
        LwListInsertTail(&pDevice->FileObjectsList, &pFileObject->DeviceLinks);
    }
    IopDeviceUnlock(pDevice);

cleanup:
    if (status)
    {
        IopFileObjectDereference(&pFileObject);
    }

    *ppFileObject = pFileObject;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

VOID
IopFileObjectFree(
    IN OUT PIO_FILE_OBJECT* ppFileObject
    )
{
    PIO_FILE_OBJECT pFileObject = *ppFileObject;

    if (pFileObject)
    {
        LWIO_ASSERT(LwListIsEmpty(&pFileObject->IrpList));

        IopIrpFreeZctIrpList(pFileObject);

        IopDeviceLock(pFileObject->pDevice);
        LwListRemove(&pFileObject->DeviceLinks);
        IopDeviceUnlock(pFileObject->pDevice);

        IopDeviceDereference(&pFileObject->pDevice);

        LwRtlCleanupConditionVariable(&pFileObject->Rundown.Condition);
        LwRtlCleanupMutex(&pFileObject->Mutex);

        LwRtlUnicodeStringFree(&pFileObject->FileName);

        IopIrpDereference(&pFileObject->pCloseIrp);

        IoMemoryFree(pFileObject);
        *ppFileObject = NULL;
    }
}

static
NTSTATUS
IopFileObjectGetCloseIrp(
    IN IO_FILE_HANDLE FileHandle,
    OUT PIRP* ppIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIRP pIrp = NULL;

    // TODO-Use InterlockedExchangePointer()

    IopFileObjectLock(FileHandle);
    pIrp = FileHandle->pCloseIrp;
    FileHandle->pCloseIrp = NULL;
    IopFileObjectUnlock(FileHandle);

    if (!LWIO_ASSERT_VALUE_MSG(pIrp, "Cannot close already closed file"))
    {
        status = STATUS_FILE_CLOSED;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);

    *ppIrp = pIrp;

    return status;
}


NTSTATUS
IoFileSetContext(
    IN IO_FILE_HANDLE FileHandle,
    IN PVOID FileContext
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    if (FileHandle->pContext)
    {
        assert(FALSE);
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    FileHandle->pContext = FileContext;

cleanup:
    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

PVOID
IoFileGetContext(
    IN IO_FILE_HANDLE FileHandle
    )
{
    return FileHandle->pContext;    
}

VOID
IoFileSetZctSupportMask(
    IN IO_FILE_HANDLE FileHandle,
    IN LW_ZCT_ENTRY_MASK ZctReadMask,
    IN LW_ZCT_ENTRY_MASK ZctWriteMask
    )
{
    // It is up to the FSD to synchornize
    IopFileObjectLock(FileHandle);
    FileHandle->ZctReadMask = ZctReadMask;
    FileHandle->ZctWriteMask = ZctWriteMask;
    IopFileObjectUnlock(FileHandle);
}

VOID
IopFileGetZctSupportMask(
    IN IO_FILE_HANDLE FileHandle,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctReadMask,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctWriteMask
    )
{
    if (ZctReadMask || ZctWriteMask)
    {
        IopFileObjectLock(FileHandle);
        if (ZctReadMask)
        {
            *ZctReadMask = FileHandle->ZctReadMask;
        }
        if (ZctWriteMask)
        {
            *ZctWriteMask = FileHandle->ZctWriteMask;
        }
        IopFileObjectUnlock(FileHandle);
    }
}

static
NTSTATUS
IopContinueAsyncCloseFile(
    IN PIO_FILE_OBJECT FileHandle,
    IN OPTIONAL PIO_ASYNC_COMPLETE_CALLBACK Callback,
    IN OPTIONAL PVOID CallbackContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_ASYNC_CONTROL_BLOCK asyncControlBlock = { 0 };
    PIO_ASYNC_CONTROL_BLOCK useAsyncControlBlock = NULL;
    BOOLEAN isOpen = FALSE;

    //
    // If the create never completed successfully, we do not want
    // to send a CLOSE IRP.
    //

    // TODO -- There is probably still a small race window
    // wrt create and device rundown.  The fix may involve
    // changing when IopFileObjectRemoveDispatched() is called
    // from IopIrpCompleteInternal() so that it is called
    // after the switch on the IRP type.

    IopFileObjectLock(FileHandle);
    isOpen = IsSetFlag(FileHandle->Flags, FILE_OBJECT_FLAG_CREATE_DONE);
    IopFileObjectUnlock(FileHandle);

    if (!isOpen)
    {
        status = STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    //
    // The file was actually opened, so do rest of close cleanup.
    //

    status = IopFileObjectGetCloseIrp(FileHandle, &pIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopIrpAttach(pIrp, IRP_TYPE_CLOSE, FileHandle);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (Callback)
    {
        asyncControlBlock.Callback = Callback;
        asyncControlBlock.CallbackContext = CallbackContext;
        useAsyncControlBlock = &asyncControlBlock;
    }

    status = IopIrpDispatch(
                    pIrp,
                    useAsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING == status)
    {
        IoDereferenceAsyncCancelContext(&asyncControlBlock.AsyncCancelContext);
    }
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    IopIrpDereference(&pIrp);

    if (!useAsyncControlBlock && IoStatusBlock && (STATUS_PENDING != status))
    {
        IO_STATUS_BLOCK ioStatusBlock = { 0 };

        ioStatusBlock.Status = status;
        *IoStatusBlock = ioStatusBlock;
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);

    return status;
}

NTSTATUS
IopFileObjectRundown(
    IN PIO_FILE_OBJECT pFileObject
    )
{
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    IopFileObjectReference(pFileObject);

    return IopFileObjectRundownEx(pFileObject, NULL, NULL, &ioStatusBlock);
}

NTSTATUS
IopFileObjectRundownEx(
    IN OUT PIO_FILE_OBJECT pFileObject,
    IN OPTIONAL PIO_ASYNC_COMPLETE_CALLBACK Callback,
    IN OPTIONAL PVOID CallbackContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    BOOLEAN isLocked = FALSE;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    IopFileObjectLock(pFileObject);
    isLocked = TRUE;

    if (IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_CLOSE_DONE))
    {
        LWIO_ASSERT(IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN));
        // Note that there can be IRP references for completed IRPs sitting
        // around where the caller has not yet gotten rid of the IRP by
        // calling IoDereferenceAsyncCancelContext().  So we cannot assert
        // that (1 == pFileObject->ReferenceCount).  Therefore, we do the
        // next best thing and check DispatchedIrpCount.  Technically, if
        // someone got far enough into a new call that they created an IRP,
        // they will also have a new IRP file objeject reference.  However,
        // the new IRP will fail to dispatch.  While that is a logic bug
        // in the caller, we cannot trap those sorts of bugs via asserts
        // in the I/O manager.
        LWIO_ASSERT(0 == pFileObject->DispatchedIrpCount);

        IopFileObjectUnlock(pFileObject);
        isLocked = FALSE;

        IopFileObjectDereference(&pFileObject);

        status = STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    if (IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN))
    {
        LWIO_LOG_ERROR("Attempt to rundown multiple times");
        status = STATUS_FILE_CLOSED;
        GOTO_CLEANUP_EE(EE);
    }

    SetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN);

    IopFileObjectUnlock(pFileObject);
    isLocked = FALSE;

    // Cancel everything now that rundown flag is set.
    IopIrpCancelFileObject(pFileObject, TRUE);

    // Now check whether we need to wait for rundown.
    IopFileObjectLock(pFileObject);
    isLocked = TRUE;

    if (0 != pFileObject->DispatchedIrpCount)
    {
        // Need to wait

        SetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN_WAIT);

        if (!Callback)
        {
            // Wait inline for synchronous case

            LwRtlWaitConditionVariable(
                    &pFileObject->Rundown.Condition,
                    &pFileObject->Mutex,
                    NULL);
            LWIO_ASSERT(0 == pFileObject->DispatchedIrpCount);
        }
        else
        {
            // Set up rundown callback for async case

            pFileObject->Rundown.Callback = Callback;
            pFileObject->Rundown.CallbackContext = CallbackContext;
            pFileObject->Rundown.pIoStatusBlock = IoStatusBlock;

            status = STATUS_PENDING;
            GOTO_CLEANUP_EE(EE);
        }
    }

    IopFileObjectUnlock(pFileObject);
    isLocked = FALSE;

    // We can now continue closing.

    status = IopContinueAsyncCloseFile(
                    pFileObject,
                    Callback,
                    CallbackContext,
                    IoStatusBlock);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    
cleanup:
    if (isLocked)
    {
        IopFileObjectUnlock(pFileObject);
    }

    if (status && (STATUS_PENDING != status))
    {
        ioStatusBlock.Status = status;
    }

    if ((STATUS_PENDING != status) && IoStatusBlock)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    // TODO-Perhaps do not ASSERT here because LwRtlInitializeEvent()
    // could have failed if disaptching close IRP synchronously.
    LWIO_ASSERT((STATUS_SUCCESS == status) ||
                (STATUS_PENDING == status) ||
                (STATUS_FILE_CLOSED == status));

    // TODO-Perhaps also remove object from device's file object
    // list such that it cannot be rundown multiple times.  This
    // would avoid the STATUS_FILE_CLOSED above.

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IopFileObjectAddDispatched(
    IN PIO_FILE_OBJECT pFileObject,
    IN IRP_TYPE Type
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    IopFileObjectLock(pFileObject);

    if ((Type != IRP_TYPE_CLOSE) &&
        (IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_CANCELLED) ||
         IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN)))
    {
        status = STATUS_CANCELLED;
    }
    else
    {
        status = STATUS_SUCCESS;

        pFileObject->DispatchedIrpCount++;
        LWIO_ASSERT(pFileObject->DispatchedIrpCount >= 1);
    }

    IopFileObjectUnlock(pFileObject);

    return status;
}

VOID
IopFileObjectRemoveDispatched(
    IN PIO_FILE_OBJECT pFileObject,
    IN IRP_TYPE Type
    )
{
    BOOLEAN needContinueAsycClose = FALSE;

    IopFileObjectLock(pFileObject);

    pFileObject->DispatchedIrpCount--;
    LWIO_ASSERT(pFileObject->DispatchedIrpCount >= 0);

    if (IsSetFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN_WAIT) &&
        (0 == pFileObject->DispatchedIrpCount))
    {
        // TODO-Perhaps remove Type parameter since the use of
        // FILE_OBJECT_FLAG_RUNDOWN_WAIT negates the need for it.
        LWIO_ASSERT(Type != IRP_TYPE_CLOSE);

        if (pFileObject->Rundown.Callback)
        {
            needContinueAsycClose = TRUE;
        }
        else
        {
            LwRtlSignalConditionVariable(&pFileObject->Rundown.Condition);
        }

        ClearFlag(pFileObject->Flags, FILE_OBJECT_FLAG_RUNDOWN_WAIT);
    }

    IopFileObjectUnlock(pFileObject);

    if (needContinueAsycClose)
    {
        // This will send the close to the FSD.  Note that the callback
        // must be called from here on a synchronous completion because
        // there was no asynchronous IRP completion to take care of calling
        // the callback.
        NTSTATUS status = IopContinueAsyncCloseFile(
                pFileObject,
                pFileObject->Rundown.Callback,
                pFileObject->Rundown.CallbackContext,
                pFileObject->Rundown.pIoStatusBlock);
        if (STATUS_PENDING != status)
        {
            pFileObject->Rundown.Callback(pFileObject->Rundown.CallbackContext);
        }
    }
}
