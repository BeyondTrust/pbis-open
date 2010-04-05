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
 *        lwiocopy.c
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



CENTERROR
GPACopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    IO_FILE_HANDLE hRemoteFile = NULL;
    int hLocalFile = -1;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    ceError = GPARemoteOpenFile(
                    pszSourcePath,
                    FILE_READ_DATA,          /* Desired access mask */
                    FILE_SHARE_READ,         /* Share access */
                    FILE_OPEN,               /* Create disposition */
                    FILE_NON_DIRECTORY_FILE, /* Create options */
                    &hRemoteFile);
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPALocalOpenFile(
                (PCSTR)pszTargetPath,
                O_WRONLY|O_TRUNC|O_CREAT,
                0666,
                &hLocalFile);
    BAIL_ON_CENTERIS_ERROR(ceError);

    do
    {
        BYTE  szBuff[BUFF_SIZE];
        DWORD dwRead = 0;
        DWORD dwWrote = 0;

        ceError = GPARemoteReadFile(
                        hRemoteFile,
                        szBuff,
                        sizeof(szBuff),
                        &dwRead);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!dwRead)
        {
            break;
        }

        if ((dwWrote = write(hLocalFile, szBuff, dwRead)) == -1)
        {
            ceError = errno;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

    } while(1);


cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    if (hLocalFile >= 0)
    {
        close(hLocalFile);
    }

    return (ceError);

error:

    goto cleanup;

}

CENTERROR
GPACopyMultipleFilesFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

    ceError = GPARemoteOpenFile(
                    pszSourcePath,
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    FILE_DIRECTORY_FILE,
                    &handle);
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (;;)
    {
        NTSTATUS status = STATUS_SUCCESS;

        status = LwNtQueryDirectoryFile(
            handle,                             /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                             /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,         /* Info level */
            FALSE,                                 /* Do not return single entry */
            NULL,                                 /* File spec */
            bRestart);                             /* Restart scan */

        switch (status)
        {
            case STATUS_NO_MORE_MATCHES:

                ceError = CENTERROR_SUCCESS;

                goto cleanup;

            case STATUS_SUCCESS:

                break;

            default:

                GPA_LOG_VERBOSE("Error: Failed to query directory. [status:0x%x]",
                              status);

                ceError = CENTERROR_GP_QUERY_DIRECTORY;

                BAIL_ON_CENTERIS_ERROR(ceError);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);
            RTL_FREE(&pszLocalPath);

            ceError = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            ceError = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszSourcePath,
                        pszEntryFilename);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwRtlCStringAllocatePrintf(
                        &pszLocalPath,
                        "%s/%s",
                        pszTargetPath,
                        pszEntryFilename);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if(pInfo->FileAttributes != FILE_ATTRIBUTE_DIRECTORY)
            {
                ceError = GPACopyFileFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_CENTERIS_ERROR(ceError);
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

    return ceError;

error:

    goto cleanup;
}

