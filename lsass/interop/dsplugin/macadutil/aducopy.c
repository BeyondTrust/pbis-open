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

#include "../includes.h"


DWORD
ADUCopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    DWORD dwError = 0;
    IO_FILE_HANDLE hRemoteFile = NULL;
    int hLocalFile = -1;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    dwError = ADURemoteOpenFile(
                    pszSourcePath,
                    FILE_READ_DATA,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    FILE_NON_DIRECTORY_FILE,
                    &hRemoteFile);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADULocalOpenFile(
                (PCSTR)pszTargetPath,
                O_WRONLY|O_TRUNC|O_CREAT,
                0666,
                &hLocalFile);
    BAIL_ON_MAC_ERROR(dwError);

    do
    {
        BYTE  szBuff[BUFF_SIZE];
        DWORD dwRead = 0;
        DWORD dwWrote = 0;

        dwError = ADURemoteReadFile(
                        hRemoteFile,
                        szBuff,
                        sizeof(szBuff),
                        &dwRead);  // number of bytes read
        if (!dwRead)
        {
            dwError = 0;
            break;
        }

        BAIL_ON_MAC_ERROR(dwError);

        if ((dwWrote = write(hLocalFile, szBuff, dwRead)) == (unsigned int)-1)
        {
            dwError = errno;
            // Can't build here because that freezes the compiler on OS X 10.6.
            // Instead the bail is outside of the loop.
            break;
        }

    } while(1);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    if (hLocalFile >= 0)
    {
        close(hLocalFile);
    }

    return (dwError);

error:

    goto cleanup;
}

DWORD
ADUCopyMultipleFilesFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    DWORD dwError = 0;
    BOOL bRestart = TRUE;
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszEntryFilename = NULL;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    dwError = ADURemoteOpenFile(
                    pszSourcePath,
                    FILE_LIST_DIRECTORY,   /* Desired access mask */
                    FILE_SHARE_READ,       /* Share access */
                    FILE_OPEN,             /* Create disposition */
                    FILE_DIRECTORY_FILE,   /* Create options */
                    &handle);
    BAIL_ON_MAC_ERROR(dwError);

    for (;;)
    {
        dwError = LwNtQueryDirectoryFile(
                        handle,                       /* File handle */
                        NULL,                         /* Async control block */
                        &ioStatus,                    /* IO status block */
                        buffer,                       /* Info structure */
                        sizeof(buffer),               /* Info structure size */
                        FileBothDirectoryInformation, /* Info level */
                        FALSE,                        /* Do not return single entry */
                        NULL,                         /* File spec */
                        bRestart);                    /* Restart scan */

        switch (dwError)
        {
            case STATUS_NO_MORE_MATCHES:
                dwError = 0;
                goto cleanup;
            case STATUS_SUCCESS:
                break;
            default:
                BAIL_ON_MAC_ERROR(dwError);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);
            RTL_FREE(&pszLocalPath);

            dwError = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_MAC_ERROR(dwError);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            dwError = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszSourcePath,
                        pszEntryFilename);
            BAIL_ON_MAC_ERROR(dwError);

            dwError = LwRtlCStringAllocatePrintf(
                        &pszLocalPath,
                        "%s/%s",
                        pszTargetPath,
                        pszEntryFilename);
            BAIL_ON_MAC_ERROR(dwError);

            if(pInfo->FileAttributes != FILE_ATTRIBUTE_DIRECTORY)
            {
                dwError = ADUCopyFileFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_MAC_ERROR(dwError);
            }
        }
    }

cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    RTL_FREE(&pszEntryFilename);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUCopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    DWORD dwError = 0;
    BOOL bRestart = TRUE;
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszEntryFilename = NULL;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    dwError = ADURemoteOpenFile(
                    pszSourcePath,
                    FILE_LIST_DIRECTORY, /* Desired access mask */
                    FILE_SHARE_READ,     /* Share access */
                    FILE_OPEN,           /* Create disposition */
                    FILE_DIRECTORY_FILE, /* Create options */
                    &handle);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADULocalCreateDir(
                pszTargetPath,
                S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_MAC_ERROR(dwError);

    for (;;)
    {
        dwError = LwNtQueryDirectoryFile(
            handle,                             /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                             /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,         /* Info level */
            FALSE,                                 /* Do not return single entry */
            NULL,                                 /* File spec */
            bRestart);                             /* Restart scan */

        switch (dwError)
        {
            case STATUS_NO_MORE_MATCHES:
                dwError = 0;
                goto cleanup;
            case STATUS_SUCCESS:
                break;
            default:
                BAIL_ON_MAC_ERROR(dwError);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);
            RTL_FREE(&pszLocalPath);

            dwError = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_MAC_ERROR(dwError);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            dwError = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszSourcePath,
                        pszEntryFilename);
            BAIL_ON_MAC_ERROR(dwError);

            dwError = LwRtlCStringAllocatePrintf(
                        &pszLocalPath,
                        "%s/%s",
                        pszTargetPath,
                        pszEntryFilename);
            BAIL_ON_MAC_ERROR(dwError);

            if(pInfo->FileAttributes == FILE_ATTRIBUTE_DIRECTORY)
            {

                dwError = ADUCopyDirFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_MAC_ERROR(dwError);
            }
            else
            {
                dwError = ADUCopyFileFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_MAC_ERROR(dwError);
          }
        }
    }

cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    RTL_FREE(&pszEntryFilename);

    return dwError;

error:

    goto cleanup;
}


DWORD
ADUCopyFileToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    DWORD dwError = 0;
    IO_FILE_HANDLE hRemoteFile = NULL;
    int hLocalFile = -1;
    DWORD dwBytesRead = 0;
    CHAR szBuf[BUFF_SIZE];

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    dwError = ADULocalOpenFile(
                (PCSTR)pszSourcePath,
                O_RDONLY,
                0,
                &hLocalFile);

    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADURemoteOpenFile(
                    pszTargetPath,
                    FILE_WRITE_DATA,
                    0,
                    FILE_OPEN_IF,
                    FILE_NON_DIRECTORY_FILE,
                    &hRemoteFile);
    BAIL_ON_MAC_ERROR(dwError);

    do
    {
        DWORD dwWritten = 0;

        memset (szBuf,0,BUFF_SIZE);

        if ((dwBytesRead = read(hLocalFile, szBuf, sizeof(szBuf))) == (unsigned int)-1)
        {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        if (dwBytesRead == 0)
        {
            break;
        }

        dwError  = ADURemoteWriteFile(
                            hRemoteFile,
                            szBuf,
                            dwBytesRead,
                            &dwWritten);

        BAIL_ON_MAC_ERROR(dwError);

    } while (dwBytesRead != 0);

cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    if (hLocalFile >= 0)
    {
        close(hLocalFile);
    }

    return (dwError);

error:

    goto cleanup;

}

