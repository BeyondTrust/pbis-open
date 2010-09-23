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
 *        migrate.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        Core share migration methods
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
LwTaskReplaceCurrent(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_DIRECTORY*         ppCurrent,
    PLW_TASK_DIRECTORY*         ppDirectoryList
    );

static
DWORD
LwTaskMigrateBuildPathW(
    PWSTR  pwszPrefix,
    PWSTR  pwszServer,
    PWSTR  pwszShare,
    PWSTR* ppwszPath
    );

static
DWORD
LwTaskMigrateProcessDir(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_DIRECTORY               pFileItem,
    PLW_TASK_DIRECTORY*              ppChildFileItems
    );

static
DWORD
LwTaskMigrateProcessFile(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_DIRECTORY          pFileItem,
    PWSTR                       pwszFilename
    );

static
DWORD
LwTaskMigrateCreateFile(
    PIO_FILE_NAME                 pFilename,
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    DWORD                         dwDesiredAccess,
    DWORD                         dwFileAttributes,
    DWORD                         dwCreateDisposition,
    DWORD                         dwCreateOptions,
    DWORD                         dwShareAccess,
    PIO_FILE_HANDLE               phFile,
    FILE_CREATE_RESULT*           pCreateResult
    );

static
DWORD
LwTaskGetFileSize(
    IO_FILE_HANDLE hFile,
    PLONG64        pllFileSize
    );

static
DWORD
LwTaskCopyFile(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    IO_FILE_HANDLE              hFileTarget,
    IO_FILE_HANDLE              hFileSource
    );

static
DWORD
LwTaskReadFile(
    HANDLE hFile,
    PBYTE  pBuffer,
    DWORD  dwNumberOfBytesToRead,
    PDWORD pdwBytesRead
    );

static
DWORD
LwTaskWriteFile(
    HANDLE hFile,
    PBYTE  pBuffer,
    DWORD  dwNumBytesToWrite,
    PDWORD pdwNumBytesWritten
    );

DWORD
LwTaskMigrateOpenRemoteShare(
    PWSTR           pwszServer,
    PWSTR           pwszShare,
    PIO_FILE_HANDLE phFileRemote
    )
{
    DWORD           dwError = 0;
    IO_FILE_HANDLE  hFile   = NULL;
    IO_FILE_NAME    fileName = {0};
    DWORD           dwDesiredAccess = READ_CONTROL|FILE_LIST_DIRECTORY;
    DWORD           dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    DWORD           dwCreateDisposition = FILE_OPEN;
    DWORD           dwCreateOptions = FILE_DIRECTORY_FILE;
    DWORD           dwShareAccess  = FILE_SHARE_READ;

    dwError = LwTaskMigrateBuildPathW(
                    &gLwTaskGlobals.wszRemoteDriverPrefix[0],
                    pwszServer,
                    pwszShare,
                    &fileName.FileName);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateCreateFile(
                    &fileName,
                    NULL,                 /* security descriptor */
                    dwDesiredAccess,
                    dwFileAttributes,
                    dwCreateDisposition,
                    dwCreateOptions,
                    dwShareAccess,
                    &hFile,
                    NULL);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *phFileRemote = hFile;

cleanup:

    LW_SAFE_FREE_MEMORY(fileName.FileName);

    return dwError;

error:

    *phFileRemote = NULL;

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    goto cleanup;
}

