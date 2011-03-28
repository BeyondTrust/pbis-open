/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        privilege.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Privileges API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
LsaSrvPrivsLookupPrivilegeValue(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PCWSTR pwszPrivilegeName,
    OUT PLUID pPrivilegeValue
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PACCESS_TOKEN accessToken = AccessToken;
    BOOLEAN releaseAccessToken = FALSE;
    ACCESS_MASK accessRights = LSA_ACCESS_VIEW_POLICY_INFO;
    ACCESS_MASK grantedAccess = 0;
    GENERIC_MAPPING genericMapping = {0};
    PSTR pszPrivilegeName = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);

        releaseAccessToken = TRUE;
    }

    if (!RtlAccessCheck(pGlobals->pPrivilegesSecDesc,
                        accessToken,
                        accessRights,
                        0,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }
                       
    err = LwWc16sToMbs(pwszPrivilegeName,
                       &pszPrivilegeName);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvGetPrivilegeEntryByName(
                       pszPrivilegeName,
                       &pPrivilegeEntry);
    BAIL_ON_LSA_ERROR(err);

    *pPrivilegeValue = pPrivilegeEntry->Luid;

error:
    if (err || ntStatus)
    {
        if (pPrivilegeValue)
        {
            pPrivilegeValue->HighPart = 0;
            pPrivilegeValue->LowPart = 0;
        }
    }

    LW_SAFE_FREE_MEMORY(pszPrivilegeName);

    if (releaseAccessToken)
    {
        RtlReleaseAccessToken(&accessToken);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
LsaSrvPrivsLookupPrivilegeName(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLUID pPrivilegeValue,
    OUT PWSTR *pPrivilegeName
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PACCESS_TOKEN accessToken = AccessToken;
    BOOLEAN releaseAccessToken = FALSE;
    ACCESS_MASK accessRights = LSA_ACCESS_VIEW_POLICY_INFO;
    ACCESS_MASK grantedAccess = 0;
    GENERIC_MAPPING genericMapping = {0};
    PWSTR privilegeName = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);

        releaseAccessToken = TRUE;
    }

    if (!RtlAccessCheck(pGlobals->pPrivilegesSecDesc,
                        accessToken,
                        accessRights,
                        0,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    err = LsaSrvGetPrivilegeEntryByValue(
                       pPrivilegeValue,
                       &pPrivilegeEntry);
    BAIL_ON_LSA_ERROR(err);

    err = LwMbsToWc16s(pPrivilegeEntry->pszName,
                       &privilegeName);
    BAIL_ON_LSA_ERROR(err);

    *pPrivilegeName = privilegeName;

error:
    if (err)
    {
        LW_SAFE_FREE_MEMORY(privilegeName);

        if (pPrivilegeName)
        {
            pPrivilegeName = NULL;
        }
    }

    if (releaseAccessToken)
    {
        RtlReleaseAccessToken(&accessToken);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
LsaSrvPrivsLookupPrivilegeDescription(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PCWSTR PrivilegeName,
    IN SHORT ClientLanguageId,
    IN SHORT ClientSystemLanguageId,
    OUT PWSTR *pPrivilegeDescription,
    OUT PUSHORT pLanguageId
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PACCESS_TOKEN accessToken = AccessToken;
    BOOLEAN releaseAccessToken = FALSE;
    ACCESS_MASK accessRights = LSA_ACCESS_VIEW_POLICY_INFO;
    ACCESS_MASK grantedAccess = 0;
    GENERIC_MAPPING genericMapping = {0};
    PSTR pszPrivilegeName = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;
    PWSTR privilegeDescription = NULL;
    USHORT descriptionLanguageId = 0;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);

        releaseAccessToken = TRUE;
    }

    if (!RtlAccessCheck(pGlobals->pPrivilegesSecDesc,
                        accessToken,
                        accessRights,
                        0,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ClientLanguageId || ClientSystemLanguageId)
    {
        //
        // Right now we don't support multilingual privilege descriptions
        // so just return 0x0409 (which means "en-US") whenever a caller
        // requests some particular language id
        //
        descriptionLanguageId = 0x0409;
    }

    err = LwWc16sToMbs(PrivilegeName,
                       &pszPrivilegeName);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvGetPrivilegeEntryByName(
                       pszPrivilegeName,
                       &pPrivilegeEntry);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateWc16String(
                       &privilegeDescription,
                       pPrivilegeEntry->pwszDescription);
    BAIL_ON_LSA_ERROR(err);

    *pPrivilegeDescription = privilegeDescription;
    *pLanguageId           = descriptionLanguageId;

error:
    if (err || ntStatus)
    {
        if (pPrivilegeDescription)
        {
            *pPrivilegeDescription = NULL;
        }

        if (pLanguageId)
        {
            *pLanguageId = 0;
        }
    }

    LW_SAFE_FREE_MEMORY(pszPrivilegeName);

    if (releaseAccessToken)
    {
        RtlReleaseAccessToken(&accessToken);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


BOOLEAN
LsaSrvIsPrivilegeNameValid(
    PCSTR pszPrivilegeName
    )
{
    BOOLEAN Valid = FALSE;
    PSTR ppszPrefixes[] = LSA_PRIVILEGE_VALID_PREFIXES;
    DWORD i = 0;
    PSTR pszPref = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszPrivilegeName))
    {
        for (i = 0;
             !Valid && i < sizeof(ppszPrefixes)/sizeof(ppszPrefixes[0]);
             i++)
        {
            LwStrStr(pszPrivilegeName,
                     ppszPrefixes[i],
                     &pszPref);
            if (pszPrivilegeName == pszPref)
            {
                Valid = TRUE;
            }
        }
    }

    return Valid;
}


BOOLEAN
LsaSrvIsPrivilegeValueValid(
    PLUID pValue
    )
{
    return (pValue->HighPart == 0);
}


DWORD
LsaSrvPrivsEnumPrivileges(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PDWORD pResume,
    IN DWORD PreferredMaxSize,
    OUT PWSTR **ppPrivilegeNames,
    OUT PLUID *ppPrivilegeValues,
    OUT PDWORD pCount
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD enumerationStatus = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    PACCESS_TOKEN accessToken = AccessToken;
    ACCESS_MASK accessRights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                               LSA_ACCESS_VIEW_POLICY_INFO;
    ACCESS_MASK grantedAccess = 0;
    GENERIC_MAPPING genericMapping = {0};
    DWORD resume = 0;
    PLSA_PRIVILEGE *ppPrivileges = NULL;
    DWORD count = 0;
    DWORD i = 0;
    PWSTR *pPrivilegeNames = NULL;
    PLUID pPrivilegeValues = NULL;

    if (!accessToken)
    {
        err = LsaSrvPrivsGetAccessTokenFromServerHandle(
                                hServer,
                                &accessToken);
        BAIL_ON_LSA_ERROR(err);
    }

    if (!RtlAccessCheck(pGlobals->pPrivilegesSecDesc,
                        accessToken,
                        accessRights,
                        0,
                        &genericMapping,
                        &grantedAccess,
                        &ntStatus))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pResume)
    {
        resume = *pResume;
    }

    err = LsaSrvPrivsGetPrivilegeEntries(
                        &resume,
                        PreferredMaxSize,
                        &ppPrivileges,
                        &count);
    if (err == ERROR_MORE_DATA ||
        err == ERROR_NO_MORE_ITEMS)
    {
        enumerationStatus = err;
        err = ERROR_SUCCESS;
    }
    else if (err != ERROR_SUCCESS)
    {
        BAIL_ON_LSA_ERROR(err);
    }

    err = LwAllocateMemory(
                        sizeof(pPrivilegeNames[0]) * count,
                        OUT_PPVOID(&pPrivilegeNames));
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateMemory(
                        sizeof(pPrivilegeValues[0]) * count,
                        OUT_PPVOID(&pPrivilegeValues));
    BAIL_ON_LSA_ERROR(err);

    for (i = 0; i < count; i++)
    {
        err = LwMbsToWc16s(
                        ppPrivileges[i]->pszName,
                        &pPrivilegeNames[i]);
        BAIL_ON_LSA_ERROR(err);

        pPrivilegeValues[i] = ppPrivileges[i]->Luid;
    }

    if (pResume)
    {
        *pResume = resume;
    }

    *ppPrivilegeNames  = pPrivilegeNames;
    *ppPrivilegeValues = pPrivilegeValues;
    *pCount            = count;

error:
    if (err || ntStatus)
    {
        for (i = 0; i < count; i++)
        {
            LW_SAFE_FREE_MEMORY(ppPrivilegeNames[i]);
        }
        LW_SAFE_FREE_MEMORY(ppPrivilegeNames);
        LW_SAFE_FREE_MEMORY(ppPrivilegeValues);

        if (ppPrivilegeNames)
        {
            *ppPrivilegeNames = NULL;
        }

        if (ppPrivilegeValues)
        {
            *ppPrivilegeValues = NULL;
        }

        if (pCount)
        {
            *pCount = 0;
        }
    }

    // Array elements are pointers from the database so they
    // must not be freed here
    LW_SAFE_FREE_MEMORY(ppPrivileges);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    if (err == ERROR_SUCCESS)
    {
        err = enumerationStatus;
    }

    return err;
}
