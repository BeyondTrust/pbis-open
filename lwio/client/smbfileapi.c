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
 *        smbfileapi.h
 *
 * Abstract:
 *
 *        SMB-specific API functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include "includes.h"

LW_NTSTATUS
LwIoGetSessionKey(
    IO_FILE_HANDLE File,
    LW_PUSHORT pKeyLength,
    LW_PBYTE* ppKeyBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_CONTEXT Context = {0};

    Status = LwIoAcquireContext(&Context);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCtxGetSessionKey(
        &Context,
        File,
        pKeyLength,
        ppKeyBuffer
        );
    BAIL_ON_NT_STATUS(Status);

error:

    LwIoReleaseContext(&Context);

    return Status;
}

LW_NTSTATUS
LwIoGetPeerAccessToken(
    IO_FILE_HANDLE File,
    PACCESS_TOKEN* ppToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_CONTEXT Context = {0};

    Status = LwIoAcquireContext(&Context);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCtxGetPeerAccessToken(
        &Context,
        File,
        ppToken
        );
    BAIL_ON_NT_STATUS(Status);

error:

    LwIoReleaseContext(&Context);

    return Status;
}


LW_NTSTATUS
LwIoGetPeerAddress(
    IO_FILE_HANDLE File,
    LW_PBYTE pAddress,
    LW_PUSHORT pusAddressLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_CONTEXT Context = {0};

    Status = LwIoAcquireContext(&Context);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCtxGetPeerAddress(
        &Context,
        File,
        pAddress,
        pusAddressLength
        );
    BAIL_ON_NT_STATUS(Status);
    
error:

    LwIoReleaseContext(&Context);

    return Status;
}

LW_NTSTATUS
LwIoConnectNamedPipe(
    IO_FILE_HANDLE File,
    PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_CONTEXT Context = {0};
    
    Status = LwIoAcquireContext(&Context);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCtxConnectNamedPipe(
        &Context,
        File,
        AsyncControlBlock,
        IoStatusBlock);
    BAIL_ON_NT_STATUS(Status);

error:

    LwIoReleaseContext(&Context);

    return Status;
}
