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
 *        ioapi.c
 *
 * Abstract:
 *
 *        IO Manager File API Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "iop.h"
#include <lwio/ioapi.h>

static
inline
IRP_IO_FLAGS
IopIoFlagsToIrpIoFlags(
    IN IO_FLAGS IoFlags
    )
{
    IRP_IO_FLAGS flags = 0;

    if (IsSetFlag(IoFlags, IO_FLAG_PAGING_IO))
    {
        SetFlag(flags, IRP_IO_FLAG_PAGING_IO);
    }
    if (IsSetFlag(IoFlags, IO_FLAG_WRITE_THROUGH))
    {
        SetFlag(flags, IRP_IO_FLAG_WRITE_THROUGH);
    }

    return flags;
}

static
inline
VOID
IOP_ASSERT_VALID_READ_WRITE_IO_FLAGS(
    IN BOOLEAN IsWrite,
    IN IO_FLAGS IoFlags
    )
{
    // Ensure valid I/O flags.
    LWIO_ASSERT(!IsSetFlag(IoFlags, ~IO_FLAGS_VALID_MASK));
    // Only certain flags are currently valid for read/write.
    LWIO_ASSERT(!IsSetFlag(IoFlags, ~(IO_FLAG_PAGING_IO | IO_FLAG_WRITE_THROUGH)));
    // Paging I/O is currently only valid for read.
    LWIO_ASSERT(!(IsWrite && IsSetFlag(IoFlags, IO_FLAG_PAGING_IO)));
    // Write through is only valid for write.
    LWIO_ASSERT(!(!IsWrite && IsSetFlag(IoFlags, IO_FLAG_WRITE_THROUGH)));
}

// Need to add a way to cancel operation from outside IRP layer.
// Probably requires something in IO_ASYNC_CONTROL_BLOCK.

BOOLEAN
IoCancelAsyncCancelContext(
    IN PIO_ASYNC_CANCEL_CONTEXT AsyncCancelContext
    )
{
    return IopIrpCancel(IopIrpGetIrpFromAsyncCancelContext(AsyncCancelContext));
}

