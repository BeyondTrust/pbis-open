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

#ifndef __LWIO_FUSE_COMMON_H__
#define __LWIO_FUSE_COMMON_H__

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <lwio/lwio.h>

#define NT_TO_FUSE_FH(_fh_) ((uint64_t) (uintptr_t) (_fh_))
#define FUSE_TO_NT_FH(_fh_) ((IO_FILE_HANDLE) (uintptr_t) (_fh_))

typedef struct _IO_FUSE_CONTEXT
{
    uid_t ownerUid;
    gid_t ownerGid;
    PSTR pszUncPath;
    PWSTR pwszInternalPath;
    PSTR pszUsername;
    PSTR pszDomain;
    PSTR pszPassword;
    PIO_CREDS pCreds;
    BOOL bHelp;
} IO_FUSE_CONTEXT, *PIO_FUSE_CONTEXT;

PIO_FUSE_CONTEXT
LwIoFuseGetContext(
    void
    );

NTSTATUS
LwIoFuseSetContextCreds(
    PIO_FUSE_CONTEXT pContext
    );

void
LwIoFuseGetCallerIdentity(
    uid_t *pUid,
    gid_t *pGid,
    pid_t *pPid
    );

NTSTATUS
LwIoFuseGetNtFilename(
    PIO_FUSE_CONTEXT pFuseContext,
    PCSTR pszPath,
    PIO_FILE_NAME pFilename
    );

NTSTATUS
LwIoFuseGetDriverRelativePath(
    PIO_FUSE_CONTEXT pFuseContext,
    PCSTR pszPath,
    PWSTR* ppwszRelativePath
    );

NTSTATUS
LwIoFuseTranslateBasicInformation(
    PFILE_BASIC_INFORMATION pInfo,
    struct stat* pStatbuf
    );

NTSTATUS
LwIoFuseTranslateStandardInformation(
    PFILE_STANDARD_INFORMATION pInfo,
    struct stat* pStatbuf
    );

NTSTATUS
LwIoFuseTranslateSecurityDescriptor(
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    struct stat* pStatbuf
    );

NTSTATUS
LwIoFuseTranslatePosixOpenFlags(
    int flags,
    ACCESS_MASK* pAccessMask,
    FILE_CREATE_DISPOSITION* pDisposition,
    FILE_CREATE_OPTIONS* pOptions
    );

NTSTATUS
LwIoFuseTranslateFsInfo(
    PFILE_FS_SIZE_INFORMATION pSizeInfo,
    struct statvfs* pStatbuf
    );

int
LwIoFuseMapNtStatus(
    NTSTATUS status
    );

LONG64
LwIoTimeSpecToWinTime(
    const struct timespec* pTs
    );

#endif
