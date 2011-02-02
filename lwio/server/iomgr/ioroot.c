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
PIO_DRIVER_OBJECT
IopRootFindDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName
    );

static
PIO_DRIVER_ENTRY
IopRootFindStaticDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PSTR pszDriverName
    );

static
NTSTATUS
IopRootReadConfigDriver(
    IN PCSTR pszDriverName,
    OUT PSTR* ppszDriverPath
    );

static
VOID
IopRootLockDriverList(
    IN PIOP_ROOT_STATE pRoot
    );

static
VOID
IopRootUnlockDriverList(
    IN PIOP_ROOT_STATE pRoot
    );

static
VOID
IopRootLockDeviceList(
    IN PIOP_ROOT_STATE pRoot
    );

static
VOID
IopRootUnlockDeviceList(
    IN PIOP_ROOT_STATE pRoot
    );


VOID
IopRootFree(
    IN OUT PIOP_ROOT_STATE* ppRoot
    )
{
    PIOP_ROOT_STATE pRoot = *ppRoot;

    if (pRoot)
    {
        // Unload drivers in reverse load order
        while (!LwListIsEmpty(&pRoot->DriverObjectList))
        {
            PLW_LIST_LINKS pLinks = LwListRemoveTail(&pRoot->DriverObjectList);
            PIO_DRIVER_OBJECT pDriverObject = LW_STRUCT_FROM_FIELD(pLinks, IO_DRIVER_OBJECT, RootLinks);

            IopDriverUnload(&pDriverObject);
        }

        LwMapSecurityFreeContext(&pRoot->MapSecurityContext);
        LwRtlCleanupMutex(&pRoot->InitMutex);
        LwRtlCleanupMutex(&pRoot->DeviceMutex);
        LwRtlCleanupMutex(&pRoot->DriverMutex);
        IoMemoryFree(pRoot);
        *ppRoot = NULL;
    }
}

