/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Module Name:
 *
 *        iodriver.h
 *
 * Abstract:
 *
 *        IO Manager Driver Header File
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#ifndef __IODRIVER_H__
#define __IODRIVER_H__

#include <lwio/lwio.h>
#include "lwzct.h"

struct _IO_DRIVER_OBJECT;
typedef struct _IO_DRIVER_OBJECT IO_DRIVER_OBJECT, *PIO_DRIVER_OBJECT;
typedef IO_DRIVER_OBJECT *IO_DRIVER_HANDLE, **PIO_DRIVER_HANDLE;

struct _IO_DEVICE_OBJECT;
typedef struct _IO_DEVICE_OBJECT IO_DEVICE_OBJECT, *PIO_DEVICE_OBJECT;
typedef IO_DEVICE_OBJECT *IO_DEVICE_HANDLE, **PIO_DEVICE_HANDLE;

typedef ULONG IRP_TYPE;

#define IRP_TYPE_UNINITIALIZED             0
#define IRP_TYPE_CREATE                    1
#define IRP_TYPE_CLOSE                     2
#define IRP_TYPE_READ                      3
#define IRP_TYPE_WRITE                     4
#define IRP_TYPE_DEVICE_IO_CONTROL         5
#define IRP_TYPE_FS_CONTROL                6
#define IRP_TYPE_FLUSH_BUFFERS             7
#define IRP_TYPE_QUERY_INFORMATION         8
#define IRP_TYPE_SET_INFORMATION           9
#define IRP_TYPE_CREATE_NAMED_PIPE        10
#define IRP_TYPE_CREATE_MAILSLOT          11
// In Windows, DIRECTORY_CONTROL has two subtypes:
// - QUERY_DIRECTORY
// - READ_CHANGE_DIRECTORY
// Here, however, two separate IRPs are used.
#define IRP_TYPE_QUERY_DIRECTORY          12
#define IRP_TYPE_READ_DIRECTORY_CHANGE    13
#define IRP_TYPE_QUERY_VOLUME_INFORMATION 14
#define IRP_TYPE_SET_VOLUME_INFORMATION   15
#define IRP_TYPE_LOCK_CONTROL             16
#define IRP_TYPE_QUERY_SECURITY           17
#define IRP_TYPE_SET_SECURITY             18
#if 0
#define IPP_TYPE_QUERY_EA                 19
#define IPP_TYPE_SET_EA                   20
#endif
#define IRP_TYPE_QUERY_QUOTA              21
#define IRP_TYPE_SET_QUOTA                22
#if 0
#define IRP_TYPE_QUERY_FULL_ATTRIBUTES    23
#define IRP_TYPE_RENAME                   24
#define IRP_TYPE_LINK                     25
#define IRP_TYPE_UNLINK                   26
#endif

// IRP_IO_FLAGS may include flags related to IO_FLAGS flags as
// well as flags that are specific to specific parameters for
// a given IRP type.  Never mix IO_FLAGS and IRP_IO_FLAGS values.

typedef ULONG IRP_IO_FLAGS;

#define IRP_IO_FLAG_PAGING_IO           0x00000001
#define IRP_IO_FLAG_WRITE_THROUGH       0x00000002

typedef ULONG IO_LOCK_CONTROL;

#define IO_LOCK_CONTROL_LOCK              1
#define IO_LOCK_CONTROL_UNLOCK            2
#define IO_LOCK_CONTROL_UNLOCK_ALL_BY_KEY 3
#define IO_LOCK_CONTROL_UNLOCK_ALL        4

typedef UINT8 IRP_ZCT_OPERATION, *PIRP_ZCT_OPERATION;

#define IRP_ZCT_OPERATION_NONE      0
#define IRP_ZCT_OPERATION_PREPARE   1
#define IRP_ZCT_OPERATION_COMPLETE  2

// "Storage" field is so we do not have to allocate
// extra memory blocks for small optional parameters
// that are provided via pointers.

typedef struct _IRP_ARGS_CREATE {
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext;
    IN IO_FILE_NAME FileName;
    IN ACCESS_MASK DesiredAccess;
    IN OPTIONAL LONG64 AllocationSize;
    IN FILE_ATTRIBUTES FileAttributes;
    IN FILE_SHARE_FLAGS ShareAccess;
    IN FILE_CREATE_DISPOSITION CreateDisposition;
    IN FILE_CREATE_OPTIONS CreateOptions;
    IN OPTIONAL PFILE_FULL_EA_INFORMATION EaBuffer;
    IN ULONG EaLength;
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;
    IN OPTIONAL PVOID SecurityQualityOfService; // TBD
    IN OPTIONAL PIO_ECP_LIST EcpList;
} IRP_ARGS_CREATE, *PIRP_ARGS_CREATE;

