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
 *        ioapi.h
 *
 * Abstract:
 *
 *        IO Manager File API
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __IO_API_H__
#define __IO_API_H__

#include <lwio/io-types.h>
#include <lwio/lwzct.h>

typedef ULONG IO_FLAGS, *PIO_FLAGS;

// At the moment, paging I/O flag is valid only for read.
#define IO_FLAG_PAGING_IO           0x00000001
// no access check is not yet valid.
#define IO_FLAG_NO_ACCESS_CHECK     0x00000002
// The write operation should not return until write is stable.
#define IO_FLAG_WRITE_THROUGH       0x00000004

#define IO_FLAGS_VALID_MASK ( \
    IO_FLAG_PAGING_IO | \
    IO_FLAG_NO_ACCESS_CHECK | \
    IO_FLAG_WRITE_THROUGH | \
    0 )

//
// Asynchoronous I/O Support
//

// Always sets cancel bit.  Returns TRUE is cacellable, FALSE otherwise.
BOOLEAN
IoCancelAsyncCancelContext(
    IN PIO_ASYNC_CANCEL_CONTEXT AsyncCancelContext
    );

VOID
IoDereferenceAsyncCancelContext(
    IN OUT PIO_ASYNC_CANCEL_CONTEXT* AsyncCancelContext
    );

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
IoCreateFile(
    OUT PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext,
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

VOID
IoCancelFile(
    IN IO_FILE_HANDLE FileHandle
    );

VOID
IoCancelForRundownFile(
    IN IO_FILE_HANDLE FileHandle
    );

NTSTATUS
IoRundownFile(
    IN OUT IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
IoAsyncCloseFile(
    IN OUT IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
IoCloseFile(
    IN OUT IO_FILE_HANDLE FileHandle
    );

NTSTATUS
IoReadFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PULONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    );

NTSTATUS
IoWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PULONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    );

VOID
IoGetZctSupportMaskFile(
    IN IO_FILE_HANDLE FileHandle,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctReadMask,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctWriteMask
    );

///
/// Prepare ZCT for a read.
///
/// Driver appends entries to the ZCT for caller to read from.  Allowed types
/// are specified by the caller in the ZCT.
///
/// @param[in,out] Zct - length by which ZCT is extended is how much
///    can actually be read.  The driver is guaranteeing this.  The caller
///    can check how much the length increased to get the amount "read"
///    (i.e., if the ZCT passed in was not empty).  This should
///    match IoStatusBlock->BytesTransferred.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_PENDING
/// @retval STATUS_NOT_SUPPORTED (or STATUS_RETRY?) -
///    try again as regular read.
///
NTSTATUS
IoPrepareZctReadFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN OUT PLW_ZCT_VECTOR Zct,
    IN ULONG Length,
    IN OPTIONAL PULONG64 ByteOffset,
    IN OPTIONAL PULONG Key,
    OUT PVOID* CompletionContext
    );

///
/// Complete ZCT read.
///
/// Caller signals that the I/O is complete.
///
NTSTATUS
IoCompleteZctReadFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN PVOID CompletionContext
    );

///
/// Prepare ZCT for a write.
///
/// Driver adds entries to the ZCT for caller to write info.  Allowed types
/// are specified by the caller in the ZCT.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_PENDING
/// @retval STATUS_NOT_SUPPORTED (or STATUS_RETRY?) -
///    try again as regular write.
///
NTSTATUS
IoPrepareZctWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN OUT PLW_ZCT_VECTOR Zct,
    IN ULONG Length,
    IN OPTIONAL PULONG64 ByteOffset,
    IN OPTIONAL PULONG Key,
    OUT PVOID* CompletionContext
    );

///
/// Complete ZCT write.
///
/// Caller signals that the I/O is complete.
///
NTSTATUS
IoCompleteZctWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN PVOID CompletionContext,
    IN ULONG BytesTransferred
    );

NTSTATUS 
IoDeviceIoControlFile(
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
IoFsControlFile(
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
IoFlushBuffersFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS 
IoQueryInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    );

NTSTATUS 
IoSetInformationFile(
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
IoQueryFullAttributesFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );
#endif

NTSTATUS 
IoQueryDirectoryFile(
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
IoReadDirectoryChangeFile(
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
IoQueryVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS
IoSetVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS 
IoLockFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG64 ByteOffset,
    IN ULONG64 Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    );

NTSTATUS 
IoUnlockFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG64 ByteOffset,
    IN ULONG64 Length,
    IN ULONG Key
    );

//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

#if 0
NTSTATUS
IoRemoveDirectoryFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
IoDeleteFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
IoLinkFile(
    IN PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    );

NTSTATUS
IoRenameFile(
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
IoQueryQuotaInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PFILE_QUOTA_INFORMATION Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PFILE_GET_QUOTA_INFORMATION SidList,
    IN ULONG SidListLength,
    IN OPTIONAL PFILE_GET_QUOTA_INFORMATION StartSid,
    IN BOOLEAN RestartScan
    );

NTSTATUS
IoSetQuotaInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_QUOTA_INFORMATION Buffer,
    IN ULONG Length
    );

NTSTATUS
IoQuerySecurityFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    ); 

NTSTATUS
IoSetSecurityFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    ); 

// TODO: QueryEaFile and SetEaFile.

#endif /* __IO_API_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
