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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        setcreds.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
