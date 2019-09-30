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

NTSTATUS
LwIoFuseRename(
    const char* pszOldPath,
    const char* pszNewPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_HANDLE handle = NULL;
    PIO_FUSE_CONTEXT pFuseContext = NULL;
    IO_FILE_NAME filename = {0};
    PFILE_RENAME_INFORMATION pRenameInfo = NULL;
    PWSTR pwszNewPath = NULL;
    size_t newPathLength = 0;
    
    pFuseContext = LwIoFuseGetContext();

    status = LwIoFuseGetDriverRelativePath(
        pFuseContext,
        pszNewPath,
        &pwszNewPath);
    BAIL_ON_NT_STATUS(status);

    newPathLength = LwRtlWC16StringNumChars(pwszNewPath);

    status = RTL_ALLOCATE(
        &pRenameInfo,
        FILE_RENAME_INFORMATION,
        sizeof(*pRenameInfo) + (newPathLength + 1) * sizeof(WCHAR));
    BAIL_ON_NT_STATUS(status);

    pRenameInfo->ReplaceIfExists = TRUE;
    pRenameInfo->RootDirectory = NULL;
    pRenameInfo->FileNameLength = newPathLength * sizeof(WCHAR);
    
    memcpy(pRenameInfo->FileName, pwszNewPath, newPathLength * sizeof(WCHAR));

    status = LwIoFuseGetNtFilename(
        pFuseContext,
        pszOldPath,
        &filename);
    BAIL_ON_NT_STATUS(status);
    
    status = LwNtCreateFile(
        &handle,                 /* File handle */
        NULL,                    /* Async control block */
        &ioStatus,               /* IO status block */
        &filename,               /* Filename */
        NULL,                    /* Security descriptor */
        NULL,                    /* Security QOS */
        DELETE,                  /* Desired access mask */
        0,                       /* Allocation size */
        0,                       /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,       /* Share access */
        FILE_OPEN,               /* Create disposition */
        0,                       /* Create options */
        NULL,                    /* EA buffer */
        0,                       /* EA length */
        NULL,                    /* ECP list */
        NULL);
    BAIL_ON_NT_STATUS(status);
    
    status = LwNtSetInformationFile(
        handle, /* File handle */
        NULL, /* Async control block */
        &ioStatus, /* IO status block */
        pRenameInfo, /* File information */
        sizeof(*pRenameInfo) + (newPathLength + 1) * sizeof(WCHAR), /* File information size */
        FileRenameInformation); /* Information class */
    BAIL_ON_NT_STATUS(status);
    
cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_UNICODE_STRING_FREE(&filename.Name);
    RTL_FREE(&pwszNewPath);
    RTL_FREE(&pRenameInfo);

    return status;

error:

    goto cleanup;
}
