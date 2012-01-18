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
 *        ntipc.c
 *
 * Abstract:
 *
 *        NT lwmsg IPC Implementaion
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"
#include "ntipcmsg.h"
#include "goto.h"
#include "ntlogmacros.h"
#include "ntipc-unicode_string.h"

#define _LWMSG_MEMBER_BOOLEAN(Type, Field) \
    LWMSG_MEMBER_TYPESPEC(Type, Field, gNtIpcTypeSpecBoolean)

#define _LWMSG_MEMBER_BUFFER(Type, BufferField, LengthField) \
    LWMSG_MEMBER_POINTER_BEGIN(Type, BufferField), \
    LWMSG_UINT8(BYTE), \
    LWMSG_POINTER_END, \
    LWMSG_ATTR_ENCODING("hex+ascii"), \
    LWMSG_ATTR_LENGTH_MEMBER(Type, LengthField)

#define _LWMSG_MEMBER_OPTIONAL_ARRAY(Type, PointerField, CountField, ElementSpec) \
    LWMSG_MEMBER_POINTER(Type, PointerField, LWMSG_TYPESPEC(ElementSpec)), \
    LWMSG_ATTR_LENGTH_MEMBER(Type, CountField)

#define _LWMSG_ATTR_HANDLE_IN \
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER

#define _LWMSG_ATTR_HANDLE_OUT \
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER

#define _LWMSG_MEMBER_HANDLE_IN_OPTIONAL(Type, Field, HandleType) \
    LWMSG_MEMBER_HANDLE(Type, Field, HandleType), \
    _LWMSG_ATTR_HANDLE_IN

#define _LWMSG_MEMBER_HANDLE_IN(Type, Field, HandleType) \
    _LWMSG_MEMBER_HANDLE_IN_OPTIONAL(Type, Field, HandleType), \
    LWMSG_ATTR_NOT_NULL

#define _LWMSG_MEMBER_HANDLE_OUT(Type, Field, HandleType) \
    LWMSG_MEMBER_HANDLE(Type, Field, HandleType), \
    _LWMSG_ATTR_HANDLE_OUT

#define _LWMSG_MEMBER_IO_FILE_HANDLE_IN_OPTIONAL(Type, Field) \
    _LWMSG_MEMBER_HANDLE_IN_OPTIONAL(Type, Field, IO_FILE_HANDLE)

#define _LWMSG_MEMBER_IO_FILE_HANDLE_IN(Type, Field) \
    _LWMSG_MEMBER_HANDLE_IN(Type, Field, IO_FILE_HANDLE)

#define _LWMSG_MEMBER_IO_FILE_HANDLE_OUT(Type, Field) \
    _LWMSG_MEMBER_HANDLE_OUT(Type, Field, IO_FILE_HANDLE)

#define _LWMSG_MEMBER_NTSTATUS(Type, Field) \
    LWMSG_MEMBER_CUSTOM(Type, Field, &gNtStatusClass, NULL)

LWMsgStatus
NtIpcPrintNtStatus(
    LWMsgDataContext* pContext,
    LWMsgType* pType,
    void* pObject,
    void* pData,
    LWMsgBuffer* pBuffer
    )
{
    NTSTATUS status = *(NTSTATUS*)pObject;
    PCSTR pName = LwNtStatusToName(status);

    if (!strcmp(pName, "UNKNOWN"))
    {
        return lwmsg_buffer_print(pBuffer, "0x%.8x", (unsigned int) status);
    }
    else
    {
        return lwmsg_buffer_print(pBuffer, "%s (0x%.8x)", pName, (unsigned int) status);
    }
}

static
LWMsgTypeSpec gNtStatusSpec[] =
{
    LWMSG_UINT32(NTSTATUS),
    LWMSG_TYPE_END
};

static LWMsgTypeClass gNtStatusClass =
{
    .transmit_type = gNtStatusSpec,
    .print = NtIpcPrintNtStatus
};

