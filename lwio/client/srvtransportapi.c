/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *     srvtransportapi.c
 *
 * Abstract:
 *
 *     SRV client API for transport control
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"
#include <lwio/srvtransportapi.h>

#define SRV_DEVICE_NAME_INITIALIZER { '\\', 's', 'r', 'v', 0 }

#define LOG_LEAVE_ON_STATUS_EE(status, EE) \
    do { \
        if (EE || status) \
        { \
            LW_RTL_LOG_DEBUG("-> 0x%08x (EE = %d)", status, EE); \
        } \
    } while (0)

#define SRV_CLOSE_HANDLE(Handle) \
    do { \
        if (*(Handle)) \
        { \
            NtCloseFile(*(Handle)); \
            *(Handle) = NULL; \
        } \
    } while (0)

static
NTSTATUS
LwIoSrvOpenDevice(
    OUT PIO_FILE_HANDLE pHandle,
    IN ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    WCHAR deviceName[] = SRV_DEVICE_NAME_INITIALIZER;
    IO_FILE_NAME name = {
        .RootFileHandle = NULL,
        .Name = LW_RTL_CONSTANT_STRING(deviceName),
        .IoNameOptions = 0,
    };
    
    status = NtCreateFile(
                    &handle,
                    NULL,
                    &ioStatusBlock,
                    &name,
                    NULL,
                    NULL,
                    DesiredAccess,
                    0,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    0,
                    NULL,
                    0,
                    NULL,
                    NULL);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        SRV_CLOSE_HANDLE(&handle);
    }

    *pHandle = handle;

    LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

LW_NTSTATUS
LwIoSrvTransportQuery(
    LW_OUT LW_PBOOLEAN pIsStared
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    BOOLEAN isStarted = FALSE;

    status = LwIoSrvOpenDevice(&handle, FILE_READ_ATTRIBUTES);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtDeviceIoControlFile(
                    handle,
                    NULL,
                    &ioStatusBlock,
                    SRV_DEVCTL_QUERY_TRANSPORT,
                    NULL,
                    0,
                    &isStarted,
                    sizeof(isStarted));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        isStarted = FALSE;
    }

    SRV_CLOSE_HANDLE(&handle);

    *pIsStared = isStarted;

    LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

LW_NTSTATUS
LwIoSrvTransportStart(
    LW_VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    status = LwIoSrvOpenDevice(&handle, FILE_WRITE_DATA);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtDeviceIoControlFile(
                    handle,
                    NULL,
                    &ioStatusBlock,
                    SRV_DEVCTL_START_TRANSPORT,
                    NULL,
                    0,
                    NULL,
                    0);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    SRV_CLOSE_HANDLE(&handle);

    LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}


LW_NTSTATUS
LwIoSrvTransportStop(
    LW_IN LW_BOOLEAN IsForce
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    status = LwIoSrvOpenDevice(&handle, FILE_WRITE_DATA);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtDeviceIoControlFile(
                    handle,
                    NULL,
                    &ioStatusBlock,
                    SRV_DEVCTL_STOP_TRANSPORT,
                    &IsForce,
                    sizeof(IsForce),
                    NULL,
                    0);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    SRV_CLOSE_HANDLE(&handle);

    LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}
