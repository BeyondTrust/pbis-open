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
 *        setcreds.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        SetCredentialsAttributes client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerSetCredentialsAttributes(
    IN PNTLM_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    IN PSecPkgCred pCred
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if (!phCredential)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(ulAttribute)
    {
    case SECPKG_CRED_ATTR_DOMAIN_NAME:
        dwError = NtlmServerSetCredDomainNameAttribute(
            *phCredential,
            pCred->pDomainName);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_CRED_ATTR_NAMES:
    case SECPKG_ATTR_SUPPORTED_ALGS:
    case SECPKG_ATTR_CIPHER_STRENGTHS:
    case SECPKG_ATTR_SUPPORTED_PROTOCOLS:
        dwError = LW_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

error:

    return dwError;
}

DWORD
NtlmServerSetCredDomainNameAttribute(
    IN NTLM_CRED_HANDLE hCred,
    IN PSecPkgCred_DomainName pDomainName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CREDENTIALS pNtlmCreds = (PNTLM_CREDENTIALS)hCred;
    BOOLEAN bInLock = FALSE;
    PSTR pName = NULL;

    if (!hCred || !pDomainName || !pDomainName->pName)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    NTLM_LOCK_MUTEX(bInLock, &pNtlmCreds->Mutex);

    dwError = LwAllocateString(
                  pDomainName->pName,
                  &pName);
    BAIL_ON_LSA_ERROR(dwError);

    LW_SAFE_FREE_STRING(pNtlmCreds->pszDomainName);
    pNtlmCreds->pszDomainName = pName;

error:

    NTLM_UNLOCK_MUTEX(bInLock, &pNtlmCreds->Mutex);

    return dwError;
}
