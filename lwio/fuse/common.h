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