VOID
IoDereferenceAsyncCancelContext(
    IN OUT PIO_ASYNC_CANCEL_CONTEXT* AsyncCancelContext
    )
{
    if (*AsyncCancelContext)
    {
        PIRP irp = IopIrpGetIrpFromAsyncCancelContext(*AsyncCancelContext);
        IopIrpDereference(&irp);
        *AsyncCancelContext = NULL;
    }
}

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
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_FILE_NAME fileName = { 0 };
    PIO_DEVICE_OBJECT pDevice = NULL;
    UNICODE_STRING remainingPath = { 0 };
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    PIO_FILE_OBJECT pFileObject = NULL;
    IRP_TYPE irpType = IRP_TYPE_CREATE;
    PIO_FILE_OBJECT pResultFileObject = NULL;

    LWIO_ASSERT(FileHandle);
    LWIO_ASSERT(IoStatusBlock);

    if (SecurityQualityOfService)
    {
        // Not yet implemented.
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    // TODO -- Add basic param validation...

    status = IopParse(FileName, &pDevice, &remainingPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    fileName.RootFileHandle = FileName->RootFileHandle;
    fileName.IoNameOptions = FileName->IoNameOptions;
    status = LwRtlUnicodeStringDuplicate(&fileName.Name, &remainingPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopFileObjectAllocate(&pFileObject, pDevice, &fileName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (EcpList)
    {
        status = IoRtlEcpListFind(
                        EcpList,
                        IO_ECP_TYPE_NAMED_PIPE,
                        NULL,
                        NULL);
        if (STATUS_SUCCESS == status)
        {
            irpType = IRP_TYPE_CREATE_NAMED_PIPE;
        }
        else if (STATUS_NOT_FOUND == status)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            assert(!NT_SUCCESS(status));
            GOTO_CLEANUP_EE(EE);
        }
    }

    status = IopIrpCreate(&pIrp, irpType, pFileObject);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.Create.SecurityContext = SecurityContext;
    IopSecurityReferenceSecurityContext(SecurityContext);

    pIrp->Args.Create.FileName = fileName;
    LwRtlZeroMemory(&fileName, sizeof(fileName));

    pIrp->Args.Create.DesiredAccess = DesiredAccess;
    pIrp->Args.Create.AllocationSize = AllocationSize;
    pIrp->Args.Create.FileAttributes = FileAttributes;
    pIrp->Args.Create.ShareAccess = ShareAccess;
    pIrp->Args.Create.CreateDisposition = CreateDisposition;
    pIrp->Args.Create.CreateOptions = CreateOptions;
    pIrp->Args.Create.EaBuffer = EaBuffer;
    pIrp->Args.Create.EaLength = EaLength;
    pIrp->Args.Create.SecurityDescriptor = SecurityDescriptor;
    pIrp->Args.Create.SecurityQualityOfService = SecurityQualityOfService;
    pIrp->Args.Create.EcpList = EcpList;

    IopIrpSetOutputCreate(pIrp, AsyncControlBlock, FileHandle);
    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        if ((STATUS_SUCCESS == status) || 
            (STATUS_OPLOCK_BREAK_IN_PROGRESS == status))
        {
            // Already referenced by IRP completion processing for
            // create-type IRPs.
            pResultFileObject = pIrp->FileHandle;
        }
    }

cleanup:
    LwRtlUnicodeStringFree(&fileName.Name);
    IopIrpDereference(&pIrp);
    IopFileObjectDereference(&pFileObject);
    IopDeviceDereference(&pDevice);

    // Ensure that status is in status block even on early bail.
    if (status && (STATUS_PENDING != status))
    {
        ioStatusBlock.Status = status;
    }

    if (STATUS_PENDING != status)
    {
        *FileHandle = pResultFileObject;
        *IoStatusBlock = ioStatusBlock;
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    LWIO_ASSERT(IsValidStatusForIrpType(status, irpType));
    return status;
}

VOID
IoCancelFile(
    IN IO_FILE_HANDLE FileHandle
    )
{
    IopIrpCancelFileObject(FileHandle, FALSE);
}

VOID
IoCancelForRundownFile(
    IN IO_FILE_HANDLE FileHandle
    )
{
    IopIrpCancelFileObject(FileHandle, TRUE);
}

NTSTATUS
IoRundownFile(
    IN OUT IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    // Assumes that close cannot fail.
    IopFileObjectReference(FileHandle);
    return IoAsyncCloseFile(FileHandle, AsyncControlBlock, IoStatusBlock);
}

NTSTATUS
IoAsyncCloseFile(
    IN OUT IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    return IopFileObjectRundownEx(
                    FileHandle,
                    AsyncControlBlock ? AsyncControlBlock->Callback : NULL,
                    AsyncControlBlock ? AsyncControlBlock->CallbackContext : NULL,
                    IoStatusBlock);
}

NTSTATUS
IoCloseFile(
    IN OUT IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    status = IoAsyncCloseFile(FileHandle, NULL, &ioStatusBlock);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    LWIO_ASSERT(NT_SUCCESS_OR_NOT(status));

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
IopReadWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN BOOLEAN bIsWrite,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PULONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsWrite ? IRP_TYPE_WRITE : IRP_TYPE_READ;

    LWIO_ASSERT(IoStatusBlock);
    IOP_ASSERT_VALID_READ_WRITE_IO_FLAGS(bIsWrite, IoFlags);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (Length && !Buffer)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Flags = IopIoFlagsToIrpIoFlags(IoFlags);

    pIrp->Args.ReadWrite.Buffer = Buffer;
    pIrp->Args.ReadWrite.Length = Length;
    pIrp->Args.ReadWrite.ByteOffset = ByteOffset;
    pIrp->Args.ReadWrite.Key = Key;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= Length);
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsWrite ? "Write" : "Read");
    return status;
}

NTSTATUS
IoReadFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PULONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    return IopReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                IoFlags,
                FALSE,
                Buffer,
                Length,
                ByteOffset,
                Key);
}

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
    )
{
    return IopReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                IoFlags,
                TRUE,
                Buffer,
                Length,
                ByteOffset,
                Key);
}

VOID
IoGetZctSupportMaskFile(
    IN IO_FILE_HANDLE FileHandle,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctReadMask,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctWriteMask
    )
{
    IopFileGetZctSupportMask(FileHandle, ZctReadMask, ZctWriteMask);
}

