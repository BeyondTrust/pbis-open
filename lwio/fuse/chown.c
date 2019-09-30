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
