/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        ntfileapi.h
 *
 * Abstract:
 *
 *        NT File API
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __NT_FILE_API_H__
#define __NT_FILE_API_H__

#include <lwio/io-types.h>
#include <lwio/iortl.h>

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
LwNtCtxCreateNamedPipeFile(
    IN PIO_CONTEXT pConnection,
    IN LW_PIO_CREDS pSecurityToken,
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
    );

NTSTATUS
LwNtCtxCreateFile(
    IN PIO_CONTEXT pConnection,
    IN LW_PIO_CREDS pSecurityToken,
    OUT PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
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
    );

NTSTATUS
LwNtCtxCloseFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle
    );

NTSTATUS
LwNtCtxReadFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    );

NTSTATUS
LwNtCtxWriteFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    );

NTSTATUS 
LwNtCtxDeviceIoControlFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
LwNtCtxFsControlFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
LwNtCtxFlushBuffersFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS 
LwNtCtxQueryInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    );

NTSTATUS 
LwNtCtxSetInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    );

//
// Additional Operations
//

#if 0
NTSTATUS
LwNtCtxQueryFullAttributesFile(
    IN PIO_CONTEXT pConnection,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );
#endif

NTSTATUS 
LwNtCtxQueryDirectoryFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PIO_MATCH_FILE_SPEC FileSpec,
    IN BOOLEAN RestartScan
    );

NTSTATUS 
LwNtCtxReadDirectoryChangeFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WatchTree,
    IN FILE_NOTIFY_CHANGE NotifyFilter,
    IN OPTIONAL PULONG MaxBufferSize
    );

NTSTATUS
LwNtCtxQueryVolumeInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS
LwNtCtxSetVolumeInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS 
LwNtCtxLockFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    );

NTSTATUS 
LwNtCtxUnlockFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key
    );

//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

#if 0
NTSTATUS
LwNtCtxRemoveDirectoryFile(
    IN PIO_CONTEXT pConnection,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
LwNtCtxDeleteFile(
    IN PIO_CONTEXT pConnection,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
LwNtCtxLinkFile(
    IN PIO_CONTEXT pConnection,
    IN PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    );

NTSTATUS
LwNtCtxRenameFile(
    IN PIO_CONTEXT pConnection,
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
LwNtCtxQueryQuotaInformationFile(
    IN PIO_CONTEXT pConnection,
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
LwNtCtxSetQuotaInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
LwNtCtxQuerySecurityFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE  Handle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    ); 

NTSTATUS
LwNtCtxSetSecurityFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE Handle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    ); 

#ifndef LW_NO_THREADS

NTSTATUS
LwNtCreateNamedPipeFile(
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
    );

LW_NTSTATUS
LwNtCreateFile(
    LW_OUT PIO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName,
    LW_IN LW_OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    LW_IN LW_OPTIONAL LW_PVOID SecurityQualityOfService, // TBD
    LW_IN ACCESS_MASK DesiredAccess,
    LW_IN LW_OPTIONAL LONG64 AllocationSize,
    LW_IN FILE_ATTRIBUTES FileAttributes,
    LW_IN FILE_SHARE_FLAGS ShareAccess,
    LW_IN FILE_CREATE_DISPOSITION CreateDisposition,
    LW_IN FILE_CREATE_OPTIONS CreateOptions,
    LW_IN LW_OPTIONAL LW_PVOID EaBuffer, // PFILE_FULL_EA_INFORMATION
    LW_IN LW_ULONG EaLength,
    LW_IN LW_OPTIONAL PIO_ECP_LIST EcpList
    );

LW_NTSTATUS
LwNtCloseFile(
    LW_IN IO_FILE_HANDLE FileHandle
    );

LW_NTSTATUS
LwNtReadFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID Buffer,
    LW_IN LW_ULONG Length,
    LW_IN LW_OPTIONAL LW_PLONG64 ByteOffset,
    LW_IN LW_OPTIONAL LW_PULONG Key
    );

LW_NTSTATUS
LwNtWriteFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID Buffer,
    LW_IN LW_ULONG Length,
    LW_IN LW_OPTIONAL LW_PLONG64 ByteOffset,
    LW_IN LW_OPTIONAL LW_PULONG Key
    );

