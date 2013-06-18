/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwfile.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) Memory Utilities
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

LW_END_EXTERN_C


#endif /* __LWFILE_H__ */

