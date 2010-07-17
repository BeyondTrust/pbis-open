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
 *        makesign.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        MakeSignature client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerMakeSignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD dwQop,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = *phContext;
    // The following pointers point into pMessage and will not be freed
    PSecBuffer pToken = NULL;
    PNTLM_SIGNATURE pSignature = NULL;

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

    if (pContext->NegotiatedFlags & NTLM_FLAG_ALWAYS_SIGN)
    {
        // Use the dummy signature 0x01000000000000000000000000000000
        pSignature = (PNTLM_SIGNATURE)pToken->pvBuffer;

        pSignature->dwVersion = NTLM_VERSION;
        pSignature->v1.encrypted.dwCounterValue = 0;
        pSignature->v1.encrypted.dwCrc32 = 0;
        pSignature->v1.encrypted.dwMsgSeqNum = 0;
    }
    else if (pContext->NegotiatedFlags & NTLM_FLAG_SIGN)
    {
        dwError = NtlmInitializeSignature(
                    pContext,
                    pMessage,
                    pSignature);
        BAIL_ON_LSA_ERROR(dwError);

        NtlmFinalizeSignature(pContext, pSignature);
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmInitializeSignature(
    PNTLM_CONTEXT pContext,
    const PSecBufferDesc pMessage,
    PNTLM_SIGNATURE pSignature
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    // Do not free
    SecBuffer *pData = NULL;
    BOOLEAN bFoundData = FALSE;

    if (!pContext->pdwSendMsgSeq)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pSignature->dwVersion = 1;
    pSignature->v2.dwMsgSeqNum = *pContext->pdwSendMsgSeq;
    (*pContext->pdwSendMsgSeq)++;

    if (pContext->NegotiatedFlags & NTLM_FLAG_NTLM2)
    {
        unsigned char tempHmac[EVP_MAX_MD_SIZE];
        HMAC_CTX c;

        HMAC_CTX_init(&c);
        HMAC_Init(
                &c,
                pContext->SignKey,
                sizeof(pContext->SignKey),
                EVP_md5());

        HMAC_Update(
                &c,
                (PBYTE)&pSignature->v2.dwMsgSeqNum,
                sizeof(pSignature->v2.dwMsgSeqNum));

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

        // Copy only the first part of the hmac
        memcpy(pSignature->v2.encrypted.hmac,
                tempHmac,
                sizeof(pSignature->v2.encrypted.hmac));
    }
    else
    {
        dwError = NtlmCrc32(
                pMessage,
                &pSignature->v1.encrypted.dwCrc32);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
NtlmFinalizeSignature(
    PNTLM_CONTEXT pContext,
    PNTLM_SIGNATURE pSignature
    )
{
    if (pContext->NegotiatedFlags & NTLM_FLAG_NTLM2)
    {
        // The davenport doc says that the hmac is sealed after being generated
        // with the signing key. In reality that only happens if the key
        // exchange flag is set.
        if (pContext->NegotiatedFlags & NTLM_FLAG_KEY_EXCH)
        {
            RC4(
                pContext->pSealKey,
                sizeof(pSignature->v2.encrypted),
                (PBYTE)&pSignature->v2.encrypted,
                (PBYTE)&pSignature->v2.encrypted);
        }
    }
    else
    {
        RC4(
            pContext->pSealKey,
            sizeof(pSignature->v1.encrypted),
            (PBYTE)&pSignature->v1.encrypted,
            (PBYTE)&pSignature->v1.encrypted);

        pSignature->v1.encrypted.dwCounterValue = NTLM_COUNTER_VALUE;
    }
}
