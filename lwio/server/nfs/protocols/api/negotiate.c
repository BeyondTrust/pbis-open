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

static
NTSTATUS
NfsBuildNegotiateResponseForDialect(
    PLWIO_NFS_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSTR*                ppszDialectArray,
    ULONG                ulNumDialects,
    PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
NfsProcessNegotiate(
    IN  PLWIO_NFS_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PSTR  pszDialectArray[128];
    ULONG ulNumDialects = 128;
    ULONG ulOffset = 0;

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = UnmarshallNegotiateRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    (uint8_t**)&pszDialectArray,
                    &ulNumDialects);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsBuildNegotiateResponseForDialect(
                    pConnection,
                    pSmbRequest,
                    pszDialectArray,
                    ulNumDialects,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    NfsConnectionSetState(pConnection, LWIO_NFS_CONN_STATE_NEGOTIATE);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
NfsBuildNegotiateResponseForDialect(
    PLWIO_NFS_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSTR*                ppszDialectArray,
    ULONG                ulNumDialects,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    ULONG iDialect = 0;
    BOOLEAN bSupportSMBV2 = FALSE;
    PSMB_PACKET pSmbResponse = NULL;

    bSupportSMBV2 = NfsProtocolConfigIsSmb2Enabled();
    if (bSupportSMBV2)
    {
        for (iDialect = 0; iDialect < ulNumDialects; iDialect++)
        {
            if (!strcmp(ppszDialectArray[iDialect],
                        NFS_NEGOTIATE_DIALECT_SMB_2))
            {
                ntStatus = NfsBuildNegotiateResponse_SMB_V2(
                                pConnection,
                                pSmbRequest,
                                &pSmbResponse);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = NfsConnectionSetProtocolVersion(
                                pConnection,
                                SMB_PROTOCOL_VERSION_2);
                BAIL_ON_NT_STATUS(ntStatus);

                goto done;
            }
        }
    }

    for (iDialect = 0; iDialect < ulNumDialects; iDialect++)
    {
        if (!strcmp(ppszDialectArray[iDialect],
                    NFS_NEGOTIATE_DIALECT_NTLM_0_12))
        {
            ntStatus = NfsBuildNegotiateResponse_SMB_V1_NTLM_0_12(
                                pConnection,
                                pSmbRequest,
                                iDialect,
                                &pSmbResponse);
            BAIL_ON_NT_STATUS(ntStatus);

            goto done;
        }
    }

    ntStatus = NfsBuildNegotiateResponse_SMB_V1_Invalid(
                        pConnection,
                        pSmbRequest,
                        &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

done:

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}


