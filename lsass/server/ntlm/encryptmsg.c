/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        encryptmsg.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