typedef struct _IRP_ARGS_READ_WRITE {
    // IN for write, OUT for read
    union {
        IN OUT PVOID Buffer;
        IN OUT PLW_ZCT_VECTOR Zct;
    };
    IN ULONG Length;
    IN OPTIONAL PULONG64 ByteOffset;
    IN OPTIONAL PULONG Key;
    // For ZCT read/write
    IN IRP_ZCT_OPERATION ZctOperation;
    IN OUT PVOID ZctCompletionContext;
    IN ULONG ZctWriteBytesTransferred;
    // Storage area for optional parameters
    struct {
        ULONG64 ByteOffset;
        ULONG Key;
    } Storage;
} IRP_ARGS_READ_WRITE, *PIRP_ARGS_READ_WRITE;

typedef struct _IRP_ARGS_IO_FS_CONTROL {
    IN ULONG ControlCode;
    IN PVOID InputBuffer;
    IN ULONG InputBufferLength;
    OUT PVOID OutputBuffer;
    IN ULONG OutputBufferLength;
} IRP_ARGS_IO_FS_CONTROL, *PIRP_ARGS_IO_FS_CONTROL;

typedef struct _IRP_ARGS_QUERY_SET_INFORMATION {
    IN OUT PVOID FileInformation;
    IN ULONG Length;
    IN FILE_INFORMATION_CLASS FileInformationClass;
} IRP_ARGS_QUERY_SET_INFORMATION, *PIRP_ARGS_QUERY_SET_INFORMATION;

typedef struct _IRP_ARGS_QUERY_DIRECTORY {
    OUT PVOID FileInformation;
    IN ULONG Length;
    IN FILE_INFORMATION_CLASS FileInformationClass;
    IN BOOLEAN ReturnSingleEntry;
    IN OPTIONAL PIO_MATCH_FILE_SPEC FileSpec;
    IN BOOLEAN RestartScan;
    struct {
        IO_MATCH_FILE_SPEC FileSpec;
    } Storage;
} IRP_ARGS_QUERY_DIRECTORY, *PIRP_ARGS_QUERY_DIRECTORY;

typedef struct _IRP_ARGS_READ_DIRECTORY_CHANGE {
    OUT PVOID Buffer;
    IN ULONG Length;
    IN BOOLEAN WatchTree;
    IN FILE_NOTIFY_CHANGE NotifyFilter;
    IN OPTIONAL PULONG MaxBufferSize;
    struct {
        ULONG MaxBufferSize;
    } Storage;
} IRP_ARGS_READ_DIRECTORY_CHANGE, *PIRP_ARGS_READ_DIRECTORY_CHANGE;

typedef struct _IRP_ARGS_QUERY_SET_VOLUME_INFORMATION {
    IN OUT PVOID FsInformation;
    IN ULONG Length;
    IN FS_INFORMATION_CLASS FsInformationClass;
} IRP_ARGS_QUERY_SET_VOLUME_INFORMATION, *PIRP_ARGS_QUERY_VOLUME_INFORMATION;

typedef struct _IRP_ARGS_LOCK_CONTROL {
    IN IO_LOCK_CONTROL LockControl;
    IN ULONG64 ByteOffset;
    IN ULONG64 Length;
    IN ULONG Key;
    IN BOOLEAN FailImmediately;
    IN BOOLEAN ExclusiveLock;
} IRP_ARGS_LOCK_CONTROL, *PIRP_ARGS_LOCK_CONTROL;

typedef struct _IRP_ARGS_QUERY_SET_SECURITY {
    IN SECURITY_INFORMATION SecurityInformation;
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;
    IN ULONG Length;
} IRP_ARGS_QUERY_SET_SECURITY, *PIRP_ARGS_QUERY_SET_SECUIRTY;

typedef struct _IRP_ARGS_QUERY_QUOTA {
    OUT PFILE_QUOTA_INFORMATION Buffer;
    IN ULONG Length;
    IN BOOLEAN ReturnSingleEntry;
    IN OPTIONAL PFILE_GET_QUOTA_INFORMATION SidList;
    IN ULONG SidListLength;
    IN OPTIONAL PFILE_GET_QUOTA_INFORMATION StartSid;
    IN BOOLEAN RestartScan;
} IRP_ARGS_QUERY_QUOTA, *PIRP_ARGS_QUERY_QUOTA;

typedef struct _IRP_ARGS_SET_QUOTA {
    IN PFILE_QUOTA_INFORMATION Buffer;
    IN ULONG Length;
} IRP_ARGS_SET_QUOTA, *PIRP_ARGS_SET_QUOTA;

