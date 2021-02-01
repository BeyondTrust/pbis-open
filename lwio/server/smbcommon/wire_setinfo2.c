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
 *        wire_setinfo2.c
 *
 * Abstract:
 *
 *        BeyondTrust SMB Subsystem (LWIO)
 *
 *        SET_INFORMATION2 marshalling
 *
 * Author: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
WireUnmarshalSetInfo2Request(
    PBYTE                  pBuffer,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PSET_INFO2_REQUEST_HEADER* ppRequestHeader
    )
{
    NTSTATUS              ntStatus       = STATUS_SUCCESS;
    PSET_INFO2_REQUEST_HEADER pRequestHeader = NULL;
    PBYTE                 pDataCursor    = pBuffer;

    if (ulBytesAvailable < sizeof(SET_INFO2_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSET_INFO2_REQUEST_HEADER)pDataCursor;

    if (pRequestHeader->usByteCount != 0)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRequestHeader = pRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
WireUnmarshalQueryInfo2Request(
    PBYTE                        pBuffer,
    ULONG                        ulBytesAvailable,
    ULONG                        ulOffset,
    PQUERY_INFO2_REQUEST_HEADER* ppRequestHeader
    )
{
    NTSTATUS                    ntStatus       = STATUS_SUCCESS;
    PQUERY_INFO2_REQUEST_HEADER pRequestHeader = NULL;
    PBYTE                       pDataCursor    = pBuffer;

    if (ulBytesAvailable < sizeof(QUERY_INFO2_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PQUERY_INFO2_REQUEST_HEADER)pDataCursor;

    if (pRequestHeader->usByteCount != 0)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRequestHeader = pRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}
