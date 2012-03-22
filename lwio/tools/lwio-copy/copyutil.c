
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        copyutil.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Tool to copy files/directories
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

BOOLEAN
IsPathRemote(
    IN PCSTR pszPath
    )
{
    BOOLEAN bIsRemote = FALSE;

    if (!strncmp(pszPath, "//", sizeof("//")-1) ||
        !strncmp(pszPath, "\\\\", sizeof("\\\\")-1))
    {
        bIsRemote = TRUE;
    }

    return bIsRemote;
}

NTSTATUS
LwioCheckRemotePathIsDirectory(
    IN     PCSTR    pszPath,
    IN OUT PBOOLEAN pbIsDirectory
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    IO_FILE_HANDLE  hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_STANDARD_INFORMATION fileStdInfo;

    ntStatus = LwioRemoteOpenFile(
                    pszPath,
                    FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    &hFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwNtQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    (PVOID*)&fileStdInfo,
                    sizeof(fileStdInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    *pbIsDirectory = fileStdInfo.Directory;

cleanup:

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    return ntStatus;

error:

    *pbIsDirectory = FALSE;

    goto cleanup;
}

NTSTATUS
LwioLocalOpenFile(
    IN PCSTR pszFileName,
    IN INT  dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int fd = -1;

    BAIL_ON_NULL_POINTER(pszFileName);

    if ((fd = open(pszFileName, dwMode, dwPerms)) == -1)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }


error:

    *dwHandle = fd;

    return status;

}

NTSTATUS
LwioLocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszTmpPath = NULL;

    BAIL_ON_NULL_POINTER(pszPath);

    status = SMBAllocateString(
                pszPath,
                &pszTmpPath);
    BAIL_ON_NT_STATUS(status);

    status = LwioLocalCreateDirInternal(
                pszTmpPath,
                NULL,
                dwFileMode);
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWIO_SAFE_FREE_STRING(pszTmpPath);
    return status;

error:
    goto cleanup;
}


NTSTATUS
LwioLocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszSlash = NULL;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bDirCreated = FALSE;

    BAIL_ON_NULL_POINTER(pszPath);

    pszSlash = pszLastSlash ? strchr(pszLastSlash + 1, '/') : strchr(pszPath, '/');

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (pszPath[0])
    {
        status = LwioLocalCheckDirExists(pszPath, &bDirExists);
        BAIL_ON_NT_STATUS(status);

        if (!bDirExists)
        {
            if (mkdir(pszPath, S_IRWXU) != 0)
            {
                status = LwErrnoToNtStatus(errno);
                BAIL_ON_NT_STATUS(status);
            }
            bDirCreated = TRUE;
        }
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    if (pszSlash)
    {
        status = LwioLocalCreateDirInternal(pszPath, pszSlash, dwFileMode);
        BAIL_ON_NT_STATUS(status);
    }

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        status = LwioLocalChangePermissions(pszPath, dwFileMode);
        BAIL_ON_NT_STATUS(status);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

cleanup:

    return status;

error:

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        LwioLocalRemoveDir(pszPath);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    goto cleanup;
}


NTSTATUS
LwioLocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (chmod(pszPath, dwFileMode) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            break;
        }
    }

error:
    return status;
}


NTSTATUS
LwioLocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszPath, &statbuf) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbDirExists = FALSE;
                break;
            }
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:
    return status;
}


NTSTATUS
LwioLocalRemoveDir(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[BUFF_SIZE+1];

    BAIL_ON_NULL_POINTER(pszPath);

    if ((pDir = opendir(pszPath)) == NULL)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    while ((pDirEntry = readdir(pDir)) != NULL)
    {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0)
        {
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            status = LwioLocalRemoveDir(szBuf);
            BAIL_ON_NT_STATUS(status);

            if (rmdir(szBuf) < 0)
            {
                status = LwErrnoToNtStatus(status);
                BAIL_ON_NT_STATUS(status);
            }
        }
        else
        {
            status = LwioLocalRemoveFile(szBuf);
            BAIL_ON_NT_STATUS(status);

        }
    }

    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        status = LwErrnoToNtStatus(status);
        BAIL_ON_NT_STATUS(status);
    }

    pDir = NULL;

    if (rmdir(pszPath) < 0)
    {
        status = LwErrnoToNtStatus(status);
        BAIL_ON_NT_STATUS(status);
    }

error:

    if (pDir)
        closedir(pDir);

    return status;
}

NTSTATUS
LwioLocalRemoveFile(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (unlink(pszPath) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            break;
        }
    }

error:

    return status;
}

