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


static
NTSTATUS
NfsIoPrepareEcpList(
    IN PNFS_SHARE_INFO pShareInfo,
    IN OUT PIO_ECP_LIST* ppEcpList
    );

static
VOID
NfsIoFreeEcpShareName(
    IN PVOID pContext
    );

NTSTATUS
NfsIoCreateFile(
    IN PNFS_SHARE_INFO pShareInfo,
    OUT PIO_FILE_HANDLE pFileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock,
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN PIO_CREATE_SECURITY_CONTEXT pSecurityContext,
    IN PIO_FILE_NAME pFileName,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN OPTIONAL PVOID pSecurityQualityOfService,
    IN ACCESS_MASK DesiredAccess,
    IN OPTIONAL LONG64 AllocationSize,
    IN FILE_ATTRIBUTES FileAttributes,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN OPTIONAL PVOID pEaBuffer,
    IN ULONG EaLength,
    IN OUT PIO_ECP_LIST* ppEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACCESS_TOKEN pAccessToken = NULL;
    ACCESS_MASK GrantedAccess = 0;
    ACCESS_MASK MappedDesiredAccess = 0;
    GENERIC_MAPPING GenericMap = {
        .GenericRead    = FILE_GENERIC_READ,
        .GenericWrite   = FILE_GENERIC_WRITE,
        .GenericExecute = FILE_GENERIC_EXECUTE,
        .GenericAll     = FILE_ALL_ACCESS };

    pAccessToken = IoSecurityGetAccessToken(pSecurityContext);
    if (pAccessToken == NULL)
    {
        ntStatus = STATUS_NO_TOKEN;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Check against the Share Security Descriptor */

    ntStatus = NfsShareAccessCheck(
                   pShareInfo,
                   pAccessToken,
                   MAXIMUM_ALLOWED,
                   &GenericMap,
                   &GrantedAccess);
    BAIL_ON_NT_STATUS(ntStatus);

    if (GrantedAccess == 0)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Empirical testing implies that the WRITE_DAC|WRITE_OWNER
       bits are not enforced in Share ACLs.  For example, it
       is possible to update the SD on a file on a "ReadOnly"
       Windows 2008 Share if the NTFS permissions allow it */

    GrantedAccess |= (WRITE_DAC|WRITE_OWNER);

    /* Special access requirements for specific CreateDispositions */

    switch (CreateDisposition)
    {
    case FILE_SUPERSEDE:
        if ((GrantedAccess & (FILE_WRITE_DATA|DELETE)) != (FILE_WRITE_DATA|DELETE))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        break;

    case FILE_CREATE:
    case FILE_OVERWRITE:
    case FILE_OVERWRITE_IF:
    case FILE_OPEN_IF:
        if (!(GrantedAccess & FILE_WRITE_DATA))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        break;
    case FILE_OPEN:
        break;
    }

    /* Is the caller asking for more bits than we allow?  */

    MappedDesiredAccess = DesiredAccess;
    RtlMapGenericMask(&MappedDesiredAccess, &GenericMap);

    if ((GrantedAccess & MappedDesiredAccess) != MappedDesiredAccess)
    {
        /* Escape clause for MAXIMUM_ALLOWED */

        if ((MappedDesiredAccess & MAXIMUM_ALLOWED) &&
            (GrantedAccess == FILE_ALL_ACCESS))
        {
            ntStatus = STATUS_SUCCESS;
        }
        else
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    ntStatus = NfsIoPrepareEcpList(pShareInfo, ppEcpList);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Do the open */

    ntStatus = IoCreateFile(
                   pFileHandle,
                   pAsyncControlBlock,
                   pIoStatusBlock,
                   pSecurityContext,
                   pFileName,
                   pSecurityDescriptor,
                   pSecurityQualityOfService,
                   DesiredAccess,
                   AllocationSize,
                   FileAttributes,
                   ShareAccess,
                   CreateDisposition,
                   CreateOptions,
                   pEaBuffer,
                   EaLength,
                   *ppEcpList);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
NfsIoPrepareEcpList(
    IN PNFS_SHARE_INFO pShareInfo,
    IN OUT PIO_ECP_LIST* ppEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUNICODE_STRING pShareName = NULL;

    if (NfsElementsGetShareNameEcpEnabled())
    {
        if (!*ppEcpList)
        {
            ntStatus = IoRtlEcpListAllocate(ppEcpList);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = RTL_ALLOCATE(&pShareName, UNICODE_STRING, sizeof(*pShareName));
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlUnicodeStringAllocateFromWC16String(
                        pShareName,
                        pShareInfo->pwszName);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoRtlEcpListInsert(
                        *ppEcpList,
                        SRV_ECP_TYPE_SHARE_NAME,
                        pShareName,
                        sizeof(*pShareName),
                        NfsIoFreeEcpShareName);
        BAIL_ON_NT_STATUS(ntStatus);

        pShareName = NULL;
    }

cleanup:

    RTL_FREE(&pShareName);

    return ntStatus;

error:

    goto cleanup;
}
    
static
VOID
NfsIoFreeEcpShareName(
    IN PVOID pContext
    )
{
    PUNICODE_STRING pShareName = (PUNICODE_STRING) pContext;

    if (pShareName)
    {
        RtlUnicodeStringFree(pShareName);
        RtlMemoryFree(pShareName);
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
