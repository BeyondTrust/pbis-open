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
 *        querycreds.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        QueryCredentialsAttributes client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerQueryCredentialsAttributes(
    IN PNTLM_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecPkgCred pCred = (PSecPkgCred)pBuffer;

    switch(ulAttribute)
    {
    case SECPKG_CRED_ATTR_NAMES:
        dwError = NtlmServerQueryCredNameAttribute(
            phCredential,
            &pCred->pNames);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_SUPPORTED_ALGS:
    case SECPKG_ATTR_CIPHER_STRENGTHS:
    case SECPKG_ATTR_SUPPORTED_PROTOCOLS:
    case SECPKG_CRED_ATTR_DOMAIN_NAME:
        dwError = LW_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmServerQueryCredNameAttribute(
    IN PNTLM_CRED_HANDLE phCred,
    OUT PSecPkgCred_Names *ppNames
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCSTR pUserName = NULL;
    PSecPkgCred_Names pName = NULL;

    *ppNames = NULL;

    dwError = LwAllocateMemory(sizeof(*pName), OUT_PPVOID(&pName));
    BAIL_ON_LSA_ERROR(dwError);

    NtlmGetCredentialInfo(
        *phCred,
        &pUserName,
        NULL,
        NULL);

    // It's possible (in the case of a server), that no name is associated with
    // the credential... handle this case.
    if(!pUserName)
    {
        pUserName = "";
    }

    dwError = LwAllocateString(pUserName, &pName->pUserName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppNames = pName;
    return dwError;
error:
    if(pName)
    {
        LW_SAFE_FREE_STRING(pName->pUserName);
    }
    LW_SAFE_FREE_MEMORY(pName);
    goto cleanup;
}