DWORD
ADUCopyDirToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    if ((pDir = opendir(pszSourcePath)) == NULL)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    // Remove the destination directory (if present), as we are going to replace the contents with the files from pszSourcePath
    LOG("Purging remote directory %s to replace with context of %s", pszTargetPath, pszSourcePath);
    dwError = ADURemoteRemoveDirRecursive(pszTargetPath, TRUE);
    if (dwError)
    {
        LOG_ERROR("Failed to remove remote directory %s [dwError:0x%x]", pszTargetPath, dwError);
        dwError = 0;
    }

    while ((pDirEntry = readdir(pDir)) != NULL)
    {
        RTL_FREE(&pszRemotePath);
        RTL_FREE(&pszLocalPath);

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        dwError = LwRtlCStringAllocatePrintf(
                    &pszLocalPath,
                    "%s/%s",
                    pszSourcePath,
                    pDirEntry->d_name);
        BAIL_ON_MAC_ERROR(dwError);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszLocalPath, &statbuf) < 0)
        {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        dwError = LwRtlCStringAllocatePrintf(
                    &pszRemotePath,
                    "%s/%s",
                    pszTargetPath,
                    pDirEntry->d_name);
        BAIL_ON_MAC_ERROR(dwError);

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            LOG("Copying directory %s to %s", pszLocalPath, pszRemotePath);
            dwError = ADUCopyDirToRemote(
                            pszLocalPath,
                            pszRemotePath);
        }
        else
        {
            LOG("Copying file %s to %s", pszLocalPath, pszRemotePath);
            dwError = ADUCopyFileToRemote(
                            pszLocalPath,
                            pszRemotePath);
        }
        BAIL_ON_MAC_ERROR(dwError);
    }

    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        dwError = LwErrnoToNtStatus(dwError);
        BAIL_ON_MAC_ERROR(dwError);
    }

    pDir = NULL;

cleanup:

    if (pDir)
        closedir(pDir);

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    return dwError;

error:
    goto cleanup;

}

DWORD
ADURemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszRemoteFileName = NULL;

    BAIL_ON_NULL_POINTER(pszFileName);

    // TODO: We will not do this when we switch to using CreateFile
    if (!strncmp(pszFileName, "//", sizeof("//")-1))
    {
         pszFileName++;
    }

    dwError = LwRtlCStringAllocatePrintf(
                    &pszRemoteFileName,
                    "/rdr%s",
                    pszFileName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = RtlUnicodeStringAllocateFromCString(&filename.Name, pszRemoteFileName);
    BAIL_ON_MAC_ERROR(dwError);

    status = LwNtCreateFile(
                    &hFile,              /* File handle */
                    NULL,                /* Async control block */
                    &ioStatus,           /* IO status block */
                    &filename,           /* Filename */
                    NULL,                /* Security descriptor */
                    NULL,                /* Security QOS */
                    ulDesiredAccess,     /* Desired access mask */
                    0,                   /* Allocation size */
                    0,                   /* File attributes */
                    ulShareAccess,       /* Share access */
                    ulCreateDisposition, /* Create disposition */
                    ulCreateOptions,     /* Create options */
                    NULL,                /* EA buffer */
                    0,                   /* EA length */
                    NULL,                /* ECP list */
                    NULL);               /* Creds */
    if (status != STATUS_SUCCESS)
    {
        LOG_ERROR("Failed to open/create file [%s][status:0x%x]",
                         pszFileName,
                         status);
        dwError = MAC_AD_ERROR_CREATE_FAILED;
    }
    BAIL_ON_MAC_ERROR(dwError);

    *phFile = hFile;

cleanup:

    RTL_FREE(&pszRemoteFileName);
    RTL_UNICODE_STRING_FREE(&filename.Name);

    return dwError;

error:

    *phFile = NULL;

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    goto cleanup;
}


DWORD
ADULocalOpenFile(
    IN PCSTR pszFileName,
    IN INT  dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    )
{
    DWORD dwError = 0;
    int fd = -1;

    BAIL_ON_NULL_POINTER(pszFileName);

    if ((fd = open(pszFileName, dwMode, dwPerms)) == -1)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }


error:

    *dwHandle = fd;

    return dwError;

}


DWORD
ADULocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    DWORD dwError = 0;
    PSTR pszTmpPath = NULL;

    BAIL_ON_NULL_POINTER(pszPath);

    dwError = LwAllocateString(
                pszPath,
                &pszTmpPath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADULocalCreateDirInternal(
                pszTmpPath,
                NULL,
                dwFileMode);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (pszTmpPath)
       LwFreeMemory(pszTmpPath);

    return dwError;

error:
    goto cleanup;
}