static
LWMsgTypeSpec gNtIpcTypeSpecBoolean[] =
{
    LWMSG_ENUM_BEGIN(BOOLEAN, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_NAMED_VALUE("FALSE", FALSE),
    LWMSG_ENUM_NAMED_VALUE("TRUE", TRUE),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecIoFileName[] =
{
    LWMSG_STRUCT_BEGIN(IO_FILE_NAME),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN_OPTIONAL(IO_FILE_NAME, RootFileHandle),
    _LWMSG_MEMBER_UNICODE_STRING(IO_FILE_NAME, Name),
    LWMSG_MEMBER_UINT32(IO_FILE_NAME, IoNameOptions),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

#define _LWMSG_MEMBER_IO_FILE_NAME(Type, Field) \
    LWMSG_MEMBER_TYPESPEC(Type, Field, gNtIpcTypeSpecIoFileName)

static
LWMsgTypeSpec gNtIpcTypeSpecHelperEcp[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_HELPER_ECP),
    LWMSG_MEMBER_PSTR(NT_IPC_HELPER_ECP, pszType),
    LWMSG_MEMBER_UINT32(NT_IPC_HELPER_ECP, Size),
    _LWMSG_MEMBER_BUFFER(NT_IPC_HELPER_ECP, pData, Size),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageGenericFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_GENERIC_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_GENERIC_FILE, FileHandle),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageGenericControlFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_GENERIC_CONTROL_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_GENERIC_CONTROL_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_GENERIC_CONTROL_FILE, ControlCode),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_GENERIC_CONTROL_FILE, InputBufferLength),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_GENERIC_CONTROL_FILE, OutputBufferLength),
    _LWMSG_MEMBER_BUFFER(NT_IPC_MESSAGE_GENERIC_CONTROL_FILE, InputBuffer, InputBufferLength),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageGenericFileIoResult[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT),
    _LWMSG_MEMBER_NTSTATUS(NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, Status),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, BytesTransferred),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageGenericFileBufferResult[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT),
    _LWMSG_MEMBER_NTSTATUS(NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, Status),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, BytesTransferred),
    _LWMSG_MEMBER_BUFFER(NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, Buffer, BytesTransferred),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageCreateFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_CREATE_FILE),
    LWMSG_MEMBER_PSECTOKEN(NT_IPC_MESSAGE_CREATE_FILE, pSecurityToken),
    _LWMSG_MEMBER_IO_FILE_NAME(NT_IPC_MESSAGE_CREATE_FILE, FileName),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, DesiredAccess),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, AllocationSize),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, FileAttributes),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, ShareAccess),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, CreateDisposition),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, CreateOptions),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, EaLength),
    _LWMSG_MEMBER_BUFFER(NT_IPC_MESSAGE_CREATE_FILE, EaBuffer, EaLength),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, SecDescLength),
    _LWMSG_MEMBER_BUFFER(NT_IPC_MESSAGE_CREATE_FILE, SecurityDescriptor, SecDescLength),
    // TODO -- Add stuff for QOS etc.
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE, EcpCount),
    _LWMSG_MEMBER_OPTIONAL_ARRAY(NT_IPC_MESSAGE_CREATE_FILE, EcpList, EcpCount, gNtIpcTypeSpecHelperEcp),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageCreateFileResult[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_CREATE_FILE_RESULT),
    _LWMSG_MEMBER_IO_FILE_HANDLE_OUT(NT_IPC_MESSAGE_CREATE_FILE_RESULT, FileHandle),
    _LWMSG_MEMBER_NTSTATUS(NT_IPC_MESSAGE_CREATE_FILE_RESULT, Status),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_CREATE_FILE_RESULT, CreateResult),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageReadFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_READ_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_READ_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_READ_FILE, Length),
    LWMSG_MEMBER_POINTER(NT_IPC_MESSAGE_READ_FILE, ByteOffset, LWMSG_INT64(ULONG64)),
    LWMSG_MEMBER_POINTER(NT_IPC_MESSAGE_READ_FILE, Key, LWMSG_UINT32(ULONG)),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageWriteFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_WRITE_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_WRITE_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_WRITE_FILE, Length),
    _LWMSG_MEMBER_BUFFER(NT_IPC_MESSAGE_WRITE_FILE, Buffer, Length),
    LWMSG_MEMBER_POINTER(NT_IPC_MESSAGE_WRITE_FILE, ByteOffset, LWMSG_INT64(ULONG64)),
    LWMSG_MEMBER_POINTER(NT_IPC_MESSAGE_WRITE_FILE, Key, LWMSG_UINT32(ULONG)),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageQueryInformationFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_QUERY_INFORMATION_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_QUERY_INFORMATION_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_INFORMATION_FILE, Length),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_INFORMATION_FILE, FileInformationClass),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageSetInformationFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_SET_INFORMATION_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_SET_INFORMATION_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_SET_INFORMATION_FILE, Length),
    _LWMSG_MEMBER_BUFFER(NT_IPC_MESSAGE_SET_INFORMATION_FILE, FileInformation, Length),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_SET_INFORMATION_FILE, FileInformationClass),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageQueryDirectoryFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE, Length),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE, FileInformationClass),
    _LWMSG_MEMBER_BOOLEAN(NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE, ReturnSingleEntry),
    // TODO-Implement marshaling of FileSpec field.
    _LWMSG_MEMBER_BOOLEAN(NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE, RestartScan),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageReadDirectoryChangeFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE, Length),
    _LWMSG_MEMBER_BOOLEAN(NT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE, WatchTree),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE, NotifyFilter),
    LWMSG_MEMBER_POINTER(NT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE, MaxBufferSize, LWMSG_UINT32(ULONG)),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageQueryVolumeInformationFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_QUERY_VOLUME_INFORMATION_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_QUERY_VOLUME_INFORMATION_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_VOLUME_INFORMATION_FILE, Length),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_VOLUME_INFORMATION_FILE, FsInformationClass),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageLockFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_LOCK_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_LOCK_FILE, FileHandle),
    LWMSG_MEMBER_INT64(NT_IPC_MESSAGE_LOCK_FILE, ByteOffset),
    LWMSG_MEMBER_INT64(NT_IPC_MESSAGE_LOCK_FILE, Length),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_LOCK_FILE, Key),
    _LWMSG_MEMBER_BOOLEAN(NT_IPC_MESSAGE_LOCK_FILE, FailImmediately),
    _LWMSG_MEMBER_BOOLEAN(NT_IPC_MESSAGE_LOCK_FILE, ExclusiveLock),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageUnlockFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_UNLOCK_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_UNLOCK_FILE, FileHandle),
    LWMSG_MEMBER_INT64(NT_IPC_MESSAGE_UNLOCK_FILE, ByteOffset),
    LWMSG_MEMBER_INT64(NT_IPC_MESSAGE_UNLOCK_FILE, Length),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_UNLOCK_FILE, Key),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageQuerySecurityFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_QUERY_SECURITY_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_QUERY_SECURITY_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_SECURITY_FILE, SecurityInformation),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_QUERY_SECURITY_FILE, Length),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgTypeSpec gNtIpcTypeSpecMessageSetSecurityFile[] =
{
    LWMSG_STRUCT_BEGIN(NT_IPC_MESSAGE_SET_SECURITY_FILE),
    _LWMSG_MEMBER_IO_FILE_HANDLE_IN(NT_IPC_MESSAGE_SET_SECURITY_FILE, FileHandle),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_SET_SECURITY_FILE, SecurityInformation),
    LWMSG_MEMBER_UINT32(NT_IPC_MESSAGE_SET_SECURITY_FILE, Length),
    _LWMSG_MEMBER_BUFFER(NT_IPC_MESSAGE_SET_SECURITY_FILE, SecurityDescriptor, Length),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
