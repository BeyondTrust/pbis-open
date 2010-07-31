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

#include "iodriver.h"
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
      (STATUS_NOTIFY_ENUM_DIR == (Status))))

typedef struct _IOP_DRIVER_CONFIG {
    PSTR pszName;
    PSTR pszPath;
    LW_LIST_LINKS Links;
} IOP_DRIVER_CONFIG, *PIOP_DRIVER_CONFIG;

typedef struct _IOP_CONFIG {
    LW_LIST_LINKS DriverConfigList;
    ULONG DriverCount;
} IOP_CONFIG, *PIOP_CONFIG;

typedef struct _IOP_CONFIG_PARSE_STATE {
    PIOP_CONFIG pConfig;
    PIOP_DRIVER_CONFIG pDriverConfig;
    NTSTATUS Status;
} IOP_CONFIG_PARSE_STATE, *PIOP_CONFIG_PARSE_STATE;

typedef struct _IOP_ROOT_STATE {
    PIOP_CONFIG Config;
    // Diagnostics Only
    ULONG DriverCount;
    LW_LIST_LINKS DriverObjectList;
    // Diagnostics Only
    ULONG DeviceCount;
    // Should really be a hash table...
    LW_LIST_LINKS DeviceObjectList;
    LW_RTL_MUTEX InitMutex;
    PLW_MAP_SECURITY_CONTEXT MapSecurityContext;
} IOP_ROOT_STATE, *PIOP_ROOT_STATE;

typedef ULONG IO_DRIVER_OBJECT_FLAGS;

#define IO_DRIVER_OBJECT_FLAG_INITIALIZED 0x00000001
#define IO_DRIVER_OBJECT_FLAG_READY       0x00000002

struct _IO_DRIVER_OBJECT {
    LONG ReferenceCount;
    IO_DRIVER_OBJECT_FLAGS Flags;
    PIOP_ROOT_STATE Root;
    PIOP_DRIVER_CONFIG Config;

    PVOID LibraryHandle;
    PIO_DRIVER_ENTRY DriverEntry;
    struct {
        PIO_DRIVER_SHUTDOWN_CALLBACK Shutdown;
        PIO_DRIVER_DISPATCH_CALLBACK Dispatch;
        PIO_DRIVER_REFRESH_CALLBACK Refresh;
    } Callback;
    PVOID Context;

    // Devices
    LW_LIST_LINKS DeviceList;
    ULONG DeviceCount;

    // For each list to which this object belongs.
    LW_LIST_LINKS RootLinks;
};

struct _IO_DEVICE_OBJECT {
    LONG ReferenceCount;
    UNICODE_STRING DeviceName;
    PIO_DRIVER_OBJECT Driver;
    PVOID Context;

    // File objects for this device.
    LW_LIST_LINKS FileObjectsList;

    // For each list to which this object belongs.
    LW_LIST_LINKS DriverLinks;
    LW_LIST_LINKS RootLinks;

    LW_RTL_MUTEX Mutex;

    LW_RTL_MUTEX CancelMutex;
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
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
    );

NTSTATUS
IopGetMapSecurityContext(
    OUT PLW_MAP_SECURITY_CONTEXT* ppContext
    );

// ioconfig.c

VOID
IopConfigFreeConfig(
    IN OUT PIOP_CONFIG* ppConfig
    );

NTSTATUS
IopConfigReadRegistry(
    OUT PIOP_CONFIG* ppConfig
    );

// ioroot.c

VOID
IopRootFree(
    IN OUT PIOP_ROOT_STATE* ppRoot
    );

NTSTATUS
IopRootCreate(
    OUT PIOP_ROOT_STATE* ppRoot,
    IN PCSTR pszConfigFilePath
    );

NTSTATUS
IopRootLoadDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PIO_STATIC_DRIVER pStaticDrivers,
    IN PWSTR pwszDriverName
    );

PIO_DRIVER_OBJECT
IopRootFindDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PWSTR pwszDriverName
    );

PIO_DEVICE_OBJECT
IopRootFindDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDeviceName
    );

NTSTATUS
IopRootRefreshDrivers(
    IN PIOP_ROOT_STATE pRoot
    );

VOID
IopRootInsertDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    );

VOID
IopRootRemoveDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    );

VOID
IopRootInsertDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    );

VOID
IopRootRemoveDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    );

NTSTATUS
IopRootParse(
    IN PIOP_ROOT_STATE pRoot,
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
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
    IN PIOP_DRIVER_CONFIG pDriverConfig,
    IN PIO_STATIC_DRIVER pStaticDrivers
    );

VOID
IopDriverInsertDevice(
    IN PIO_DRIVER_OBJECT pDriver,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    );

VOID
IopDriverRemoveDevice(
    IN PIO_DRIVER_OBJECT pDriver,
    IN PLW_LIST_LINKS pDeviceDriverLinks
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
    IN PIO_STATUS_BLOCK pIoStatusBlock
    );

VOID
IopIrpCancelFileObject(
    IN PIO_FILE_OBJECT pFileObject
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
    IN PIO_FILE_OBJECT pFileObject,
    IN OPTIONAL PIO_ASYNC_COMPLETE_CALLBACK Callback,
    IN OPTIONAL PVOID CallbackContext,
    IN OPTIONAL PIO_STATUS_BLOCK IoStatusBlock
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
