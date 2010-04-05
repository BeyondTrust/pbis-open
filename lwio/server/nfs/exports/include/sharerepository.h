/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        sharerepository.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Share repository interface
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __NFS_SHARE_REPOSITORY_H__
#define __NFS_SHARE_REPOSITORY_H__

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_OPEN)(
                        OUT PHANDLE phRepository
                        );

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_FIND_BY_NAME)(
                        IN  HANDLE           hRepository,
                        IN  PWSTR            pwszShareName,
                        OUT PNFS_SHARE_INFO* ppShareInfo
                        );

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_ADD)(
                        IN  HANDLE        hRepository,
                        IN  PWSTR         pwszShareName,
                        IN  PWSTR         pwszPath,
                        IN  PWSTR         pwszComment,
                        IN  PBYTE         pSecDesc,
                        IN  ULONG         ulSecDescLen,
                        IN  PWSTR         pwszService
                        );

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_BEGIN_ENUM)(
                        IN  HANDLE  hRepository,
                        IN  ULONG   ulLimit,
                        OUT PHANDLE phResume
                        );

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_ENUM)(
                        IN     HANDLE            hRepository,
                        IN     HANDLE            hResume,
                        OUT    PNFS_SHARE_INFO** pppShareInfoList,
                        IN OUT PULONG            pulNumSharesFound
                        );

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_END_ENUM)(
                        IN HANDLE           hRepository,
                        IN HANDLE           hResume
                        );

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_DELETE)(
                        IN HANDLE hRepository,
                        IN PWSTR  pwszShareName
                        );

typedef VOID (*PFN_NFS_SHARE_REPOSITORY_CLOSE)(
                        IN HANDLE hRepository
                        );

typedef struct _NFS_SHARE_REPOSITORY_FUNCTION_TABLE
{

    PFN_NFS_SHARE_REPOSITORY_OPEN         pfnShareRepositoryOpen;
    PFN_NFS_SHARE_REPOSITORY_FIND_BY_NAME pfnShareRepositoryFindByName;
    PFN_NFS_SHARE_REPOSITORY_ADD          pfnShareRepositoryAdd;
    PFN_NFS_SHARE_REPOSITORY_BEGIN_ENUM   pfnShareRepositoryBeginEnum;
    PFN_NFS_SHARE_REPOSITORY_ENUM         pfnShareRepositoryEnum;
    PFN_NFS_SHARE_REPOSITORY_END_ENUM     pfnShareRepositoryEndEnum;
    PFN_NFS_SHARE_REPOSITORY_DELETE       pfnShareRepositoryDelete;
    PFN_NFS_SHARE_REPOSITORY_CLOSE        pfnShareRepositoryClose;

} NFS_SHARE_REPOSITORY_FUNCTION_TABLE, *PNFS_SHARE_REPOSITORY_FUNCTION_TABLE;

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_INITIALIZE)(
                    OUT PNFS_SHARE_REPOSITORY_FUNCTION_TABLE* ppFnTable
                    );

typedef NTSTATUS (*PFN_NFS_SHARE_REPOSITORY_SHUTDOWN)(
                    IN PNFS_SHARE_REPOSITORY_FUNCTION_TABLE pFnTable
                    );

#endif /* __NFS_SHARE_REPOSITORY_H__ */