typedef struct _IRP {
    IN IRP_TYPE Type;
    IN IRP_IO_FLAGS Flags;
    OUT IO_STATUS_BLOCK IoStatusBlock;
    IN IO_DRIVER_HANDLE DriverHandle;
    IN IO_DEVICE_HANDLE DeviceHandle;
    IN IO_FILE_HANDLE FileHandle;
    union {
        // IRP_TYPE_CREATE
        IRP_ARGS_CREATE Create;
        // IRP_TYPE_READ, IRP_TYPE_WRITE
        IRP_ARGS_READ_WRITE ReadWrite;
        // IRP_TYPE_IO_CONTROL, IRP_TYPE_FS_CONTROL
        IRP_ARGS_IO_FS_CONTROL IoFsControl;
        // IRP_TYPE_QUERY_INFORMATION, IRP_TYPE_SET_INFORMATION
        IRP_ARGS_QUERY_SET_INFORMATION QuerySetInformation;
        // IRP_TYPE_QUERY_DIRECTORY
        IRP_ARGS_QUERY_DIRECTORY QueryDirectory;
        // IRP_TYPE_READ_DIRECTORY_CHANGE
        IRP_ARGS_READ_DIRECTORY_CHANGE ReadDirectoryChange;
        // IRP_TYPE_QUERY_VOLUME
        IRP_ARGS_QUERY_SET_VOLUME_INFORMATION QuerySetVolumeInformation;
        // IRP_TYPE_LOCK_CONTROL
        IRP_ARGS_LOCK_CONTROL LockControl;	    
        // IRP_TYPE_QUERY_SECURITY, IRP_TYPE_SET_SECURITY
        IRP_ARGS_QUERY_SET_SECURITY QuerySetSecurity;
        // IRP_TYPE_QUERY_QUOTA
        IRP_ARGS_QUERY_QUOTA QueryQuota;
        // IRP_TYPE_SET_QUOTA
        IRP_ARGS_SET_QUOTA SetQuota;
        // No args for IRP_TYPE_CLOSE, IRP_TYPE_FLUSH
    } Args;
    // TODO: Rename Args to Params?
    // Internal data at the end...
} IRP, *PIRP;

typedef VOID (*PIO_IRP_CALLBACK)(
    IN PIRP Irp,
    IN PVOID CallbackContext
    );

typedef VOID (*PIO_DRIVER_SHUTDOWN_CALLBACK)(
    IN IO_DRIVER_HANDLE DriverHandle
    );

typedef NTSTATUS (*PIO_DRIVER_DISPATCH_CALLBACK)(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP Irp
    );

typedef NTSTATUS (*PIO_DRIVER_REFRESH_CALLBACK)(
    IN IO_DRIVER_HANDLE DriverHandle
    );

typedef NTSTATUS (*PIO_DRIVER_ENTRY)(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    );

typedef struct _IO_STATIC_DRIVER
{
    PCSTR pszName;
    PIO_DRIVER_ENTRY pEntry;
} IO_STATIC_DRIVER, *PIO_STATIC_DRIVER;

#define IO_DRIVER_ENTRY_FUNCTION_NAME "DriverEntry"
#define IO_DRIVER_ENTRY_INTERFACE_VERSION 1

// Driver functions

// Can only be called in DriverEntry.
NTSTATUS
IoDriverInitialize(
    IN OUT IO_DRIVER_HANDLE DriverHandle,
    IN OPTIONAL PVOID DriverContext,
    IN PIO_DRIVER_SHUTDOWN_CALLBACK ShutdownCallback,
    IN PIO_DRIVER_DISPATCH_CALLBACK DispatchCallback
    );

NTSTATUS
IoDriverRegisterRefreshCallback(
    IN OUT IO_DRIVER_HANDLE DriverHandle,
    IN PIO_DRIVER_REFRESH_CALLBACK RefreshCallback
    );

PCSTR
IoDriverGetName(
    IN IO_DRIVER_HANDLE DriverHandle
    );

PVOID
IoDriverGetContext(
    IN IO_DRIVER_HANDLE DriverHandle
    );


// Device functions

NTSTATUS
IoDeviceCreate(
    OUT PIO_DEVICE_HANDLE pDeviceHandle,
    IN IO_DRIVER_HANDLE DriverHandle,
    IN PCSTR pszName,
    IN OPTIONAL PVOID DeviceContext
    );

VOID
IoDeviceDelete(
    IN OUT PIO_DEVICE_HANDLE pDeviceHandle
    );

PVOID
IoDeviceGetContext(
    IN IO_DEVICE_HANDLE DeviceHandle
    );

// File functions

NTSTATUS
IoFileSetContext(
    IN IO_FILE_HANDLE FileHandle,
    IN PVOID FileContext
    );

PVOID
IoFileGetContext(
    IN IO_FILE_HANDLE FileHandle
    );

// The FSD must synchronize calling this itself
VOID
IoFileSetZctSupportMask(
    IN IO_FILE_HANDLE FileHandle,
    IN LW_ZCT_ENTRY_MASK ZctReadMask,
    IN LW_ZCT_ENTRY_MASK ZctWriteMask
    );

