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