DWORD
ADULocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    )
{
    DWORD dwError = 0;
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
        dwError = ADULocalCheckDirExists(pszPath, &bDirExists);
        BAIL_ON_MAC_ERROR(dwError);

        if (!bDirExists)
        {
            if (mkdir(pszPath, S_IRWXU) != 0)
            {
                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);
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
        dwError = ADULocalCreateDirInternal(pszPath, pszSlash, dwFileMode);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        dwError = ADULocalChangePermissions(pszPath, dwFileMode);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

cleanup:

    return dwError;

error:

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        ADULocalRemoveDir(pszPath);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    goto cleanup;
}


DWORD
ADULocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (chmod(pszPath, dwFileMode) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        else
        {
            break;
        }
    }

error:
    return dwError;
}


DWORD
ADULocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    )
{
    DWORD dwError = 0;
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
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:
    return dwError;
}


DWORD
ADULocalRemoveDir(
    IN PCSTR pszPath
    )
{
    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[BUFF_SIZE+1];

    BAIL_ON_NULL_POINTER(pszPath);

    if ((pDir = opendir(pszPath)) == NULL)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
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
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            dwError = ADULocalRemoveDir(szBuf);
            BAIL_ON_MAC_ERROR(dwError);

            if (rmdir(szBuf) < 0)
            {
                dwError = LwErrnoToNtStatus(dwError);
                BAIL_ON_MAC_ERROR(dwError);
            }
        }
        else
        {
            dwError = ADULocalRemoveFile(szBuf);
            BAIL_ON_MAC_ERROR(dwError);

        }
    }

    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        dwError = LwErrnoToNtStatus(dwError);
        BAIL_ON_MAC_ERROR(dwError);
    }

    pDir = NULL;

    if (rmdir(pszPath) < 0)
    {
        dwError = LwErrnoToNtStatus(dwError);
        BAIL_ON_MAC_ERROR(dwError);
    }

error:

    if (pDir)
        closedir(pDir);

    return dwError;
}


DWORD
ADULocalRemoveFile(
    IN PCSTR pszPath
    )
{
    DWORD dwError = 0;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (unlink(pszPath) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        else
        {
            break;
        }
    }

error:

    return dwError;
}

DWORD
ADURemoteReadFile(
    IN  IO_FILE_HANDLE hFile,
    OUT PVOID          pBuffer,
    IN  DWORD          dwNumberOfBytesToRead,
    OUT PDWORD         pdwBytesRead
    )
{
    DWORD dwError = 0;
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
    switch (status)
    {
       case STATUS_SUCCESS:
       case STATUS_END_OF_FILE:

            break;

       default:

            LOG_ERROR("Failed to read from file [status:0x%x]", status);

            dwError = MAC_AD_ERROR_READ_FAILED;

            break;
    }
    BAIL_ON_MAC_ERROR(dwError);

    *pdwBytesRead = (int) ioStatus.BytesTransferred;

cleanup:

    return dwError;

error:

    *pdwBytesRead = 0;
    pBuffer = NULL;

    goto cleanup;
}

DWORD
ADURemoteWriteFile(
    IN IO_FILE_HANDLE hFile,
    IN PVOID          pBuffer,
    IN DWORD          dwNumBytesToWrite,
    OUT PDWORD        pdwNumBytesWritten
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus ;

    BAIL_ON_NULL_POINTER(pBuffer);

    status = LwNtWriteFile(
                    hFile,                     // File handle
                    NULL,                      // Async control block
                    &ioStatus,                 // IO status block
                    pBuffer,                   // Buffer
                    (ULONG) dwNumBytesToWrite, // Buffer size
                    NULL,                      // File offset
                    NULL);                     // Key
    if (status != STATUS_SUCCESS)
    {
        LOG_ERROR("Failed to write to file [status:0x%x]", status);
        dwError = MAC_AD_ERROR_WRITE_FAILED;
    }
    BAIL_ON_MAC_ERROR(dwError);

    *pdwNumBytesWritten = (int) ioStatus.BytesTransferred;

cleanup:

    return dwError;

error:

    *pdwNumBytesWritten = 0;

    goto cleanup;

}

