/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include <lwrpc/sidhelper.h>
#include <lw/security-api.h>
#include <lwrpc/allocate.h>
#include <string.h>

NTSTATUS
MsRpcDuplicateSid(
    OUT PSID* ppNewSid,
    IN PSID pSourceSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pResultSid = NULL;
    ULONG size = 0;

    if (!ppNewSid || !pSourceSid)
    {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    size = RtlLengthSid(pSourceSid);

    // msrpc allocator is malloc by definition.   It really should
    // use a wrapper.
    pResultSid = malloc(size);
    if (!pResultSid)
    {
        status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    memcpy(pResultSid, pSourceSid, size);

cleanup:
    if (status)
    {
        SAFE_FREE(pResultSid);
    }

    *ppNewSid = pResultSid;

    return status;
}

NTSTATUS
MsRpcAllocateSidAppendRid(
    OUT PSID* ppSid,
    IN PSID pDomainSid,
    IN ULONG Rid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pResultSid = NULL;
    ULONG size = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);

    // msrpc allocator is malloc by definition.   It really should
    // use a wrapper.
    pResultSid = malloc(size);
    if (!pResultSid)
    {
        status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    status = RtlCopySid(size, pResultSid, pDomainSid);
    if (status)
    {
        goto cleanup;
    }

    status = RtlAppendRidSid(size, pResultSid, Rid);
    if (status)
    {
        goto cleanup;
    }

cleanup:
    if (status)
    {
        SAFE_FREE(pResultSid);
    }

    *ppSid = pResultSid;

    return status;
}

VOID
MsRpcFreeSid(
    IN OUT PSID pSid
    )
{
    free(pSid);
}
