/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
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

