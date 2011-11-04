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

static
NTSTATUS
IopDeviceAllocate(
    OUT PIO_DEVICE_OBJECT* ppDeviceObject,
    IN PIO_DRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pDeviceName,
    IN OPTIONAL PVOID DeviceContext
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_DEVICE_OBJECT pDeviceObject = NULL;

    status = IO_ALLOCATE(&pDeviceObject, IO_DEVICE_OBJECT, sizeof(*pDeviceObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pDeviceObject->ReferenceCount = 1;
    pDeviceObject->Driver = pDriverObject;
    IopDriverReference(pDriverObject);

    pDeviceObject->Context = DeviceContext;

    LwListInit(&pDeviceObject->FileObjectsList);

    // Initialize links too
    LwListInit(&pDeviceObject->DriverLinks);
    LwListInit(&pDeviceObject->RootLinks);
    LwListInit(&pDeviceObject->RundownLinks);

    status = LwRtlUnicodeStringDuplicate(&pDeviceObject->DeviceName, pDeviceName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeMutex(&pDeviceObject->Mutex, TRUE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeMutex(&pDeviceObject->CancelMutex, TRUE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        IopDeviceDereference(&pDeviceObject);
    }

    *ppDeviceObject = pDeviceObject;

    IO_LOG_ENTER_LEAVE_STATUS_EE(status, EE);
    return status;
}

static
VOID
IopDeviceFree(
    IN OUT PIO_DEVICE_OBJECT* ppDeviceObject
    )
{
    PIO_DEVICE_OBJECT pDeviceObject = *ppDeviceObject;

    if (pDeviceObject)
    {
        // The object should have already been rundown

        LWIO_ASSERT(LwListIsEmpty(&pDeviceObject->FileObjectsList));

        // The device should not be in any lists.
        LWIO_ASSERT(LwListIsEmpty(&pDeviceObject->RootLinks));
        LWIO_ASSERT(LwListIsEmpty(&pDeviceObject->DriverLinks));

        RtlUnicodeStringFree(&pDeviceObject->DeviceName);

        LwRtlCleanupMutex(&pDeviceObject->Mutex);
        LwRtlCleanupMutex(&pDeviceObject->CancelMutex);

        IopDriverDereference(&pDeviceObject->Driver);

        IoMemoryFree(pDeviceObject);
        *ppDeviceObject = NULL;
    }
}

NTSTATUS
IoDeviceCreate(
    OUT PIO_DEVICE_HANDLE pDeviceHandle,
    IN IO_DRIVER_HANDLE DriverHandle,
    IN PCSTR pszName,
    IN OPTIONAL PVOID DeviceContext
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_DEVICE_OBJECT pDeviceObject = NULL;
    PIO_DEVICE_OBJECT pFoundDevice = NULL;
    UNICODE_STRING deviceName = { 0 };

    if (!DriverHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    // iomgr does not (currently) support unnamed devices.
    if (IsNullOrEmptyString(pszName))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = RtlUnicodeStringAllocateFromCString(&deviceName, pszName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pFoundDevice = IopRootFindDevice(DriverHandle->Root, &deviceName);
    if (pFoundDevice)
    {
        IopDeviceDereference(&pFoundDevice);

        status = STATUS_OBJECT_NAME_COLLISION;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IopDeviceAllocate(&pDeviceObject, DriverHandle, &deviceName, DeviceContext);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Can fail if another device with the same name was created by another
    // thread since the initial check.
    status = IopRootInsertDevice(pDeviceObject->Driver->Root, pDeviceObject);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    IopDriverInsertDevice(pDeviceObject->Driver, &pDeviceObject->DriverLinks);

    *pDeviceHandle = pDeviceObject;

cleanup:
    if (status)
    {
        IoDeviceDelete(&pDeviceObject);
    }

    RtlUnicodeStringFree(&deviceName);

    IO_LOG_ENTER_LEAVE_STATUS_EE(status, EE);
    return status;
}

VOID
IoDeviceDelete(
    IN OUT PIO_DEVICE_HANDLE pDeviceHandle
    )
{
    PIO_DEVICE_OBJECT pDeviceObject = *pDeviceHandle;

    if (pDeviceObject)
    {
        IopDeviceRundown(pDeviceObject);
        IopDriverRemoveDevice(pDeviceObject->Driver, &pDeviceObject->DriverLinks);
        IopDeviceDereference(&pDeviceObject);

        *pDeviceHandle = NULL;
    }
}


PVOID
IoDeviceGetContext(
    IN IO_DEVICE_HANDLE DeviceHandle
    )
{
    return DeviceHandle->Context;
}

NTSTATUS
IopDeviceCallDriver(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN OUT PIRP pIrp
    )
{
    return DeviceHandle->Driver->Callback.Dispatch(DeviceHandle, pIrp);
}

VOID
IopDeviceLock(
    IN PIO_DEVICE_OBJECT pDeviceObject
    )
{
    LwRtlLockMutex(&pDeviceObject->Mutex);
}

VOID
IopDeviceUnlock(
    IN PIO_DEVICE_OBJECT pDeviceObject
    )
{
    LwRtlUnlockMutex(&pDeviceObject->Mutex);
}

VOID
IopDeviceReference(
    IN PIO_DEVICE_OBJECT pDeviceObject
    )
{
    LONG count = InterlockedIncrement(&pDeviceObject->ReferenceCount);
    LWIO_ASSERT(count > 1);
}

VOID
IopDeviceDereference(
    IN OUT PIO_DEVICE_OBJECT* ppDeviceObject
    )
{
    PIO_DEVICE_OBJECT pDeviceObject = *ppDeviceObject;

    if (pDeviceObject)
    {
        // Rundown of the device happens if it was ever inserted into the
        // root's device list.  So there is no need to lock the root's device
        // list here to synchronize with a "find" in the root's device list.
        // (The same applies wrt the driver's device list.)
        LONG count = InterlockedDecrement(&pDeviceObject->ReferenceCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            IopDeviceFree(&pDeviceObject);
        }
        *ppDeviceObject = NULL;
    }
}

NTSTATUS
IopDeviceRundown(
    IN PIO_DEVICE_OBJECT pDeviceObject
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    BOOLEAN isLocked = FALSE;
    PLW_LIST_LINKS pLinks = NULL;
    LW_LIST_LINKS rundownList = { 0 };

    LwListInit(&rundownList);

    IopDeviceLock(pDeviceObject);
    isLocked = TRUE;

    if (IsSetFlag(pDeviceObject->Flags, IO_DEVICE_OBJECT_FLAG_RUNDOWN))
    {
        // TODO: Perhaps wait if currently running down.

        status = STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    SetFlag(pDeviceObject->Flags, IO_DEVICE_OBJECT_FLAG_RUNDOWN);

    // Gather rundown list

    for (pLinks = pDeviceObject->FileObjectsList.Next;
         pLinks != &pDeviceObject->FileObjectsList;
         pLinks = pLinks->Next)
    {
        PIO_FILE_OBJECT pFileObject = LW_STRUCT_FROM_FIELD(pLinks, IO_FILE_OBJECT, DeviceLinks);

        IopFileObjectReference(pFileObject);
        LwListInsertTail(&rundownList, &pFileObject->RundownLinks);
    }

    IopDeviceUnlock(pDeviceObject);
    isLocked = FALSE;

    // Now, actually run down every file w/o holding the device lock.

    while (!LwListIsEmpty(&rundownList))
    {
        PIO_FILE_OBJECT pFileObject = NULL;

        pLinks = LwListRemoveHead(&rundownList);
        pFileObject = LW_STRUCT_FROM_FIELD(pLinks, IO_FILE_OBJECT, RundownLinks);

        IopFileObjectRundown(pFileObject);
    }

    IopRootRemoveDevice(pDeviceObject->Driver->Root, &pDeviceObject->RootLinks);

cleanup:
    if (isLocked)
    {
        IopDeviceUnlock(pDeviceObject);
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}
