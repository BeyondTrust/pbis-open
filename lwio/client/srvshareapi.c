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
 *        srvshareapi.h
 *
 * Abstract:
 *
 *        Client API around SRV share IOCTLs
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include "includes.h"

static WCHAR gSrvDriverName[] = { '\\', 's', 'r', 'v', 0 };
static IO_FILE_NAME gSrvDriverFilename =
{
    .RootFileHandle = NULL,
    .Name = LW_RTL_CONSTANT_STRING(gSrvDriverName),
    .IoNameOptions = 0
};

NTSTATUS
LwIoSrvShareAdd(
    LW_IN PCWSTR pServer,
    LW_IN ULONG Level,
    LW_IN PVOID pInfo
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    SHARE_INFO_ADD_PARAMS AddParams = { 0 };
    static const ULONG IoControlCode = SRV_DEVCTL_ADD_SHARE;

    AddParams.dwInfoLevel = Level;
    switch (AddParams.dwInfoLevel) {
    case 0:
    case 1:
    case 2:
    case 501:
    case 502:
    case 1005:
        /* Since all arms of the union are pointers, this is valid */
        AddParams.info.p0 = pInfo;
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    ntStatus = LwShareInfoMarshalAddParameters(
                        &AddParams,
                        &pInBuffer,
                        &dwInLength
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RTL_ALLOCATE(&pOutBuffer, BYTE, dwOutLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
                        &hFile,
                        NULL,
                        &IoStatusBlock,
                        &gSrvDriverFilename,
                        NULL,
                        NULL,
                        DesiredAccess,
                        AllocationSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        NULL,
                        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(
                    hFile,
                    NULL,
                    &IoStatusBlock,
                    IoControlCode,
                    pInBuffer,
                    dwInLength,
                    pOutBuffer,
                    dwOutLength
                    );
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    RTL_FREE(&pInBuffer);
    RTL_FREE(&pOutBuffer);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwIoSrvShareDel(
    LW_IN PCWSTR pServer,
    LW_IN PCWSTR pNetname
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    SHARE_INFO_DELETE_PARAMS DeleteParams = { 0 };
    static const ULONG IoControlCode = SRV_DEVCTL_DELETE_SHARE;

    DeleteParams.servername = (PWSTR) pServer;
    DeleteParams.netname    = (PWSTR) pNetname;

    ntStatus = LwShareInfoMarshalDeleteParameters(
                        &DeleteParams,
                        &pInBuffer,
                        &dwInLength
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RTL_ALLOCATE(&pOutBuffer, BYTE, dwOutLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
                        &hFile,
                        NULL,
                        &IoStatusBlock,
                        &gSrvDriverFilename,
                        NULL,
                        NULL,
                        DesiredAccess,
                        AllocationSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        NULL,
                        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(
                        hFile,
                        NULL,
                        &IoStatusBlock,
                        IoControlCode,
                        pInBuffer,
                        dwInLength,
                        pOutBuffer,
                        dwOutLength
                        );
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    RTL_FREE(&pInBuffer);
    RTL_FREE(&pOutBuffer);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwIoSrvShareEnum(
    LW_IN PCWSTR pServer,
    LW_IN ULONG Level,
    LW_OUT PVOID* ppInfo,
    LW_OUT PULONG pCount
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    SHARE_INFO_ENUM_PARAMS EnumParamsIn = { 0 };
    PSHARE_INFO_ENUM_PARAMS pEnumParamsOut = NULL;
    static const ULONG IoControlCode = SRV_DEVCTL_ENUM_SHARE;

    EnumParamsIn.dwInfoLevel = Level;

    switch (EnumParamsIn.dwInfoLevel) {
    case 0:
    case 1:
    case 2:
    case 501:
    case 502:
    case 1005:
        break;
    default:
        ntStatus = STATUS_INVALID_LEVEL;
        BAIL_ON_NT_STATUS(ntStatus);
    }


    ntStatus = LwShareInfoMarshalEnumParameters(
        &EnumParamsIn,
        &pInBuffer,
        &dwInLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
        &hFile,
        NULL,
        &IoStatusBlock,
        &gSrvDriverFilename,
        NULL,
        NULL,
        DesiredAccess,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0,
        NULL,
        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RTL_ALLOCATE(&pOutBuffer, BYTE, dwOutLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(
        hFile,
        NULL,
        &IoStatusBlock,
        IoControlCode,
        pInBuffer,
        dwInLength,
        pOutBuffer,
        dwOutLength
    );

    while (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        RTL_FREE(&pOutBuffer);
        dwOutLength *= 2;

        ntStatus = RTL_ALLOCATE(&pOutBuffer, BYTE, dwOutLength);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NtDeviceIoControlFile(
            hFile,
            NULL,
            &IoStatusBlock,
            IoControlCode,
            pInBuffer,
            dwInLength,
            pOutBuffer,
            dwOutLength);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwShareInfoUnmarshalEnumParameters(
        pOutBuffer,
        IoStatusBlock.BytesTransferred,
        &pEnumParamsOut);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pEnumParamsOut->dwInfoLevel != Level)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppInfo = (PVOID) pEnumParamsOut->info.p0;
    *pCount = pEnumParamsOut->dwNumEntries;

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    RTL_FREE(&pInBuffer);
    RTL_FREE(&pOutBuffer);
    RTL_FREE(&pEnumParamsOut);

    return ntStatus;

error:

    *ppInfo = NULL;
    *pCount = 0;

    goto cleanup;
}

NTSTATUS
LwIoSrvShareGetInfo(
    IN PCWSTR pServer,
    IN PCWSTR pNetname,
    IN ULONG Level,
    OUT PVOID* ppInfo
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = {0};
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    SHARE_INFO_GETINFO_PARAMS GetParamsIn = {0};
    PSHARE_INFO_GETINFO_PARAMS pGetParamsOut = NULL;
    static const ULONG IoControlCode = SRV_DEVCTL_GET_SHARE_INFO;

    /* Validate info level */
    switch (Level){
    case 0:
    case 1:
    case 2:
    case 501:
    case 502:
    case 1005:
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memset(&GetParamsIn, 0, sizeof(GetParamsIn));

    GetParamsIn.pwszNetname = (PWSTR) pNetname;
    GetParamsIn.dwInfoLevel = Level;

    ntStatus = LwShareInfoMarshalGetParameters(
        &GetParamsIn,
        &pInBuffer,
        &dwInLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
        &hFile,
        NULL,
        &IoStatusBlock,
        &gSrvDriverFilename,
        NULL,
        NULL,
        DesiredAccess,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0,
        NULL,
        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RTL_ALLOCATE(&pOutBuffer, BYTE, dwOutLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(
        hFile,
        NULL,
        &IoStatusBlock,
        IoControlCode,
        pInBuffer,
        dwInLength,
        pOutBuffer,
        dwOutLength);
    BAIL_ON_NT_STATUS(ntStatus);

    while (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        /* We need more space in output buffer to make this call */
        RTL_FREE(&pOutBuffer);
        dwOutLength *= 2;

        ntStatus = RTL_ALLOCATE(&pOutBuffer, BYTE, dwOutLength);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NtDeviceIoControlFile(
            hFile,
            NULL,
            &IoStatusBlock,
            IoControlCode,
            pInBuffer,
            dwInLength,
            pOutBuffer,
            dwOutLength);
    }

    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwShareInfoUnmarshalGetParameters(
        pOutBuffer,
        IoStatusBlock.BytesTransferred,
        &pGetParamsOut);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pGetParamsOut->dwInfoLevel != Level)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppInfo = (PVOID) pGetParamsOut->Info.p0;

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    RTL_FREE(&pInBuffer);
    RTL_FREE(&pOutBuffer);
    RTL_FREE(&pGetParamsOut);

    return ntStatus;

error:

    *ppInfo = NULL;

    goto cleanup;
}

NTSTATUS
LwIoSrvShareSetInfo(
    IN PCWSTR pServer,
    IN PCWSTR pNetname,
    IN ULONG Level,
    IN PVOID pInfo
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    SHARE_INFO_SETINFO_PARAMS SetParams = { 0 };
    static const ULONG IoControlCode = SRV_DEVCTL_SET_SHARE_INFO;

    SetParams.dwInfoLevel = Level;
    SetParams.pwszNetname = (PWSTR) pNetname;

    switch (SetParams.dwInfoLevel) {
    case 0:
    case 1:
    case 2:
    case 501:
    case 502:
    case 1005:
        SetParams.Info.p0 = pInfo;
        break;
    default:
        ntStatus = STATUS_INVALID_LEVEL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwShareInfoMarshalSetParameters(
        &SetParams,
        &pInBuffer,
        &dwInLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RTL_ALLOCATE(&pOutBuffer, BYTE, dwOutLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
        &hFile,
        NULL,
        &IoStatusBlock,
        &gSrvDriverFilename,
        NULL,
        NULL,
        DesiredAccess,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        NULL,
        0,
        NULL,
        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(
        hFile,
        NULL,
        &IoStatusBlock,
        IoControlCode,
        pInBuffer,
        dwInLength,
        pOutBuffer,
        dwOutLength);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    RTL_FREE(&pInBuffer);
    RTL_FREE(&pOutBuffer);

    return ntStatus;

error:

    goto cleanup;
}

VOID
LwIoSrvShareFreeInfo(
    ULONG Level,
    ULONG Count,
    PVOID pInfo
    )
{
    return LwShareInfoFree(Level, Count, pInfo);
}

NTSTATUS
LwIoSrvShareReloadConfiguration(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    static const ULONG ioControlCode = SRV_DEVCTL_RELOAD_SHARES;

    ntStatus = NtCreateFile(&hFile, NULL, &ioStatusBlock, &gSrvDriverFilename,
                            NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(hFile, NULL, &ioStatusBlock, ioControlCode,
                                     NULL, 0, NULL, 0);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    return ntStatus;

error:

    goto cleanup;
}