NTSTATUS
IopRootCreate(
    OUT PIOP_ROOT_STATE* ppRoot,
    IN OPTIONAL PIO_STATIC_DRIVER pStaticDrivers
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_ROOT_STATE pRoot = NULL;

    status = IO_ALLOCATE(&pRoot, IOP_ROOT_STATE, sizeof(*pRoot));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInit(&pRoot->DriverObjectList);
    LwListInit(&pRoot->DeviceObjectList);

    status = LwRtlInitializeMutex(&pRoot->DriverMutex, TRUE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeMutex(&pRoot->DeviceMutex, TRUE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeMutex(&pRoot->InitMutex, FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Try to initialize the map security context, but do not
    // error if it fails.
    status = LwMapSecurityCreateContext(&pRoot->MapSecurityContext);
    if (status)
    {
        LWIO_LOG_ERROR("cannot load map security context (status = 0x%08x)", status);
        status = 0;
    }

    pRoot->pStaticDrivers = pStaticDrivers;

cleanup:
    if (status)
    {
        IopRootFree(&pRoot);
    }

    *ppRoot = pRoot;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IopRootQueryStateDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName,
    OUT PLWIO_DRIVER_STATE pState
    )
{
    PIO_DRIVER_OBJECT pDriver = NULL;

    pDriver = IopRootFindDriver(pRoot, pDriverName);
    *pState = pDriver ? LWIO_DRIVER_STATE_LOADED : LWIO_DRIVER_STATE_UNLOADED;
    IopDriverDereference(&pDriver);

    return STATUS_SUCCESS;
}

NTSTATUS
IopRootLoadDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_DRIVER_OBJECT pDriverObject = NULL;
    PIO_DRIVER_OBJECT pFoundDriverObject = NULL;
    PIO_DRIVER_ENTRY pStaticDriverEntry = NULL;
    PSTR pszDriverName = NULL;
    PSTR pszDriverPath = NULL;

    status = RtlCStringAllocateFromUnicodeString(&pszDriverName, pDriverName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pFoundDriverObject = IopRootFindDriver(pRoot, pDriverName);
    if (pFoundDriverObject)
    {
        IopDriverDereference(&pFoundDriverObject);

        LWIO_LOG_VERBOSE("Attempted to load already loaded driver '%s'",
                         pszDriverName);

        status = STATUS_OBJECT_NAME_COLLISION;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    pStaticDriverEntry = IopRootFindStaticDriver(pRoot, pszDriverName);
    if (!pStaticDriverEntry)
    {
        status = IopRootReadConfigDriver(pszDriverName, &pszDriverPath);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IopDriverLoad(
                    &pDriverObject,
                    pRoot,
                    pDriverName,
                    pszDriverName,
                    pStaticDriverEntry,
                    pszDriverPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    LWIO_SAFE_FREE_MEMORY(pszDriverName);
    LWIO_SAFE_FREE_MEMORY(pszDriverPath);

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);

    return status;
}

NTSTATUS
IopRootUnloadDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_DRIVER_OBJECT pDriverObject = NULL;
    BOOLEAN isLocked = FALSE;
    PIO_DRIVER_OBJECT pUnloadDriverObject = NULL;

    pDriverObject = IopRootFindDriver(pRoot, pDriverName);
    if (!pDriverObject)
    {
        status = STATUS_NOT_FOUND;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    // Only allow one "unload" call to actually do the unload.

    IopDriverLock(pDriverObject);
    isLocked = TRUE;

    if (IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_UNLOADING))
    {
        // TODO: Perhaps an error or block until unload completes.

        status = STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    SetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_UNLOADING);

    IopDriverUnlock(pDriverObject);
    isLocked = FALSE;

    // Need to get rid of the "load" reference seperately from the "find"
    // reference from above.
    pUnloadDriverObject = pDriverObject;
    IopDriverUnload(&pUnloadDriverObject);

cleanup:
    if (isLocked)
    {
        IopDriverUnlock(pDriverObject);
    }

    IopDriverDereference(&pDriverObject);

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IopRootRefreshConfig(
    IN PIOP_ROOT_STATE pRoot
    )
{
    NTSTATUS status = 0;
    NTSTATUS subStatus = 0;
    PLW_LIST_LINKS pLinks = NULL;

    // This handles calls to IoMgrRefreshConfig() before the I/O Manager
    // is initialized.
    // TODO: Make lwiod track whether I/O Manager has been initialized
    //       and only call into IoMgrRefreshConfig() if it has been
    //       initialized.
    if (!pRoot)
    {
        goto cleanup;
    }

    IopRootLockDriverList(pRoot);

    for (pLinks = pRoot->DriverObjectList.Next;
         pLinks != &pRoot->DriverObjectList;
         pLinks = pLinks->Next)
    {
        PIO_DRIVER_OBJECT pDriverObject = LW_STRUCT_FROM_FIELD(pLinks, IO_DRIVER_OBJECT, RootLinks);

        if (pDriverObject->Callback.Refresh)
        {
            subStatus = pDriverObject->Callback.Refresh(pDriverObject);
            if (subStatus)
            {
                LWIO_LOG_ERROR("Failed to refresh driver: %s (0x%08x)",
                               LwNtStatusToName(subStatus), subStatus);
            }
            if (!status)
            {
                status = subStatus;
            }
        }
    }

    IopRootUnlockDriverList(pRoot);

cleanup:
    return status;
}

static
PIO_DRIVER_OBJECT
IopRootFindDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName
    )
{
    PIO_DRIVER_OBJECT pFoundDriver = NULL;
    PLW_LIST_LINKS pLinks = NULL;

    IopRootLockDriverList(pRoot);

    for (pLinks = pRoot->DriverObjectList.Next;
         pLinks != &pRoot->DriverObjectList;
         pLinks = pLinks->Next)
    {
        PIO_DRIVER_OBJECT pDriverObject = LW_STRUCT_FROM_FIELD(pLinks, IO_DRIVER_OBJECT, RootLinks);

        if (RtlUnicodeStringIsEqual(pDriverName, &pDriverObject->DriverName, TRUE))
        {
            pFoundDriver = pDriverObject;
            IopDriverReference(pFoundDriver);
            break;
        }
    }

    IopRootUnlockDriverList(pRoot);

    return pFoundDriver;
}

PIO_DEVICE_OBJECT
IopRootFindDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDeviceName
    )
{
    PIO_DEVICE_OBJECT pFoundDevice = NULL;
    PLW_LIST_LINKS pLinks = NULL;

    IopRootLockDeviceList(pRoot);

    for (pLinks = pRoot->DeviceObjectList.Next;
         pLinks != &pRoot->DeviceObjectList;
         pLinks = pLinks->Next)
    {
        PIO_DEVICE_OBJECT pDevice = LW_STRUCT_FROM_FIELD(pLinks, IO_DEVICE_OBJECT, RootLinks);
        if (RtlUnicodeStringIsEqual(pDeviceName, &pDevice->DeviceName, FALSE))
        {
            pFoundDevice = pDevice;
            IopDeviceReference(pDevice);
            break;
        }
    }

    IopRootUnlockDeviceList(pRoot);

    return pFoundDevice;
}

NTSTATUS
IopRootInsertDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PIO_DRIVER_OBJECT pDriver
    )
{
    NTSTATUS status = 0;
    PIO_DRIVER_OBJECT pFoundDriver = NULL;

    IopRootLockDriverList(pRoot);

    // Check again that there are no collisions
    pFoundDriver = IopRootFindDriver(pRoot, &pDriver->DriverName);
    if (pFoundDriver)
    {
        status = STATUS_OBJECT_NAME_COLLISION;
    }
    else
    {
        LwListInsertTail(&pRoot->DriverObjectList,
                         &pDriver->RootLinks);
        pRoot->DriverCount++;
    }

    IopRootUnlockDriverList(pRoot);

    return status;
}

VOID
IopRootRemoveDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    )
{
    IopRootLockDriverList(pRoot);

    if (!LwListIsEmpty(pDriverRootLinks))
    {
        LwListRemove(pDriverRootLinks);
        pRoot->DriverCount--;
    }

    IopRootUnlockDriverList(pRoot);
}

NTSTATUS
IopRootInsertDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PIO_DEVICE_OBJECT pDevice
    )
{
    NTSTATUS status = 0;
    PIO_DEVICE_OBJECT pFoundDevice = NULL;

    IopRootLockDeviceList(pRoot);

    // Check again that there are no collisions
    pFoundDevice = IopRootFindDevice(pRoot, &pDevice->DeviceName);
    if (pFoundDevice)
    {
        IopDeviceDereference(&pFoundDevice);
        status = STATUS_OBJECT_NAME_COLLISION;
    }
    else
    {
        LwListInsertTail(&pRoot->DeviceObjectList,
                         &pDevice->RootLinks);
        pRoot->DeviceCount++;
    }

    IopRootUnlockDeviceList(pRoot);

    return status;
}