static
NTSTATUS
IopPrepareZctReadWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN BOOLEAN bIsWrite,
    IN OUT PLW_ZCT_VECTOR Zct,
    IN ULONG Length,
    IN OPTIONAL PULONG64 ByteOffset,
    IN OPTIONAL PULONG Key,
    OUT PVOID* CompletionContext
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsWrite ? IRP_TYPE_WRITE : IRP_TYPE_READ;
    LW_ZCT_ENTRY_MASK mask = 0;
    PVOID completionContext = NULL;
    PIRP pCompletionIrp = NULL;

    LWIO_ASSERT(IoStatusBlock);
    IOP_ASSERT_VALID_READ_WRITE_IO_FLAGS(bIsWrite, IoFlags);
    LWIO_ASSERT(CompletionContext);

    if (!FileHandle || !Zct)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    //
    // Ensure that the FSD can handle ZCT I/O.
    //

    IopFileGetZctSupportMask(
            FileHandle,
            bIsWrite ? NULL : &mask,
            bIsWrite ? &mask : NULL);
    if (!mask)
    {
        status = STATUS_NOT_SUPPORTED;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreateDetached(&pCompletionIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Flags = IopIoFlagsToIrpIoFlags(IoFlags);

    pIrp->Args.ReadWrite.Zct = Zct;
    pIrp->Args.ReadWrite.Length = Length;
    if (ByteOffset)
    {
        pIrp->Args.ReadWrite.Storage.ByteOffset = *ByteOffset;
        pIrp->Args.ReadWrite.ByteOffset = &pIrp->Args.ReadWrite.Storage.ByteOffset;
    }
    if (Key)
    {
        pIrp->Args.ReadWrite.Storage.Key = *Key;
        pIrp->Args.ReadWrite.Key = &pIrp->Args.ReadWrite.Storage.Key;
    }
    pIrp->Args.ReadWrite.ZctOperation = IRP_ZCT_OPERATION_PREPARE;

    pCompletionIrp->Args.ReadWrite = pIrp->Args.ReadWrite;
    if (ByteOffset)
    {
        pCompletionIrp->Args.ReadWrite.ByteOffset = &pCompletionIrp->Args.ReadWrite.Storage.ByteOffset;
    }
    if (Key)
    {
        pCompletionIrp->Args.ReadWrite.Key = &pCompletionIrp->Args.ReadWrite.Storage.Key;
    }

    IopIrpSetOutputPrepareZctReadWrite(
            pIrp,
            AsyncControlBlock,
            CompletionContext,
            pCompletionIrp);
    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= Length);
        completionContext = pIrp->Args.ReadWrite.ZctCompletionContext;
        LWIO_ASSERT(LW_IS_BOTH_OR_NEITHER(completionContext, STATUS_SUCCESS == status));
        if (STATUS_SUCCESS == status)
        {
            completionContext = IopIrpSaveZctIrp(
                                        pIrp->FileHandle,
                                        pCompletionIrp,
                                        pIrp->Args.ReadWrite.ZctCompletionContext);
        }
    }

cleanup:
    IopIrpDereference(&pIrp);
    IopIrpDereference(&pCompletionIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
        *CompletionContext = completionContext;
    }

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsWrite ? "Write" : "Read");
    return status;
}

static
NTSTATUS
IopCompleteZctReadWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN BOOLEAN bIsWrite,
    IN PVOID CompletionContext,
    IN OPTIONAL ULONG BytesTransferred
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsWrite ? IRP_TYPE_WRITE : IRP_TYPE_READ;

    LWIO_ASSERT(IoStatusBlock);
    IOP_ASSERT_VALID_READ_WRITE_IO_FLAGS(bIsWrite, IoFlags);
    LWIO_ASSERT(!BytesTransferred || bIsWrite);

    if (!FileHandle || !CompletionContext)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    pIrp = IopIrpLoadZctIrp(FileHandle, CompletionContext);
    LWIO_ASSERT(pIrp);

    status = IopIrpAttach(pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LWIO_ASSERT(FileHandle == pIrp->FileHandle);
    LWIO_ASSERT(irpType == pIrp->Type);
    LWIO_ASSERT(IopIoFlagsToIrpIoFlags(IoFlags) == pIrp->Flags);

    pIrp->Args.ReadWrite.Zct = NULL;
    pIrp->Args.ReadWrite.ZctOperation = IRP_ZCT_OPERATION_COMPLETE;
    pIrp->Args.ReadWrite.ZctWriteBytesTransferred = BytesTransferred;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        // TODO -- Do we expect 0 or the actual bytes committed?
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= BytesTransferred);
    }

