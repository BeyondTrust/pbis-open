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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        driver.c
 *
 * Abstract:
 *
 *        IO Test (IT) Driver
 *
 *        Driver Entry Function
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

#include <lwio/ioapi.h>
#include <lw/rtlstring.h>


PIT_DRIVER_STATE
ItGetDriverState(
    IN PIRP pIrp
    )
{
    return (PIT_DRIVER_STATE) IoDriverGetContext(pIrp->DriverHandle);
}

static
VOID
ItpDestroyDriverState(
    IN OUT PIT_DRIVER_STATE* ppState
    )
{
    PIT_DRIVER_STATE pState = *ppState;

    if (pState)
    {
        ItDestroyWorkQueue(&pState->pWorkQueue);
        RTL_FREE(&pState);
        *ppState = NULL;
    }
}

static
NTSTATUS
ItpCreateDriverState(
    OUT PIT_DRIVER_STATE* ppState
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIT_DRIVER_STATE pState = NULL;

    status = RTL_ALLOCATE(&pState, IT_DRIVER_STATE, sizeof(*pState));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = ItCreateWorkQueue(&pState->pWorkQueue);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        ItpDestroyDriverState(&pState);
    }

    *ppState = pState;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
VOID
ItDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    PIT_DRIVER_STATE pState = NULL;

    IO_LOG_ENTER("");

    pState = (PIT_DRIVER_STATE) IoDriverGetContext(DriverHandle);
    ItpDestroyDriverState(&pState);

    IO_LOG_LEAVE("");
}

static
NTSTATUS
ItDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    int EE = 0;

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:
            status = ItDispatchCreate(pIrp);
            break;

        case IRP_TYPE_CLOSE:
            status = ItDispatchClose(pIrp);
            break;

        case IRP_TYPE_READ:
            status = ItDispatchRead(pIrp);
            break;

        case IRP_TYPE_WRITE:
            status = ItDispatchWrite(pIrp);
            break;

        case IRP_TYPE_DEVICE_IO_CONTROL:
            status = ItDispatchDeviceIoControl(pIrp);
            break;

        case IRP_TYPE_FS_CONTROL:
            status = ItDispatchFsControl(pIrp);
            break;

        case IRP_TYPE_FLUSH_BUFFERS:
            status = ItDispatchFlushBuffers(pIrp);
            break;

        case IRP_TYPE_QUERY_INFORMATION:
            status = ItDispatchQueryInformation(pIrp);
            break;

        case IRP_TYPE_SET_INFORMATION:
            status = ItDispatchSetInformation(pIrp);
            break;

        case IRP_TYPE_CREATE_NAMED_PIPE:
            status = ItDispatchCreateNamedPipe(pIrp);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "Type = %u (%s)", pIrp->Type,
                              IoGetIrpTypeString(pIrp->Type));
    return status;
}

// TODO -- Add driver context
// TODO -- Add device context

NTSTATUS
DriverEntry(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_DEVICE_HANDLE deviceHandle = NULL;
    PIT_DRIVER_STATE pState = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    status = ItpCreateDriverState(&pState);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoDriverInitialize(DriverHandle,
                                pState,
                                ItDriverShutdown,
                                ItDriverDispatch);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoDeviceCreate(&deviceHandle,
                            DriverHandle,
                            IOTEST_DEVICE_NAME,
                            NULL);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LWIO_ASSERT(ItTestStartup(IOTEST_DEVICE_PATH) == STATUS_SUCCESS);
    LWIO_ASSERT(ItTestStartup(IOTEST_PATH_ALLOW) == STATUS_SUCCESS);

cleanup:
    if (status)
    {
        // Shutdown is called only if initialization succeeds.
        ItpDestroyDriverState(&pState);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