CENTERROR
GPACopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

    ceError = GPARemoteOpenFile(
                           pszSourcePath,
                           FILE_LIST_DIRECTORY, /* Desired access mask */
                           FILE_SHARE_READ,     /* Share access */
                           FILE_OPEN,           /* Create disposition */
                           FILE_DIRECTORY_FILE, /* Create options */
                           &handle);
    if (ceError == CENTERROR_GP_PATH_NOT_FOUND ||
        ceError == CENTERROR_GP_CREATE_FAILED)
    {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPALocalCreateDir(
                pszTargetPath,
                S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (;;)
    {
        ceError = LwNtQueryDirectoryFile(
            handle,                             /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                             /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,         /* Info level */
            FALSE,                                 /* Do not return single entry */
            NULL,                                 /* File spec */
            bRestart);                             /* Restart scan */

        switch (ceError)
        {
        case STATUS_NO_MORE_MATCHES:
            ceError = CENTERROR_SUCCESS;
            goto cleanup;
        default:
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);
            RTL_FREE(&pszLocalPath);

            ceError = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            ceError = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszSourcePath,
                        pszEntryFilename);
            BAIL_ON_CENTERIS_ERROR(ceError);

            ceError = LwRtlCStringAllocatePrintf(
                        &pszLocalPath,
                        "%s/%s",
                        pszTargetPath,
                        pszEntryFilename);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if(pInfo->FileAttributes == FILE_ATTRIBUTE_DIRECTORY)
            {
                ceError = GPACopyDirFromRemote(
                            pszRemotePath,
                            pszLocalPath);
            }
            else
            {
                ceError = GPACopyFileFromRemote(
                            pszRemotePath,
                            pszLocalPath);
            }
            BAIL_ON_CENTERIS_ERROR(ceError);
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

    return ceError;

error:

    goto cleanup;
}

CENTERROR
GPARemoteOpenFile(
    IN  PCSTR           pszFileName,
    IN  ULONG           ulDesiredAccess,
    IN  ULONG           ulShareAccess,
    IN  ULONG           ulCreateDisposition,
    IN  ULONG           ulCreateOptions,
    OUT PIO_FILE_HANDLE phFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

    status = LwRtlWC16StringAllocateFromCString(
                    &filename.FileName,
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
                NULL);                   /* ECP list */
    BAIL_ON_NT_STATUS(status);

    *phFile = handle;

cleanup:

    RTL_FREE(&filename.FileName);
    RTL_FREE(&pszRemoteFileName);

    return ceError;

error:

    *phFile = NULL;

    if (status != STATUS_SUCCESS)
    {
        if (status == LW_STATUS_OBJECT_PATH_NOT_FOUND)
        {
            GPA_LOG_VERBOSE("Error: Failed to open remote file or directory [%s][status:0x%x]",
                           pszFileName,
                           status);

            ceError = CENTERROR_GP_PATH_NOT_FOUND;
        }
        else
        {
            GPA_LOG_VERBOSE("Error: Failed to create file [%s][status:0x%x]",
                           pszFileName,
                           status);

            ceError = CENTERROR_GP_CREATE_FAILED;
        }
    }

    goto cleanup;
}

CENTERROR
GPALocalOpenFile(
    IN PCSTR pszFileName,
    IN INT  dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int fd = -1;

    BAIL_ON_NULL_POINTER(pszFileName);

    if ((fd = open(pszFileName, dwMode, dwPerms)) == -1)
    {
        ceError = errno;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }


error:

    *dwHandle = fd;

    return ceError;

}


CENTERROR
GPALocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszTmpPath = NULL;

    BAIL_ON_NULL_POINTER(pszPath);

    ceError = LwAllocateString(
                pszPath,
                &pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPALocalCreateDirInternal(
                pszTmpPath,
                NULL,
                dwFileMode);
    BAIL_ON_CENTERIS_ERROR(ceError);

cleanup:

    LW_SAFE_FREE_STRING(pszTmpPath);
    return ceError;

error:
    goto cleanup;
}


CENTERROR
GPALocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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
        ceError = GPALocalCheckDirExists(pszPath, &bDirExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bDirExists)
        {
            if (mkdir(pszPath, S_IRWXU) != 0)
            {
                ceError = errno;
                BAIL_ON_CENTERIS_ERROR(ceError);
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
        ceError = GPALocalCreateDirInternal(pszPath, pszSlash, dwFileMode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        ceError = GPALocalChangePermissions(pszPath, dwFileMode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

cleanup:

    return ceError;

error:

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        GPALocalRemoveDir(pszPath);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    goto cleanup;
}


CENTERROR
GPALocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (chmod(pszPath, dwFileMode) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            ceError = errno;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
        {
            break;
        }
    }

error:
    return ceError;
}


CENTERROR
GPALocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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
            ceError = errno;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:
    return ceError;
}


CENTERROR
GPALocalRemoveDir(
    IN PCSTR pszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[BUFF_SIZE+1];

    BAIL_ON_NULL_POINTER(pszPath);

    if ((pDir = opendir(pszPath)) == NULL)
    {
        ceError = errno;
        BAIL_ON_CENTERIS_ERROR(ceError);
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
            ceError = errno;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            ceError = GPALocalRemoveDir(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (rmdir(szBuf) < 0)
            {
                ceError = errno;
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
        else
        {
            ceError = GPALocalRemoveFile(szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
    }

    if (closedir(pDir) < 0)
    {
        pDir = NULL;
        ceError = errno;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pDir = NULL;

    if (rmdir(pszPath) < 0)
    {
        ceError = errno;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pDir)
        closedir(pDir);

    return ceError;
}


CENTERROR
GPALocalRemoveFile(
    IN PCSTR pszPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (unlink(pszPath) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            ceError = errno;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
        {
            break;
        }
    }

error:

    return ceError;
}


CENTERROR
GPARemoteReadFile(
    IN  IO_FILE_HANDLE hFile,
    OUT PVOID          pBuffer,
    IN  DWORD          dwNumberOfBytesToRead,
    OUT PDWORD         pdwBytesRead
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

    return ceError;

error:

    *pdwBytesRead = 0;
    pBuffer = NULL;

    if (status != STATUS_SUCCESS)
    {
        GPA_LOG_VERBOSE("Error: Failed to read from file. [status:0x%x]", status);

        ceError = CENTERROR_GP_READ_FAILED;
    }

    goto cleanup;
}

CENTERROR
GPARemoteWriteFile(
    IN  IO_FILE_HANDLE hFile,
    IN  PVOID          pBuffer,
    IN  DWORD          dwNumBytesToWrite,
    OUT PDWORD         pdwNumBytesWritten
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

    return ceError;

error:

    *pdwNumBytesWritten = 0;

    if (status != STATUS_SUCCESS)
    {
        GPA_LOG_VERBOSE("Error: Failed to write to file. [status:0x%x]", status);

        ceError = CENTERROR_GP_WRITE_FAILED;
    }

    goto cleanup;
}
