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
IopDriverRundownEx(
    IN PIO_DRIVER_OBJECT pDriverObject,
    IN BOOLEAN ForceFileRundown
    );


static
NTSTATUS
IopDriverAllocate(
    OUT PIO_DRIVER_OBJECT* ppDriverObject,
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName,
    IN PCSTR pszDriverName,
    IN OPTIONAL PIO_DRIVER_ENTRY pStaticDriverEntry,
    IN OPTIONAL PCSTR pszDriverPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_DRIVER_OBJECT pDriverObject = NULL;

    LWIO_ASSERT(!LW_IS_BOTH_OR_NEITHER(pStaticDriverEntry, pszDriverPath));

    status = IO_ALLOCATE(&pDriverObject, IO_DRIVER_OBJECT, sizeof(*pDriverObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInit(&pDriverObject->DeviceList);
    LwListInit(&pDriverObject->RootLinks);

    pDriverObject->ReferenceCount = 1;
    pDriverObject->Root = pRoot;

    status = RtlUnicodeStringDuplicate(&pDriverObject->DriverName, pDriverName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCStringDuplicate(&pDriverObject->pszDriverName, pszDriverName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (pStaticDriverEntry)
    {
        pDriverObject->DriverEntry = pStaticDriverEntry;
    }
    else
    {
        status = RtlCStringDuplicate(&pDriverObject->pszDriverPath, pszDriverPath);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    LwRtlInitializeMutex(&pDriverObject->Mutex, FALSE);

cleanup:
    if (status)
    {
        IopDriverDereference(&pDriverObject);
    }

    *ppDriverObject = pDriverObject;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

static
VOID
IopDriverFree(
    IN OUT PIO_DRIVER_OBJECT* ppDriverObject
    )
{
    PIO_DRIVER_OBJECT pDriverObject = *ppDriverObject;

    if (pDriverObject)
    {
        // The object should have already been rundown and unloaded.

        LWIO_ASSERT(LwListIsEmpty(&pDriverObject->DeviceList));
        LWIO_ASSERT(!pDriverObject->LibraryHandle);
        LWIO_ASSERT(!pDriverObject->DriverEntry);

        // The device should not be in any lists.
        LWIO_ASSERT(LwListIsEmpty(&pDriverObject->RootLinks));

        LwRtlCleanupMutex(&pDriverObject->Mutex);

        RtlUnicodeStringFree(&pDriverObject->DriverName);
        RTL_FREE(&pDriverObject->pszDriverName);
        RTL_FREE(&pDriverObject->pszDriverPath);
        
        IoMemoryFree(pDriverObject);
        
        *ppDriverObject = NULL;
    }
}

VOID
IopDriverUnload(
    IN OUT PIO_DRIVER_OBJECT* ppDriverObject
    )
{
    PIO_DRIVER_OBJECT pDriverObject = *ppDriverObject;

    if (pDriverObject)
    {
        if (pDriverObject->pszDriverName)
        {
            LWIO_LOG_DEBUG("Unloading driver '%s'", pDriverObject->pszDriverName);
        }

        if (IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_READY))
        {
            IopDriverRundownEx(pDriverObject, TRUE);
            IopRootRemoveDriver(pDriverObject->Root, &pDriverObject->RootLinks);
        }

        if (IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
        {
            pDriverObject->Callback.Shutdown(pDriverObject);
        }

        // The driver is supposed to have removed all of its devices.
        LWIO_ASSERT(LwListIsEmpty(&pDriverObject->DeviceList));

        if (pDriverObject->LibraryHandle)
        {
            int err = dlclose(pDriverObject->LibraryHandle);
            if (err)
            {
                LWIO_LOG_ERROR("Failed to dlclose() for driver '%s' from '%s'",
                               pDriverObject->pszDriverName,
                               pDriverObject->pszDriverPath);
            }
            pDriverObject->LibraryHandle = NULL;
        }

        pDriverObject->DriverEntry = NULL;

        IopDriverDereference(&pDriverObject);
    }
}

NTSTATUS
IopDriverLoad(
    OUT PIO_DRIVER_OBJECT* ppDriverObject,
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName,
    IN PCSTR pszDriverName,
    IN OPTIONAL PIO_DRIVER_ENTRY pStaticDriverEntry,
    IN OPTIONAL PCSTR pszDriverPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_DRIVER_OBJECT pDriverObject = NULL;
    PCSTR pszError = NULL;

    LWIO_LOG_DEBUG("Loading driver '%s'", pszDriverName);

    LWIO_ASSERT(!LW_IS_BOTH_OR_NEITHER(pStaticDriverEntry, pszDriverPath));

    status = IopDriverAllocate(&pDriverObject,
                               pRoot,
                               pDriverName,
                               pszDriverName,
                               pStaticDriverEntry,
                               pszDriverPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (pDriverObject->pszDriverPath)
    {
        dlerror();

        pDriverObject->LibraryHandle = dlopen(pDriverObject->pszDriverPath, RTLD_NOW | RTLD_LOCAL);
        if (!pDriverObject->LibraryHandle)
        {
            pszError = dlerror();

            LWIO_LOG_ERROR("Failed to load driver '%s' from '%s' (%s)",
                           pszDriverName,
                           pszDriverPath,
                           LWIO_SAFE_LOG_STRING(pszError));

            status = STATUS_DLL_NOT_FOUND;
            GOTO_CLEANUP_EE(EE);
        }

        dlerror();
        pDriverObject->DriverEntry = (PIO_DRIVER_ENTRY)dlsym(pDriverObject->LibraryHandle, IO_DRIVER_ENTRY_FUNCTION_NAME);
        if (!pDriverObject->DriverEntry)
        {
            pszError = dlerror();

            LWIO_LOG_ERROR("Failed to load " IO_DRIVER_ENTRY_FUNCTION_NAME " "
                           "function for driver %s from %s (%s)",
                           pszDriverName,
                           pszDriverPath,
                           LWIO_SAFE_LOG_STRING(pszError));

            status = STATUS_BAD_DLL_ENTRYPOINT;
            GOTO_CLEANUP_EE(EE);
        }
    }

    status = pDriverObject->DriverEntry(pDriverObject, IO_DRIVER_ENTRY_INTERFACE_VERSION);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
    {
        LWIO_LOG_ERROR(IO_DRIVER_ENTRY_FUNCTION_NAME " did not initialize "
                       "driver '%s' from '%s'",
                       pszDriverName,
                       pszDriverPath);

        status = STATUS_DLL_INIT_FAILED;
        GOTO_CLEANUP_EE(EE);
    }

    SetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_READY);

    status = IopRootInsertDriver(pDriverObject->Root, pDriverObject);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        IopDriverUnload(&pDriverObject);
    }

    *ppDriverObject = pDriverObject;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoDriverInitialize(
    IN OUT IO_DRIVER_HANDLE DriverHandle,
    IN OPTIONAL PVOID DriverContext,
    IN PIO_DRIVER_SHUTDOWN_CALLBACK ShutdownCallback,
    IN PIO_DRIVER_DISPATCH_CALLBACK DispatchCallback
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    if (!ShutdownCallback || !DispatchCallback)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (IsSetFlag(DriverHandle->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
    {
        // Already initialized.
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    DriverHandle->Callback.Shutdown = ShutdownCallback;
    DriverHandle->Callback.Dispatch = DispatchCallback;
    DriverHandle->Context = DriverContext;

    SetFlag(DriverHandle->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED);

cleanup:
    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoDriverRegisterRefreshCallback(
    IN OUT IO_DRIVER_HANDLE DriverHandle,
    IN PIO_DRIVER_REFRESH_CALLBACK RefreshCallback
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    if (!RefreshCallback || !DriverHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (!IsSetFlag(DriverHandle->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
    {
        // Not initialized yet
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    DriverHandle->Callback.Refresh = RefreshCallback;

cleanup:
    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

PCSTR
IoDriverGetName(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    return DriverHandle->pszDriverName;
}

PVOID
IoDriverGetContext(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    return DriverHandle->Context;
}

VOID
IopDriverInsertDevice(
    IN PIO_DRIVER_OBJECT pDriverObject,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    )
{
    IopDriverLock(pDriverObject);

    LwListInsertTail(&pDriverObject->DeviceList,
                     pDeviceDriverLinks);
    pDriverObject->DeviceCount++;

    IopDriverUnlock(pDriverObject);
}

VOID
IopDriverRemoveDevice(
    IN PIO_DRIVER_OBJECT pDriverObject,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    )
{
    IopDriverLock(pDriverObject);

    if (!LwListIsEmpty(pDeviceDriverLinks))
    {
        LwListRemove(pDeviceDriverLinks);
        pDriverObject->DeviceCount--;
    }

    IopDriverUnlock(pDriverObject);
}

VOID
IopDriverLock(
    IN PIO_DRIVER_OBJECT pDriverObject
    )
{
    LwRtlLockMutex(&pDriverObject->Mutex);
}

VOID
IopDriverUnlock(
    IN PIO_DRIVER_OBJECT pDriverObject
    )
{
    LwRtlUnlockMutex(&pDriverObject->Mutex);
}

VOID
IopDriverReference(
    IN PIO_DRIVER_OBJECT pDriverObject
    )
{
    LONG count = InterlockedIncrement(&pDriverObject->ReferenceCount);
    LWIO_ASSERT(count > 1);
}

VOID
IopDriverDereference(
    IN OUT PIO_DRIVER_OBJECT* ppDriverObject
    )
{
    PIO_DRIVER_OBJECT pDriverObject = *ppDriverObject;

    if (pDriverObject)
    {
        // Rundown of the driver happens if it was ever inserted into the
        // root's driver list.  So there is no need to lock the root's driver
        // list here to synchronize with a "find" in the root's driver list.
        LONG count = InterlockedDecrement(&pDriverObject->ReferenceCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            IopDriverFree(&pDriverObject);
        }
        *ppDriverObject = NULL;
    }
}

// STATUS_SUCCESS
// STATUS_FILES_OPEN
// STATUS_FILE_CLOSED
static
NTSTATUS
IopDriverRundownEx(
    IN PIO_DRIVER_OBJECT pDriverObject,
    IN BOOLEAN ForceFileRundown
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    BOOLEAN isLocked = FALSE;
    PLW_LIST_LINKS pLinks = NULL;
    LW_LIST_LINKS rundownList = { 0 };

    LwListInit(&rundownList);

    IopDriverLock(pDriverObject);
    isLocked = TRUE;

    if (IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_RUNDOWN))
    {
        // TODO: Perhaps wait if currently running down.
        status = STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    //
    // To handle !ForceFileRundown, we need to grab *all* the
    // device locks to decide whether or not to go ahead
    // with the rundown.  This is because we cannot permit
    // an open to proceed -- presumably because we do not
    // want to have to rundown any opens.
    //

    if (!ForceFileRundown)
    {
        BOOLEAN canRundown = TRUE;

        for (pLinks = pDriverObject->DeviceList.Next;
             pLinks != &pDriverObject->DeviceList;
             pLinks = pLinks->Next)
        {
            PIO_DEVICE_OBJECT pDevice = LW_STRUCT_FROM_FIELD(pLinks, IO_DEVICE_OBJECT, DriverLinks);

            LwListInsertTail(&rundownList, &pDevice->RundownLinks);
            IopDeviceLock(pDevice);

            if (!LwListIsEmpty(&pDevice->FileObjectsList))
            {
                canRundown = FALSE;
                break;
            }
        }

        if (!canRundown)
        {
            // Unlock everybody and leave

            while (!LwListIsEmpty(&rundownList))
            {
                PIO_DEVICE_OBJECT pDevice = NULL;

                pLinks = LwListRemoveHead(&rundownList);
                pDevice = LW_STRUCT_FROM_FIELD(pLinks, IO_DEVICE_OBJECT, RundownLinks);

                IopDeviceUnlock(pDevice);
            }

            status = STATUS_FILES_OPEN;
            GOTO_CLEANUP_EE(EE);
        }
    }

    // We can rundown.

    SetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_RUNDOWN);

    for (pLinks = pDriverObject->DeviceList.Next;
         pLinks != &pDriverObject->DeviceList;
         pLinks = pLinks->Next)
    {
        PIO_DEVICE_OBJECT pDevice = LW_STRUCT_FROM_FIELD(pLinks, IO_DEVICE_OBJECT, DriverLinks);

        IopDeviceReference(pDevice);

        // If !ForceFileRundown, the devices are already locked and
        // inserted into the rundown list.
        if (ForceFileRundown)
        {
            IopDeviceLock(pDevice);
            LwListInsertTail(&rundownList, &pDevice->RundownLinks);
        }

        SetFlag(pDevice->Flags, IO_DEVICE_OBJECT_FLAG_RUNDOWN_DRIVER);

        IopDeviceUnlock(pDevice);
    }

    IopDriverUnlock(pDriverObject);
    isLocked = FALSE;

    // Now, actually run down every device w/o holding the driver lock.

    while (!LwListIsEmpty(&rundownList))
    {
        PIO_DEVICE_OBJECT pDevice = NULL;

        pLinks = LwListRemoveHead(&rundownList);
        pDevice = LW_STRUCT_FROM_FIELD(pLinks, IO_DEVICE_OBJECT, RundownLinks);

        IopDeviceRundown(pDevice);
        IopDeviceDereference(&pDevice);
    }

cleanup:
    if (isLocked)
    {
        IopDriverUnlock(pDriverObject);
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
