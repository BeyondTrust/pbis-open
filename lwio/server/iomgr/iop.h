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

#include "config.h"
#include "lwiosys.h"

#include <lwio/iodriver.h>
#include "ioinit.h"
#include <lw/base.h>
#include <lw/rtlgoto.h>
#include <reg/lwntreg.h>

#include "lwlist.h"
#include "lwioutils.h"
#include "ntlogmacros.h"

#include "lwthreads.h"

#define NT_PENDING_OR_SUCCESS_OR_NOT(status) \
    (LW_NT_SUCCESS_OR_NOT(status) || (STATUS_PENDING == (status)))

#define IsValidStatusForIrpType(Status, IrpType) \
    (NT_PENDING_OR_SUCCESS_OR_NOT(Status) || \
     ((IRP_TYPE_READ_DIRECTORY_CHANGE == (IrpType)) && \
      (STATUS_NOTIFY_ENUM_DIR == (Status))) || \
     ((IRP_TYPE_CREATE == (IrpType)) && \
      (STATUS_OPLOCK_BREAK_IN_PROGRESS == (Status))) || \
     ((IRP_TYPE_FS_CONTROL == (IrpType)) && \
      (STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE == (Status))) || \
     ((IRP_TYPE_DEVICE_IO_CONTROL == (IrpType)) && \
      (STATUS_MORE_ENTRIES == (Status))))

typedef struct _IOP_ROOT_STATE {
    // Must point to global memory.
    PIO_STATIC_DRIVER pStaticDrivers;

    // Protects driver count/list
    LW_RTL_MUTEX DriverMutex;
    // Diagnostics Only
    ULONG DriverCount;
    // List of IO_DRIVER_OBJECT
    LW_LIST_LINKS DriverObjectList;

    // Protects device count/list
    LW_RTL_MUTEX DeviceMutex;
    // Diagnostics Only
    ULONG DeviceCount;
    // List of IO_DEVICE_OBJECT (should really be a hash table)
    LW_LIST_LINKS DeviceObjectList;

    // Protects initialization of MapSecurityContext
    LW_RTL_MUTEX InitMutex;
    PLW_MAP_SECURITY_CONTEXT MapSecurityContext;
} IOP_ROOT_STATE, *PIOP_ROOT_STATE;

typedef ULONG IO_DRIVER_OBJECT_FLAGS, *PIO_DRIVER_OBJECT_FLAGS;

#define IO_DRIVER_OBJECT_FLAG_INITIALIZED   0x00000001
#define IO_DRIVER_OBJECT_FLAG_READY         0x00000002
#define IO_DRIVER_OBJECT_FLAG_RUNDOWN       0x00000004
#define IO_DRIVER_OBJECT_FLAG_UNLOADING     0x00000008

struct _IO_DRIVER_OBJECT {
    LONG ReferenceCount;
    IO_DRIVER_OBJECT_FLAGS Flags;

    // Immutable after initialization
    PIOP_ROOT_STATE Root;
    UNICODE_STRING DriverName;
    PSTR pszDriverName;
    PSTR pszDriverPath;
    PVOID LibraryHandle;
    PIO_DRIVER_ENTRY DriverEntry;

    // Immutable after DriverEntry
    struct {
        PIO_DRIVER_SHUTDOWN_CALLBACK Shutdown;
        PIO_DRIVER_DISPATCH_CALLBACK Dispatch;
        PIO_DRIVER_REFRESH_CALLBACK Refresh;
    } Callback;
    PVOID Context;

    // Protects Flags and DeviceList.
    LW_RTL_MUTEX Mutex;

    // Devices - list of IO_DEVICE_OBJECT
    LW_LIST_LINKS DeviceList;
    ULONG DeviceCount;

    // For each list to which this object belongs:
    // - Entry in IOP_ROOT_STATE.DriverObjectList
    LW_LIST_LINKS RootLinks;
};

typedef ULONG IO_DEVICE_OBJECT_FLAGS, *PIO_DEVICE_OBJECT_FLAGS;

#define IO_DEVICE_OBJECT_FLAG_RUNDOWN       0x00000001
#define IO_DEVICE_OBJECT_FLAG_RUNDOWN_DRIVER 0x00000002

struct _IO_DEVICE_OBJECT {
    LONG ReferenceCount;
    IO_DEVICE_OBJECT_FLAGS Flags;

    // Immutable after initialization
    UNICODE_STRING DeviceName;
    PIO_DRIVER_OBJECT Driver;
    PVOID Context;
    // TODO: Add to IoDeviceCreate
    DEVICE_TYPE DeviceType;

    // Protects Flags and FileObjectsList.
    LW_RTL_MUTEX Mutex;

    // File objects for this device (list of IO_FILE_OBJECT).
    LW_LIST_LINKS FileObjectsList;

    // Used to synchronize IRP dispatch/cancel/complete.
    LW_RTL_MUTEX CancelMutex;