// IRP functions for async processing

#if 0
BOOLEAN
IoIrpIsCancelled(
    IN PIRP pIrp
    );
#endif

VOID
IoIrpMarkPending(
    IN PIRP pIrp,
    IN PIO_IRP_CALLBACK CancelCallback,
    IN OPTIONAL PVOID CancelCallbackContext
    );

#if 0
VOID
IoIrpSetCancelRoutine(
    IN PIRP pIrp,
    IN OPTIONAL PIO_IRP_CALLBACK CancelCallback,
    IN OPTIONAL PVOID CancelCallbackContext
    );
#endif

// must have set IO status block in IRP.
VOID
IoIrpComplete(
    IN OUT PIRP Irp
    );

// Drivrer memory

VOID
IoMemoryZero(
    IN OUT PVOID pMemory,
    IN size_t Size
    );

PVOID
IoMemoryAllocate(
    IN size_t Size
    );

VOID
IoMemoryFree(
    IN OUT PVOID pMemory
    );

#define IO_ALLOCATE(ppMemory, Type, Size) \
    ( (*(ppMemory)) = (Type*) IoMemoryAllocate(Size), (*(ppMemory)) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES )

#define IO_FREE(ppMemory) \
    do { \
        if (*(ppMemory)) \
        { \
            IoMemoryFree(*(ppMemory)); \
            (*(ppMemory)) = NULL; \
        } \
    } while (0)

//
// Security Context
//

PIO_SECURITY_CONTEXT_PROCESS_INFORMATION
IoSecurityGetProcessInfo(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    );

PACCESS_TOKEN
IoSecurityGetAccessToken(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    );

LW_PIO_CREDS
IoSecurityGetCredentials(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    );

VOID
IoSecurityDereferenceSecurityContext(
    IN OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext
    );

NTSTATUS
IoSecurityCreateSecurityContextFromUidGid(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN uid_t Uid,
    IN gid_t Gid,
    IN OPTIONAL LW_PIO_CREDS Credentials
    );

NTSTATUS
IoSecurityCreateSecurityContextFromUsername(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN PUNICODE_STRING Username
    );

NTSTATUS
IoSecurityCreateSecurityContextFromGssContext(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN LW_MAP_SECURITY_GSS_CONTEXT hGssContext
    );

NTSTATUS
IoSecurityCreateSecurityContextFromNtlmLogon(
    OUT PIO_CREATE_SECURITY_CONTEXT* ppSecurityContext,
    OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmLogonResult,
    IN PLW_MAP_SECURITY_NTLM_LOGON_INFO pNtlmLogonInfo
    );

VOID
IoSecurityFreeNtlmLogonResult(
    IN OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmLogonResult
    );

//
// Logging
//

PCSTR
IoGetIrpTypeString(
    IN IRP_TYPE Type
    );

#define IO_LOG_ENTER(Format, ...) \
    LWIO_LOG_DEBUG("ENTER: " Format, ## __VA_ARGS__)

#define IO_LOG_LEAVE(Format, ...) \
    LWIO_LOG_DEBUG("LEAVE: " Format, ## __VA_ARGS__)

#define IO_LOG_ENTER_LEAVE(Format, ...) \
    LWIO_LOG_DEBUG("ENTER/LEAVE: " Format, ## __VA_ARGS__)

#define IO_LOG_LEAVE_ON_STATUS_EE(status, EE) \
    do { \
        if (EE || status) \
        { \
            IO_LOG_LEAVE("-> 0x%08x (EE = %d)", status, EE); \
        } \
    } while (0)

#define IO_LOG_ENTER_LEAVE_STATUS_EE(status, EE) \
    IO_LOG_ENTER_LEAVE("-> 0x%08x (EE = %d)", status, EE)

#define IO_LOG_ENTER_LEAVE_STATUS_EE_EX(status, EE, Format, ...) \
    IO_LOG_ENTER_LEAVE(Format " -> 0x%08x (EE = %d)", ## __VA_ARGS__, status, EE)

#define IO_LOG_LEAVE_STATUS_EE(status, EE) \
    IO_LOG_LEAVE("-> 0x%08x (EE = %d)", status, EE)

#define IO_LOG_LEAVE_STATUS_EE_EX(status, EE, Format, ...) \
    IO_LOG_LEAVE(Format " -> 0x%08x (EE = %d)", ## __VA_ARGS__, status, EE)

#ifdef ENABLE_STATIC_DRIVERS
#define IO_DRIVER_ENTRY(name) DriverEntry_##name
#else
#define IO_DRIVER_ENTRY(name) DriverEntry
#endif

#define IO_STATIC_DRIVER_ENTRY(name) {#name, IO_DRIVER_ENTRY(name)}
#define IO_STATIC_DRIVER_END {NULL, NULL}

#endif /* __IODRIVER_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
