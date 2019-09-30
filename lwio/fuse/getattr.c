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
LwIoFuseGetattr(
    const char *pszPath,
    struct stat *pStatbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_FILE_NAME filename = {0};
    PIO_FUSE_CONTEXT pFuseContext = NULL;
    FILE_BASIC_INFORMATION basicInfo = {0};
    FILE_STANDARD_INFORMATION standardInfo = {0};
    BYTE securityBuffer[2048];

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
        READ_CONTROL | FILE_READ_ATTRIBUTES,  /* Desired access mask */
        0,                     /* Allocation size */
        0,                     /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,     /* Share access */
        FILE_OPEN,             /* Create disposition */
        0,                     /* Create options */
        NULL,                  /* EA buffer */
        0,                     /* EA length */
        NULL,                  /* ECP list */
        NULL);
    BAIL_ON_NT_STATUS(status);

    status = LwNtQueryInformationFile(
        handle,                /* File handle */
        NULL,                  /* Async control block */
        &ioStatus,             /* IO status block */
        &basicInfo,            /* Information struct */
        sizeof(basicInfo),     /* Information struct size */
        FileBasicInformation); /* Information level */
    BAIL_ON_NT_STATUS(status);

    status = LwNtQueryInformationFile(
        handle,                /* File handle */
        NULL,                  /* Async control block */
        &ioStatus,             /* IO status block */
        &standardInfo,            /* Information struct */
        sizeof(standardInfo),     /* Information struct size */
        FileStandardInformation); /* Information level */
    BAIL_ON_NT_STATUS(status);
    
    status = LwNtQuerySecurityFile(
        handle,
        NULL, /* Async control block */
        &ioStatus,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
        (PSECURITY_DESCRIPTOR_RELATIVE) (PBYTE)securityBuffer,
        sizeof(securityBuffer));
    BAIL_ON_NT_STATUS(status);

    memset(pStatbuf, 0, sizeof(*pStatbuf));

    status = LwIoFuseTranslateBasicInformation(
        &basicInfo,
        pStatbuf);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseTranslateStandardInformation(
        &standardInfo,
        pStatbuf);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseTranslateSecurityDescriptor(
        (PSECURITY_DESCRIPTOR_RELATIVE) (PBYTE)securityBuffer,
        pStatbuf);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_UNICODE_STRING_FREE(&filename.Name);

    return status;

error:
    
    goto cleanup;
}
