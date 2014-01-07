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
 *        initsecctxt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        InitializeSecurityContext client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerInitializeSecurityContext(
    IN OPTIONAL NTLM_CRED_HANDLE hCredential,
    IN OPTIONAL const NTLM_CONTEXT_HANDLE hContext,
    IN OPTIONAL SEC_CHAR* pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL const SecBuffer* pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PNTLM_CONTEXT_HANDLE phNewContext,
    OUT PSecBuffer pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CREDENTIALS pCred = (PNTLM_CREDENTIALS)hCredential;
    PNTLM_CONTEXT pNtlmContext = NULL;
    PSTR pWorkstation = NULL;
    PSTR pDomain = NULL;
    PNTLM_CHALLENGE_MESSAGE pMessage = NULL;
    DWORD dwMessageSize ATTRIBUTE_UNUSED = 0;
    BOOLEAN bInLock = FALSE;

    pOutput->pvBuffer = NULL;

    if (hContext)
    {
        pNtlmContext = hContext;
    }

    if (!pNtlmContext)
    {
        if (pCred)
        {
            NTLM_LOCK_MUTEX(bInLock, &pCred->Mutex);

            dwError = NtlmGetNameInformation(
                    pCred->pszDomainName,
                    &pWorkstation,
                    &pDomain,
                    NULL,
                    NULL);
            BAIL_ON_LSA_ERROR(dwError);

            NTLM_UNLOCK_MUTEX(bInLock, &pCred->Mutex);
        }
        else
        {
            dwError = NtlmGetNameInformation(
                    NULL,
                    &pWorkstation,
                    &pDomain,
                    NULL,
                    NULL);
            BAIL_ON_LSA_ERROR(dwError);
        }

        // If we start with a NULL context, create a negotiate message
        dwError = NtlmCreateNegotiateContext(
            hCredential,
            fContextReq,
            pDomain,
            pWorkstation,
            (PBYTE)&gXpSpoof,  //for now add OS ver info... config later
            &pNtlmContext,
            pOutput);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_WARNING_CONTINUE_NEEDED;
    }
    else
    {
        if (pInput->BufferType != SECBUFFER_TOKEN ||
           pInput->cbBuffer == 0)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        pMessage = pInput->pvBuffer;
        dwMessageSize = pInput->cbBuffer;

        dwError = NtlmCreateResponseContext(
            pMessage,
            hCredential,
            pNtlmContext->bDoAnonymous,
            &pNtlmContext,
            pOutput);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *phNewContext = pNtlmContext;

    if (pfContextAttr)
    {
        NtlmGetContextInfo(
            pNtlmContext,
            NULL,
            pfContextAttr,
            NULL,
            NULL,
            NULL);
     }


cleanup:
    if (pCred)
    {
        NTLM_UNLOCK_MUTEX(bInLock, &pCred->Mutex);
    }

    LW_SAFE_FREE_STRING(pWorkstation);
    LW_SAFE_FREE_STRING(pDomain);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pOutput->pvBuffer);
    pOutput->cbBuffer = 0;
    pOutput->BufferType = 0;

    // If this function has already succeed once, we MUST make sure phNewContext
    // is set so the caller can cleanup whatever context is remaining.  It
    // could be the original negotiate context or a new response context but
    // either way it is vital that the caller get a context they can actually
    // cleanup ONCE they've received ANY context from this function.
    //
    // If hContext is NULL, that indicates this is the first time through this
    // call and we can safely release our context.
    if ( pNtlmContext && !hContext)
    {
        NtlmReleaseContext(&pNtlmContext);
        phNewContext = NULL;
    }

    goto cleanup;
}