LWMsgProtocolSpec gNtIpcProtocolSpec[] =
{
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_CREATE_FILE,        gNtIpcTypeSpecMessageCreateFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_CREATE_FILE_RESULT, gNtIpcTypeSpecMessageCreateFileResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_CLOSE_FILE,         gNtIpcTypeSpecMessageGenericFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_CLOSE_FILE_RESULT,  gNtIpcTypeSpecMessageGenericFileIoResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_READ_FILE,          gNtIpcTypeSpecMessageReadFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_READ_FILE_RESULT,   gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_WRITE_FILE,         gNtIpcTypeSpecMessageWriteFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_WRITE_FILE_RESULT,  gNtIpcTypeSpecMessageGenericFileIoResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE,        gNtIpcTypeSpecMessageGenericControlFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE_RESULT, gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE,               gNtIpcTypeSpecMessageGenericControlFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE_RESULT,        gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE,            gNtIpcTypeSpecMessageGenericFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE_RESULT,     gNtIpcTypeSpecMessageGenericFileIoResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE,        gNtIpcTypeSpecMessageQueryInformationFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE_RESULT, gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE,          gNtIpcTypeSpecMessageSetInformationFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE_RESULT,   gNtIpcTypeSpecMessageGenericFileIoResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE,          gNtIpcTypeSpecMessageQueryDirectoryFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE_RESULT,   gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_READ_DIRECTORY_CHANGE_FILE,          gNtIpcTypeSpecMessageReadDirectoryChangeFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_READ_DIRECTORY_CHANGE_FILE_RESULT,   gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_VOLUME_INFORMATION_FILE,      gNtIpcTypeSpecMessageQueryVolumeInformationFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_VOLUME_INFORMATION_FILE_RESULT,gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_LOCK_FILE,                     gNtIpcTypeSpecMessageLockFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_LOCK_FILE_RESULT,              gNtIpcTypeSpecMessageGenericFileIoResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_UNLOCK_FILE,                   gNtIpcTypeSpecMessageUnlockFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_UNLOCK_FILE_RESULT,            gNtIpcTypeSpecMessageGenericFileIoResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_SECURITY_FILE,           gNtIpcTypeSpecMessageQuerySecurityFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_QUERY_SECURITY_FILE_RESULT,    gNtIpcTypeSpecMessageGenericFileBufferResult),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_SET_SECURITY_FILE,             gNtIpcTypeSpecMessageSetSecurityFile),
    LWMSG_MESSAGE(NT_IPC_MESSAGE_TYPE_SET_SECURITY_FILE_RESULT,      gNtIpcTypeSpecMessageGenericFileIoResult),
    LWMSG_PROTOCOL_END
};