DWORD
LwTaskMigrateCreateShare(
    PSHARE_INFO_502 pShareInfoRemote,
    BOOLEAN         bAddShare,
    PIO_FILE_HANDLE phShare
    )
{
    DWORD dwError = 0;
    DWORD dwParmError = 0;
    PWSTR pwszLocalPathWithDriveLetter = NULL;
    PWSTR pwszLocalPath = NULL;
    IO_FILE_HANDLE hFile = NULL;
#if 0
    PIO_ASYNC_CONTROL_BLOCK pAcb = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
#endif
    IO_FILE_NAME fileName = {0};
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
#if 0
    DWORD dwSecDescLen = 0;
#endif
    SHARE_INFO_502 shareInfoLocal = {0};
    DWORD dwDesiredAccess = WRITE_OWNER|WRITE_DAC|READ_CONTROL|FILE_TRAVERSE;
    DWORD dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    DWORD dwCreateDisposition = FILE_OPEN_IF;
    DWORD dwCreateOptions = FILE_DIRECTORY_FILE;

    dwError = LwTaskGetLocalSharePathW(
    				pShareInfoRemote->shi502_path,
					&pwszLocalPathWithDriveLetter);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskGetMappedSharePathW(
                    &gLwTaskGlobals.wszDiskDriverPrefix[0],
                    pwszLocalPathWithDriveLetter,
                    &pwszLocalPath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    fileName.FileName = pwszLocalPath;

    dwError = LwTaskMigrateCreateFile(
                    &fileName,
                    pSecDesc,
                    dwDesiredAccess,
                    dwFileAttributes,
                    dwCreateDisposition,
                    dwCreateOptions,
                    0, /* Share access */
                    &hFile,
                    NULL);
    BAIL_ON_LW_TASK_ERROR(dwError);

#if 0
    //
    // TODO: The server should apply the default security descriptor
    //       Otherwise, we should create the one we want and apply it
    //
    if (ioStatusBlock.CreateResult == FILE_CREATED)
    {
        SECURITY_INFORMATION secInfo = (OWNER_SECURITY_INFORMATION|
                                            GROUP_SECURITY_INFORMATION|
                                            DACL_SECURITY_INFORMATION);

        dwError = LwNtStatusToWin32Error(
                        LwNtSetSecurityFile(
                            hFile,
                            pAcb,
                            &ioStatusBlock,
                            secInfo,
                            pSecDesc,
                            dwSecDescLen
                            ));
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
#endif

    if (bAddShare)
    {
        shareInfoLocal.shi502_netname  = pShareInfoRemote->shi502_netname;
        shareInfoLocal.shi502_max_uses = pShareInfoRemote->shi502_max_uses;
        // TODO: Map other drive letters appropriately
        shareInfoLocal.shi502_path     = pwszLocalPathWithDriveLetter;
        shareInfoLocal.shi502_type     = pShareInfoRemote->shi502_type;
        shareInfoLocal.shi502_remark   = pShareInfoRemote->shi502_remark;
        shareInfoLocal.shi502_reserved = pShareInfoRemote->shi502_reserved;
        shareInfoLocal.shi502_security_descriptor =
                    pShareInfoRemote->shi502_security_descriptor;

        dwError = NetShareAddW(
                        NULL,
                        502,
                        (PBYTE)&shareInfoLocal,
                        &dwParmError);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    *phShare = hFile;

cleanup:

    if (pSecDesc)
    {
        LwFreeMemory(pSecDesc);
    }

    LW_SAFE_FREE_MEMORY(pwszLocalPathWithDriveLetter);
    LW_SAFE_FREE_MEMORY(pwszLocalPath);

    return dwError;

error:

    *phShare = NULL;

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    goto cleanup;
}

DWORD
LwTaskMigrateShareEx(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_FILE               pRemoteFile,
    PLW_TASK_FILE               pLocalFile,
    LW_MIGRATE_FLAGS            dwFlags
    )
{
    DWORD dwError = 0;
    PLW_TASK_DIRECTORY pDirectoryList = NULL;

    dwError = LwTaskCreateDirectory(
                    NULL,
                    pRemoteFile,
                    pLocalFile,
                    &pContext->pHead);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pContext->pTail = pContext->pHead;

    while (pContext->pHead)
    {
        PLW_TASK_DIRECTORY pCursor = pContext->pHead;

        while (pCursor)
        {
            PLW_TASK_DIRECTORY pCurrent = pCursor;

            dwError = LwTaskMigrateProcessDir(
                                pContext,
                                pCursor,
                                &pDirectoryList);
            BAIL_ON_LW_TASK_ERROR(dwError);

            pCursor = pCursor->pNext; // First advance the cursor

            LwTaskReplaceCurrent(pContext, &pCurrent, &pDirectoryList);
        }
    }

cleanup:

    if (pDirectoryList)
    {
        LwTaskFreeDirectoryList(pDirectoryList);
    }

    return dwError;

error:

    goto cleanup;
}

static
VOID
LwTaskReplaceCurrent(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_DIRECTORY*         ppCurrent,
    PLW_TASK_DIRECTORY*         ppDirectoryList
    )
{
    PLW_TASK_DIRECTORY pCurrent = *ppCurrent;
    PLW_TASK_DIRECTORY pDirectoryList = *ppDirectoryList;

    if (pContext->pHead == pCurrent)
    {
        if (pDirectoryList)
        {
            pContext->pHead = pDirectoryList;
            // Seek end of list
            while (pDirectoryList->pNext)
            {
                pDirectoryList = pDirectoryList->pNext;
            }
            pDirectoryList->pNext = pCurrent->pNext;
            if (pCurrent->pNext)
            {
                pCurrent->pNext->pPrev = pDirectoryList;
            }

            if (!pDirectoryList->pNext)
            {
                pContext->pTail = pDirectoryList;
            }
        }
        else
        {
            pContext->pHead = pCurrent->pNext;
            if (!pContext->pHead)
            {
                pContext->pTail = NULL;
            }
            else
            {
                pContext->pHead->pPrev = NULL;
            }
        }
    }
    else if (pContext->pTail == pCurrent)
    {
        pContext->pTail = pCurrent->pPrev;
        pCurrent->pPrev->pNext = pDirectoryList;
        while (pContext->pTail->pNext)
        {
            pContext->pTail = pContext->pTail->pNext;
        }
    }
    else
    {
        if (pDirectoryList)
        {
            pCurrent->pPrev->pNext = pDirectoryList;
            while (pDirectoryList->pNext)
            {
                pDirectoryList = pDirectoryList->pNext;
            }

            pDirectoryList->pNext = pCurrent->pNext;
            pCurrent->pNext->pPrev = pDirectoryList;
        }
        else
        {
            pCurrent->pPrev->pNext = pCurrent->pNext;
            pCurrent->pNext->pPrev = pCurrent->pPrev;
        }
    }

    pCurrent->pPrev = pCurrent->pNext = NULL;

    *ppDirectoryList = NULL;

    if (pCurrent)
    {
        LwTaskFreeDirectoryList(pCurrent);
        *ppCurrent = NULL;
    }
}

static
DWORD
LwTaskMigrateBuildPathW(
    PWSTR  pwszPrefix,
    PWSTR  pwszServer,
    PWSTR  pwszShare,
    PWSTR* ppwszPath
    )
{
    DWORD dwError = 0;
    wchar16_t wszSeparator[] = { '/', 0 };
    DWORD dwLen = 0;
    PWSTR pwszPath = NULL;
    PWSTR pwszCursor = NULL;

    dwLen =  (wc16slen(pwszPrefix) +
              ((sizeof(wszSeparator)/sizeof(wszSeparator[0])) - 1) +
              wc16slen(pwszServer) +
              ((sizeof(wszSeparator)/sizeof(wszSeparator[0])) - 1) +
              wc16slen(pwszShare)) * sizeof(wchar16_t);

    dwError = LwAllocateMemory(dwLen, (PVOID*)&pwszPath);
    BAIL_ON_LW_TASK_ERROR(dwError);

    pwszCursor = pwszPath;
    dwLen = wc16slen(pwszPrefix);
    memcpy((PBYTE)pwszCursor, (PBYTE)pwszPrefix, dwLen * sizeof(wchar16_t));
    pwszCursor += dwLen;

    *pwszCursor++ = wszSeparator[0];

    dwLen = wc16slen(pwszServer);
    memcpy((PBYTE)pwszCursor, (PBYTE)pwszServer, dwLen * sizeof(wchar16_t));
    pwszCursor += dwLen;

    *pwszCursor++ = wszSeparator[0];

    dwLen = wc16slen(pwszShare);
    memcpy((PBYTE)pwszCursor, (PBYTE)pwszShare, dwLen * sizeof(wchar16_t));
    // pwszCursor += dwLen;

    *ppwszPath = pwszPath;

cleanup:

    return dwError;

error:

    *ppwszPath = NULL;

    LW_SAFE_FREE_MEMORY(pwszPath);

    goto cleanup;
}

static
DWORD
LwTaskMigrateProcessDir(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_DIRECTORY          pFileItem,
    PLW_TASK_DIRECTORY*         ppChildDirs
    )
{
    DWORD dwError = 0;
    BOOLEAN bDone = FALSE;
    BOOLEAN bRestart = TRUE;
    wchar16_t wszDot[]    = {'.'};
    wchar16_t wszDotDot[] = { '.', '.'};
    PLW_TASK_DIRECTORY pChildDirsHead = NULL;
    PLW_TASK_DIRECTORY pChildDirsTail = NULL;
    PLW_TASK_DIRECTORY pChildDir = NULL;
    PWSTR pwszFilename = NULL;
    PLW_TASK_FILE pChildRemote = NULL;
    PLW_TASK_FILE pChildLocal  = NULL;

    do
    {
        NTSTATUS        status = STATUS_SUCCESS;
        IO_STATUS_BLOCK ioStatusBlock = {0};
        BYTE            buffer[MAX_BUFFER] = {0};

        dwError = LwNtStatusToWin32Error(
                        LwIoSetThreadCreds(pContext->pRemoteCreds->pKrb5Creds));
        BAIL_ON_LW_TASK_ERROR(dwError);

        status = LwNtQueryDirectoryFile(
                        pFileItem->pParentRemote->hFile,
                        NULL,                          /* Async control block */
                        &ioStatusBlock,                /* IO status block     */
                        buffer,                        /* Info structure      */
                        sizeof(buffer),                /* Info structure size */
                        FileBothDirectoryInformation,  /* Info level          */
                        FALSE,                         /* no single entry     */
                        NULL,                          /* File spec           */
                        bRestart);                     /* Restart scan        */

        switch (status)
        {
            case STATUS_NO_MORE_MATCHES:

                status = STATUS_SUCCESS;

                bDone = TRUE;

                break;

            default:

                dwError = LwNtStatusToWin32Error(status);
                BAIL_ON_LW_TASK_ERROR(dwError);
        }

        if (!bDone)
        {
            PFILE_BOTH_DIR_INFORMATION pInfo = NULL;

            bRestart = FALSE;

            pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer;
            while (pInfo)
            {
                // TODO: Handle short (8.3) file names
                if ((pInfo->FileNameLength == sizeof(wszDot) &&
                     pInfo->FileName[0] == wszDot[0]) ||
                    (pInfo->FileNameLength == sizeof(wszDotDot) &&
                     pInfo->FileName[0] == wszDotDot[0] &&
                     pInfo->FileName[1] == wszDotDot[1]))
                {
                    if (pInfo->NextEntryOffset)
                    {
                        pInfo = (PFILE_BOTH_DIR_INFORMATION)(((PBYTE) pInfo) + pInfo->NextEntryOffset);
                    }
                    else
                    {
                        pInfo = NULL;
                    }

                    continue;
                }

                if (pInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                {
                    ;
                }
                else if(pInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    IO_FILE_NAME fileName = {0};
                    DWORD  dwDesiredAccess =
                                 READ_CONTROL|FILE_LIST_DIRECTORY|FILE_TRAVERSE;
                    DWORD  dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                    DWORD  dwCreateDisposition = FILE_OPEN;
                    DWORD  dwCreateOptions = FILE_DIRECTORY_FILE;
                    IO_STATUS_BLOCK ioStatusBlock = {0};
                    SECURITY_INFORMATION dwSecInfo =
                    								OWNER_SECURITY_INFORMATION |
                    								GROUP_SECURITY_INFORMATION |
                    								DACL_SECURITY_INFORMATION;
                    BYTE  secDescBuffer[2048];
                    DWORD dwSecDescLength = sizeof(secDescBuffer);

                    LW_SAFE_FREE_MEMORY(pwszFilename);
                    pwszFilename = NULL;

                    dwError = LwAllocateMemory(
                                    pInfo->FileNameLength + sizeof(wchar16_t),
                                    (PVOID*)&pwszFilename);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    memcpy( (PBYTE)pwszFilename,
                            (PBYTE)pInfo->FileName,
                            pInfo->FileNameLength);

                    fileName.FileName = pwszFilename;
                    fileName.RootFileHandle = pFileItem->pParentRemote->hFile;

                    if (pChildRemote)
                    {
                        LwTaskReleaseFile(pChildRemote);
                        pChildRemote = NULL;
                    }

                    dwError = LwTaskCreateFile(&pChildRemote);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    // Make sure we are using remote creds
                    dwError = LwNtStatusToWin32Error(
                                    LwIoSetThreadCreds(
                                        pContext->pRemoteCreds->pKrb5Creds));
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    dwError = LwTaskMigrateCreateFile(
                                    &fileName,
                                    NULL,
                                    dwDesiredAccess,
                                    dwFileAttributes,
                                    dwCreateDisposition,
                                    dwCreateOptions,
                                    FILE_SHARE_READ,
                                    &pChildRemote->hFile,
                                    NULL);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    memset(&secDescBuffer, 0, sizeof(secDescBuffer));

                    dwError = LwNtStatusToWin32Error(
                                    LwNtQuerySecurityFile(
                                        pChildRemote->hFile,
                                        NULL,
                                        &ioStatusBlock,
                                        dwSecInfo,
                                        (PSECURITY_DESCRIPTOR_RELATIVE)&secDescBuffer[0],
                                        dwSecDescLength));
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    fileName.RootFileHandle = pFileItem->pParentLocal->hFile;

                    if (pChildLocal)
                    {
                        LwTaskReleaseFile(pChildLocal);
                        pChildLocal = NULL;
                    }

                    dwError = LwTaskCreateFile(&pChildLocal);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    dwDesiredAccess =
                            WRITE_OWNER|WRITE_DAC|READ_CONTROL|FILE_TRAVERSE;
                    dwCreateDisposition = FILE_OPEN_IF;

                    // switch to local creds
                    dwError = LwNtStatusToWin32Error(
                                    LwIoSetThreadCreds(
                                            pContext->pLocalCreds));
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    dwError = LwTaskMigrateCreateFile(
                                    &fileName,
                                    (PSECURITY_DESCRIPTOR_RELATIVE)&secDescBuffer[0],
                                    dwDesiredAccess,
                                    dwFileAttributes,
                                    dwCreateDisposition,
                                    dwCreateOptions,
                                    0, /* Share access */
                                    &pChildLocal->hFile,
                                    NULL);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    dwError = LwTaskCreateDirectory(
                                    NULL,
                                    pChildRemote,
                                    pChildLocal,
                                    &pChildDir);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    if (!pChildDirsHead)
                    {
                        pChildDirsHead = pChildDirsTail = pChildDir;
                    }
                    else
                    {
                        pChildDirsTail->pNext = pChildDir;
                        pChildDir->pPrev = pChildDirsTail;
                        pChildDirsTail = pChildDir;
                        pChildDir = NULL;
                    }
                }
                else // File
                {
                    LW_SAFE_FREE_MEMORY(pwszFilename);
                    pwszFilename = NULL;

                    dwError = LwAllocateMemory(
                                    pInfo->FileNameLength + sizeof(wchar16_t),
                                    (PVOID*)&pwszFilename);
                    BAIL_ON_LW_TASK_ERROR(dwError);

                    memcpy( (PBYTE)pwszFilename,
                            (PBYTE)pInfo->FileName,
                            pInfo->FileNameLength);

                    dwError = LwTaskMigrateProcessFile(
                                    pContext,
                                    pFileItem,
                                    pwszFilename);
                    BAIL_ON_LW_TASK_ERROR(dwError);
                }

                if (pInfo->NextEntryOffset)
                {
                    pInfo = (PFILE_BOTH_DIR_INFORMATION)(((PBYTE) pInfo) + pInfo->NextEntryOffset);
                }
                else
                {
                    pInfo = NULL;
                }
            }
        }

    } while (!bDone);

    *ppChildDirs = pChildDirsHead;

cleanup:

    LW_SAFE_FREE_MEMORY(pwszFilename);

    if (pChildRemote)
    {
        LwTaskReleaseFile(pChildRemote);
    }
    if (pChildLocal)
    {
        LwTaskReleaseFile(pChildLocal);
    }

    return dwError;

error:

    *ppChildDirs = NULL;

    if (pChildDirsHead)
    {
        LwTaskFreeDirectoryList(pChildDirsHead);
    }

    if (pChildDir)
    {
        LwTaskFreeDirectoryList(pChildDir);
    }

    goto cleanup;
}

static
DWORD
LwTaskMigrateProcessFile(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    PLW_TASK_DIRECTORY          pFileItem,
    PWSTR                       pwszFilename
    )
{
    DWORD dwError = 0;
    DWORD dwDesiredAccess = READ_CONTROL|GENERIC_READ;
    DWORD dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    DWORD dwCreateDisposition = FILE_OPEN;
    DWORD dwCreateOptions = FILE_NON_DIRECTORY_FILE;
    IO_FILE_NAME   fileName    = {0};
    IO_FILE_HANDLE hFileRemote = NULL;
    IO_FILE_HANDLE hFileLocal  = NULL;
    LONG64 llRemoteFileSize    = 0LL;
    LONG64 llLocalFileSize     = 0LL;
    FILE_CREATE_RESULT createResult = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    SECURITY_INFORMATION dwSecInfo = 	OWNER_SECURITY_INFORMATION |
    									GROUP_SECURITY_INFORMATION |
    									DACL_SECURITY_INFORMATION;
    BYTE  secDescBuffer[2048];
    DWORD dwSecDescLength = sizeof(secDescBuffer);

    fileName.FileName = pwszFilename;
    fileName.RootFileHandle = pFileItem->pParentRemote->hFile;

    // Make sure we are using remote creds
    dwError = LwNtStatusToWin32Error(
                    LwIoSetThreadCreds(
                        pContext->pRemoteCreds->pKrb5Creds));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateCreateFile(
                    &fileName,
                    NULL,
                    dwDesiredAccess,
                    dwFileAttributes,
                    dwCreateDisposition,
                    dwCreateOptions,
                    0, /* Share access */
                    &hFileRemote,
                    NULL);
    BAIL_ON_LW_TASK_ERROR(dwError);

    memset(&secDescBuffer, 0, sizeof(secDescBuffer));

    dwError = LwNtStatusToWin32Error(
                    LwNtQuerySecurityFile(
                        hFileRemote,
                        NULL,
                        &ioStatusBlock,
                        dwSecInfo,
                        (PSECURITY_DESCRIPTOR_RELATIVE)&secDescBuffer[0],
                        dwSecDescLength));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskGetFileSize(hFileRemote, &llRemoteFileSize);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwDesiredAccess = WRITE_DAC|WRITE_OWNER|READ_CONTROL|GENERIC_WRITE;
    dwCreateDisposition = FILE_OPEN_IF;

    fileName.RootFileHandle = pFileItem->pParentLocal->hFile;

    // switch to local creds
    dwError = LwNtStatusToWin32Error(
                    LwIoSetThreadCreds(
                            pContext->pLocalCreds));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskMigrateCreateFile(
                    &fileName,
                    (PSECURITY_DESCRIPTOR_RELATIVE)&secDescBuffer[0],
                    dwDesiredAccess,
                    dwFileAttributes,
                    dwCreateDisposition,
                    dwCreateOptions,
                    0, /* Share access */
                    &hFileLocal,
                    &createResult);
    BAIL_ON_LW_TASK_ERROR(dwError);

    switch (createResult)
    {
        case FILE_OPENED:

            dwError = LwTaskGetFileSize(hFileLocal, &llLocalFileSize);
            BAIL_ON_LW_TASK_ERROR(dwError);

        default:

            break;
    }

    if (llLocalFileSize != llRemoteFileSize)
    {
        dwError = LwTaskCopyFile(pContext, hFileLocal, hFileRemote);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

cleanup:

    if (hFileRemote)
    {
        LwNtCloseFile(hFileRemote);
    }
    if (hFileLocal)
    {
        LwNtCloseFile(hFileLocal);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwTaskMigrateCreateFile(
    PIO_FILE_NAME                 pFilename,
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    DWORD                         dwDesiredAccess,
    DWORD                         dwFileAttributes,
    DWORD                         dwCreateDisposition,
    DWORD                         dwCreateOptions,
    DWORD                         dwShareAccess,
    PIO_FILE_HANDLE               phFile,
    FILE_CREATE_RESULT*           pCreateResult
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_HANDLE hFile = NULL;

    ntStatus = LwNtCreateFile(
                    &hFile,
                    NULL,                           /* Async control block */
                    &ioStatusBlock,
                    pFilename,
                    pSecDesc,
                    NULL,                           /* Security QOS        */
                    dwDesiredAccess,
                    0,                              /* AllocationSize      */
                    dwFileAttributes,
                    dwShareAccess,
                    dwCreateDisposition,
                    dwCreateOptions,
                    NULL,                           /* EaBuffer            */
                    0,                              /* EaLength            */
                    NULL,                           /* EcpList             */
                    NULL                            /* credentials         */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    *phFile = hFile;
    if (pCreateResult)
    {
        *pCreateResult = ioStatusBlock.CreateResult;
    }

cleanup:

    return dwError;

error:

    *phFile = NULL;
    if (pCreateResult)
    {
        *pCreateResult = 0;
    }

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    dwError = LwNtStatusToWin32Error(ntStatus);

    goto cleanup;
}

static
DWORD
LwTaskGetFileSize(
    IO_FILE_HANDLE hFile,
    PLONG64        pllFileSize
    )
{
    DWORD dwError = 0;
    FILE_STANDARD_INFORMATION fileStdInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};

    dwError = LwNtStatusToWin32Error(
                    LwNtQueryInformationFile(
                        hFile,
                        NULL, /* Async control block */
                        &ioStatusBlock,
                        &fileStdInfo,
                        sizeof(fileStdInfo),
                        FileStandardInformation));
    BAIL_ON_LW_TASK_ERROR(dwError);

    *pllFileSize = fileStdInfo.EndOfFile;

cleanup:

    return dwError;

error:

    *pllFileSize = 0;

    goto cleanup;
}

static
DWORD
LwTaskCopyFile(
    PLW_SHARE_MIGRATION_CONTEXT pContext,
    IO_FILE_HANDLE              hFileTarget,
    IO_FILE_HANDLE              hFileSource
    )
{
    DWORD dwError = 0;
    DWORD dwRead  = 0;

    do
    {
        BYTE  buffer[MAX_BUFFER] = {0};
        DWORD dwWritten = 0;

        dwError = LwNtStatusToWin32Error(
                        LwIoSetThreadCreds(
                            pContext->pRemoteCreds->pKrb5Creds));
        BAIL_ON_LW_TASK_ERROR(dwError);

        dwRead = 0;

        dwError = LwTaskReadFile(
                        hFileSource,
                        buffer,
                        sizeof(buffer),
                        &dwRead);
        BAIL_ON_LW_TASK_ERROR(dwError);

        if (dwRead)
        {
            dwError = LwNtStatusToWin32Error(
                            LwIoSetThreadCreds(
                                    pContext->pLocalCreds));
            BAIL_ON_LW_TASK_ERROR(dwError);

            dwError = LwTaskWriteFile(
                            hFileTarget,
                            buffer,
                            dwRead,
                            &dwWritten);
            BAIL_ON_LW_TASK_ERROR(dwError);
        }

    } while (dwRead > 0);

error:

    return dwError;
}

static
DWORD
LwTaskReadFile(
    HANDLE hFile,
    PBYTE  pBuffer,
    DWORD  dwNumberOfBytesToRead,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    status = LwNtReadFile(
                    hFile,
                    NULL,                                // Async control block
                    &ioStatusBlock,
                    pBuffer,
                    dwNumberOfBytesToRead,
                    NULL,                                // File offset
                    NULL);                               // Key
    if (status != STATUS_END_OF_FILE)
    {
        dwError = LwNtStatusToWin32Error(status);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }
    // else
    // {
    //     status = STATUS_SUCCESS;
    // }

    *pdwBytesRead = ioStatusBlock.BytesTransferred;

cleanup:

    return dwError;

error:

    *pdwBytesRead = 0;

    goto cleanup;
}

static
DWORD
LwTaskWriteFile(
    HANDLE hFile,
    PBYTE  pBuffer,
    DWORD  dwNumBytesToWrite,
    PDWORD pdwNumBytesWritten
    )
{
    DWORD dwError = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    dwError = LwNtStatusToWin32Error(
                LwNtWriteFile(
                    hFile,
                    NULL,                                 // Async control block
                    &ioStatusBlock,
                    pBuffer,
                    dwNumBytesToWrite,
                    NULL,                                 // File offset
                    NULL));                               // Key
    BAIL_ON_LW_TASK_ERROR(dwError);

    *pdwNumBytesWritten = ioStatusBlock.BytesTransferred;

cleanup:

    return dwError;

error:

    *pdwNumBytesWritten = 0;

    goto cleanup;
}





