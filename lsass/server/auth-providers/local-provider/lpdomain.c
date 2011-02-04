/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lpdomain.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Domain Management
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
DWORD
LocalGetSingleStringAttrValue(
    PATTRIBUTE_VALUE pAttrs,
    DWORD            dwNumAttrs,
    PSTR*            ppszValue
    );

static
DWORD
LocalGetSingleLargeIntegerAttrValue(
    PATTRIBUTE_VALUE pAttrs,
    DWORD            dwNumAttrs,
    PLONG64          pllAttrValue
    );


DWORD
LocalSyncDomainInfo(
    PWSTR                    pwszUserDN,
    PWSTR                    pwszCredentials,
    ULONG                    ulMethod,
    PLOCAL_PROVIDER_GLOBALS  pGlobals
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwNewMinPasswordAge = LOCAL_MIN_PASSWORD_AGE;
    DWORD dwNewMaxPasswordAge = LOCAL_MAX_PASSWORD_AGE;
    DWORD dwNewMinPasswordLength = LOCAL_MIN_PASSWORD_LENGTH;
    DWORD dwNewPasswordPromptTime = LOCAL_PASSWORD_PROMPT_TIME;
    DWORD dwNewLockoutThreshold = LOCAL_LOCKOUT_THRESHOLD;
    DWORD dwNewLockoutDuration = LOCAL_LOCKOUT_DURATION;
    DWORD dwNewLockoutWindow = LOCAL_LOCKOUT_WINDOW;
    HANDLE hDirectory = NULL;
    PSTR pszFilterFmt = LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u";
    DWORD dwDomainObjectClass = LOCAL_OBJECT_CLASS_DOMAIN;
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    WCHAR wszAttrDn[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrDomain[] = LOCAL_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNetBIOSName[] = LOCAL_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrSequenceNumber[] = LOCAL_DIR_ATTR_SEQUENCE_NUMBER;
    WCHAR wszAttrMinPasswordAge[] = LOCAL_DIR_ATTR_MIN_PWD_AGE;
    WCHAR wszAttrMaxPasswordAge[] = LOCAL_DIR_ATTR_MAX_PWD_AGE;
    WCHAR wszAttrMinPasswordLength[] = LOCAL_DIR_ATTR_MIN_PWD_LENGTH;
    WCHAR wszAttrPasswordPromptTime[] = LOCAL_DIR_ATTR_PWD_PROMPT_TIME;
    WCHAR wszAttrLockoutThreshold[] = LOCAL_DIR_ATTR_LOCKOUT_THRESHOLD;
    WCHAR wszAttrLockoutDuration[] = LOCAL_DIR_ATTR_LOCKOUT_DURATION;
    WCHAR wszAttrLockoutWindow[] = LOCAL_DIR_ATTR_LOCKOUT_WINDOW;
    ULONG ulScope = 0;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwNumEntries = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    PWSTR pwszDomainDn = NULL;
    LONG64 llSequenceNumber = 0;
    PWSTR pwszDomainName = NULL;
    PSTR pszDomainName = NULL;
    PWSTR pwszNetBIOSName = NULL;
    PSTR pszNetBIOSName = NULL;
    PWSTR pwszSid = NULL;
    PSID pLocalDomainSid = NULL;
    LONG64 llMinPasswordAge = 0;
    LONG64 llNewMinPasswordAge = 0;
    LONG64 llMaxPasswordAge = 0;
    LONG64 llNewMaxPasswordAge = 0;
    DWORD dwMinPasswordLength = 0;
    LONG64 llPasswordPromptTime = 0;
    LONG64 llNewPasswordPromptTime = 0;
    DWORD dwLockoutThreshold = 0;
    LONG64 llLockoutDuration = 0;
    LONG64 llNewLockoutDuration = 0;
    LONG64 llLockoutWindow = 0;
    LONG64 llNewLockoutWindow = 0;
    DWORD iMod = 0;

    LSA_CONFIG SamConfig[] = {
        {
            "MinPasswordAge",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &dwNewMinPasswordAge,
            NULL
        },
        {
            "MaxPasswordAge",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &dwNewMaxPasswordAge,
            NULL
        },
        {
            "MinPasswordLength",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &dwNewMinPasswordLength,
            NULL
        },
        {
            "PasswordPromptTime",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &dwNewPasswordPromptTime,
            NULL
        },
        {
            "LockoutThreshold",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &dwNewLockoutThreshold,
            NULL
        },
        {
            "LockoutDuration",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &dwNewLockoutDuration,
            NULL
        },
        {
            "LockoutWindow",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &dwNewLockoutWindow,
            NULL
        }
    };

    PWSTR wszAttributes[] = {
        wszAttrDn,
        wszAttrDomain,
        wszAttrNetBIOSName,
        wszAttrObjectSID,
        wszAttrSequenceNumber,
        wszAttrMinPasswordAge,
        wszAttrMaxPasswordAge,
        wszAttrMinPasswordLength,
        wszAttrPasswordPromptTime,
        wszAttrLockoutThreshold,
        wszAttrLockoutDuration,
        wszAttrLockoutWindow,
        NULL
    };

    enum AttrValueIndex {
        ATTR_VAL_IDX_MIN_PASSWORD_AGE = 0,
        ATTR_VAL_IDX_MAX_PASSWORD_AGE,
        ATTR_VAL_IDX_MIN_PASSWORD_LENGTH,
        ATTR_VAL_IDX_PASSWORD_PROMPT_TIME,
        ATTR_VAL_IDX_LOCKOUT_THRESHOLD,
        ATTR_VAL_IDX_LOCKOUT_DURATION,
        ATTR_VAL_IDX_LOCKOUT_WINDOW,
        ATTR_VAL_IDX_SEQUENCE_NUMBER,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        { /* ATTR_VAL_IDX_MIN_PASSWORD_AGE */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        { /* ATTR_VAL_IDX_MAX_PASSWORD_AGE */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        { /* ATTR_VAL_IDX_MIN_PASSWORD_LENGTH */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        { /* ATTR_VAL_IDX_PASSWORD_PROMPT_TIME */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        { /* ATTR_VAL_IDX_LOCKOUT_THRESHOLD */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        { /* ATTR_VAL_IDX_LOCKOUT_DURATION */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        { /* ATTR_VAL_IDX_LOCKOUT_WINDOW */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        },
        { /* ATTR_VAL_IDX_SEQUENCE_NUMBER */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.llValue = 0
        }
    };

    DIRECTORY_MOD ModMinPasswordAge = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrMinPasswordAge,
        1,
        &AttrValues[ATTR_VAL_IDX_MIN_PASSWORD_AGE]
    };

    DIRECTORY_MOD ModMaxPasswordAge = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrMaxPasswordAge,
        1,
        &AttrValues[ATTR_VAL_IDX_MAX_PASSWORD_AGE]
    };

    DIRECTORY_MOD ModMinPasswordLength = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrMinPasswordLength,
        1,
        &AttrValues[ATTR_VAL_IDX_MIN_PASSWORD_LENGTH]
    };

    DIRECTORY_MOD ModPasswordPromptTime = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrPasswordPromptTime,
        1,
        &AttrValues[ATTR_VAL_IDX_PASSWORD_PROMPT_TIME]
    };

    DIRECTORY_MOD ModLockoutThreshold = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrLockoutThreshold,
        1,
        &AttrValues[ATTR_VAL_IDX_LOCKOUT_THRESHOLD]
    };

    DIRECTORY_MOD ModLockoutDuration = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrLockoutDuration,
        1,
        &AttrValues[ATTR_VAL_IDX_LOCKOUT_DURATION]
    };

    DIRECTORY_MOD ModLockoutWindow = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrLockoutWindow,
        1,
        &AttrValues[ATTR_VAL_IDX_LOCKOUT_WINDOW]
    };

    DIRECTORY_MOD ModSequenceNumber = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrSequenceNumber,
        1,
        &AttrValues[ATTR_VAL_IDX_SEQUENCE_NUMBER]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(Mods, 0, sizeof(Mods));

    dwError = LsaProcessConfig(
                "Services\\lsass\\Parameters\\SAM",
                "Policy\\Services\\lsass\\Parameters\\SAM",
                SamConfig,
                sizeof(SamConfig)/sizeof(SamConfig[0]));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryBind(
                    hDirectory,
                    pwszUserDN,
                    pwszCredentials,
                    ulMethod);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszFilterFmt,
                    dwDomainObjectClass);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    hDirectory,
                    NULL,
                    ulScope,
                    pwszFilter,
                    wszAttributes,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    /* There has to be exactly one domain object */
    if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEntry = &(pEntries[0]);

    /*
     * Get the basic domain info
     */

    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrDomain,
                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    &pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrNetBIOSName,
                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    &pwszNetBIOSName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrObjectSID,
                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszDomainName, &pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszNetBIOSName, &pszNetBIOSName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(
                    &pLocalDomainSid,
                    pwszSid);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Check if we should update any of domain parameters
     */

    /* MinPasswordAge */
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrMinPasswordAge,
                    DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                    &llMinPasswordAge);
    BAIL_ON_LSA_ERROR(dwError);

    llNewMinPasswordAge = LW_WINTIME_TO_NTTIME_REL(dwNewMinPasswordAge);

    if (llMinPasswordAge != llNewMinPasswordAge)
    {
        AttrValues[ATTR_VAL_IDX_MIN_PASSWORD_AGE].data.llValue =
            llNewMinPasswordAge;
        Mods[iMod++] = ModMinPasswordAge;

        llMinPasswordAge = llNewMinPasswordAge;
    }

    /* MaxPasswordAge */
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrMaxPasswordAge,
                    DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                    &llMaxPasswordAge);
    BAIL_ON_LSA_ERROR(dwError);

    llNewMaxPasswordAge = LW_WINTIME_TO_NTTIME_REL(dwNewMaxPasswordAge);

    if (llMaxPasswordAge != llNewMaxPasswordAge)
    {
        AttrValues[ATTR_VAL_IDX_MAX_PASSWORD_AGE].data.llValue =
            llNewMaxPasswordAge;
        Mods[iMod++] = ModMaxPasswordAge;

        llMaxPasswordAge = llNewMaxPasswordAge;
    }

    /* MinPasswordLength */
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrMinPasswordLength,
                    DIRECTORY_ATTR_TYPE_INTEGER,
                    &dwMinPasswordLength);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwMinPasswordLength != dwNewMinPasswordLength)
    {
        AttrValues[ATTR_VAL_IDX_MIN_PASSWORD_LENGTH].data.ulValue =
            dwNewMinPasswordLength;
        Mods[iMod++] = ModMinPasswordLength;

        dwMinPasswordLength = dwNewMinPasswordLength;
    }

    /* PasswordPromptTime */
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrPasswordPromptTime,
                    DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                    &llPasswordPromptTime);
    BAIL_ON_LSA_ERROR(dwError);

    llNewPasswordPromptTime = LW_WINTIME_TO_NTTIME_REL(dwNewPasswordPromptTime);

    if (llPasswordPromptTime != llNewPasswordPromptTime)
    {
        AttrValues[ATTR_VAL_IDX_PASSWORD_PROMPT_TIME].data.llValue =
            llNewPasswordPromptTime;
        Mods[iMod++] = ModPasswordPromptTime;

        llPasswordPromptTime = llNewPasswordPromptTime;
    }

    /* LockoutThreshold */
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrLockoutThreshold,
                    DIRECTORY_ATTR_TYPE_INTEGER,
                    &dwLockoutThreshold);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwLockoutThreshold != dwNewLockoutThreshold)
    {
        AttrValues[ATTR_VAL_IDX_LOCKOUT_THRESHOLD].data.ulValue =
            dwLockoutThreshold;
        Mods[iMod++] = ModLockoutThreshold;

        dwLockoutThreshold = dwNewLockoutThreshold;
    }

    /* LockoutDuration */
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrLockoutDuration,
                    DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                    &llLockoutDuration);
    BAIL_ON_LSA_ERROR(dwError);

    llNewLockoutDuration = LW_WINTIME_TO_NTTIME_REL(dwNewLockoutDuration);

    if (llLockoutDuration != llNewLockoutDuration)
    {
        AttrValues[ATTR_VAL_IDX_LOCKOUT_DURATION].data.llValue =
            llNewLockoutDuration;
        Mods[iMod++] = ModLockoutDuration;

        llLockoutDuration = llNewLockoutDuration;
    }

    /* LockoutWindow */
    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrLockoutWindow,
                    DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                    &llLockoutWindow);
    BAIL_ON_LSA_ERROR(dwError);

    llNewLockoutWindow = LW_WINTIME_TO_NTTIME_REL(dwNewLockoutWindow);

    if (llLockoutWindow != llNewLockoutWindow)
    {
        AttrValues[ATTR_VAL_IDX_LOCKOUT_DURATION].data.llValue =
            llNewLockoutWindow;
        Mods[iMod++] = ModLockoutWindow;

        llLockoutWindow = llNewLockoutWindow;
    }

    /*
     * Update domain object with current SAM configuration if
     * there was any change found
     */

    if (iMod)
    {
        dwError = DirectoryGetEntryAttrValueByName(
                        pEntry,
                        wszAttrSequenceNumber,
                        DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                        &llSequenceNumber);
        BAIL_ON_LSA_ERROR(dwError);

        AttrValues[ATTR_VAL_IDX_SEQUENCE_NUMBER].data.llValue =
            ++llSequenceNumber;
        Mods[iMod++] = ModSequenceNumber;

        dwError = DirectoryGetEntryAttrValueByName(
                        pEntry,
                        wszAttrDn,
                        DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                        &pwszDomainDn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryModifyObject(
                        hDirectory,
                        pwszDomainDn,
                        Mods);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGlobals->pszLocalDomain       = pszDomainName;
    pGlobals->pszNetBIOSName       = pszNetBIOSName;
    pGlobals->pLocalDomainSID      = pLocalDomainSid;
    pGlobals->llMinPwdAge          = llMinPasswordAge;
    pGlobals->llMaxPwdAge          = llMaxPasswordAge;
    pGlobals->dwMinPwdLength       = dwMinPasswordLength;
    pGlobals->llPwdChangeTime      = llPasswordPromptTime;
    pGlobals->dwLockoutThreshold   = dwLockoutThreshold;
    pGlobals->llLockoutDuration    = llLockoutDuration;
    pGlobals->llLockoutWindow      = llLockoutWindow;

cleanup:
    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
LocalGetDomainInfo(
    PWSTR   pwszUserDN,
    PWSTR   pwszCredentials,
    ULONG   ulMethod,
    PSTR*   ppszNetBIOSName,
    PSTR*   ppszDomainName,
    PSID*   ppDomainSID,
    PLONG64 pllMaxPwdAge,
    PLONG64 pllPwdChangeTime
    )
{
    DWORD  dwError = 0;
    HANDLE hDirectory  = NULL;
    DWORD  objectClass = LOCAL_OBJECT_CLASS_DOMAIN;
    PCSTR  pszFilterTemplate = LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %u";
    PSTR   pszFilter = NULL;
    PWSTR  pwszFilter = NULL;
    wchar16_t wszAttrNameDomain[]      = LOCAL_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameNetBIOSName[] = LOCAL_DIR_ATTR_NETBIOS_NAME;
    wchar16_t wszAttrNameObjectSID[]   = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameMaxPwdAge[]   = LOCAL_DIR_ATTR_MAX_PWD_AGE;
    wchar16_t wszAttrNamePwdChangeTime[] = LOCAL_DIR_ATTR_PWD_PROMPT_TIME;
    PWSTR  wszAttrs[] =
    {
            &wszAttrNameDomain[0],
            &wszAttrNameNetBIOSName[0],
            &wszAttrNameObjectSID[0],
            &wszAttrNameMaxPwdAge[0],
            &wszAttrNamePwdChangeTime[0],
            NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry   = NULL;
    DWORD            dwNumEntries = 0;
    DWORD            iAttr = 0;
    ULONG            ulScope = 0;
    PSTR   pszDomainName  = NULL;
    PSTR   pszNetBIOSName = NULL;
    PSTR   pszDomainSID   = NULL;
    PSID   pDomainSID     = NULL;
    LONG64 llMaxPwdAge = 0;
    LONG64 llPwdChangeTime = 0;

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    objectClass);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryBind(
                    hDirectory,
                    pwszUserDN,
                    pwszCredentials,
                    ulMethod);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    hDirectory,
                    NULL,
                    ulScope,
                    pwszFilter,
                    wszAttrs,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (!dwNumEntries)
    {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    if (dwNumEntries > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameDomain[0]))
        {
            dwError = LocalGetSingleStringAttrValue(
                            pAttr->pValues,
                            pAttr->ulNumValues,
                            &pszDomainName);
        }
        else
        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameNetBIOSName[0]))
        {
            dwError = LocalGetSingleStringAttrValue(
                            pAttr->pValues,
                            pAttr->ulNumValues,
                            &pszNetBIOSName);
        }
        else
        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameMaxPwdAge[0]))
        {
            dwError = LocalGetSingleLargeIntegerAttrValue(
                            pAttr->pValues,
                            pAttr->ulNumValues,
                            &llMaxPwdAge);
        }
        else
        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePwdChangeTime[0]))
        {
            dwError = LocalGetSingleLargeIntegerAttrValue(
                            pAttr->pValues,
                            pAttr->ulNumValues,
                            &llPwdChangeTime);
        }
        else
        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            dwError = LocalGetSingleStringAttrValue(
                            pAttr->pValues,
                            pAttr->ulNumValues,
                            &pszDomainSID);
        }
        else
        {
            dwError = LW_ERROR_DATA_ERROR;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = RtlAllocateSidFromCString(
                    &pDomainSID,
                    pszDomainSID);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszDomainName = pszDomainName;
    *ppszNetBIOSName = pszNetBIOSName;
    *ppDomainSID = pDomainSID;
    *pllMaxPwdAge = llMaxPwdAge;
    *pllPwdChangeTime = llPwdChangeTime;

cleanup:

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_STRING(pszDomainSID);
    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:

    *ppszDomainName = NULL;
    *ppszNetBIOSName = NULL;
    *ppDomainSID = NULL;
    *pllMaxPwdAge = 0;
    *pllPwdChangeTime = 0;

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszNetBIOSName);
    RTL_FREE(&pDomainSID);

    goto cleanup;
}

