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
 *        wire_setinfo.c
 *
 * Abstract:
 *
 *        BeyondTrust SMB Subsystem (LWIO)
 *
 *        SET_INFORMATION marshalling
 *
 * Author: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
WireUnmarshalSetInfoRequest(
    PBYTE                  pBuffer,
    ULONG                  ulBytesAvailable,
    ULONG                  ulOffset,
    PSET_INFO_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                 ppwszPath
    )
{
    NTSTATUS              ntStatus       = STATUS_SUCCESS;
    PSET_INFO_REQUEST_HEADER pRequestHeader = NULL;
    PBYTE                 pDataCursor    = pBuffer;
    PWSTR  pwszPath = NULL;
    USHORT usByteCountAvailable = 0;
    UCHAR  ucBufferFormat = 0;

    if (ulBytesAvailable < sizeof(SET_INFO_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSET_INFO_REQUEST_HEADER)pDataCursor;

    pDataCursor += sizeof(SET_INFO_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SET_INFO_REQUEST_HEADER);
    ulOffset += sizeof(SET_INFO_REQUEST_HEADER);

    usByteCountAvailable = pRequestHeader->usByteCount;

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

    goto cleanup;
}

