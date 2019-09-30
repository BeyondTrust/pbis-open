/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *
 * Module Name:
 *
 *        parse_name.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Functions for parsing a user or group name
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "api.h"

DWORD
LsaSrvCrackDomainQualifiedName(
    PCSTR pszName,
    PLSA_LOGIN_NAME_INFO* ppNameInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pNameInfo = NULL;
    PCSTR pszIndex = NULL;
    int idx = 0;
    PSTR pszNameCopy = NULL;

    BAIL_ON_INVALID_STRING(pszName);

    dwError = LwAllocateMemory(
                    sizeof(LSA_LOGIN_NAME_INFO),
                    (PVOID*)&pNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    pszName,
                    &pszNameCopy);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrCharReplace(pszNameCopy, LsaSrvSpaceReplacement(),' ');

    if ((pszIndex = strchr(pszNameCopy, LsaSrvDomainSeparator())) != NULL) {
        idx = pszIndex-pszNameCopy;
        dwError = LwStrndup(pszNameCopy, idx, &pNameInfo->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);

        if (!LW_IS_NULL_OR_EMPTY_STR(pszNameCopy+idx+1)) {
            dwError = LwAllocateString(pszNameCopy+idx+1, &pNameInfo->pszName);
        }
        else
        {
            dwError = ERROR_INVALID_NAME;
        }
        BAIL_ON_LSA_ERROR(dwError);

        pNameInfo->nameType = NameType_NT4;
    }
    else if ((pszIndex = strchr(pszNameCopy, '@')) != NULL) {
        idx = pszIndex-pszNameCopy;
        dwError = LwStrndup(pszNameCopy, idx, &pNameInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);

        if (!LW_IS_NULL_OR_EMPTY_STR(pszNameCopy+idx+1)) {
            dwError = LwAllocateString(pszNameCopy+idx+1, &pNameInfo->pszDomain);
        }
        else
        {
            dwError = ERROR_INVALID_NAME;
        }
        BAIL_ON_LSA_ERROR(dwError);

        pNameInfo->nameType = NameType_UPN;
    }
    else {
        dwError = LwAllocateString(pszNameCopy, &pNameInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        pNameInfo->nameType = NameType_Alias;
    }

    *ppNameInfo = pNameInfo;

cleanup:
    LW_SAFE_FREE_STRING(pszNameCopy);
    return(dwError);

error:

    *ppNameInfo = NULL;

    if (pNameInfo)
    {
        LsaSrvFreeNameInfo(pNameInfo);
    }

    goto cleanup;
}

void
LsaSrvFreeNameInfo(
    PLSA_LOGIN_NAME_INFO pNameInfo
    )
{
    LW_SAFE_FREE_STRING(pNameInfo->pszDomain);
    LW_SAFE_FREE_STRING(pNameInfo->pszName);
    LwFreeMemory(pNameInfo);
}
