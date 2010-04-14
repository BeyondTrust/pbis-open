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

static volatile PIOP_ROOT_STATE gpIoRoot = NULL;
static volatile PIO_STATIC_DRIVER gpStaticDrivers = NULL;
static pthread_mutex_t gDriverLock = PTHREAD_MUTEX_INITIALIZER;

VOID
IoCleanup(
    )
{
    PIOP_ROOT_STATE pRoot = gpIoRoot;
    IopRootFree(&pRoot);
    gpIoRoot = pRoot;
}

NTSTATUS
IoInitialize(
    IN PCSTR pszConfigFilePath,
    IN PIO_STATIC_DRIVER pStaticDrivers
    )
{
    NTSTATUS status = 0;
    PIOP_ROOT_STATE pRoot = NULL;

    status = IopRootCreate(&pRoot, pszConfigFilePath);
    GOTO_CLEANUP_ON_STATUS(status);

    gpIoRoot = pRoot;
    gpStaticDrivers = pStaticDrivers;

cleanup:
    if (status)
    {
        IopRootFree(&pRoot);
    }

    gpIoRoot = pRoot;

    return status;
}

LWIO_DRIVER_STATUS
IoMgrGetDriverStatus(
    PWSTR pwszDriverName
    )
{
    LWIO_DRIVER_STATUS status = 0;

    LwThreadsAcquireMutex(&gDriverLock);

    status = IopRootFindDriver(gpIoRoot, pwszDriverName) ? 
        LWIO_DRIVER_LOADED : LWIO_DRIVER_UNLOADED;

    LwThreadsReleaseMutex(&gDriverLock);

    return status;
}

NTSTATUS
IoMgrLoadDriver(
    PWSTR pwszDriverName
    )
{
    NTSTATUS status = 0;

    LwThreadsAcquireMutex(&gDriverLock);
    
    status = IopRootLoadDriver(gpIoRoot, gpStaticDrivers, pwszDriverName);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    LwThreadsReleaseMutex(&gDriverLock);

    return status;
}

NTSTATUS
IoMgrUnloadDriver(
    PWSTR pwszDriverName
    )
{
    NTSTATUS status = 0;
    PIO_DRIVER_OBJECT pDriverObject = NULL;

    LwThreadsAcquireMutex(&gDriverLock);

    pDriverObject = IopRootFindDriver(gpIoRoot, pwszDriverName);
    
    if (!pDriverObject)
    {
        status = STATUS_NOT_FOUND;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    IopDriverUnload(&pDriverObject);

cleanup:

    LwThreadsReleaseMutex(&gDriverLock);

    return status;
}

NTSTATUS
IoMgrRefresh(
    VOID
    )
{
    NTSTATUS status = 0;

    LwThreadsAcquireMutex(&gDriverLock);

    if (gpIoRoot)
    {
        status = IopRootRefreshDrivers(gpIoRoot);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:

    LwThreadsReleaseMutex(&gDriverLock);

    return status;
}

NTSTATUS
IopParse(
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
    )
{
    PIOP_ROOT_STATE pRoot = gpIoRoot;
    
    return IopRootParse(pRoot, pFileName, ppDevice);
}

NTSTATUS
IopGetMapSecurityContext(
    OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    )
{
    PIOP_ROOT_STATE pRoot = gpIoRoot;

    return IopRootGetMapSecurityContext(pRoot, ppContext);
}
