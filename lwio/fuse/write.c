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
LwIoFuseWrite(
    const char* pszPath,
    const char* pData,
    size_t length,
    off_t offset,
    struct fuse_file_info* pFileInfo,
    int* pBytesWritten
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_HANDLE handle = NULL;
    ULONG64 byteOffset = (ULONG64) offset;

    handle = FUSE_TO_NT_FH(pFileInfo->fh);

    status = LwNtWriteFile(
        handle, /* File handle */
        NULL, /* Async control block */
        &ioStatus, /* IO status block */
        (PVOID) pData, /* Buffer */
        (ULONG) length, /* Buffer size */
        &byteOffset, /* File offset */
        NULL); /* Key */
    BAIL_ON_NT_STATUS(status);
    
    *pBytesWritten = (int) ioStatus.BytesTransferred;

cleanup:

    return status;

error:

    goto cleanup;
}
