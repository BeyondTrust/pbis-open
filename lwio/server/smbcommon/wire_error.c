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
WireMarshallErrorResponse(
    PBYTE                   pParams,
    ULONG                   ulBytesAvailable,
    ULONG                   ulOffset,
    PERROR_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT                 pusParamBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pParams;
    PERROR_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT usParamBytesUsed = 0;

    if (ulBytesAvailable < sizeof(ERROR_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PERROR_RESPONSE_HEADER)pDataCursor;

    pDataCursor += sizeof(ERROR_RESPONSE_HEADER);
    usParamBytesUsed += sizeof(ERROR_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(ERROR_RESPONSE_HEADER);
    // ulOffset += sizeof(ERROR_RESPONSE_HEADER);

    *ppResponseHeader = pResponseHeader;
    *pusParamBytesUsed = usParamBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppResponseHeader = NULL;
    *pusParamBytesUsed = 0;

    goto cleanup;
}
