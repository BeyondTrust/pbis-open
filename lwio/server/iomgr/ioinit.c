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
