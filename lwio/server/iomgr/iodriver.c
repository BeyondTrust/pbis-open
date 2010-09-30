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
            // TODO -- Add code to cancel IO and wait for IO to complete.
            IopRootRemoveDriver(pDriverObject->Root, &pDriverObject->RootLinks);
        }
        if (IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
        {
            pDriverObject->Callback.Shutdown(pDriverObject);

            // TODO -- Verify that devices have been removed
            // TODO -- refcount?
        }
        if (pDriverObject->LibraryHandle)
        {
            int err = dlclose(pDriverObject->LibraryHandle);
            if (err)
            {
                LWIO_LOG_ERROR("Failed to dlclose() for driver '%s' from '%s'",
                               pDriverObject->pszDriverName,
                               pDriverObject->pszDriverPath);
            }
        }
        RtlUnicodeStringFree(&pDriverObject->DriverName);
        RTL_FREE(&pDriverObject->pszDriverName);
        RTL_FREE(&pDriverObject->pszDriverPath);
        IoMemoryFree(pDriverObject);
        *ppDriverObject = NULL;
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

    status = IO_ALLOCATE(&pDriverObject, IO_DRIVER_OBJECT, sizeof(*pDriverObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInit(&pDriverObject->DeviceList);

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

        dlerror();
        
        pDriverObject->LibraryHandle = dlopen(pszDriverPath, RTLD_NOW | RTLD_GLOBAL);
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
    IopRootInsertDriver(pDriverObject->Root, &pDriverObject->RootLinks);

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
    IN PIO_DRIVER_OBJECT pDriver,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    )
{
    LwListInsertTail(&pDriver->DeviceList,
                     pDeviceDriverLinks);
    pDriver->DeviceCount++;
}

VOID
IopDriverRemoveDevice(
    IN PIO_DRIVER_OBJECT pDriver,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    )
{
    LwListRemove(pDeviceDriverLinks);
    pDriver->DeviceCount--;
}

VOID
IopDriverReference(
    IN PIO_DRIVER_OBJECT pDriver
    )
{
    LONG count = InterlockedIncrement(&pDriver->ReferenceCount);
    LWIO_ASSERT(count > 1);
}

VOID
IopDriverDereference(
    IN OUT PIO_DRIVER_OBJECT* ppDriver
    )
{
    PIO_DRIVER_OBJECT pDriver = *ppDriver;

    if (pDriver)
    {
        // Note that we do not have to acquire the device
        // lock here since it is impossible to resolve
        // a file object from a device.
        LONG count = InterlockedDecrement(&pDriver->ReferenceCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            LWIO_ASSERT_MSG(FALSE, "Refcount reached zero for driver object");
            // IopDriverFree(&pDriver);
        }
        *ppDriver = NULL;
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