    // For each list to which this object belongs:
    // - Entry in IO_DRIVER_OBJECT.DevicetList
    LW_LIST_LINKS DriverLinks;
    // - Entry in IOP_ROOT_STATE.DeviceObjectList
    LW_LIST_LINKS RootLinks;
    // - Entry in IopDeviceRundown()'s rundownList
    LW_LIST_LINKS RundownLinks;
};

typedef ULONG FILE_OBJECT_FLAGS;

#define FILE_OBJECT_FLAG_CREATE_DONE        0x00000001
#define FILE_OBJECT_FLAG_CANCELLED          0x00000002
#define FILE_OBJECT_FLAG_RUNDOWN            0x00000004
#define FILE_OBJECT_FLAG_CLOSE_DONE         0x00000008
#define FILE_OBJECT_FLAG_RELATIVE           0x00000010
#define FILE_OBJECT_FLAG_RUNDOWN_WAIT       0x00000020

struct _IO_FILE_OBJECT {
    LONG ReferenceCount;
    PIO_DEVICE_OBJECT pDevice;
    PVOID pContext;

    UNICODE_STRING FileName;

    FILE_OBJECT_FLAGS Flags;

    // TODO -- Track file vs named pipe

    LW_RTL_MUTEX Mutex;

    // Count of IRPs that have dispatched but are not complete.
    LONG DispatchedIrpCount;

    // List of referencing IRPs (via IRP_INTERNAL.FileObjectLinks)
    LW_LIST_LINKS IrpList;

    // Links for IO_DEVICE_OBJECT.FileObjectsList
    LW_LIST_LINKS DeviceLinks;

    // Links used for device object rundown
    LW_LIST_LINKS RundownLinks;

    struct {
        LW_RTL_CONDITION_VARIABLE Condition;
        PIO_ASYNC_COMPLETE_CALLBACK Callback;
        PVOID CallbackContext;
        PIO_STATUS_BLOCK pIoStatusBlock;
    } Rundown;

    LW_ZCT_ENTRY_MASK ZctReadMask;
    LW_ZCT_ENTRY_MASK ZctWriteMask;

    // Extra IRPs -- These are allocated before the operation
    // that requires the IRP so that we can cleanup without
    // allocating more memory and/or so we can remember parameters
    // from an earlier operation (in the case of ZCT completion).
    PIRP pCloseIrp;
    LW_LIST_LINKS ZctCompletionIrpList;
};

// ioinit.c

NTSTATUS
IopParse(
    IN PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice,
    OUT PUNICODE_STRING pRemainingPath
    );

NTSTATUS
IopGetMapSecurityContext(
    OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    );

// ioroot.c

VOID
IopRootFree(
    IN OUT PIOP_ROOT_STATE* ppRoot
    );

NTSTATUS
IopRootCreate(
    OUT PIOP_ROOT_STATE* ppRoot,
    IN OPTIONAL PIO_STATIC_DRIVER pStaticDrivers
    );

NTSTATUS
IopRootQueryStateDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName,
    OUT PLWIO_DRIVER_STATE pState
    );

NTSTATUS
IopRootLoadDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName
    );

NTSTATUS
IopRootUnloadDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName
    );

NTSTATUS
IopRootRefreshConfig(
    IN PIOP_ROOT_STATE pRoot
    );

PIO_DEVICE_OBJECT
IopRootFindDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDeviceName
    );

NTSTATUS
IopRootInsertDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PIO_DRIVER_OBJECT pDriver
    );

VOID
IopRootRemoveDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    );

NTSTATUS
IopRootInsertDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PIO_DEVICE_OBJECT pDevice
    );

VOID
IopRootRemoveDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    );

NTSTATUS
IopRootParse(
    IN PIOP_ROOT_STATE pRoot,
    IN PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice,
    OUT PUNICODE_STRING pRemainingPath
    );

NTSTATUS
IopRootGetMapSecurityContext(
    IN PIOP_ROOT_STATE pRoot,
    OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    );

// iodriver.c

VOID
IopDriverUnload(
    IN OUT PIO_DRIVER_OBJECT* ppDriverObject
    );

NTSTATUS
IopDriverLoad(
    OUT PIO_DRIVER_OBJECT* ppDriverObject,
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDriverName,
    IN PCSTR pszDriverName,
    IN OPTIONAL PIO_DRIVER_ENTRY pStaticDriverEntry,
    IN OPTIONAL PCSTR pszDriverPath
    );

VOID
IopDriverInsertDevice(
    IN PIO_DRIVER_OBJECT pDriverObject,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    );

VOID
IopDriverRemoveDevice(
    IN PIO_DRIVER_OBJECT pDriverObject,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    );

VOID
IopDriverReference(
    IN PIO_DRIVER_OBJECT pDriverObject
    );

VOID
IopDriverDereference(
    IN OUT PIO_DRIVER_OBJECT* ppDriverObject
    );

