/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
