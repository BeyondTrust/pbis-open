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
 *        querycreds.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
