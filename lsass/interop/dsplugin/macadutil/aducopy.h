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

#ifndef __ADUCOPY_H__
#define __ADUCOPY_H__

#define BUFF_SIZE 1024
#define MAX_BUFFER 4096

#define BAIL_ON_NULL_POINTER(p)                      \
        if (NULL == p) {                             \
           dwError = MAC_AD_ERROR_INVALID_PARAMETER; \
           goto error;                               \
        }

DWORD
ADULocalOpenFile(
    IN PCSTR pszFileName,
    IN INT dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    );

DWORD
ADULocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );

DWORD
ADULocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    );

DWORD
ADULocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );

DWORD
ADULocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    );

DWORD
ADULocalRemoveDir(
    IN PCSTR pszPath
    );

DWORD
ADULocalRemoveFile(
    IN PCSTR pszPath
    );

DWORD
ADURemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    );

DWORD
ADURemoteWriteFile(
    IN IO_FILE_HANDLE hFile,
    IN PVOID          pBuffer,
    IN DWORD          dwNumBytesToWrite,
    OUT PDWORD        pdwNumBytesWritten
    );

DWORD
ADURemoteReadFile(
    IN IO_FILE_HANDLE hFile,
    OUT PVOID         pBuffer,
    IN DWORD          dwNumberOfBytesToRead,
    OUT PDWORD        pdwBytesRead
    );

DWORD
ADUCopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyMultipleFilesFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyFileToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADUCopyDirToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    );

DWORD
ADURemoteRemoveFile(
    const char* pszPath
    );

DWORD
ADURemoteRemoveDirRecursive(
    const char* pszPath,
    BOOL bDeleteContentsOnly
    );
 
DWORD
ADURemoteCreateDirectory(
    const char* pszPath
    );

#endif
