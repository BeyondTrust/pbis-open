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
SrvUnmarshallNtFileName(
    PBYTE  pData,
    ULONG  ulBytesAvailable,
    ULONG  ulOffset,
    PWSTR* ppwszName,
    PULONG pulBytesUsed
    );

NTSTATUS
WireUnmarshallNtRenameRequest(
    PBYTE                          pParams,
    ULONG                          ulBytesAvailable,
    ULONG                          ulOffset,
    PSMB_NT_RENAME_REQUEST_HEADER* ppRequestHeader,
    PWSTR*                         ppwszOldName,
    PWSTR*                         ppwszNewName
    )
{
    NTSTATUS                      ntStatus = 0;
    PSMB_NT_RENAME_REQUEST_HEADER pRequestHeader = NULL;
    PBYTE    pDataCursor = pParams;
    ULONG    ulBytesUsed = 0;
    PWSTR    pwszOldName = NULL;
    PWSTR    pwszNewName = NULL;

    if (ulBytesAvailable < sizeof(SMB_NT_RENAME_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB_NT_RENAME_REQUEST_HEADER)pDataCursor;

    ulBytesAvailable -= sizeof(SMB_NT_RENAME_REQUEST_HEADER);
    pDataCursor      += sizeof(SMB_NT_RENAME_REQUEST_HEADER);
    ulOffset         += sizeof(SMB_NT_RENAME_REQUEST_HEADER);

    if ((pRequestHeader->usByteCount < 4) ||
        (ulBytesAvailable < pRequestHeader->usByteCount))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvUnmarshallNtFileName(
                    pDataCursor,
                    ulBytesAvailable,
                    ulOffset,
                    &pwszOldName,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor      += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulOffset         += ulBytesUsed;

    ntStatus = SrvUnmarshallNtFileName(
                    pDataCursor,
                    ulBytesAvailable,
                    ulOffset,
                    &pwszNewName,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor      += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulOffset         += ulBytesUsed;

    *ppRequestHeader = pRequestHeader;
    *ppwszOldName    = pwszOldName;
    *ppwszNewName    = pwszNewName;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppwszOldName    = NULL;
    *ppwszNewName    = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallNtRenameResponse(
    PBYTE                           pParams,
    ULONG                           ulBytesAvailable,
    ULONG                           ulOffset,
    PSMB_NT_RENAME_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT                         pusPackageBytesUsed
    )
{
    NTSTATUS                       ntStatus = 0;
    PSMB_NT_RENAME_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT                         usPackageBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB_NT_RENAME_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB_NT_RENAME_RESPONSE_HEADER)pParams;
    usPackageBytesUsed += sizeof(SMB_NT_RENAME_RESPONSE_HEADER);

    pResponseHeader->usByteCount = usPackageBytesUsed;
    pResponseHeader->ucBuffer[0] = 0;

    *ppResponseHeader = pResponseHeader;
    *pusPackageBytesUsed = usPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppResponseHeader = NULL;
    *pusPackageBytesUsed = 0;

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallNtFileName(
    PBYTE  pData,
    ULONG  ulBytesAvailable,
    ULONG  ulOffset,
    PWSTR* ppwszName,
    PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pDataCursor = pData;
    ULONG    ulBytesUsed = 0;
    PWSTR    pwszName = NULL;
    PWSTR    pwszNameCursor = NULL;
    UCHAR    ucBufferFormat = 0;
    USHORT   usAlignment = 0;

    if (ulBytesAvailable < sizeof(ucBufferFormat))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ucBufferFormat = *pDataCursor;

    if (ucBufferFormat != 0x4)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable -= sizeof(ucBufferFormat);
    pDataCursor += sizeof(ucBufferFormat);
    ulOffset += sizeof(ucBufferFormat);
    ulBytesUsed += sizeof(ucBufferFormat);

    usAlignment = ulOffset %2;
    if (ulBytesAvailable < usAlignment)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable -= usAlignment;
    pDataCursor += usAlignment;
    ulOffset += usAlignment;
    ulBytesUsed += usAlignment;

    if (!ulBytesAvailable)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    do
    {
        if (ulBytesAvailable < sizeof(wchar16_t))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!pwszName)
        {
            pwszName = pwszNameCursor = (PWSTR)pDataCursor;
        }
        else
        {
            pwszNameCursor++;
        }

        ulBytesAvailable -= sizeof(wchar16_t);
        pDataCursor += sizeof(wchar16_t);
        ulOffset += sizeof(wchar16_t);
        ulBytesUsed += sizeof(wchar16_t);

    } while(ulBytesAvailable && pwszNameCursor && *pwszNameCursor);

    if (!pwszNameCursor || *pwszNameCursor)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszName = pwszName;
    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppwszName = NULL;
    *pulBytesUsed = 0;

    goto cleanup;
}
