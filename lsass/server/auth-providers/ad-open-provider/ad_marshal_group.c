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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        AD LDAP Group Marshalling
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
ADMarshalGetCanonicalName(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_SECURITY_OBJECT   pObject,
    PSTR*                  ppszResult)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR    pszResult = NULL;
    PSTR pszDefaultPrefix = NULL;

    dwError = AD_GetUserDomainPrefix(pState, &pszDefaultPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    if(pObject->type == LSA_OBJECT_TYPE_GROUP &&
       !LW_IS_NULL_OR_EMPTY_STR(pObject->groupInfo.pszAliasName))
    {
        dwError = LwAllocateString(
            pObject->groupInfo.pszAliasName,
            &pszResult);
        BAIL_ON_LSA_ERROR(dwError);
        
        LwStrCharReplace(
            pszResult,
            ' ',
            LsaSrvSpaceReplacement());
    }
    else if(pObject->type == LSA_OBJECT_TYPE_USER &&
            !LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszAliasName))
    {
        dwError = LwAllocateString(
            pObject->userInfo.pszAliasName,
            &pszResult);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrCharReplace(
            pszResult,
            ' ',
            LsaSrvSpaceReplacement());
    }
    else if (AD_ShouldAssumeDefaultDomain(pState) &&
        pObject->enabled &&
        !strcmp(pObject->pszNetbiosDomainName, pszDefaultPrefix))
    {
        dwError = LwAllocateString(
            pObject->pszSamAccountName,
            &pszResult);
        BAIL_ON_LSA_ERROR(dwError);
        
        LwStrCharReplace(
            pszResult,
            ' ',
            LsaSrvSpaceReplacement());

        LwStrToLower(pszResult);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
            &pszResult,
            "%s%c%s",
            pObject->pszNetbiosDomainName,
            LsaSrvDomainSeparator(),
            pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrCharReplace(
            pszResult,
            ' ',
            LsaSrvSpaceReplacement());

        LwStrnToUpper(
            pszResult,
            strlen(pObject->pszNetbiosDomainName));

        LwStrToLower(
            pszResult + strlen(pObject->pszNetbiosDomainName) + 1);
    }

    *ppszResult = pszResult;

cleanup:
    LW_SAFE_FREE_STRING(pszDefaultPrefix);
    return dwError;

error:
    *ppszResult = NULL;
    LW_SAFE_FREE_STRING(pszResult);
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
