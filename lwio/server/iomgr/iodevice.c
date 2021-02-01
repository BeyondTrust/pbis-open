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
