/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
