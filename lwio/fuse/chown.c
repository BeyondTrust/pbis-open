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
LwIoFuseChown(
    const char* pszPath,
    uid_t uid,
    gid_t gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;
    PSID pUserSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pRelative = NULL;
    ULONG ulLength = 0;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_HANDLE handle = NULL;
    PIO_FUSE_CONTEXT pFuseContext = NULL;
    IO_FILE_NAME filename = {0};

    status = LwMapSecurityCreateContext(&pContext);
    BAIL_ON_NT_STATUS(status);

    if (uid != (uid_t) -1)
    {
        status = LwMapSecurityGetSidFromId(pContext, &pUserSid, TRUE, (ULONG) uid);
        BAIL_ON_NT_STATUS(status);
    }

    if (gid != (gid_t) -1)
    {
        status = LwMapSecurityGetSidFromId(pContext, &pGroupSid, FALSE, (ULONG) gid);
        BAIL_ON_NT_STATUS(status);
    }

    status = RTL_ALLOCATE(&pAbsolute, VOID, SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(status);

    status = RtlCreateSecurityDescriptorAbsolute(pAbsolute, SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(status);

    if (pUserSid)
    {
        status = RtlSetOwnerSecurityDescriptor(pAbsolute, pUserSid, FALSE);
        BAIL_ON_NT_STATUS(status);
    }

    if (pGroupSid)
    {
        status = RtlSetGroupSecurityDescriptor(pAbsolute, pGroupSid, FALSE);
        BAIL_ON_NT_STATUS(status);
    }

    status = RtlAbsoluteToSelfRelativeSD(
        pAbsolute,
        NULL,
        &ulLength);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        BAIL_ON_NT_STATUS(status);
    }
    status = STATUS_SUCCESS;

    status = RTL_ALLOCATE(&pRelative, VOID, ulLength);
    BAIL_ON_NT_STATUS(status);

    status = RtlAbsoluteToSelfRelativeSD(
        pAbsolute,
        pRelative,
        &ulLength);
    BAIL_ON_NT_STATUS(status);

    pFuseContext = LwIoFuseGetContext();

    status = LwIoFuseGetNtFilename(
        pFuseContext,
        pszPath,
        &filename);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
        &handle,                 /* File handle */
        NULL,                    /* Async control block */
        &ioStatus,               /* IO status block */
        &filename,               /* Filename */
        NULL,                    /* Security descriptor */
        NULL,                    /* Security QOS */
        WRITE_OWNER,             /* Desired access mask */
        0,                       /* Allocation size */
        0,                       /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,       /* Share access */
        FILE_OPEN,               /* Create disposition */
        0,                       /* Create options */
        NULL,                    /* EA buffer */
        0,                       /* EA length */
        NULL,                     /* ECP list */
        NULL);
    BAIL_ON_NT_STATUS(status);

    status = LwNtSetSecurityFile(
        handle, /* File handle */
        NULL, /* Async control block */
        &ioStatus, /* IO status block */
        (pUserSid ? OWNER_SECURITY_INFORMATION : 0) |
        (pGroupSid ? GROUP_SECURITY_INFORMATION : 0),
        pRelative,
        ulLength);
    BAIL_ON_NT_STATUS(status);

error:

    LwMapSecurityFreeContext(&pContext);

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_UNICODE_STRING_FREE(&filename.Name);
    RTL_FREE(&pUserSid);
    RTL_FREE(&pGroupSid);
    RTL_FREE(&pRelative);
    RTL_FREE(&pAbsolute);

    return status;
}