DWORD
NtlmCreateNegotiateContext(
    IN NTLM_CRED_HANDLE hCred,
    IN DWORD fContextReq,
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PNTLM_CONTEXT* ppNtlmContext,
    OUT PSecBuffer pOutput
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pNtlmContext = NULL;
    DWORD dwMessageSize = 0;
    PNTLM_NEGOTIATE_MESSAGE_V1 pMessage = NULL;
    NTLM_CONFIG config;
    DWORD dwDefaultOptions =
            // Always do signing and sealing since they cannot be turned off on
            // Windows
            NTLM_FLAG_SIGN                  |
            NTLM_FLAG_SEAL                  |
            NTLM_FLAG_OEM                   |
            NTLM_FLAG_REQUEST_TARGET        |
            NTLM_FLAG_NTLM                  |
            NTLM_FLAG_DOMAIN                |
            0;
    DWORD dwDceStyleOptions =
            NTLM_FLAG_OEM                   |
            NTLM_FLAG_REQUEST_TARGET        |
            NTLM_FLAG_NTLM                  |
            NTLM_FLAG_DOMAIN                |
            0;
    DWORD dwOptions = 0;

    *ppNtlmContext = NULL;

    dwError = NtlmCreateContext(hCred, &pNtlmContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);

    if (fContextReq & ISC_REQ_USE_DCE_STYLE)
    {
        dwOptions = dwDceStyleOptions;
    }
    else
    {
        dwOptions = dwDefaultOptions;
    }

    if (config.bSupportUnicode)
    {
        dwOptions |= NTLM_FLAG_UNICODE;
    }
    if (config.bSupportNTLM2SessionSecurity)
    {
        dwOptions |= NTLM_FLAG_NTLM2;
    }
    if (config.bSupportKeyExchange)
    {
        dwOptions |= NTLM_FLAG_KEY_EXCH;
    }
    if (config.bSupport56bit)
    {
        dwOptions |= NTLM_FLAG_56;
    }
    if (config.bSupport128bit)
    {
        dwOptions |= NTLM_FLAG_128;
    }

    if (fContextReq & ISC_REQ_INTEGRITY)
    {
        dwOptions |= NTLM_FLAG_SIGN;
    }

    if (fContextReq & ISC_REQ_CONFIDENTIALITY)
    {
        dwOptions |= NTLM_FLAG_SEAL;
    }

    if (fContextReq & ISC_REQ_NULL_SESSION)
    {
        pNtlmContext->bDoAnonymous = TRUE;
    }

    dwError = NtlmCreateNegotiateMessage(
        dwOptions,
        pDomain,
        pWorkstation,
        pOsVersion,
        &dwMessageSize,
        &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pOutput->cbBuffer = dwMessageSize;
    pOutput->BufferType = SECBUFFER_TOKEN;
    pOutput->pvBuffer = pMessage;
    pNtlmContext->NtlmState = NtlmStateNegotiate;

cleanup:

    *ppNtlmContext = pNtlmContext;

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pMessage);
    pOutput->cbBuffer = 0;
    pOutput->BufferType = 0;
    pOutput->pvBuffer = NULL;
    if (pNtlmContext)
    {
        NtlmFreeContext(&pNtlmContext);
    }
    goto cleanup;
}