NTSTATUS
LwioCheckLocalFileExists(
    IN  PCSTR pszPath,
    OUT PBOOLEAN pbFileExists
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

    BAIL_ON_NULL_POINTER(pszPath);

    memset(&statbuf, 0, sizeof(struct stat));

    while (1)
    {
        if (stat(pszPath, &statbuf) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbFileExists = FALSE;
                break;
            }

            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);

        }
        else
        {
            *pbFileExists = (((statbuf.st_mode & S_IFMT) == S_IFREG) ? TRUE : FALSE);
            break;
        }
    }

cleanup:

    return status;

error:

    *pbFileExists = FALSE;

    goto cleanup;
}

NTSTATUS
LwioCheckLocalDirectoryExists(
    IN  PCSTR pszPath,
    OUT PBOOLEAN pbDirExists
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

    while (1)
    {

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszPath, &statbuf) < 0) 
        {

            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbDirExists = FALSE;
                break;
            }

            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);

        }

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

cleanup:

    return status;

error:

    *pbDirExists = FALSE;

    goto cleanup;
}

NTSTATUS
LwioGetLocalFileOwnerAndPerms(
    IN  PCSTR pszSrcPath,
    OUT uid_t * uid,
    OUT gid_t * gid,
    OUT mode_t * mode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszSrcPath, &statbuf) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return status;
}

NTSTATUS
LwioChangeLocalFileOwnerAndPerms(
    IN PCSTR pszPath,
    IN uid_t uid,
    IN gid_t gid,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwioChangeLocalFileOwner(pszPath, uid, gid);
    BAIL_ON_NT_STATUS(status);

    status = LwioChangeLocalFilePerms(pszPath, dwFileMode);
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

NTSTATUS
LwioChangeLocalFileOwner(
    IN PCSTR pszPath,
    IN uid_t uid,
    IN gid_t gid
    )
{
    
    NTSTATUS status = STATUS_SUCCESS;

    while (1)
    {
        if (chown(pszPath, uid, gid) < 0)
        {
            if (errno == EINTR) 
            {
                continue;
            }
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        } 
        else 
        {
            break;
        }
    }

error:

    return status;
}

NTSTATUS
LwioChangeLocalFilePerms(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    
    NTSTATUS status = STATUS_SUCCESS;

    while (1)
    {
        if (chmod(pszPath, dwFileMode) < 0) 
        {
            if (errno == EINTR)
            {
                continue;
            }
            status = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            break;
        }
    }

error:

    return status;
}


NTSTATUS
LwioRemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszRemoteFileName = NULL;

    BAIL_ON_NULL_POINTER(pszFileName);

    status = LwRtlCStringAllocatePrintf(
                    &pszRemoteFileName,
                    "/rdr%s",
                    !strncmp(pszFileName, "//", sizeof("//")-1) ? pszFileName+1 : pszFileName);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlUnicodeStringAllocateFromCString(
        &filename.Name,
        pszRemoteFileName);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
                &handle,                 /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                &filename,               /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                ulDesiredAccess,         /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                ulShareAccess,           /* Share access */
                ulCreateDisposition,     /* Create disposition */
                ulCreateOptions,         /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL,                    /* ECP list */
                NULL);
    BAIL_ON_NT_STATUS(status);

    *phFile = handle;

cleanup:

    RTL_FREE(&pszRemoteFileName);

    return status;

error:

    *phFile = NULL;

    goto cleanup;
}

NTSTATUS
LwioRemoteReadFile(
    IN HANDLE hFile,
    OUT PVOID pBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT PDWORD pdwBytesRead
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus;

    status = LwNtReadFile(
        hFile,                               // File handle
        NULL,                                // Async control block
        &ioStatus,                           // IO status block
        pBuffer,                             // Buffer
        (ULONG) dwNumberOfBytesToRead,       // Buffer size
        NULL,                                // File offset
        NULL);                               // Key
    if (status == STATUS_END_OF_FILE)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(status);

    *pdwBytesRead = (int) ioStatus.BytesTransferred;

cleanup:

    return status;

error:

    *pdwBytesRead = 0;
    pBuffer = NULL;

    goto cleanup;
}

NTSTATUS
LwioRemoteWriteFile(
    IN HANDLE hFile,
    IN PVOID pBuffer,
    IN DWORD dwNumBytesToWrite,
    OUT PDWORD pdwNumBytesWritten
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus ;

    BAIL_ON_NULL_POINTER(pBuffer);

    status = LwNtWriteFile(
        hFile,                                 // File handle
        NULL,                                 // Async control block
        &ioStatus,                             // IO status block
        pBuffer,                             // Buffer
        (ULONG) dwNumBytesToWrite,             // Buffer size
        NULL,                                 // File offset
        NULL);                                 // Key
    BAIL_ON_NT_STATUS(status);

    *pdwNumBytesWritten = (int) ioStatus.BytesTransferred;

cleanup:

    return status;

error:

    *pdwNumBytesWritten = 0;

    goto cleanup;

}
