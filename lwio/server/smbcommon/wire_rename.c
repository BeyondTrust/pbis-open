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
SrvUnmarshallFileName(
    PBYTE  pData,
    ULONG  ulBytesAvailable,
    ULONG  ulOffset,
    PWSTR* ppwszName,
    PULONG pulBytesUsed
    );

NTSTATUS
WireUnmarshallRenameRequest(
    PBYTE                       pParams,
    ULONG                       ulBytesAvailable,
    ULONG                       ulOffset,
    PSMB_RENAME_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                      ppwszOldName,
    PWSTR*                      ppwszNewName
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RENAME_REQUEST_HEADER pRequestHeader = NULL;
    PBYTE    pDataCursor = pParams;
    ULONG    ulBytesUsed = 0;
    PWSTR    pwszOldName = NULL;
    PWSTR    pwszNewName = NULL;

    if (ulBytesAvailable < sizeof(SMB_RENAME_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB_RENAME_REQUEST_HEADER)pDataCursor;

    ulBytesAvailable -= sizeof(SMB_RENAME_REQUEST_HEADER);
    pDataCursor += sizeof(SMB_RENAME_REQUEST_HEADER);
    ulOffset += sizeof(SMB_RENAME_REQUEST_HEADER);

    ntStatus = SrvUnmarshallFileName(
                    pDataCursor,
                    ulBytesAvailable,
                    ulOffset,
                    &pwszOldName,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulOffset += ulBytesUsed;

    ntStatus = SrvUnmarshallFileName(
                    pDataCursor,
                    ulBytesAvailable,
                    ulOffset,
                    &pwszNewName,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulOffset += ulBytesUsed;

    *ppRequestHeader = pRequestHeader;
    *ppwszOldName = pwszOldName;
    *ppwszNewName = pwszNewName;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppwszOldName = NULL;
    *ppwszNewName = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallRenameResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_RENAME_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RENAME_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT   usPackageBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB_RENAME_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB_RENAME_RESPONSE_HEADER)pParams;
    usPackageBytesUsed += sizeof(SMB_RENAME_RESPONSE_HEADER);

    pResponseHeader->usByteCount = usPackageBytesUsed;

    *ppResponseHeader = pResponseHeader;
    *pusPackageBytesUsed = usPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppResponseHeader = NULL;
    *pusPackageBytesUsed = 0;

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallFileName(
    PBYTE  pData,
    ULONG  ulBytesAvailable,
    ULONG  ulOffset,
    PWSTR* ppwszName,
    PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pDataCursor = pData;
    ULONG    ulBytesUsed = 0;
    PWSTR    pwszName = NULL;
    PWSTR    pwszNameCursor = NULL;
    UCHAR    ucBufferFormat = 0;
    USHORT   usAlignment = 0;

    if (ulBytesAvailable < sizeof(ucBufferFormat))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ucBufferFormat = *pDataCursor;

    if (ucBufferFormat != 0x4)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable -= sizeof(ucBufferFormat);
    pDataCursor += sizeof(ucBufferFormat);
    ulOffset += sizeof(ucBufferFormat);
    ulBytesUsed += sizeof(ucBufferFormat);

    usAlignment = ulOffset %2;
    if (ulBytesAvailable < usAlignment)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable -= usAlignment;
    pDataCursor += usAlignment;
    ulOffset += usAlignment;
    ulBytesUsed += usAlignment;

    if (!ulBytesAvailable)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    do
    {
        if (ulBytesAvailable < sizeof(wchar16_t))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!pwszName)
        {
            pwszName = pwszNameCursor = (PWSTR)pDataCursor;
        }
        else
        {
            pwszNameCursor++;
        }

        ulBytesAvailable -= sizeof(wchar16_t);
        pDataCursor += sizeof(wchar16_t);
        ulOffset += sizeof(wchar16_t);
        ulBytesUsed += sizeof(wchar16_t);

    } while(ulBytesAvailable && pwszNameCursor && *pwszNameCursor);

    if (!pwszNameCursor || *pwszNameCursor)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszName = pwszName;
    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppwszName = NULL;
    *pulBytesUsed = 0;

    goto cleanup;
}
