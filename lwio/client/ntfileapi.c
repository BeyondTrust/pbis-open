/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntfileapi.c
 *
 * Abstract:
 *
 *        NT File API Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

static
VOID
NtpInitializeIoStatusBlock(
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    memset(IoStatusBlock, 0, sizeof(*IoStatusBlock));
}

// Need to add a way to cancel operation from outside IRP layer.
// Probably requires something in IO_ASYNC_CONTROL_BLOCK.

//
// The operations below are in categories:
//
// - Core I/O
// - Additional
// - Namespace
// - Advanced
//

//
// Core I/O Operations
//

NTSTATUS
NtCreateNamedPipeFile(
    OUT PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN OPTIONAL PVOID SecurityDescriptor, // TBD
    IN OPTIONAL PVOID SecurityQualityOfService, // TBD
    IN ACCESS_MASK DesiredAccess,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN FILE_PIPE_TYPE_MASK NamedPipeType,
    IN FILE_PIPE_READ_MODE_MASK ReadMode,
    IN FILE_PIPE_COMPLETION_MODE_MASK CompletionMode,
    IN ULONG MaximumInstances,
    IN ULONG InboundQuota,
    IN ULONG OutboundQuota,
    IN OPTIONAL PLONG64 DefaultTimeout
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    *FileHandle = NULL;
    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxCreateNamedPipeFile(
                    &context,
                    NULL,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileName,
                    SecurityDescriptor,
                    SecurityQualityOfService,
                    DesiredAccess,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions,
                    NamedPipeType,
                    ReadMode,
                    CompletionMode,
                    MaximumInstances,
                    InboundQuota,
                    OutboundQuota,
                    DefaultTimeout);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtCreateFile(
    OUT PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN OPTIONAL PVOID SecurityDescriptor, // TBD
    IN OPTIONAL PVOID SecurityQualityOfService, // TBD
    IN ACCESS_MASK DesiredAccess,
    IN OPTIONAL LONG64 AllocationSize,
    IN FILE_ATTRIBUTES FileAttributes,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN OPTIONAL PVOID EaBuffer, // PFILE_FULL_EA_INFORMATION
    IN ULONG EaLength,
    IN OPTIONAL PIO_ECP_LIST EcpList
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    *FileHandle = NULL;
    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxCreateFile(
                    &context,
                    NULL,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileName,
                    SecurityDescriptor,
                    SecurityQualityOfService,
                    DesiredAccess,
                    AllocationSize,
                    FileAttributes,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions,
                    EaBuffer,
                    EaLength,
                    EcpList);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtCloseFile(
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    status = LwIoAcquireContext(&context);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxCloseFile(
                    &context,
                    FileHandle);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtReadFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxReadFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    Buffer,
                    Length,
                    ByteOffset,
                    Key);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxWriteFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    Buffer,
                    Length,
                    ByteOffset,
                    Key);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS 
NtDeviceIoControlFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxDeviceIoControlFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    IoControlCode,
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtFsControlFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxFsControlFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FsControlCode,
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtFlushBuffersFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxFlushBuffersFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS 
NtQueryInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxQueryInformationFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileInformation,
                    Length,
                    FileInformationClass);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS 
NtReadDirectoryChangeFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WatchTree,
    IN FILE_NOTIFY_CHANGE NotifyFilter,
    IN OPTIONAL PULONG MaxBufferSize
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxReadDirectoryChangeFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    Buffer,
                    Length,
                    WatchTree,
                    NotifyFilter,
                    MaxBufferSize);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS 
NtSetInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxSetInformationFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileInformation,
                    Length,
                    FileInformationClass);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

//
// Additional Operations
//

#if 0
NTSTATUS
NtQueryFullAttributesFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );
#endif

NTSTATUS 
NtQueryDirectoryFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PIO_MATCH_FILE_SPEC FileSpec,
    IN BOOLEAN RestartScan
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxQueryDirectoryFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileInformation,
                    Length,
                    FileInformationClass,
                    ReturnSingleEntry,
                    FileSpec,
                    RestartScan);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtQueryVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxQueryVolumeInformationFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FsInformation,
                    Length,
                    FsInformationClass);

cleanup:

    LwIoReleaseContext(&context);

    return status;
}

NTSTATUS
NtSetVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS 
NtLockFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxLockFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    ByteOffset,
		    Length,
		    Key,
		    FailImmediately,
		    ExclusiveLock);    

cleanup:

    LwIoReleaseContext(&context);

    return status;
}


NTSTATUS 
NtUnlockFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxUnlockFile(
                    &context,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    ByteOffset,
		    Length,
		    Key);

cleanup:

    LwIoReleaseContext(&context);

    return status;
}


//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

#if 0
NTSTATUS
NtRemoveDirectoryFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
NtDeleteFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
NtLinkFile(
    IN PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    );

NTSTATUS
NtRenameFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FromName,
    IN PIO_FILE_NAME ToName
    );
#endif

//
// Advanced Operations
//

NTSTATUS
NtQueryQuotaInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PVOID SidList,
    IN ULONG SidListLength,
    IN OPTIONAL PSID StartSid,
    IN BOOLEAN RestartScan
    );

NTSTATUS
NtSetQuotaInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
NtQuerySecurityFile(
    IN IO_FILE_HANDLE  Handle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    
    status = NtCtxQuerySecurityFile(
                    &context,
                    Handle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    SecurityInformation,
                    SecurityDescriptor,
                    Length);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

NTSTATUS
NtSetSecurityFile(
    IN IO_FILE_HANDLE Handle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_CONTEXT context = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = LwIoAcquireContext(&context);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    
    status = NtCtxSetSecurityFile(
                    &context,
                    Handle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    SecurityInformation,
                    SecurityDescriptor,
                    Length);

cleanup:
    LwIoReleaseContext(&context);
    return status;
}

// TODO: QueryEaFile and SetEaFile.

 
/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