NTSTATUS
NtIpcLWMsgStatusToNtStatus(
    IN LWMsgStatus LwMsgStatus
    )
{
    NTSTATUS status = STATUS_NONE_MAPPED;

    switch (LwMsgStatus)
    {
    case LWMSG_STATUS_SUCCESS:
        status = STATUS_SUCCESS;
        break;
    case LWMSG_STATUS_PENDING:
        status = STATUS_PENDING;
        break;
    case LWMSG_STATUS_ERROR:
        status = STATUS_UNSUCCESSFUL;
        break;
    case LWMSG_STATUS_MEMORY:
        status = STATUS_INSUFFICIENT_RESOURCES;
        break;
    case LWMSG_STATUS_EOF:
        status = STATUS_END_OF_FILE;
        break;
    case LWMSG_STATUS_NOT_FOUND:
        status = STATUS_NOT_FOUND;
        break;
    case LWMSG_STATUS_UNIMPLEMENTED:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    case LWMSG_STATUS_INVALID_PARAMETER:
        status = STATUS_INVALID_PARAMETER;
        break;
    case LWMSG_STATUS_OVERFLOW:
        status = STATUS_INTEGER_OVERFLOW;
        break;
    case LWMSG_STATUS_UNDERFLOW:
        // TODO -- check this:
        status = STATUS_INTEGER_OVERFLOW;
        break;
    case LWMSG_STATUS_TIMEOUT:
        status = STATUS_IO_TIMEOUT;
        break;
    case LWMSG_STATUS_SECURITY:
        status = STATUS_ACCESS_DENIED;
        break;
    case LWMSG_STATUS_FILE_NOT_FOUND:
        // Cannot use either of these because we do not know
        // whether the last component or some intermediate component
        // was not found.
        // - STATUS_OBJECT_NAME_NOT_FOUND
        // - STATUS_OBJECT_PATH_NOT_FOUND
        // So we use this:
        // - STATUS_NOT_FOUND
        // TODO  -- it may be that we should do STATUS_OBJECT_NAME_NOT_FOUND
        // (need to check Win32 for that).
        status = STATUS_NOT_FOUND;
        break;
    case LWMSG_STATUS_INVALID_HANDLE:
        status = STATUS_INVALID_HANDLE;
        break;
    case LWMSG_STATUS_UNSUPPORTED:
        status = STATUS_NOT_SUPPORTED;
        break;
    case LWMSG_STATUS_CANCELLED:
        status = STATUS_CANCELLED;
        break;
        // TODO -- map these lwmsg status codes:
    case LWMSG_STATUS_AGAIN:
    case LWMSG_STATUS_MALFORMED:
    case LWMSG_STATUS_SYSTEM:
    case LWMSG_STATUS_CONNECTION_REFUSED:
    case LWMSG_STATUS_INVALID_STATE:
    case LWMSG_STATUS_PEER_RESET:
    case LWMSG_STATUS_PEER_CLOSE:
    case LWMSG_STATUS_PEER_ABORT:
    case LWMSG_STATUS_SESSION_LOST:
        status = STATUS_UNSUCCESSFUL;
        break;
        
    default:
        LWIO_LOG_ERROR("Failed to map lwmsg error %", LwMsgStatus);
        status = STATUS_NONE_MAPPED;
        break;
    }

    return status;
}

