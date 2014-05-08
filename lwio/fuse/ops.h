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

#ifndef __LWIO_FUSE_OPS_H__
#define __LWIO_FUSE_OPS_H__

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include "common.h"

NTSTATUS
LwIoFuseInit(
    PIO_FUSE_CONTEXT pFuseContext,
    struct fuse_conn_info* pConn
    );

NTSTATUS
LwIoFuseGetattr(
    const char *pszPath,
    struct stat *pStatbuf
    );

NTSTATUS
LwIoFuseReaddir(
    const char *pszPath,
    PVOID pbuf,
    fuse_fill_dir_t pfFill,
    off_t offset,
    struct fuse_file_info *pFileInfo
    );

NTSTATUS
LwIoFuseOpen(
    const char* pszPath,
    struct fuse_file_info* pFileInfo
    );

NTSTATUS
LwIoFuseRelease(
    const char* pszPath,
    struct fuse_file_info* pFileInfo
    );

NTSTATUS
LwIoFuseRead(
    const char* pszPath,
    char* pData,
    size_t length,
    off_t offset,
    struct fuse_file_info* pFileInfo,
    int* pBytesRead
    );

NTSTATUS
LwIoFuseWrite(
    const char* pszPath,
    const char* pData,
    size_t length,
    off_t offset,
    struct fuse_file_info* pFileInfo,
    int* pBytesWritten
    );

NTSTATUS
LwIoFuseTruncate(
    const char* pszPath,
    off_t size
    );

NTSTATUS
LwIoFuseMknod(
    const char* pszPath,
    mode_t mode,
    dev_t dev
    );

NTSTATUS
LwIoFuseUnlink(
    const char* pszPath
    );

NTSTATUS
LwIoFuseMkdir(
    const char* pszPath,
    mode_t mode
    );

NTSTATUS
LwIoFuseRmdir(
    const char* pszPath
    );

NTSTATUS
LwIoFuseRename(
    const char* pszOldPath,
    const char* pszNewPath
    );

NTSTATUS
LwIoFuseChmod(
    const char* pszPath,
    mode_t mode
    );

NTSTATUS
LwIoFuseChown(
    const char* pszPath,
    uid_t uid,
    gid_t gid
    );

NTSTATUS
LwIoFuseStatfs(
    const char *pszPath,
    struct statvfs *pStatbuf
    );

NTSTATUS
LwIoFuseCreate(
    const char* pszPath,
    mode_t mode,
    struct fuse_file_info* pFileInfo
    );

NTSTATUS
LwIoFuseUtimens(
    const char* pszPath,
    const struct timespec tv[2]
    );

#endif