DWORD
NtlmCreateResponseContext(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN NTLM_CRED_HANDLE hCred,
    IN BOOLEAN bDoAnonymous,
    OUT PNTLM_CONTEXT* ppNtlmContext,
    OUT PSecBuffer pOutput
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_RESPONSE_MESSAGE_V1 pMessage = NULL;
    PCSTR pUserNameTemp = NULL;
    PCSTR pPassword = NULL;
    PNTLM_CONTEXT pNtlmContext = NULL;
    PBYTE pMasterKey = NULL;
    BYTE LmUserSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE NtlmUserSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE LanManagerSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE SecondaryKey[NTLM_SESSION_KEY_SIZE] = {0};
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    DWORD dwMessageSize = 0;
    NTLM_CONFIG config;
    DWORD dwNtRespType = 0;
    DWORD dwLmRespType = 0;

    *ppNtlmContext = NULL;

    dwError = NtlmReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);

    if (bDoAnonymous)
    {
        pUserNameTemp = "";
        pPassword = "";
    }
    else
    {
        NtlmGetCredentialInfo(
            hCred,
            &pUserNameTemp,
            &pPassword,
            NULL);

        if (!pUserNameTemp[0] && !pPassword[0])
        {
            bDoAnonymous = TRUE;
        }
    }

    if (bDoAnonymous)
    {
        dwError = LwAllocateMemory(
                        sizeof(*pUserNameInfo),
                        OUT_PPVOID(&pUserNameInfo));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                        "",
                        &pUserNameInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwAllocateString(
                        "",
                        &pUserNameInfo->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaSrvCrackDomainQualifiedName(
                            pUserNameTemp,
                            &pUserNameInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = NtlmCreateContext(hCred, &pNtlmContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                pUserNameTemp,
                &pNtlmContext->pszClientUsername);
    BAIL_ON_LSA_ERROR(dwError);

    if (bDoAnonymous)
    {
        dwNtRespType = NTLM_RESPONSE_TYPE_ANON_NTLM;
        dwLmRespType = NTLM_RESPONSE_TYPE_ANON_LM;
    }
    else if (config.bSendNTLMv2)
    {
        dwNtRespType = NTLM_RESPONSE_TYPE_NTLMv2;
        // TODO: the correct thing is to use LMv2
        dwLmRespType = NTLM_RESPONSE_TYPE_LM;
    }
    else if(LW_LTOH32(pChlngMsg->NtlmFlags) & NTLM_FLAG_NTLM2)
    {
        dwLmRespType = NTLM_RESPONSE_TYPE_NTLM2;
        dwNtRespType = NTLM_RESPONSE_TYPE_NTLM2;
    }
    else
    {
        dwNtRespType = NTLM_RESPONSE_TYPE_NTLM;
        dwLmRespType = NTLM_RESPONSE_TYPE_LM;
    }

    dwError = NtlmCreateResponseMessage(
        pChlngMsg,
        pUserNameInfo->pszName,
        pUserNameInfo->pszDomain,
        pPassword,
        (PBYTE)&gXpSpoof,
        dwNtRespType,
        dwLmRespType,
        &dwMessageSize,
        &pMessage,
        LmUserSessionKey,
        NtlmUserSessionKey
        );
    BAIL_ON_LSA_ERROR(dwError);

    // As a side effect of creating the response, we must also set/produce the
    // master session key...

    pMasterKey = NtlmUserSessionKey;

    if (LW_LTOH32(pChlngMsg->NtlmFlags) & NTLM_FLAG_LM_KEY)
    {
        NtlmGenerateLanManagerSessionKey(
            pMessage,
            LmUserSessionKey,
            LanManagerSessionKey);

        pMasterKey = LanManagerSessionKey;
    }

    if (LW_LTOH32(pChlngMsg->NtlmFlags) & NTLM_FLAG_KEY_EXCH)
    {
        // This is the key we will use for session security...
        dwError = NtlmGetRandomBuffer(
            SecondaryKey,
            NTLM_SESSION_KEY_SIZE);
        BAIL_ON_LSA_ERROR(dwError);

        // Encrypt it with the "master key" set above and send it along with the
        // response
        NtlmStoreSecondaryKey(
            pMasterKey,
            SecondaryKey,
            pMessage);

        pMasterKey = SecondaryKey;
    }

    NtlmWeakenSessionKey(
        pChlngMsg,
        pMasterKey,
        &pNtlmContext->cbSessionKeyLen);

    memcpy(pNtlmContext->SessionKey, pMasterKey, NTLM_SESSION_KEY_SIZE);

    pNtlmContext->NegotiatedFlags = LW_LTOH32(pChlngMsg->NtlmFlags);
    pOutput->cbBuffer = dwMessageSize;
    pOutput->BufferType = SECBUFFER_TOKEN;
    pOutput->pvBuffer = pMessage;
    pNtlmContext->NtlmState = NtlmStateResponse;
    pNtlmContext->bInitiatedSide = TRUE;
    pNtlmContext->bDoAnonymous = bDoAnonymous;

    dwError = NtlmInitializeKeys(pNtlmContext);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pUserNameInfo)
    {
        LsaSrvFreeNameInfo(pUserNameInfo);
    }

    *ppNtlmContext = pNtlmContext;

    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pMessage);
    if (pNtlmContext)
    {
        NtlmFreeContext(&pNtlmContext);
    }
    pOutput->cbBuffer = 0;
    pOutput->BufferType = 0;
    pOutput->pvBuffer = NULL;

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