LW_NTSTATUS 
LwNtDeviceIoControlFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_ULONG IoControlCode,
    LW_IN LW_PVOID InputBuffer,
    LW_IN LW_ULONG InputBufferLength,
    LW_OUT LW_PVOID OutputBuffer,
    LW_IN LW_ULONG OutputBufferLength
    );

LW_NTSTATUS
LwNtFsControlFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_ULONG FsControlCode,
    LW_IN LW_PVOID InputBuffer,
    LW_IN LW_ULONG InputBufferLength,
    LW_OUT LW_PVOID OutputBuffer,
    LW_IN LW_ULONG OutputBufferLength
    );

LW_NTSTATUS
LwNtFlushBuffersFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock
    );

LW_NTSTATUS 
LwNtQueryInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID FileInformation,
    LW_IN LW_ULONG Length,
    LW_IN FILE_INFORMATION_CLASS FileInformationClass
    );

LW_NTSTATUS 
LwNtSetInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID FileInformation,
    LW_IN LW_ULONG Length,
    LW_IN FILE_INFORMATION_CLASS FileInformationClass
    );

//
// Additional Operations
//

#if 0
LW_NTSTATUS
LwNtQueryFullAttributesFile(
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName,
    LW_OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );
#endif

LW_NTSTATUS 
LwNtQueryDirectoryFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID FileInformation,
    LW_IN LW_ULONG Length,
    LW_IN FILE_INFORMATION_CLASS FileInformationClass,
    LW_IN LW_BOOLEAN ReturnSingleEntry,
    LW_IN LW_OPTIONAL PIO_MATCH_FILE_SPEC FileSpec,
    LW_IN LW_BOOLEAN RestartScan
    );

NTSTATUS 
LwNtReadDirectoryChangeFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN WatchTree,
    IN FILE_NOTIFY_CHANGE NotifyFilter,
    IN OPTIONAL PULONG MaxBufferSize
    );

LW_NTSTATUS
LwNtQueryVolumeInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID FsInformation,
    LW_IN LW_ULONG Length,
    LW_IN FS_INFORMATION_CLASS FsInformationClass
    );

LW_NTSTATUS
LwNtSetVolumeInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID FsInformation,
    LW_IN LW_ULONG Length,
    LW_IN FS_INFORMATION_CLASS FsInformationClass
    );

LW_NTSTATUS 
LwNtLockFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LONG64 ByteOffset,
    LW_IN LONG64 Length,
    LW_IN LW_ULONG Key,
    LW_IN LW_BOOLEAN FailImmediately,
    LW_IN LW_BOOLEAN ExclusiveLock
    );

LW_NTSTATUS 
LwNtUnlockFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LONG64 ByteOffset,
    LW_IN LONG64 Length,
    LW_IN LW_ULONG Key
    );

//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

#if 0
LW_NTSTATUS
LwNtRemoveDirectoryFile(
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName
    );

LW_NTSTATUS
LwNtDeleteFile(
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName
    );

LW_NTSTATUS
LwNtLinkFile(
    LW_IN PIO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME LinkName
    );

LW_NTSTATUS
LwNtRenameFile(
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FromName,
    LW_IN PIO_FILE_NAME ToName
    );
#endif

//
// Advanced Operations
//

LW_NTSTATUS
LwNtQueryQuotaInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID Buffer,
    LW_IN LW_ULONG Length,
    LW_IN LW_BOOLEAN ReturnSingleEntry,
    LW_IN LW_OPTIONAL LW_PVOID SidList,
    LW_IN LW_ULONG SidListLength,
    LW_IN LW_OPTIONAL PSID StartSid,
    LW_IN LW_BOOLEAN RestartScan
    );

LW_NTSTATUS
LwNtSetQuotaInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID Buffer,
    LW_IN LW_ULONG Length
    );

LW_NTSTATUS
LwNtQuerySecurityFile(
    LW_IN IO_FILE_HANDLE  Handle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN SECURITY_INFORMATION SecurityInformation,
    LW_OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    LW_IN LW_ULONG Length
    ); 

LW_NTSTATUS
LwNtSetSecurityFile(
    LW_IN IO_FILE_HANDLE Handle,
    LW_IN LW_OUT LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN SECURITY_INFORMATION SecurityInformation,
    LW_IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    LW_IN LW_ULONG Length
    ); 

#endif /* ! LW_NO_THREADS */

