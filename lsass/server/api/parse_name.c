/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