cleanup:
    IopIrpDereference(&pIrp);

    // ZCT read completion on an already run-down file is ok
    if ((STATUS_CANCELLED == status) && !bIsWrite)
    {
        status = STATUS_SUCCESS;
        ioStatusBlock.Status = status;
    }

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsWrite ? "Write" : "Read");
    return status;
}

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
    )
{
    return IopPrepareZctReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                IoFlags,
                FALSE,
                Zct,
                Length,
                ByteOffset,
                Key,
                CompletionContext);
}

NTSTATUS
IoCompleteZctReadFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN PVOID CompletionContext
    )
{
    return IopCompleteZctReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                IoFlags,
                FALSE,
                CompletionContext,
                0);
}

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
    )
{
    return IopPrepareZctReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                IoFlags,
                TRUE,
                Zct,
                Length,
                ByteOffset,
                Key,
                CompletionContext);
}

NTSTATUS
IoCompleteZctWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN IO_FLAGS IoFlags,
    IN PVOID CompletionContext,
    IN ULONG BytesTransferred
    )
{
    return IopCompleteZctReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                IoFlags,
                TRUE,
                CompletionContext,
                BytesTransferred);
}

static
NTSTATUS
IopControlFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN bIsFsControl,
    IN ULONG ControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsFsControl? IRP_TYPE_FS_CONTROL : IRP_TYPE_DEVICE_IO_CONTROL;

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.IoFsControl.ControlCode = ControlCode;
    pIrp->Args.IoFsControl.InputBuffer = InputBuffer;
    pIrp->Args.IoFsControl.InputBufferLength = InputBufferLength;
    pIrp->Args.IoFsControl.OutputBuffer = OutputBuffer;
    pIrp->Args.IoFsControl.OutputBufferLength = OutputBufferLength;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsFsControl ? "FsControl" : "DeviceIoControl" );
    return status;
}

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
    )
{
    return IopControlFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                FALSE,
                IoControlCode,
                InputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength);
}

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
    )
{
    return IopControlFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                TRUE,
                FsControlCode,
                InputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength);
}

NTSTATUS
IoFlushBuffersFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, IRP_TYPE_FLUSH_BUFFERS, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
IopQuerySetInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN bIsSet,
    IN OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsSet ? IRP_TYPE_SET_INFORMATION : IRP_TYPE_QUERY_INFORMATION;

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.QuerySetInformation.FileInformation = FileInformation;
    pIrp->Args.QuerySetInformation.Length = Length;
    pIrp->Args.QuerySetInformation.FileInformationClass = FileInformationClass;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= Length);
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsSet ? "Set" : "Query");
    return status;
}

NTSTATUS
IoQueryInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    return IopQuerySetInformationFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                FALSE,
                FileInformation,
                Length,
                FileInformationClass);
}

NTSTATUS
IoSetInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    return IopQuerySetInformationFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                TRUE,
                FileInformation,
                Length,
                FileInformationClass);
}


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
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}
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
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    PIO_MATCH_FILE_SPEC fileSpec = NULL;

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (FileSpec)
    {
        status = IO_ALLOCATE(&fileSpec, IO_MATCH_FILE_SPEC, sizeof(*fileSpec));
        ioStatusBlock.Status = status;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        status = LwRtlUnicodeStringDuplicate(&fileSpec->Pattern, &FileSpec->Pattern);
        ioStatusBlock.Status = status;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        fileSpec->Type = FileSpec->Type;
        fileSpec->Options = FileSpec->Options;
    }

    status = IopIrpCreate(&pIrp, IRP_TYPE_QUERY_DIRECTORY, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.QueryDirectory.FileInformation = FileInformation;
    pIrp->Args.QueryDirectory.Length = Length;
    pIrp->Args.QueryDirectory.FileInformationClass = FileInformationClass;
    pIrp->Args.QueryDirectory.RestartScan = RestartScan;
    pIrp->Args.QueryDirectory.ReturnSingleEntry = ReturnSingleEntry;
    // Handled by IopIrpFree():
    pIrp->Args.QueryDirectory.FileSpec = fileSpec;
    fileSpec = NULL;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= Length);
    }

