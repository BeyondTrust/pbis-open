/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        verifysign.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        VerifySignature client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerVerifySignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN const SecBufferDesc* pMessage,
    IN DWORD MessageSeqNo,
    OUT PDWORD pQop
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = *phContext;
    // The following pointers point into pMessage and will not be freed
    const SecBuffer* pToken = NULL;

    NtlmGetSecBuffers(
            (PSecBufferDesc)pMessage,
            (PSecBuffer *)&pToken,
            NULL);

    // Do a full sanity check here
    if (!pToken ||
        pToken->cbBuffer != NTLM_SIGNATURE_SIZE ||
        !pToken->pvBuffer)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = NtlmVerifySignature(
        pContext,
        pMessage,
        pToken
        );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmVerifySignature(
    IN PNTLM_CONTEXT pContext,
    IN const SecBufferDesc* pMessage,
    IN const SecBuffer* pToken
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwCrc32 = 0;
    NTLM_SIGNATURE signature;
    DWORD dwIndex = 0;
    BOOLEAN bFoundData = FALSE;
    // Do not free
    SecBuffer* pData = NULL;

    if (pToken->cbBuffer != sizeof(signature))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    memcpy(&signature, pToken->pvBuffer, sizeof(signature));

    if (pContext->NegotiatedFlags & NTLM_FLAG_NTLM2)
    {
        unsigned char tempHmac[EVP_MAX_MD_SIZE];
        HMAC_CTX c;

        if (!(pContext->NegotiatedFlags & NTLM_FLAG_SIGN))
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        HMAC_CTX_init(&c);
        HMAC_Init(
                &c,
                pContext->VerifyKey,
                sizeof(pContext->VerifyKey),
                EVP_md5());

        HMAC_Update(
                &c,
                (PBYTE)&signature.v2.dwMsgSeqNum,
                sizeof(signature.v2.dwMsgSeqNum));

        for (dwIndex = 0 ; dwIndex < pMessage->cBuffers ; dwIndex++)
        {
            pData = &pMessage->pBuffers[dwIndex];

            if ((pData->BufferType & ~SECBUFFER_ATTRMASK) == SECBUFFER_DATA)
            {
                if (!pData->pvBuffer)
                {
                    HMAC_CTX_cleanup(&c);
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                bFoundData = TRUE;

                HMAC_Update(
                        &c,
                        pData->pvBuffer,
                        pData->cbBuffer);
            }
        }

        if (!bFoundData)
        {
            HMAC_CTX_cleanup(&c);
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        HMAC_Final(
                &c,
                tempHmac,
                NULL);

        HMAC_CTX_cleanup(&c);

        // The davenport doc says that the hmac is sealed after being generated
        // with the signing key. In reality that only happens if the key
        // exchange flag is set.
        if (pContext->NegotiatedFlags & NTLM_FLAG_KEY_EXCH)
        {
            RC4(
                pContext->pUnsealKey,
                sizeof(signature.v2.encrypted.hmac),
                signature.v2.encrypted.hmac,
                signature.v2.encrypted.hmac);
        }

        if (memcmp(signature.v2.encrypted.hmac,
                    tempHmac,
                    sizeof(signature.v2.encrypted.hmac)))
        {
            dwError = ERROR_CRC;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        RC4(
            pContext->pUnsealKey,
            sizeof(signature.v1.encrypted),
            (PBYTE)&signature.v1.encrypted,
            (PBYTE)&signature.v1.encrypted);
        signature.v1.encrypted.dwCounterValue = 0;

        if (pContext->NegotiatedFlags & NTLM_FLAG_ALWAYS_SIGN)
        {
            // Use the dummy signature 0x01000000000000000000000000000000
            dwCrc32 = 0;
        }
        else if (pContext->NegotiatedFlags & NTLM_FLAG_SIGN)
        {
            // generate a crc for the message
            dwError = NtlmCrc32(pMessage, &dwCrc32);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (dwCrc32 != signature.v1.encrypted.dwCrc32)
        {
            dwError = ERROR_CRC;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (!pContext->pdwRecvMsgSeq)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (*pContext->pdwRecvMsgSeq != signature.v2.dwMsgSeqNum)
    {
        dwError = ERROR_REQUEST_OUT_OF_SEQUENCE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    (*pContext->pdwRecvMsgSeq)++;

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
NtlmGetSecBuffers(
    PSecBufferDesc pMessage,
    PSecBuffer* ppToken,
    PSecBuffer* ppPadding
    )
{
    DWORD dwIndex = 0;
    PSecBuffer pToken = NULL;
    PSecBuffer pPadding = NULL;

    for (dwIndex = 0; dwIndex < pMessage->cBuffers; dwIndex++)
    {
        if (pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_TOKEN)
        {
            if (!pToken)
            {
                pToken = &pMessage->pBuffers[dwIndex];
            }
        }
        else if (pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_PADDING)
        {
            if (!pPadding)
            {
                pPadding = &pMessage->pBuffers[dwIndex];
            }
        }
    }

    if (ppToken)
    {
        *ppToken = pToken;
    }

    if (ppPadding)
    {
        *ppPadding = pPadding;
    }
}
