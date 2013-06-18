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

static PIOP_ROOT_STATE gpIoRoot = NULL;

VOID
IoCleanup(
    VOID
    )
{
    IopRootFree(&gpIoRoot);
}

NTSTATUS
IoInitialize(
    IN OPTIONAL PIO_STATIC_DRIVER pStaticDrivers
    )
{
    LWIO_ASSERT(!gpIoRoot);

    return IopRootCreate(&gpIoRoot, pStaticDrivers);
}

NTSTATUS
IoMgrQueryStateDriver(
    IN PWSTR pwszDriverName,
    OUT PLWIO_DRIVER_STATE pDriverState
    )
{
    NTSTATUS status = 0;
    UNICODE_STRING driverName = { 0 };
    LWIO_DRIVER_STATE driverState = LWIO_DRIVER_STATE_UNKNOWN;

    status = RtlUnicodeStringInitEx(&driverName, pwszDriverName);
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopRootQueryStateDriver(gpIoRoot, &driverName, &driverState);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (status)
    {
        driverState = LWIO_DRIVER_STATE_UNKNOWN;
    }

    *pDriverState = driverState;

    return status;
}

NTSTATUS
IoMgrLoadDriver(
    IN PWSTR pwszDriverName
    )
{
    NTSTATUS status = 0;
    UNICODE_STRING driverName = { 0 };

    status = RtlUnicodeStringInitEx(&driverName, pwszDriverName);
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopRootLoadDriver(gpIoRoot, &driverName);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    return status;
}

NTSTATUS
IoMgrUnloadDriver(
    IN PWSTR pwszDriverName
    )
{
    NTSTATUS status = 0;
    UNICODE_STRING driverName = { 0 };

    status = RtlUnicodeStringInitEx(&driverName, pwszDriverName);
    GOTO_CLEANUP_ON_STATUS(status);

    IopRootUnloadDriver(gpIoRoot, &driverName);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    return status;
}

NTSTATUS
IoMgrRefreshConfig(
    VOID
    )
{
    return IopRootRefreshConfig(gpIoRoot);
}

NTSTATUS
IopParse(
    IN PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice,
    OUT PUNICODE_STRING pRemainingPath
    )
{
    return IopRootParse(gpIoRoot, pFileName, ppDevice, pRemainingPath);
}

NTSTATUS
IopGetMapSecurityContext(
    OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    )
{
    return IopRootGetMapSecurityContext(gpIoRoot, ppContext);
}