cleanup:
    if (fileSpec)
    {
        LwRtlUnicodeStringFree(&fileSpec->Pattern);
        IO_FREE(&fileSpec);
    }
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

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
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, IRP_TYPE_READ_DIRECTORY_CHANGE, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.ReadDirectoryChange.Buffer = Buffer;
    pIrp->Args.ReadDirectoryChange.Length = Length;
    pIrp->Args.ReadDirectoryChange.WatchTree = WatchTree;
    pIrp->Args.ReadDirectoryChange.NotifyFilter = NotifyFilter;
    pIrp->Args.ReadDirectoryChange.MaxBufferSize = MaxBufferSize;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= Length);
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
IopQuerySetVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN bIsSet,
    IN OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsSet ? IRP_TYPE_SET_VOLUME_INFORMATION : IRP_TYPE_QUERY_VOLUME_INFORMATION;

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.QuerySetVolumeInformation.FsInformation = FsInformation;
    pIrp->Args.QuerySetVolumeInformation.Length = Length;
    pIrp->Args.QuerySetVolumeInformation.FsInformationClass = FsInformationClass;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= Length);
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsSet ? "Set" : "Query");
    return status;
}

NTSTATUS
IoQueryVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    return IopQuerySetVolumeInformationFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                FALSE,
                FsInformation,
                Length,
                FsInformationClass);
}

NTSTATUS
IoSetVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    return IopQuerySetVolumeInformationFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                TRUE,
                FsInformation,
                Length,
                FsInformationClass);
}

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
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, IRP_TYPE_LOCK_CONTROL, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.LockControl.LockControl = IO_LOCK_CONTROL_LOCK;    
    pIrp->Args.LockControl.ByteOffset = ByteOffset;
    pIrp->Args.LockControl.Length = Length;
    pIrp->Args.LockControl.Key = Key;
    pIrp->Args.LockControl.FailImmediately = FailImmediately;
    pIrp->Args.LockControl.ExclusiveLock = ExclusiveLock;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoUnlockFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG64 ByteOffset,
    IN ULONG64 Length,
    IN ULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, IRP_TYPE_LOCK_CONTROL, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.LockControl.LockControl = IO_LOCK_CONTROL_UNLOCK;
    pIrp->Args.LockControl.ByteOffset = ByteOffset;
    pIrp->Args.LockControl.Length = Length;
    pIrp->Args.LockControl.Key = Key;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

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
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoDeleteFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoLinkFile(
    IN PIO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoRenameFile(
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FromName,
    IN PIO_FILE_NAME ToName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}
#endif

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
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = IRP_TYPE_QUERY_QUOTA;

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.QueryQuota.Buffer = Buffer;
    pIrp->Args.QueryQuota.Length = Length;
    pIrp->Args.QueryQuota.ReturnSingleEntry = ReturnSingleEntry;
    pIrp->Args.QueryQuota.SidList = SidList;
    pIrp->Args.QueryQuota.SidListLength = SidListLength;
    pIrp->Args.QueryQuota.StartSid = StartSid;
    pIrp->Args.QueryQuota.RestartScan = RestartScan;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred <= Length);
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

//
// Advanced Operations
//

NTSTATUS
IoSetQuotaInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_QUOTA_INFORMATION Buffer,
    IN ULONG Length
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = IRP_TYPE_SET_QUOTA;

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.SetQuota.Buffer = Buffer;
    pIrp->Args.SetQuota.Length = Length;
    
    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
        LWIO_ASSERT(ioStatusBlock.BytesTransferred == 0);
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
IopQuerySetSecurityFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN bIsSet,
    IN SECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsSet ? IRP_TYPE_SET_SECURITY : IRP_TYPE_QUERY_SECURITY;

    LWIO_ASSERT(IoStatusBlock);

    if (!FileHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.QuerySetSecurity.SecurityInformation = SecurityInformation;    
    pIrp->Args.QuerySetSecurity.SecurityDescriptor = SecurityDescriptor;
    pIrp->Args.QuerySetSecurity.Length = Length;

    status = IopIrpDispatch(
                    pIrp,
                    AsyncControlBlock,
                    IoStatusBlock);
    if (STATUS_PENDING != status)
    {
        ioStatusBlock = pIrp->IoStatusBlock;
    }

cleanup:
    IopIrpDereference(&pIrp);

    if (STATUS_PENDING != status)
    {
        *IoStatusBlock = ioStatusBlock;
    }

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsSet ? "Set" : "Query");
    return status;
}

NTSTATUS
IoQuerySecurityFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    )
{
    return IopQuerySetSecurityFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                FALSE,
                SecurityInformation,
                SecurityDescriptor,
                Length);
}

NTSTATUS
IoSetSecurityFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Length
    )
{
    return IopQuerySetSecurityFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                TRUE,
                SecurityInformation,
                SecurityDescriptor,
                Length);
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
 
