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
 *        credentials.c
 *
 * Abstract:
 *        NTLM credential wrapper
 *
 * Authors:
 *
 */

#include "ntlmsrvapi.h"

/******************************************************************************/
VOID
NtlmReleaseCredential(
    IN PNTLM_CRED_HANDLE phCred
    )
{
    PNTLM_CREDENTIALS pCreds = NULL;

    if (phCred && *phCred)
    {
        pCreds = *phCred;

        pCreds->nRefCount--;

        LW_ASSERT(pCreds->nRefCount >= 0);

        if (!(pCreds->nRefCount))
        {
            NtlmFreeCredential(pCreds);
        }

        *phCred = NULL;
    }
}

/******************************************************************************/
DWORD
NtlmCreateCredential(
    IN PLSA_CRED_HANDLE pLsaCredHandle,
    IN DWORD dwDirection,
    OUT PNTLM_CREDENTIALS* ppNtlmCreds
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CREDENTIALS pCreds = NULL;

    if (!ppNtlmCreds)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppNtlmCreds = NULL;

    dwError = LwAllocateMemory(sizeof(*pCreds), OUT_PPVOID(&pCreds));
    BAIL_ON_LSA_ERROR(dwError);

    // A reference is not needed since the way we get the credential that's
    // passed in is already sufficient referenced (we either got it through uid
    // lookup OR we created our own that didn't go into the tree anyway.
    pCreds->CredHandle = *pLsaCredHandle;
    pCreds->nRefCount = 1;
    pCreds->dwCredDirection = dwDirection;

    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pCreds->Mutex, NULL));
    BAIL_ON_LSA_ERROR(dwError);
    pCreds->pMutex = &pCreds->Mutex;

cleanup:
    *ppNtlmCreds = pCreds;
    return dwError;
error:
    if (pCreds)
    {
        LsaReleaseCredential(&pCreds->CredHandle);
    }

    LW_SAFE_FREE_MEMORY(pCreds);
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmGetCredentialInfo(
    IN NTLM_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR* pszUserName,
    OUT OPTIONAL PCSTR* pszPassword,
    OUT OPTIONAL uid_t* pUid
    )
{
    if (CredHandle)
    {
        PNTLM_CREDENTIALS pCred = CredHandle;

        if(pCred->CredHandle)
        {
            LsaGetCredentialInfo(
                pCred->CredHandle,
                pszUserName,
                pszPassword,
                pUid);
        }
        else
        {
            if(pszUserName)
            {
                *pszUserName = NULL;
            }
            if(pszPassword)
            {
                *pszPassword = NULL;
            }
            if(pUid)
            {
                *pUid = 0;
            }
        }
    }
}

/******************************************************************************/
VOID
NtlmReferenceCredential(
    IN NTLM_CRED_HANDLE hCredential
    )
{
    PNTLM_CREDENTIALS pCred = NULL;

    if (hCredential)
    {
        pCred = hCredential;

        pCred->nRefCount++;
    }
}
/******************************************************************************/
VOID
NtlmFreeCredential(
    IN PNTLM_CREDENTIALS pCreds
    )
{
    LsaReleaseCredential(&pCreds->CredHandle);
    LW_SAFE_FREE_STRING(pCreds->pszDomainName);
    if (pCreds->pMutex)
    {
        pthread_mutex_destroy(pCreds->pMutex);
    }
    LW_SAFE_FREE_MEMORY(pCreds);
}

