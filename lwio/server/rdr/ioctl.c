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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ioctl.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        Device-level ioctl
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include "rdr.h"

static
NTSTATUS
RdrIoctlSetDomainHints(
    PVOID pBuffer,
    ULONG ulLength
    );

static
NTSTATUS
RdrIoctl1GetPhysicalPath(
    PRDR_CCB pFile,
    PIRP pIrp,
    PBYTE pOut,
    ULONG OutLength
    );

static
NTSTATUS
RdrIoctl2GetPhysicalPath(
    PRDR_CCB2 pFile,
    PIRP pIrp,
    PBYTE pOut,
    ULONG OutLength
    );

NTSTATUS
RdrCreateRoot(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo =
        IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);
    PRDR_ROOT_CCB pFile = NULL;

    status = LW_RTL_ALLOCATE_AUTO(&pFile);
    BAIL_ON_NT_STATUS(status);

    /* FIXME: use a real security descriptor and access check */
    pFile->bIsLocalSystem = pProcessInfo->Uid == 0;

    status = IoFileSetContext(pIrp->FileHandle, pFile);
    BAIL_ON_NT_STATUS(status);

cleanup:

    pIrp->IoStatusBlock.Status = status;

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrCloseRoot(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_ROOT_CCB pFile = IoFileGetContext(pIrp->FileHandle);

    pIrp->IoStatusBlock.Status = status;

    RTL_FREE(&pFile);

    return status;
}

NTSTATUS
RdrIoctlRoot(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_ROOT_CCB pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.IoFsControl.ControlCode)
    {
    case RDR_DEVCTL_SET_DOMAIN_HINTS:
        if (!pFile->bIsLocalSystem)
        {
            status = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(status);
        }

        status = RdrIoctlSetDomainHints(
            pIrp->Args.IoFsControl.InputBuffer,
            pIrp->Args.IoFsControl.InputBufferLength);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    pIrp->IoStatusBlock.Status = status;

    return status;

error:

    goto cleanup;
}

static
VOID
FreePair(
    PLW_HASHMAP_PAIR pPair,
    PVOID pUnused
    )
{
    RTL_FREE(&pPair->pKey);
}

static
NTSTATUS
RdrIoctlSetDomainHints(
    PVOID pBuffer,
    ULONG ulLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszStart = NULL;
    PWSTR pwszEnd = NULL;
    PWSTR pwszLimit = (PWSTR) ((PBYTE) pBuffer + ulLength);
    PWSTR pwszDns = NULL;
    PWSTR pwszDnsRef = NULL;
    PWSTR pwszNetbios = NULL;
    PWSTR pwszColon = NULL;
    PLW_HASHMAP pMap = NULL;
    LW_HASHMAP_PAIR pair = {NULL, NULL};

    status = LwRtlCreateHashMap(
        &pMap,
        LwRtlHashDigestPwstrCaseless,
        LwRtlHashEqualPwstrCaseless,
        NULL);
    BAIL_ON_NT_STATUS(status);

    pwszStart = pwszEnd = (PWSTR) pBuffer;

    for (pwszStart = (PWSTR) pBuffer; pwszStart < pwszLimit; pwszStart = pwszEnd + 1)
    {
        pwszColon = NULL;
        for (pwszEnd = pwszStart; pwszEnd < pwszLimit && *pwszEnd; pwszEnd++)
        {
            if (*pwszEnd == ':')
            {
                pwszColon = pwszEnd;
            }
        }

        if (pwszEnd == pwszLimit && *pwszEnd)
        {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

        if (pwszColon)
        {
            *pwszColon = '\0';
        }

        status = LwRtlWC16StringDuplicate(&pwszDns, pwszStart);
        BAIL_ON_NT_STATUS(status);
        pwszDnsRef = pwszDns;

        status = LwRtlHashMapInsert(pMap, pwszDns, pwszDns, &pair);
        BAIL_ON_NT_STATUS(status);
        FreePair(&pair, NULL);
        pwszDns = NULL;

        if (pwszColon)
        {
            status = LwRtlWC16StringDuplicate(&pwszNetbios, pwszColon + 1);
            BAIL_ON_NT_STATUS(status);

            status = LwRtlHashMapInsert(pMap, pwszNetbios, pwszDnsRef, &pair);
            BAIL_ON_NT_STATUS(status);
            FreePair(&pair, NULL);
            pwszNetbios = NULL;
        }
    }

    RdrSwapDomainHints(&pMap);

cleanup:

    RTL_FREE(&pwszDns);
    RTL_FREE(&pwszNetbios);

    if (pMap)
    {
        LwRtlHashMapClear(pMap, FreePair, NULL);
        LwRtlFreeHashMap(&pMap);
    }

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrIoctl1(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.IoFsControl.ControlCode)
    {
    case RDR_DEVCTL_GET_PHYSICAL_PATH:
        status = RdrIoctl1GetPhysicalPath(
            pFile,
            pIrp,
            pIrp->Args.IoFsControl.OutputBuffer,
            pIrp->Args.IoFsControl.OutputBufferLength);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    pIrp->IoStatusBlock.Status = status;

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrIoctl1GetPhysicalPath(
    PRDR_CCB pFile,
    PIRP pIrp,
    PBYTE pOut,
    ULONG OutLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG PathLen = 0;
    PWSTR pPath = NULL;

    status = LwRtlWC16StringAllocatePrintf(
        &pPath,
        "%ws%ws",
        pFile->pTree->pwszPath + 1,
        pFile->pwszPath);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pPath[LwRtlWC16StringNumChars(pPath) - 1] == '\\')
    {
        pPath[LwRtlWC16StringNumChars(pPath) - 1] = '\0';
    }

    PathLen = LwRtlWC16StringNumChars(pPath) * sizeof(WCHAR);

    if (PathLen > OutLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

#ifdef WORDS_BIGENDIAN
    swab(pPath, pOut, PathLen);
#else
    memcpy(pOut, pPath, PathLen);
#endif

    pIrp->IoStatusBlock.BytesTransferred = PathLen;

cleanup:

    RTL_FREE(&pPath);

    return status;
}

NTSTATUS
RdrIoctl2(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_CCB2 pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.IoFsControl.ControlCode)
    {
    case RDR_DEVCTL_GET_PHYSICAL_PATH:
        status = RdrIoctl2GetPhysicalPath(
            pFile,
            pIrp,
            pIrp->Args.IoFsControl.OutputBuffer,
            pIrp->Args.IoFsControl.OutputBufferLength);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    pIrp->IoStatusBlock.Status = status;

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
RdrIoctl2GetPhysicalPath(
    PRDR_CCB2 pFile,
    PIRP pIrp,
    PBYTE pOut,
    ULONG OutLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG PathLen = 0;
    PWSTR pPath = NULL;

    status = LwRtlWC16StringAllocatePrintf(
        &pPath,
        "%ws%ws",
        pFile->pTree->pwszPath + 1,
        pFile->pwszPath);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pPath[LwRtlWC16StringNumChars(pPath) - 1] == '\\')
    {
        pPath[LwRtlWC16StringNumChars(pPath) - 1] = '\0';
    }

    PathLen = LwRtlWC16StringNumChars(pPath) * sizeof(WCHAR);

    if (PathLen > OutLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

#ifdef WORDS_BIGENDIAN
    swab(pPath, pOut, PathLen);
#else
    memcpy(pOut, pPath, PathLen);
#endif

    pIrp->IoStatusBlock.BytesTransferred = PathLen;

cleanup:

    RTL_FREE(&pPath);

    return status;
}