static
DWORD
LocalGetSingleStringAttrValue(
    PATTRIBUTE_VALUE pAttrs,
    DWORD            dwNumAttrs,
    PSTR*            ppszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszValue = NULL;

    if ((dwNumAttrs != 1) ||
        (pAttrs[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pAttrs[0].data.pwszStringValue)
    {
        dwError = LwWc16sToMbs(
                        pAttrs[0].data.pwszStringValue,
                        &pszValue);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszValue = pszValue;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszValue);

    *ppszValue = NULL;

    goto cleanup;
}


DWORD
LocalGetSequenceNumber(
    IN HANDLE hProvider,
    OUT PLONG64 pllSequenceNumber
    )
{
    wchar_t wszDomainFilterFmt[] = L"%ws = %u";
    DWORD dwError = ERROR_SUCCESS;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    WCHAR wszAttrNameObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrNameSeqNumber[] = LOCAL_DIR_ATTR_SEQUENCE_NUMBER;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    LONG64 llSequenceNumber = 0;

    PWSTR wszAttrs[] = {
        wszAttrNameSeqNumber,
        NULL
    };

    dwFilterLen = ((sizeof(wszAttrNameSeqNumber)/sizeof(wszAttrNameSeqNumber[0])) +
                   10 +
                   (sizeof(wszDomainFilterFmt)/sizeof(wszDomainFilterFmt[0])));

    dwError = LwAllocateMemory(sizeof(pwszFilter[0]) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszDomainFilterFmt,
                    wszAttrNameObjectClass, LOCAL_OBJECT_CLASS_DOMAIN) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectorySearch(
        pContext->hDirectory,
        NULL,
        0,
        pwszFilter,
        wszAttrs,
        FALSE,
        &pEntry,
        &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryGetEntryAttrValueByName(
                          pEntry,
                          wszAttrNameSeqNumber,
                          DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                          &llSequenceNumber);
    BAIL_ON_LSA_ERROR(dwError);

    *pllSequenceNumber = llSequenceNumber;

cleanup:
    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:
    if (pllSequenceNumber)
    {
        *pllSequenceNumber = 0;
    }

    goto cleanup;
}

static
DWORD
LocalGetSingleLargeIntegerAttrValue(
    PATTRIBUTE_VALUE pAttrs,
    DWORD            dwNumAttrs,
    PLONG64          pllAttrValue
    )
{
    DWORD dwError = 0;

    if ((dwNumAttrs != 1) ||
        (pAttrs[0].Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pllAttrValue = pAttrs[0].data.llValue;

cleanup:

    return dwError;

error:

    *pllAttrValue = 0;

    goto cleanup;
}


DWORD
LocalDirSetDomainName(
    IN PCSTR  pszNewDomainName
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    PWSTR pwszNewDomainName = NULL;
    PWSTR pwszDomainObjectDN = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PWSTR pwszDomainFilter = NULL;
    DWORD dwDomainFilterLen = 0;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    ULONG ulAttributesOnly = 0;
    PDIRECTORY_ENTRY pDomainEntries = NULL;
    PDIRECTORY_ENTRY pDomainEntry = NULL;
    DWORD dwNumDomainEntries = 0;
    DWORD iMod = 0;
    PWSTR pwszUserDN = NULL;
    PWSTR pwszCredentials = NULL;
    ULONG ulMethod = 0;
    BOOLEAN bLocked = FALSE;

    WCHAR wszAttrObjectClass[] = DIRECTORY_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDN[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrDomainName[] = DIRECTORY_ATTR_DOMAIN_NAME;
    WCHAR wszAttrNetbiosName[] = DIRECTORY_ATTR_NETBIOS_NAME;
    WCHAR wszAttrCommonName[] = DIRECTORY_ATTR_COMMON_NAME;
    WCHAR wszAttrSamAccountName[] = DIRECTORY_ATTR_SAM_ACCOUNT_NAME;

    PWSTR wszAttributes[] = {
        &wszAttrObjectDN[0],
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_DOMAIN_NAME  = 0,
        ATTR_IDX_NETBIOS_NAME,
        ATTR_IDX_COMMON_NAME,
        ATTR_IDX_SAM_ACCOUNT_NAME,
        ATTR_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_IDX_DOMAIN_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_IDX_NETBIOS_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_IDX_COMMON_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_IDX_SAM_ACCOUNT_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
    };

    DIRECTORY_MOD modDomainName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrDomainName[0],
        1,
        &AttrValues[ATTR_IDX_DOMAIN_NAME]
    };

    DIRECTORY_MOD modNetbiosName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrNetbiosName[0],
        1,
        &AttrValues[ATTR_IDX_NETBIOS_NAME]
    };

    DIRECTORY_MOD modCommonName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrCommonName[0],
        1,
        &AttrValues[ATTR_IDX_COMMON_NAME]
    };

    DIRECTORY_MOD modSamAccountName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrSamAccountName[0],
        1,
        &AttrValues[ATTR_IDX_SAM_ACCOUNT_NAME]
    };

    DIRECTORY_MOD mods[ATTR_IDX_SENTINEL + 1];

    BAIL_ON_INVALID_POINTER(pszNewDomainName);

    if (strlen(pszNewDomainName) > 15)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*
     * Freeze local provider to prevent from changing
     * database contents in the background
     */
    LOCAL_WRLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    dwError = LwMbsToWc16s(pszNewDomainName,
                           &pwszNewDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Search and modify MACHINE domain object
     */

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = LwAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                               (PVOID*)&pwszDomainFilter);
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_DOMAIN) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszDomainFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pDomainEntries,
                              &dwNumDomainEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumDomainEntries != 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pDomainEntry = &(pDomainEntries[0]);

    dwError = DirectoryGetEntryAttrValueByName(
                                pDomainEntry,
                                wszAttrObjectDN,
                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                &pwszDomainObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    iMod = 0;
    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_DOMAIN_NAME].data.pwszStringValue  = pwszNewDomainName;
    AttrValues[ATTR_IDX_NETBIOS_NAME].data.pwszStringValue = pwszNewDomainName;
    AttrValues[ATTR_IDX_COMMON_NAME].data.pwszStringValue  = pwszNewDomainName;
    AttrValues[ATTR_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue
        = pwszNewDomainName;

    mods[iMod++] = modDomainName;
    mods[iMod++] = modNetbiosName;
    mods[iMod++] = modCommonName;
    mods[iMod++] = modSamAccountName;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszDomainObjectDN,
                                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Reload local domain info
     */

    LW_SAFE_FREE_STRING(gLPGlobals.pszLocalDomain);
    LW_SAFE_FREE_STRING(gLPGlobals.pszNetBIOSName);
    LW_SAFE_FREE_MEMORY(gLPGlobals.pLocalDomainSID);

    dwError = LocalGetDomainInfo(
                    pwszUserDN,
                    pwszCredentials,
                    ulMethod,
                    &gLPGlobals.pszLocalDomain,
                    &gLPGlobals.pszNetBIOSName,
                    &gLPGlobals.pLocalDomainSID,
                    &gLPGlobals.llMaxPwdAge,
                    &gLPGlobals.llPwdChangeTime);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LW_SAFE_FREE_MEMORY(pwszNewDomainName);
    LW_SAFE_FREE_MEMORY(pwszDomainFilter);

    return dwError;

error:
    goto cleanup;
}


DWORD
LocalDirSetDomainSid(
    IN PCSTR  pszSid
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszNewDomainSid = NULL;
    PSID pNewDomainSid = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PWSTR pwszDomainFilter = NULL;
    DWORD dwDomainFilterLen = 0;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    ULONG ulAttributesOnly = 0;
    PDIRECTORY_ENTRY pDomainEntries = NULL;
    PDIRECTORY_ENTRY pDomainEntry = NULL;
    DWORD dwNumDomainEntries = 0;
    PWSTR pwszDomainObjectDN = NULL;
    PWSTR pwszUserDN = NULL;
    PWSTR pwszCredentials = NULL;
    ULONG ulMethod = 0;
    BOOLEAN bLocked = FALSE;

    WCHAR wszAttrObjectDN[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DIRECTORY_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSID[] = DIRECTORY_ATTR_OBJECT_SID;

    PWSTR wszAttributes[] = {
        &wszAttrObjectDN[0],
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_OBJECT_SID = 0,
        ATTR_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD modObjectSID = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrObjectSID[0],
        1,
        &AttrValues[ATTR_IDX_OBJECT_SID]
    };

    DIRECTORY_MOD mods[ATTR_IDX_SENTINEL + 1];


    /*
     * Freeze local provider to prevent from changing
     * database contents in the background
     */
    LOCAL_WRLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    dwError = LwMbsToWc16s(pszSid,
                           &pwszNewDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(&pNewDomainSid,
                                            pwszNewDomainSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = LwAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                               (PVOID*)&pwszDomainFilter);
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_DOMAIN) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszDomainFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pDomainEntries,
                              &dwNumDomainEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumDomainEntries != 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pDomainEntry = &(pDomainEntries[0]);

    dwError = DirectoryGetEntryAttrValueByName(
                                pDomainEntry,
                                wszAttrObjectDN,
                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                &pwszDomainObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewDomainSid;
    mods[0] = modObjectSID;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszDomainObjectDN,
                                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Reload local domain info
     */

    LW_SAFE_FREE_STRING(gLPGlobals.pszLocalDomain);
    LW_SAFE_FREE_STRING(gLPGlobals.pszNetBIOSName);
    LW_SAFE_FREE_MEMORY(gLPGlobals.pLocalDomainSID);

    dwError = LocalGetDomainInfo(
                    pwszUserDN,
                    pwszCredentials,
                    ulMethod,
                    &gLPGlobals.pszLocalDomain,
                    &gLPGlobals.pszNetBIOSName,
                    &gLPGlobals.pLocalDomainSID,
                    &gLPGlobals.llMaxPwdAge,
                    &gLPGlobals.llPwdChangeTime);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LW_SAFE_FREE_MEMORY(pwszDomainFilter);
    LW_SAFE_FREE_MEMORY(pwszNewDomainSid);
    RTL_FREE(&pNewDomainSid);

    return dwError;

error:
    goto cleanup;
}