VOID
IopRootRemoveDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    )
{
    IopRootLockDeviceList(pRoot);

    if (!LwListIsEmpty(pDeviceRootLinks))
    {
        LwListRemove(pDeviceRootLinks);
        pRoot->DeviceCount--;
    }

    IopRootUnlockDeviceList(pRoot);
}

NTSTATUS
IopRootParse(
    IN PIOP_ROOT_STATE pRoot,
    IN PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice,
    OUT PUNICODE_STRING pRemainingPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    UNICODE_STRING deviceName = { 0 };
    UNICODE_STRING remainingPath = { 0 };
    PIO_DEVICE_OBJECT pDevice = NULL;

    if (pFileName->RootFileHandle)
    {
        // Relative path
        //
        // Allowed cases:
        //
        // 1) Open self - no relative name
        // 2) Subpath - does not start with separator
        // 3) Open by ID - higher layer will convert this to (1)
        //

        if ((pFileName->Name.Length > 0) &&
            IoRtlPathIsSeparator(pFileName->Name.Buffer[0]))
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_EE(EE);
        }

        pDevice = pFileName->RootFileHandle->pDevice;
        IopDeviceReference(pDevice);

        remainingPath = pFileName->Name;

        status = STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    // Absolute path.

    if (!pFileName->Name.Length)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (!IoRtlPathIsSeparator(pFileName->Name.Buffer[0]))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    IoRtlPathDissect(&pFileName->Name, &deviceName, &remainingPath);

    pDevice = IopRootFindDevice(pRoot, &deviceName);
    if (!pDevice)
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        IopDeviceDereference(&pDevice);
        LwRtlZeroMemory(&remainingPath, sizeof(remainingPath));
    }

    *ppDevice = pDevice;
    *pRemainingPath = remainingPath;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IopRootGetMapSecurityContext(
    IN PIOP_ROOT_STATE pRoot,
    OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;

    LwRtlLockMutex(&pRoot->InitMutex);

    if (!pRoot->MapSecurityContext)
    {
        status = LwMapSecurityCreateContext(&pRoot->MapSecurityContext);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    pContext = pRoot->MapSecurityContext;

cleanup:
    LwRtlUnlockMutex(&pRoot->InitMutex);

    *ppContext = pContext;

    return status;
}

static
NTSTATUS
IopRootReadConfigDriver(
    IN PCSTR pszDriverName,
    OUT PSTR* ppszDriverPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PSTR pszDriverPath = NULL;
    PSTR pszDriverKey = NULL;
    PLWIO_CONFIG_REG pReg = NULL;

    status = LwRtlCStringAllocatePrintf(
                    &pszDriverKey,
                    "Services\\lwio\\Parameters\\Drivers\\%s",
                    pszDriverName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwIoOpenConfig(
                    pszDriverKey,
                    NULL,
                    &pReg);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwIoReadConfigString(pReg, "Path", FALSE, &pszDriverPath);
    if (status)
    {
        LWIO_LOG_ERROR("Status 0x%08x (%s) reading path config for "
                       "driver '%s'",
                       status, LwNtStatusToName(status), pszDriverName);
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (IsNullOrEmptyString(pszDriverPath))
    {
        LWIO_LOG_ERROR("Empty path for driver '%s'", pszDriverName);
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        RTL_FREE(&pszDriverPath);
    }

    RTL_FREE(&pszDriverKey);
    LwIoCloseConfig(pReg);

    *ppszDriverPath = pszDriverPath;

    return status;
}

static
PIO_DRIVER_ENTRY
IopRootFindStaticDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PSTR pszDriverName
    )
{
    PIO_DRIVER_ENTRY pFoundDriverEntry = NULL;
    DWORD i = 0;

    if (pRoot->pStaticDrivers)
    {
        /* First, look for driver in static list */
        for (i = 0; pRoot->pStaticDrivers[i].pszName != NULL; i++)
        {
            if (!strcmp(pRoot->pStaticDrivers[i].pszName, pszDriverName))
            {
                pFoundDriverEntry = pRoot->pStaticDrivers[i].pEntry;
                LWIO_LOG_DEBUG("Driver '%s' found in static list", pszDriverName);
                break;
            }
        }
    }

    return pFoundDriverEntry;
}

static
VOID
IopRootLockDriverList(
    IN PIOP_ROOT_STATE pRoot
    )
{
    LwRtlLockMutex(&pRoot->DriverMutex);
}

static
VOID
IopRootUnlockDriverList(
    IN PIOP_ROOT_STATE pRoot
    )
{
    LwRtlUnlockMutex(&pRoot->DriverMutex);
}

static
VOID
IopRootLockDeviceList(
    IN PIOP_ROOT_STATE pRoot
    )
{
    LwRtlLockMutex(&pRoot->DeviceMutex);
}

static
VOID
IopRootUnlockDeviceList(
    IN PIOP_ROOT_STATE pRoot
    )
{
    LwRtlUnlockMutex(&pRoot->DeviceMutex);
}
