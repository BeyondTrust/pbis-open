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
WireUnmarshallFlushRequest(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulOffset,
    PFLUSH_REQUEST_HEADER* ppRequestHeader
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pParams;
    PFLUSH_REQUEST_HEADER pRequestHeader = NULL;

    if (ulBytesAvailable < sizeof(FLUSH_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PFLUSH_REQUEST_HEADER)pDataCursor;

    // pDataCursor += sizeof(FLUSH_REQUEST_HEADER);
    // ulOffset += sizeof(FLUSH_REQUEST_HEADER);

    *ppRequestHeader = pRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallFlushResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PFLUSH_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PFLUSH_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT   usPackageBytesUsed = 0;

    if (ulBytesAvailable < sizeof(FLUSH_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PFLUSH_RESPONSE_HEADER)pParams;
    usPackageBytesUsed += sizeof(FLUSH_RESPONSE_HEADER);

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
