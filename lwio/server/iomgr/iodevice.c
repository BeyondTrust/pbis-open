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

    if (IsNullOrEmptyString(pszName))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = RtlUnicodeStringAllocateFromCString(&deviceName, pszName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // TODO-Add locking

    pFoundDevice = IopRootFindDevice(DriverHandle->Root, &deviceName);
    if (pFoundDevice)
    {
        status = STATUS_OBJECT_NAME_COLLISION;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IO_ALLOCATE(&pDeviceObject, IO_DEVICE_OBJECT, sizeof(*pDeviceObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pDeviceObject->ReferenceCount = 1;
    pDeviceObject->Driver = DriverHandle;
    pDeviceObject->DeviceName = deviceName;
    IoMemoryZero(&deviceName, sizeof(deviceName));
    pDeviceObject->Context = DeviceContext;

    LwListInit(&pDeviceObject->FileObjectsList);

    IopDriverInsertDevice(pDeviceObject->Driver, &pDeviceObject->DriverLinks);
    IopRootInsertDevice(pDeviceObject->Driver->Root, &pDeviceObject->RootLinks);

    status = LwRtlInitializeMutex(&pDeviceObject->Mutex, TRUE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeMutex(&pDeviceObject->CancelMutex, TRUE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    *pDeviceHandle = pDeviceObject;

cleanup:
    if (status)
    {
        if (pDeviceObject)
        {
            IoDeviceDelete(&pDeviceObject);
        }
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

    LWIO_ASSERT(pDeviceObject);

    if (pDeviceObject)
    {
        // TODO -- add proper drain support...
        LWIO_ASSERT(LwListIsEmpty(&pDeviceObject->FileObjectsList));
        IopDriverRemoveDevice(pDeviceObject->Driver, &pDeviceObject->DriverLinks);
        IopRootRemoveDevice(pDeviceObject->Driver->Root, &pDeviceObject->RootLinks);
        RtlUnicodeStringFree(&pDeviceObject->DeviceName);
        LwRtlCleanupMutex(&pDeviceObject->Mutex);
        LwRtlCleanupMutex(&pDeviceObject->CancelMutex);
        IoMemoryFree(pDeviceObject);
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
