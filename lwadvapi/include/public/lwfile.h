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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwfile.h
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi) Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWFILE_H__
#define __LWFILE_H__

typedef enum _LWFILE_TYPE
{
    LWFILE_REGULAR,
    LWFILE_DIRECTORY,
    LWFILE_SYMLINK,
    LWFILE_SOCKET,
    LWFILE_PIPE,
} LWFILE_TYPE;


LW_BEGIN_EXTERN_C

DWORD
LwRemoveFile(
    PCSTR pszPath
    );

DWORD
LwMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LwChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LwChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
LwChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
LwGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
LwGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
LwFindFilesInPaths(
    IN PCSTR pszName,
    IN LWFILE_TYPE type,
    IN const PCSTR* ppszSearchPaths,
    OUT PDWORD pdwFoundCount,
    OUT PSTR** pppszFoundPaths
    );

DWORD
LwCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbExists
    );

DWORD
LwCheckFileTypeExists(
    PCSTR pszPath,
    LWFILE_TYPE type,
    PBOOLEAN pbExists
    );

#define LwCheckDirectoryExists(pszPath, pbExists) LwCheckFileTypeExists(pszPath, LWFILE_DIRECTORY, pbExists)
#define LwCheckLinkExists(pszPath, pbExists) LwCheckFileTypeExists(pszPath, LWFILE_SYMLINK, pbExists)

DWORD
LwCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LwRemoveDuplicateInodes(
    IN OUT PDWORD pdwFoundCount,
    IN OUT PSTR* ppszFoundPaths
    );

DWORD
LwCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );


LW_END_EXTERN_C


#endif /* __LWFILE_H__ */