VOID
LwNtCancelAsyncCancelContext(
    LW_IN PIO_ASYNC_CANCEL_CONTEXT pAsyncCancelContext
    );

VOID
LwNtDereferenceAsyncCancelContext(
    LW_IN LW_OUT PIO_ASYNC_CANCEL_CONTEXT* ppAsyncCancelContext
    );

// TODO: QueryEaFile and SetEaFile.

#ifndef LW_STRICT_NAMESPACE

#define NtCtxCreateNamedPipeFile           LwNtCtxCreateNamedPipeFile
#define NtCtxCreateFile                    LwNtCtxCreateFile
#define NtCtxCloseFile                     LwNtCtxCloseFile
#define NtCtxReadFile                      LwNtCtxReadFile
#define NtCtxWriteFile                     LwNtCtxWriteFile
#define NtCtxDeviceIoControlFile           LwNtCtxDeviceIoControlFile
#define NtCtxFsControlFile                 LwNtCtxFsControlFile
#define NtCtxFlushBuffersFile              LwNtCtxFlushBuffersFile
#define NtCtxQueryInformationFile          LwNtCtxQueryInformationFile
#define NtCtxSetInformationFile            LwNtCtxSetInformationFile
#define NtCtxQueryFullAttributesFile       LwNtCtxQueryFullAttributesFile
#define NtCtxQueryDirectoryFile            LwNtCtxQueryDirectoryFile
#define NtCtxReadDirectoryChangeFile       LwNtCtxReadDirectoryChangeFile
#define NtCtxQueryVolumeInformationFile    LwNtCtxQueryVolumeInformationFile
#define NtCtxSetVolumeInformationFile      LwNtCtxSetVolumeInformationFile
#define NtCtxLockFile                      LwNtCtxLockFile
#define NtCtxUnlockFile                    LwNtCtxUnlockFile
#define NtCtxRemoveDirectoryFile           LwNtCtxRemoveDirectoryFile
#define NtCtxDeleteFile                    LwNtCtxDeleteFile
#define NtCtxLinkFile                      LwNtCtxLinkFile
#define NtCtxRenameFile                    LwNtCtxRenameFile
#define NtCtxQueryQuotaInformationFile     LwNtCtxQueryQuotaInformationFile
#define NtCtxSetQuotaInformationFile       LwNtCtxSetQuotaInformationFile
#define NtCtxQuerySecurityFile             LwNtCtxQuerySecurityFile
#define NtCtxSetSecurityFile               LwNtCtxSetSecurityFile

#ifndef LW_NO_THREADS

#define NtCreateNamedPipeFile           LwNtCreateNamedPipeFile
#define NtCreateFile                    LwNtCreateFile
#define NtCloseFile                     LwNtCloseFile
#define NtReadFile                      LwNtReadFile
#define NtWriteFile                     LwNtWriteFile
#define NtDeviceIoControlFile           LwNtDeviceIoControlFile
#define NtFsControlFile                 LwNtFsControlFile
#define NtFlushBuffersFile              LwNtFlushBuffersFile
#define NtQueryInformationFile          LwNtQueryInformationFile
#define NtSetInformationFile            LwNtSetInformationFile
#define NtQueryFullAttributesFile       LwNtQueryFullAttributesFile
#define NtQueryDirectoryFile            LwNtQueryDirectoryFile
#define NtReadDirectoryChangeFile       LwNtReadDirectoryChangeFile
#define NtQueryVolumeInformationFile    LwNtQueryVolumeInformationFile
#define NtSetVolumeInformationFile      LwNtSetVolumeInformationFile
#define NtLockFile                      LwNtLockFile
#define NtUnlockFile                    LwNtUnlockFile
#define NtRemoveDirectoryFile           LwNtRemoveDirectoryFile
#define NtDeleteFile                    LwNtDeleteFile
#define NtLinkFile                      LwNtLinkFile
#define NtRenameFile                    LwNtRenameFile
#define NtQueryQuotaInformationFile     LwNtQueryQuotaInformationFile
#define NtSetQuotaInformationFile       LwNtSetQuotaInformationFile
#define NtQuerySecurityFile             LwNtQuerySecurityFile
#define NtSetSecurityFile               LwNtSetSecurityFile

#endif /* ! LW_NO_THREADS */

#endif /* ! LW_STRICT_NAMESPACE */

#endif /* __NT_FILE_API__ */



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
