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
WireUnmarshallWriteRequest(
    const PBYTE            pParams,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PWRITE_REQUEST_HEADER* ppHeader,
    PBYTE*                 ppData
    )
{
    NTSTATUS ntStatus = 0;
    PWRITE_REQUEST_HEADER pHeader = NULL;
    PBYTE pDataCursor = pParams;
    PBYTE pData = NULL;

    if (ulBytesAvailable < sizeof(WRITE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PWRITE_REQUEST_HEADER)pDataCursor;

    ulBytesAvailable -= sizeof(WRITE_REQUEST_HEADER);
    pDataCursor += sizeof(WRITE_REQUEST_HEADER);

    if (pHeader->dataLength)
    {
        if (ulBytesAvailable < pHeader->dataLength)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pData = pDataCursor;
    }

    *ppHeader = pHeader;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppData = NULL;

    goto cleanup;
}
