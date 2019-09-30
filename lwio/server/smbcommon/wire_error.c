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