DWORD
ADURemoteRemoveFile(
    IN PCSTR pszPath
    )
{
    DWORD dwError = 0;
    IO_FILE_HANDLE hRemoteFile = NULL;

    BAIL_ON_NULL_POINTER(pszPath);

    dwError = ADURemoteOpenFile(
                    pszPath,
                    DELETE,
                    0,
                    FILE_OPEN,
                    FILE_DELETE_ON_CLOSE,
                    &hRemoteFile);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    return (dwError);

error:

    goto cleanup;
}

DWORD
ADURemoteRemoveDirRecursive(
    IN PCSTR pszPath,
    BOOL bDeleteContentsOnly
    )
{
    DWORD dwError = 0;
    BOOL bRestart = TRUE;
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszEntryFilename = NULL;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
    PSTR pszRemotePath = NULL;
    ULONG ulCreateOption = FILE_DIRECTORY_FILE;

    BAIL_ON_NULL_POINTER(pszPath);

    if (!bDeleteContentsOnly)
    {
         ulCreateOption |= FILE_DELETE_ON_CLOSE;
    }

    dwError = ADURemoteOpenFile(
                    pszPath,
                    DELETE | FILE_LIST_DIRECTORY, /* Desired access mask */
                    0,                            /* Share access */
                    FILE_OPEN,           /* Create disposition */
                    ulCreateOption,      /* Create options */
                    &handle);
    BAIL_ON_MAC_ERROR(dwError);

    for (;;)
    {
        dwError = LwNtQueryDirectoryFile(
            handle,                             /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                          /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,       /* Info level */
            FALSE,                              /* Do not return single entry */
            NULL,                               /* File spec */
            bRestart);                          /* Restart scan */

        switch (dwError)
        {
            case STATUS_NO_MORE_MATCHES:
                dwError = 0;
                goto cleanup;
            case STATUS_SUCCESS:
                break;
            default:
                BAIL_ON_MAC_ERROR(dwError);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);

            dwError = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_MAC_ERROR(dwError);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            dwError = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszPath,
                        pszEntryFilename);
            BAIL_ON_MAC_ERROR(dwError);

            if(pInfo->FileAttributes == FILE_ATTRIBUTE_DIRECTORY)
            {
                // Get rid of the contents of the sub-directory
                dwError = ADURemoteRemoveDirRecursive(pszRemotePath, FALSE);
                BAIL_ON_MAC_ERROR(dwError);
                LOG("Removed remote sub-directory %s", pszRemotePath);
            }
            else
            {
                // Get rid of the this file from the directory
                dwError = ADURemoteRemoveFile(pszRemotePath);
                BAIL_ON_MAC_ERROR(dwError);
                LOG("Removed remote file %s", pszRemotePath);
            }
        }
    }

    LOG("Removed remote directory %s", pszPath);

cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_FREE(&pszRemotePath);
    RTL_FREE(&pszEntryFilename);

    return dwError;

error:

    LOG_ERROR("Failed remove directory recursive operation for remote directory %s, [dwError: 0x%x]", pszPath, dwError);
    goto cleanup;
}

DWORD
ADURemoteCreateDirectory(
    const char* pszPath
    )
{
    DWORD dwError = 0;
    IO_FILE_HANDLE handle = NULL;

    BAIL_ON_NULL_POINTER(pszPath);

    dwError = ADURemoteOpenFile(
                    pszPath,
                    GENERIC_WRITE,         /* Desired access mask */
                    0,                     /* Share access */
                    FILE_CREATE,           /* Create disposition */
                    FILE_DIRECTORY_FILE,   /* Create options */
                    &handle);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    return (dwError);

error:

    goto cleanup;
}