VOID
IopDriverLock(
    IN PIO_DRIVER_OBJECT pDriverObject
    );

VOID
IopDriverUnlock(
    IN PIO_DRIVER_OBJECT pDriverObject
    );

// iodevice.c

NTSTATUS
IopDeviceCallDriver(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN OUT PIRP pIrp
    );

VOID
IopDeviceLock(
    IN PIO_DEVICE_OBJECT pDeviceObject
    );

VOID
IopDeviceUnlock(
    IN PIO_DEVICE_OBJECT pDeviceObject
    );

VOID
IopDeviceReference(
    IN PIO_DEVICE_OBJECT pDeviceObject
    );

VOID
IopDeviceDereference(
    IN OUT PIO_DEVICE_OBJECT* ppDeviceObject
    );

NTSTATUS
IopDeviceRundown(
    IN PIO_DEVICE_OBJECT pDeviceObject
    );

// ioirp.c

NTSTATUS
IopIrpCreateDetached(
    OUT PIRP* ppIrp
    );

NTSTATUS
IopIrpAttach(
    IN OUT PIRP pIrp,
    IN IRP_TYPE Type,
    IN PIO_FILE_OBJECT pFileObject
    );

NTSTATUS
IopIrpCreate(
    OUT PIRP* ppIrp,
    IN IRP_TYPE Type,
    IN PIO_FILE_OBJECT pFileObject
    );

VOID
IopIrpReference(
    IN PIRP Irp
    );

VOID
IopIrpDereference(
    IN OUT PIRP* Irp
    );

PIRP
IopIrpGetIrpFromAsyncCancelContext(
    IN PIO_ASYNC_CANCEL_CONTEXT Context
    );

BOOLEAN
IopIrpCancel(
    IN PIRP pIrp
    );

VOID
IopIrpSetOutputCreate(
    IN OUT PIRP pIrp,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    IN PIO_FILE_HANDLE pCreateFileHandle
    );

VOID
IopIrpSetOutputPrepareZctReadWrite(
    IN OUT PIRP pIrp,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    IN PVOID* pCompletionContext,
    IN PIRP pCompletionIrp
    );

NTSTATUS
IopIrpDispatch(
    IN PIRP pIrp,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK pIoStatusBlock
    );

VOID
IopIrpCancelFileObject(
    IN PIO_FILE_OBJECT pFileObject,
    IN BOOLEAN IsForRundown
    );

VOID
IopIrpFreeZctIrpList(
    IN OUT PIO_FILE_OBJECT pFileObject
    );

PVOID
IopIrpSaveZctIrp(
    IN OUT PIO_FILE_OBJECT pFileObject,
    IN PIRP pIrp,
    IN PVOID pCompletionContext
    );

PIRP
IopIrpLoadZctIrp(
    IN OUT PIO_FILE_OBJECT pFileObject,
    IN PVOID pCompletionContext
    );

// iofile.c

VOID
IopFileObjectLock(
    IN PIO_FILE_OBJECT pFileObject
    );

VOID
IopFileObjectUnlock(
    IN PIO_FILE_OBJECT pFileObject
    );

VOID
IopFileObjectReference(
    IN PIO_FILE_OBJECT pFileObject
    );

VOID
IopFileObjectDereference(
    IN OUT PIO_FILE_OBJECT* ppFileObject
    );

NTSTATUS
IopFileObjectAllocate(
    OUT PIO_FILE_OBJECT* ppFileObject,
    IN PIO_DEVICE_OBJECT pDevice,
    IN PIO_FILE_NAME FileName
    );

VOID
IopFileObjectFree(
    IN OUT PIO_FILE_OBJECT* ppFileObject
    );

VOID
IopFileGetZctSupportMask(
    IN IO_FILE_HANDLE FileHandle,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctReadMask,
    OUT OPTIONAL PLW_ZCT_ENTRY_MASK ZctWriteMask
    );

NTSTATUS
IopFileObjectRundown(
    IN PIO_FILE_OBJECT pFileObject
    );

NTSTATUS
IopFileObjectRundownEx(
    IN OUT PIO_FILE_OBJECT pFileObject,
    IN OPTIONAL PIO_ASYNC_COMPLETE_CALLBACK Callback,
    IN OPTIONAL PVOID CallbackContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTSTATUS
IopFileObjectAddDispatched(
    IN PIO_FILE_OBJECT pFileObject,
    IN IRP_TYPE Type
    );

VOID
IopFileObjectRemoveDispatched(
    IN PIO_FILE_OBJECT pFileObject,
    IN IRP_TYPE Type
    );

// iosecurity.c

VOID
IopSecurityReferenceSecurityContext(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    );

// ioapi.c -- internally available

// TODO--Replace with Iop helper function
VOID
IoDereferenceAsyncCancelContext(
    IN OUT PIO_ASYNC_CANCEL_CONTEXT* AsyncCancelContext
    );
