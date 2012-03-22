/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        encryptmsg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        EncryptMessage client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerEncryptMessage(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN BOOLEAN bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD dwMsgSeqNum
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = *phContext;
    // The following pointers point into pMessage and will not be freed
    PSecBuffer pToken = NULL;
    PSecBuffer pData = NULL;
    PNTLM_SIGNATURE pSignature = NULL;
    DWORD dwIndex = 0;

    // Sanity check to see if we handle sealing
    if (bEncrypt && !(pContext->NegotiatedFlags & NTLM_FLAG_SEAL))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // The message should be in the format of:
    // SECBUFFER_TOKEN      - Where the signature is placed
    // SECBUFFER_DATA       - The data we are signing
    // SECBUFFER_PADDING    - Padding (for RC4 or CRC32?) - ignore padding
    //
    // Find these buffers... the first one found of each type will be the one
    // that is used.
    NtlmGetSecBuffers(pMessage, &pToken, NULL);

    // Do a full sanity check here
    if (!pToken ||
        pToken->cbBuffer != NTLM_SIGNATURE_SIZE ||
        !pToken->pvBuffer)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pSignature = (PNTLM_SIGNATURE)pToken->pvBuffer;

    // Sign the original message before sealing it.
    dwError = NtlmInitializeSignature(
                pContext,
                pMessage,
                pSignature);
    BAIL_ON_LSA_ERROR(dwError);

    // Always encrypt the message to match Windows' behavior
    for (dwIndex = 0 ; dwIndex < pMessage->cBuffers ; dwIndex++)
    {
        pData = &pMessage->pBuffers[dwIndex];

        if (pData->BufferType == SECBUFFER_DATA) 
        {
            if (!pData->pvBuffer)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }

            RC4(
                pContext->pSealKey,
                pData->cbBuffer,
                pData->pvBuffer,
                pData->pvBuffer);
        }
    }

    NtlmFinalizeSignature(pContext, pSignature);

cleanup:
    return dwError;
error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
