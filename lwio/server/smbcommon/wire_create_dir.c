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
WireUnmarshallCreateDirectoryRequest(
    PBYTE                       pParams,
    ULONG                       ulBytesAvailable,
    ULONG                       ulOffset,
    PSMB_CREATE_DIRECTORY_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                      ppwszPath
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_CREATE_DIRECTORY_REQUEST_HEADER pRequestHeader = NULL;
    PWSTR  pwszPath = NULL;
    PBYTE  pDataCursor = pParams;
    USHORT usByteCountAvailable = 0;
    UCHAR  ucBufferFormat = 0;

    if (ulBytesAvailable < sizeof(SMB_CREATE_DIRECTORY_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB_CREATE_DIRECTORY_REQUEST_HEADER)pDataCursor;

    pDataCursor += sizeof(SMB_CREATE_DIRECTORY_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB_CREATE_DIRECTORY_REQUEST_HEADER);
    ulOffset += sizeof(SMB_CREATE_DIRECTORY_REQUEST_HEADER);

    usByteCountAvailable = pRequestHeader->ByteCount;

    if (ulBytesAvailable < usByteCountAvailable)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulBytesAvailable < sizeof(ucBufferFormat))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ucBufferFormat = *pDataCursor;

    pDataCursor += sizeof(ucBufferFormat);
    ulBytesAvailable -= sizeof(ucBufferFormat);
    ulOffset += sizeof(ucBufferFormat);
    usByteCountAvailable -= sizeof(ucBufferFormat);

    if (ucBufferFormat != SMB_BUFFER_FORMAT_ASCII)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulOffset % 2)
    {
        USHORT usAlignment = ulOffset % 2;

        if (ulBytesAvailable < usAlignment)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset += usAlignment;
        pDataCursor += usAlignment;
        ulBytesAvailable -= usAlignment;
        usByteCountAvailable -= usAlignment;
    }

    if (ulBytesAvailable && usByteCountAvailable)
    {
        PWSTR pwszCursor = NULL;

        pwszPath = pwszCursor = (PWSTR)pDataCursor;
        usByteCountAvailable -= sizeof(wchar16_t);

        while (usByteCountAvailable && *pwszCursor)
        {
            pwszCursor++;
            usByteCountAvailable -= sizeof(wchar16_t);
        }

        // Ensure null termination
        if (*pwszCursor)
        {
            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppRequestHeader = pRequestHeader;
    *ppwszPath = pwszPath;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppwszPath = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallCreateDirectoryResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_CREATE_DIRECTORY_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_CREATE_DIRECTORY_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT   usPackageBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB_CREATE_DIRECTORY_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB_CREATE_DIRECTORY_RESPONSE_HEADER)pParams;
    usPackageBytesUsed += sizeof(SMB_CREATE_DIRECTORY_RESPONSE_HEADER);

    pResponseHeader->usByteCount = 0;

    *ppResponseHeader = pResponseHeader;
    *pusPackageBytesUsed = usPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppResponseHeader = NULL;
    *pusPackageBytesUsed = 0;

    goto cleanup;
}
