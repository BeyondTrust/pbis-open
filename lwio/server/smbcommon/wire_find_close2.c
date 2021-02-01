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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        wire_find_close2.c
 *
 * Abstract:
 *
 *        BeyondTrust SMB Subsystem (LWIO)
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
