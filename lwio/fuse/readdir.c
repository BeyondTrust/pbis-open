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

#include "includes.h"

#define MAX_BUFFER 4096

static
PFILE_BOTH_DIR_INFORMATION
LwIoFuseNextDirInfo(
    PFILE_BOTH_DIR_INFORMATION pInfo
    );

NTSTATUS
LwIoFuseReaddir(
    const char* pszPath,
    void* pBuffer,
    fuse_fill_dir_t pfFill,
    off_t offset,
    struct fuse_file_info* pFileInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL bRestart = TRUE;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_FILE_NAME filename = {0};
    PIO_FUSE_CONTEXT pFuseContext = NULL;
    PSTR pszEntryFilename = NULL;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;

    pFuseContext = LwIoFuseGetContext();

    status = LwIoFuseGetNtFilename(
        pFuseContext,
        pszPath,
        &filename);
    BAIL_ON_NT_STATUS(status);
    
    status = LwNtCreateFile(
        &handle,               /* File handle */
        NULL,                  /* Async control block */
        &ioStatus,             /* IO status block */
        &filename,             /* Filename */
        NULL,                  /* Security descriptor */
        NULL,                  /* Security QOS */
        FILE_LIST_DIRECTORY,   /* Desired access mask */
        0,                     /* Allocation size */
        0,                     /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,     /* Share access */
        FILE_OPEN,             /* Create disposition */
        FILE_DIRECTORY_FILE,   /* Create options */
        NULL,                  /* EA buffer */
        0,                     /* EA length */
        NULL,                  /* ECP list */
        NULL);
    BAIL_ON_NT_STATUS(status);

    for (;;)
    {
        status = LwNtQueryDirectoryFile(
            handle, /* File handle */
            NULL,   /* Async control block */
            &ioStatus, /* IO status block */
            buffer, /* Info structure */
            sizeof(buffer), /* Info structure size */
            FileBothDirectoryInformation, /* Info level */
            FALSE, /* Do not return single entry */
            NULL, /* File spec */
            bRestart); /* Restart scan */

        switch (status)
        {
        case STATUS_NO_MORE_MATCHES:
            status = STATUS_SUCCESS;
            goto cleanup;
        default:
            BAIL_ON_NT_STATUS(status);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo; pInfo = LwIoFuseNextDirInfo(pInfo))
        {
            RTL_FREE(&pszEntryFilename);
            
            status = LwRtlCStringAllocateFromWC16String(
                &pszEntryFilename,
                pInfo->FileName
                );
            BAIL_ON_NT_STATUS(status);
            
            if (pfFill(pBuffer, pszEntryFilename, NULL, 0))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                BAIL_ON_NT_STATUS(status);
            }
        }
    }
    
cleanup:

    if (handle)
    {
        NtCloseFile(handle);
    }

    RTL_FREE(&pszEntryFilename);
    RTL_UNICODE_STRING_FREE(&filename.Name);

    return status;

error:

    goto cleanup;
}

static
PFILE_BOTH_DIR_INFORMATION
LwIoFuseNextDirInfo(
    PFILE_BOTH_DIR_INFORMATION pInfo
    )
{
    if (pInfo->NextEntryOffset)
    {
        return (PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset);
    }
    else
    {
        return NULL;
    }
}
