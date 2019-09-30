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

static
NTSTATUS
WireUnmarshallEchoData(
    const PBYTE pBuffer,
    ULONG       ulBufferLen,
    PBYTE*      ppEchoBlob,
    USHORT      usEchoBlobLength
    );

NTSTATUS
WireUnmarshallEchoRequest(
    const PBYTE           pBuffer,
    ULONG                 ulBufferLen,
    PECHO_REQUEST_HEADER* ppHeader,
    PBYTE*                ppEchoBlob
    )
{
    NTSTATUS ntStatus = 0;
    ULONG    ulBufferUsed = 0;
    PECHO_REQUEST_HEADER pEchoHeader = NULL;
    PBYTE    pEchoBlob = NULL;

    /* NOTE: The buffer format cannot be trusted! */
    ulBufferUsed = sizeof(ECHO_REQUEST_HEADER);

    if (ulBufferLen < ulBufferUsed)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* @todo: endian swap as appropriate */
    pEchoHeader = (PECHO_REQUEST_HEADER) pBuffer;

    if (pEchoHeader->byteCount)
    {
        ntStatus = WireUnmarshallEchoData(
                        pBuffer + ulBufferUsed,
                        ulBufferLen - ulBufferUsed,
                        &pEchoBlob,
                        pEchoHeader->byteCount);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppEchoBlob = pEchoBlob;
    *ppHeader = pEchoHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppEchoBlob = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallEchoResponseData(
    const PBYTE pBuffer,
    ULONG       ulBufferLen,
    PBYTE       pEchoBlob,
    USHORT      usEchoBlobLength,
    PUSHORT     pusPackageByteCount
    )
{
    NTSTATUS ntStatus = 0;

    if (!pEchoBlob)
    {
        ntStatus = STATUS_INVALID_PARAMETER_4;
    }
    if (usEchoBlobLength < 4)
    {
        ntStatus = STATUS_INVALID_PARAMETER_5;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulBufferLen < usEchoBlobLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pBuffer, pEchoBlob, usEchoBlobLength);

    *pusPackageByteCount = usEchoBlobLength;

cleanup:

    return ntStatus;

error:

    *pusPackageByteCount = 0;

    goto cleanup;
}

static
NTSTATUS
WireUnmarshallEchoData(
    const PBYTE pBuffer,
    ULONG       ulBufferLen,
    PBYTE*      ppEchoBlob,
    USHORT      usEchoBlobLength
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pEchoBlob = NULL;

    if (usEchoBlobLength > ulBufferLen)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (usEchoBlobLength)
    {
        pEchoBlob = (PBYTE) pBuffer;
    }

    *ppEchoBlob = pEchoBlob;

cleanup:

    return ntStatus;

error:

    *ppEchoBlob = NULL;

    goto cleanup;
}
