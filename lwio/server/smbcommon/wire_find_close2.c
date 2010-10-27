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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        wire_find_close2.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        COM_FIND_CLOSE2
 *
 * Author: Krishna Ganugapati (krishnag@likewise.com)
 *         Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
WireUnmarshallFindClose2Request(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulOffset,
    PUSHORT     pusSearchId
    )
{
    NTSTATUS ntStatus = 0;
    PFIND_CLOSE2_REQUEST_HEADER pRequestHeader = NULL;

    if (ulBytesAvailable < sizeof(FIND_CLOSE2_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PFIND_CLOSE2_REQUEST_HEADER)pParams;

    *pusSearchId = pRequestHeader->sid;

cleanup:

    return ntStatus;

error:

    *pusSearchId = 0;

    goto cleanup;
}

NTSTATUS
WireMarshallFindClose2Response(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PUSHORT pusBytesUsed,
    PFIND_CLOSE2_RESPONSE_HEADER* ppResponseHeader
    )
{
    NTSTATUS ntStatus = 0;
    PFIND_CLOSE2_RESPONSE_HEADER pResponseHeader = NULL;

    if (ulBytesAvailable < sizeof(FIND_CLOSE2_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PFIND_CLOSE2_RESPONSE_HEADER)pParams;

    pResponseHeader->usByteCount = 0;

    *ppResponseHeader = pResponseHeader;
    *pusBytesUsed = sizeof(FIND_CLOSE2_RESPONSE_HEADER);

cleanup:

    return ntStatus;

error:

    *ppResponseHeader = NULL;
    *pusBytesUsed = 0;

    goto cleanup;
}
