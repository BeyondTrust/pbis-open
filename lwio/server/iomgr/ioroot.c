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

    status = LwRtlInitializeMutex(&pRoot->DriverMutex, FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = LwRtlInitializeMutex(&pRoot->DeviceMutex, FALSE);
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

    LwRtlLockMutex(&pRoot->DriverMutex);

    pDriver = IopRootFindDriver(pRoot, pDriverName);
    *pState = pDriver ? LWIO_DRIVER_STATE_LOADED : LWIO_DRIVER_STATE_UNLOADED;
    IopDriverDereference(&pDriver);

    LwRtlUnlockMutex(&pRoot->DriverMutex);

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

    LwRtlLockMutex(&pRoot->DriverMutex);

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
    LwRtlUnlockMutex(&pRoot->DriverMutex);

    LWIO_SAFE_FREE_MEMORY(pszDriverName);

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

    LwRtlLockMutex(&pRoot->DriverMutex);

    pDriverObject = IopRootFindDriver(pRoot, pDriverName);
    if (!pDriverObject)
    {
        status = STATUS_NOT_FOUND;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    IopDriverUnload(&pDriverObject);

cleanup:
    LwRtlUnlockMutex(&pRoot->DriverMutex);

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);

    return status;
}

PIO_DRIVER_OBJECT
IopRootFindDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName
    )
{
    PIO_DRIVER_OBJECT pFoundDriver = NULL;
    PLW_LIST_LINKS pLinks = NULL;

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

    return pFoundDriver;
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

    LwRtlLockMutex(&pRoot->DriverMutex);

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

    LwRtlUnlockMutex(&pRoot->DriverMutex);

cleanup:
    return status;
}

PIO_DEVICE_OBJECT
IopRootFindDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDeviceName
    )
{
    PLW_LIST_LINKS pLinks = NULL;
    PIO_DEVICE_OBJECT pFoundDevice = NULL;

    for (pLinks = pRoot->DeviceObjectList.Next;
         pLinks != &pRoot->DeviceObjectList;
         pLinks = pLinks->Next)
    {
        PIO_DEVICE_OBJECT pDevice = LW_STRUCT_FROM_FIELD(pLinks, IO_DEVICE_OBJECT, RootLinks);
        if (RtlUnicodeStringIsEqual(pDeviceName, &pDevice->DeviceName, FALSE))
        {
            pFoundDevice = pDevice;
            break;
        }
    }

    return pFoundDevice;
}

VOID
IopRootInsertDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    )
{
    LwListInsertTail(&pRoot->DriverObjectList,
                     pDriverRootLinks);
    pRoot->DriverCount++;
}

VOID
IopRootRemoveDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    )
{
    LwListRemove(pDriverRootLinks);
    pRoot->DriverCount--;
}



VOID
IopRootInsertDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    )
{
    LwListInsertTail(&pRoot->DeviceObjectList,
                     pDeviceRootLinks);
    pRoot->DeviceCount++;
}

VOID
IopRootRemoveDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    )
{
    LwListRemove(pDeviceRootLinks);
    pRoot->DeviceCount--;
}

NTSTATUS
IopRootParse(
    IN PIOP_ROOT_STATE pRoot,
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PWSTR pszCurrent = NULL;
    UNICODE_STRING deviceName = { 0 };
    PIO_DEVICE_OBJECT pDevice = NULL;

    if (pFileName->RootFileHandle)
    {
        // Relative path

        if (pFileName->FileName &&
            (!pFileName->FileName[0] || IoRtlPathIsSeparator(pFileName->FileName[0])))
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_EE(EE);
        }

        pDevice = pFileName->RootFileHandle->pDevice;

        status = STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    // Absolute path.

    if (!pFileName->FileName)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (!IoRtlPathIsSeparator(pFileName->FileName[0]))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    pszCurrent = pFileName->FileName + 1;
    while (pszCurrent[0] && !IoRtlPathIsSeparator(pszCurrent[0]))
    {
        pszCurrent++;
    }

    deviceName.Buffer = (PWSTR) pFileName->FileName + 1;
    deviceName.Length = (pszCurrent - deviceName.Buffer) * sizeof(deviceName.Buffer[0]);
    deviceName.MaximumLength = deviceName.Length;

    pDevice = IopRootFindDevice(pRoot, &deviceName);
    if (!pDevice)
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        GOTO_CLEANUP_EE(EE);
    }

    pFileName->FileName = pszCurrent;

cleanup:
    *ppDevice = pDevice;

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

