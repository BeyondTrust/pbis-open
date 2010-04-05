/*
 * Copyright Likewise Software    2004-2009
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
 *        shareapi.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - NFS
 *
 *        Share API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SHAREAPI_H__
#define __SHAREAPI_H__

#define LWIO_NFS_FILE_SYSTEM_PREFIX_A "C:\\"
#define LWIO_NFS_FILE_SYSTEM_PREFIX_W { 'C', ':', '\\', 0 }

#define LWIO_NFS_DEFAULT_SHARE_PATH_A "\\lwcifs"
#define LWIO_NFS_DEFAULT_SHARE_PATH_W { '\\', 'l', 'w', 'c', 'i', 'f', 's', 0 }

#define LWIO_NFS_FILE_SYSTEM_ROOT_A   "\\pvfs"
#define LWIO_NFS_FILE_SYSTEM_ROOT_W   { '\\', 'p', 'v', 'f', 's', 0 }

#define LWIO_NFS_PIPE_SYSTEM_ROOT_A   "\\npfs"
#define LWIO_NFS_PIPE_SYSTEM_ROOT_W   { '\\', 'n', 'p', 'f', 's', 0 }

#define LWIO_NFS_SHARE_STRING_ID_ANY_A "????"
#define LWIO_NFS_SHARE_STRING_ID_ANY_W {'?','?','?','?',0}

#define LWIO_NFS_SHARE_STRING_ID_IPC_A "IPC"
#define LWIO_NFS_SHARE_STRING_ID_IPC_W {'I','P','C',0}

#define LWIO_NFS_SHARE_STRING_ID_COMM_A "COMM"
#define LWIO_NFS_SHARE_STRING_ID_COMM_W {'C','O','M','M',0}

#define LWIO_NFS_SHARE_STRING_ID_PRINTER_A "LPT1:"
#define LWIO_NFS_SHARE_STRING_ID_PRINTER_W {'L','P','T','1',':',0}

#define LWIO_NFS_SHARE_STRING_ID_DISK_A "A:"
#define LWIO_NFS_SHARE_STRING_ID_DISK_W {'A',':',0}

typedef enum
{
    SHARE_SERVICE_DISK_SHARE = 0,
    SHARE_SERVICE_PRINTER,
    SHARE_SERVICE_COMM_DEVICE,
    SHARE_SERVICE_NAMED_PIPE,
    SHARE_SERVICE_ANY,
    SHARE_SERVICE_UNKNOWN

} SHARE_SERVICE;

typedef struct _NFS_SHARE_INFO
{
    LONG refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PWSTR pwszName;
    PWSTR pwszPath;
    PWSTR pwszComment;

    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc;
    ULONG ulSecDescLen;

    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsSecDesc;

    SHARE_SERVICE service;

    BOOLEAN bMarkedForDeletion;

} NFS_SHARE_INFO, *PNFS_SHARE_INFO;

typedef struct _NFS_SHARE_ENTRY
{
    PNFS_SHARE_INFO pInfo;

    struct _NFS_SHARE_ENTRY  *pNext;

} NFS_SHARE_ENTRY, *PNFS_SHARE_ENTRY;

typedef struct _LWIO_NFS_SHARE_ENTRY_LIST
{
    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PNFS_SHARE_ENTRY  pShareEntry;

    PLWRTL_RB_TREE    pShareCollection;

} LWIO_NFS_SHARE_ENTRY_LIST, *PLWIO_NFS_SHARE_ENTRY_LIST;

NTSTATUS
NfsShareInit(
    VOID
    );

NTSTATUS
NfsShareMapIdToServiceStringW(
    IN  SHARE_SERVICE  service,
    OUT PWSTR*         ppwszService
    );

NTSTATUS
NfsShareMapIdToServiceStringA(
    IN  SHARE_SERVICE  service,
    OUT PSTR*          ppszService
    );

NTSTATUS
NfsShareMapServiceStringToIdA(
    IN     PCSTR          pszService,
    IN OUT SHARE_SERVICE* pService
    );

NTSTATUS
NfsShareMapServiceStringToIdW(
    IN     PWSTR          pwszService,
    IN OUT SHARE_SERVICE* pService
    );

NTSTATUS
NfsShareMapFromWindowsPath(
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    );

NTSTATUS
NfsShareMapToWindowsPath(
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    );

NTSTATUS
NfsGetShareName(
    IN  PCSTR  pszHostname,
    IN  PCSTR  pszDomain,
    IN  PWSTR  pwszPath,
    OUT PWSTR* ppwszSharename
    );

NTSTATUS
NfsGetMaximalShareAccessMask(
    PNFS_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    );

NTSTATUS
NfsGetGuestShareAccessMask(
    PNFS_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    );

VOID
NfsShareFreeSecurity(
    IN PNFS_SHARE_INFO pShareInfo
    );

NTSTATUS
NfsShareAccessCheck(
    PNFS_SHARE_INFO pShareInfo,
    PACCESS_TOKEN pToken,
    ACCESS_MASK DesiredAccess,
    PGENERIC_MAPPING pGenericMap,
    PACCESS_MASK pGrantedAccess
    );

NTSTATUS
NfsShareSetSecurity(
    IN  PNFS_SHARE_INFO pShareInfo,
    IN  PSECURITY_DESCRIPTOR_RELATIVE pIncRelSecDesc,
    IN  ULONG ulIncRelSecDescLen
    );

NTSTATUS
NfsShareInitList(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList
    );

NTSTATUS
NfsShareFindByName(
    IN  PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PNFS_SHARE_INFO*           ppShareInfo
    );

NTSTATUS
NfsShareAdd(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName,
    IN     PWSTR                      pwszPath,
    IN     PWSTR                      pwszComment,
    IN     PBYTE                      pSecDesc,
    IN     ULONG                      ulSecDescLen,
    IN     PWSTR                      pwszShareType
    );

NTSTATUS
NfsShareUpdate(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PNFS_SHARE_INFO            pShareInfo
    );

NTSTATUS
NfsShareDelete(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    );

NTSTATUS
NfsShareEnum(
    IN     PLWIO_NFS_SHARE_ENTRY_LIST pShareList,
    OUT    PNFS_SHARE_INFO**          pppShareInfo,
    IN OUT PULONG                     pulNumEntries
    );

NTSTATUS
NfsShareDuplicateInfo(
    PNFS_SHARE_INFO  pShareInfo,
    PNFS_SHARE_INFO* ppShareInfo
    );

VOID
NfsShareFreeListContents(
    IN OUT PLWIO_NFS_SHARE_ENTRY_LIST pShareList
    );

VOID
NfsShareFreeEntry(
    IN PNFS_SHARE_ENTRY pShareEntry
    );

VOID
NfsShareFreeInfoList(
    PNFS_SHARE_INFO* ppInfoList,
    ULONG            ulNumInfos
    );

PNFS_SHARE_INFO
NfsShareAcquireInfo(
    IN PNFS_SHARE_INFO pShareInfo
    );

VOID
NfsShareReleaseInfo(
    IN PNFS_SHARE_INFO pShareInfo
    );

NTSTATUS
NfsShareShutdown(
    VOID
    );

#endif /* __SHAREAPI_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
