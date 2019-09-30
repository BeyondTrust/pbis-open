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