LWMsgStatus
NtIpcNtStatusToLWMsgStatus(
    IN NTSTATUS Status
    )
{
    LWMsgStatus lwMsgStatus = LWMSG_STATUS_ERROR;

    // TODO -- Implement this properly

    switch (Status)
    {
    case STATUS_SUCCESS:
        lwMsgStatus = LWMSG_STATUS_SUCCESS;
        break;
    case STATUS_PENDING:
        lwMsgStatus = LWMSG_STATUS_PENDING;
        break;
    case STATUS_CANCELLED:
        lwMsgStatus = LWMSG_STATUS_CANCELLED;
        break;
    case STATUS_NOT_IMPLEMENTED:
        lwMsgStatus = LWMSG_STATUS_UNIMPLEMENTED;
        break;
    default:
        lwMsgStatus = LWMSG_STATUS_ERROR;
    }

    return lwMsgStatus;
}

NTSTATUS
NtIpcAddProtocolSpec(
    IN OUT LWMsgProtocol* pProtocol
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    LWMsgStatus msgStatus = 0;

    msgStatus = lwmsg_protocol_add_protocol_spec(pProtocol, gNtIpcProtocolSpec);
    status = NtIpcLWMsgStatusToNtStatus(msgStatus);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
NtIpcUnregisterFileHandle(
    IN LWMsgCall* pCall,
    IN LWMsgHandle* FileHandle
    )
{
    NTSTATUS status = 0;
    LWMsgSession* pSession = NULL;

    pSession = lwmsg_call_get_session(pCall);

    status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_session_unregister_handle(
            pSession,
            FileHandle));
    return status;
}

VOID
NtIpcReleaseFileHandle(
    IN LWMsgCall* pCall,
    IN LWMsgHandle* FileHandle
    )
{
    LWMsgSession* pSession = NULL;

    pSession = lwmsg_call_get_session(pCall);

    lwmsg_session_release_handle(pSession, FileHandle);
}
