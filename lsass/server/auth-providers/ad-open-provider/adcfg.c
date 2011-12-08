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
 *        adcfg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 *        Wrappers for accessing global configuration variables
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "adprovider.h"

#define AD_PROVIDER_POLICY_REGKEY "Policy\\" AD_PROVIDER_REGKEY
#define AD_PROVIDER_POLICY_DOMAINJOIN_REGKEY "Policy\\" AD_PROVIDER_DOMAINJOIN_REGKEY

static
DWORD
AD_CheckList(
    PCSTR pszNeedle,
    PCSTR pszaHaystack,
    PBOOLEAN pbFoundIt
    );

static
DWORD
AD_SetConfig_Umask(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_RequireMembershipOf(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    DWORD          dwMachinePasswordSyncPwdLifetime
    );

static
DWORD
AD_SetConfig_DomainManager_TrustExceptionList(
    PLSA_AD_CONFIG pConfig,
    PCSTR pszTrustsListMultiString
    );


DWORD
AD_TransferConfigContents(
    PLSA_AD_CONFIG pSrcConfig,
    PLSA_AD_CONFIG pDstConfig
    )
{
    AD_FreeConfigContents(pDstConfig);

    *pDstConfig = *pSrcConfig;

    memset(pSrcConfig, 0, sizeof(LSA_AD_CONFIG));

    return 0;
}

DWORD
AD_InitializeConfig(
    PLSA_AD_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(LSA_AD_CONFIG));

    pConfig->bAssumeDefaultDomain = FALSE;
    pConfig->bCreateHomeDir   = TRUE;
    pConfig->bCreateK5Login   = TRUE;
    pConfig->bLDAPSignAndSeal = FALSE;
    pConfig->bSyncSystemTime  = TRUE;
    pConfig->dwCacheEntryExpirySecs   = AD_CACHE_ENTRY_EXPIRY_DEFAULT_SECS;
    pConfig->dwCacheSizeCap           = 0;
    pConfig->dwMachinePasswordSyncLifetime = AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS;
    pConfig->dwUmask          = AD_DEFAULT_UMASK;

    pConfig->bEnableEventLog = FALSE;
    pConfig->bShouldLogNetworkConnectionEvents = TRUE;
    pConfig->bRefreshUserCreds = TRUE;
    pConfig->CellSupport = AD_CELL_SUPPORT_UNPROVISIONED;
    pConfig->CacheBackend = AD_CACHE_IN_MEMORY;
    pConfig->bTrimUserMembershipEnabled = TRUE;
    pConfig->bNssGroupMembersCacheOnlyEnabled = TRUE;
    pConfig->bNssUserMembershipCacheOnlyEnabled = FALSE;
    pConfig->bNssEnumerationEnabled = FALSE;

    pConfig->DomainManager.dwCheckDomainOnlineSeconds = 5 * LSA_SECONDS_IN_MINUTE;
    pConfig->DomainManager.dwUnknownDomainCacheTimeoutSeconds = 1 * LSA_SECONDS_IN_HOUR;
    pConfig->DomainManager.bIgnoreAllTrusts = FALSE;
    pConfig->DomainManager.ppszTrustExceptionList = NULL;
    pConfig->DomainManager.dwTrustExceptionCount = 0;

    pConfig->bMultiTenancyEnabled = FALSE;
    pConfig->bAddDomainToLocalGroupsEnabled = FALSE;

    dwError = LwAllocateString(
                    AD_DEFAULT_SHELL,
                    &pConfig->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_HOMEDIR_PREFIX,
                    &pConfig->pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_HOMEDIR_TEMPLATE,
                    &pConfig->pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_SKELDIRS,
                    &pConfig->pszSkelDirs);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    AD_FreeConfigContents(pConfig);

    goto cleanup;
}

VOID
AD_FreeConfig(
    PLSA_AD_CONFIG pConfig
    )
{
    AD_FreeConfigContents(pConfig);
    LwFreeMemory(pConfig);
}

VOID
AD_FreeConfigContents(
    PLSA_AD_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    LW_SAFE_FREE_STRING(pConfig->pszShell);
    LW_SAFE_FREE_STRING(pConfig->pszSkelDirs);
    LW_SAFE_FREE_MEMORY(pConfig->pszaIgnoreUserNameList);
    LW_SAFE_FREE_MEMORY(pConfig->pszaIgnoreGroupNameList);
    LW_SAFE_FREE_STRING(pConfig->pszUserDomainPrefix);

    if (pConfig->pUnresolvedMemberList)
    {
        LwDLinkedListForEach(
                        pConfig->pUnresolvedMemberList,
                        &AD_FreeConfigMemberInList,
                        NULL);
        LwDLinkedListFree(pConfig->pUnresolvedMemberList);
        pConfig->pUnresolvedMemberList = NULL;
    }

    LwFreeStringArray(
        pConfig->DomainManager.ppszTrustExceptionList,
        pConfig->DomainManager.dwTrustExceptionCount);
    pConfig->DomainManager.ppszTrustExceptionList = NULL;
    pConfig->DomainManager.dwTrustExceptionCount = 0;
}

VOID
AD_FreeConfigMemberInList(
    PVOID pItem,
    PVOID pUserData
    )
{
    LW_SAFE_FREE_MEMORY(pItem);
}

DWORD
AD_ReadRegistry(
    IN OPTIONAL PCSTR pszDomainName,
    OUT PLSA_AD_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PSTR pszDomainKey = NULL;
    PSTR pszDomainPolicyKey = NULL;
    PSTR pszUmask = NULL;
    PSTR pszUnresolvedMemberList = NULL;
    DWORD dwMachinePasswordSyncLifetime = 0;
    PSTR pszExcludeTrustsListMultiString = NULL;
    PSTR pszIncludeTrustsListMultiString = NULL;
    LSA_AD_CONFIG StagingConfig;

    const PCSTR CellSupport[] = {
        "unprovisioned"
    };

    const PCSTR CacheBackend[] = 
    {
        "sqlite",
        "memory"
    };

    LWREG_CONFIG_ITEM ADConfigDescription[] =
    {
        {
            "HomeDirUmask",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &pszUmask,
            NULL
        },
        {
            "RequireMembershipOf",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &pszUnresolvedMemberList,
            NULL
        },
        {
            "LoginShellTemplate",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszShell,
            NULL
        },
        {
            "HomeDirTemplate",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszHomedirTemplate,
            NULL
        },
        {
            "UserDomainPrefix",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszUserDomainPrefix
        },
        {
            "MachinePasswordLifespan",
            TRUE,
            LwRegTypeDword,
            0,  /* Valid range is 0 or [AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS,*/
            MAXDWORD, /* AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS] */
            NULL,
            &dwMachinePasswordSyncLifetime,
            NULL
        },
        {
            "CacheEntryExpiry",
            TRUE,
            LwRegTypeDword,
            AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS,
            AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS,
            NULL,
            &StagingConfig.dwCacheEntryExpirySecs,
            NULL
        },
        {
            "MemoryCacheSizeCap",
            TRUE,
            LwRegTypeDword,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.dwCacheSizeCap,
            NULL
        },
        {
            "LdapSignAndSeal",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bLDAPSignAndSeal,
            NULL
        },
        {
            "AssumeDefaultDomain",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bAssumeDefaultDomain,
            NULL
        },
        {
            "SyncSystemTime",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bSyncSystemTime,
            NULL
        },
        {
            "LogNetworkConnectionEvents",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bShouldLogNetworkConnectionEvents,
            NULL
        },
        {
            "CreateK5Login",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bCreateK5Login,
            NULL
        },
        {
            "CreateHomeDir",
            TRUE,
            LwRegTypeBoolean,
            0,
            -1,
            NULL,
            &StagingConfig.bCreateHomeDir,
            NULL
        },
        {
            "SkeletonDirs",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszSkelDirs,
            NULL
        },
        {
            "HomeDirPrefix",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszHomedirPrefix,
            NULL
        },
        {
            "RefreshUserCredentials",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bRefreshUserCreds,
            NULL
        },
        {
            "TrimUserMembership",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bTrimUserMembershipEnabled,
            NULL
        },
        {
            "CellSupport",
            TRUE,
            LwRegTypeEnum,
            AD_CELL_SUPPORT_UNPROVISIONED,
            AD_CELL_SUPPORT_UNPROVISIONED,
            CellSupport,
            &StagingConfig.CellSupport,
            NULL
        },
        {
            "CacheType",
            TRUE,
            LwRegTypeEnum,
            AD_CACHE_SQLITE,
            AD_CACHE_IN_MEMORY,
            CacheBackend,
            &StagingConfig.CacheBackend,
            NULL
        },
        {
            "NssGroupMembersQueryCacheOnly",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bNssGroupMembersCacheOnlyEnabled,
            NULL
        },
        {
            "NssUserMembershipQueryCacheOnly",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bNssUserMembershipCacheOnlyEnabled,
            NULL
        },
        {
            "NssEnumerationEnabled",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bNssEnumerationEnabled,
            NULL
        },
        {
            "DomainManagerCheckDomainOnlineInterval",
            TRUE,
            LwRegTypeDword,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.DomainManager.dwCheckDomainOnlineSeconds,
            NULL
        },
        {
            "DomainManagerUnknownDomainCacheTimeout",
            TRUE,
            LwRegTypeDword,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.DomainManager.dwUnknownDomainCacheTimeoutSeconds,
            NULL
        },
        {
            "DomainManagerIgnoreAllTrusts",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.DomainManager.bIgnoreAllTrusts,
            NULL
        },
        {
            "DomainManagerExcludeTrustsList",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &pszExcludeTrustsListMultiString,
            NULL
        },
        {
            "DomainManagerIncludeTrustsList",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &pszIncludeTrustsListMultiString,
            NULL
        },
        {
            "IgnoreUserNameList",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszaIgnoreUserNameList,
            NULL
        },
        {
            "IgnoreGroupNameList",
            TRUE,
            LwRegTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszaIgnoreGroupNameList,
            NULL
        },
        {
            "MultiTenancyEnabled",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bMultiTenancyEnabled,
            NULL
        },
        {
            "AddDomainToLocalGroupsEnabled",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bAddDomainToLocalGroupsEnabled,
            NULL
        }
    };

    LWREG_CONFIG_ITEM LsaConfigDescription[] =
    {
        {
            "EnableEventlog",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bEnableEventLog,
            NULL
        }
    };

    dwError = AD_InitializeConfig(&StagingConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwMachinePasswordSyncLifetime = AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS;

    dwError = RegProcessConfig(
                AD_PROVIDER_REGKEY,
                AD_PROVIDER_POLICY_REGKEY,
                ADConfigDescription,
                sizeof(ADConfigDescription)/sizeof(ADConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);

    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                     &pszDomainKey,
                     "%s\\%s",
                     AD_PROVIDER_DOMAINJOIN_REGKEY,
                     pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
                     &pszDomainPolicyKey,
                     "%s\\%s",
                     AD_PROVIDER_POLICY_DOMAINJOIN_REGKEY,
                     pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = RegProcessConfig(
                      pszDomainKey,
                      pszDomainPolicyKey,
                      ADConfigDescription,
                      sizeof(ADConfigDescription)/sizeof(ADConfigDescription[0]));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters",
                "Policy\\Services\\lsass\\Parameters",
                LsaConfigDescription,
                sizeof(LsaConfigDescription)/sizeof(LsaConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);

    AD_SetConfig_Umask(
            &StagingConfig,
            "HomeDirUmask",
            pszUmask);

    AD_SetConfig_RequireMembershipOf(
            &StagingConfig,
            "RequireMembershipOf",
            pszUnresolvedMemberList);

    AD_SetConfig_MachinePasswordLifespan(
            &StagingConfig,
            "MachinePasswordLifespan",
            dwMachinePasswordSyncLifetime);

    AD_SetConfig_DomainManager_TrustExceptionList(
            &StagingConfig,
            (StagingConfig.DomainManager.bIgnoreAllTrusts ?
             pszIncludeTrustsListMultiString :
             pszExcludeTrustsListMultiString));

    AD_TransferConfigContents(&StagingConfig, pConfig);

cleanup:
    LW_SAFE_FREE_STRING(pszDomainKey);
    LW_SAFE_FREE_STRING(pszDomainPolicyKey);
    LW_SAFE_FREE_STRING(pszUmask);
    LW_SAFE_FREE_STRING(pszUnresolvedMemberList);
    LW_SAFE_FREE_STRING(pszExcludeTrustsListMultiString);
    LW_SAFE_FREE_STRING(pszIncludeTrustsListMultiString);

    AD_FreeConfigContents(&StagingConfig);

    return dwError;

error:
    AD_FreeConfigContents(pConfig);
    goto cleanup;
}

static
DWORD
AD_SetConfig_Umask(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PCSTR cp = NULL;
    DWORD dwOct = 0;
    DWORD dwVal = 0;
    DWORD dwCnt = 0;
    char  cp2[2];

    // Convert the umask octal string to a decimal number

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto error;
    }

    cp2[1] = 0;

    for ( cp = pszValue, dwCnt = 0 ; isdigit((int)*cp) ; cp++, dwCnt++ )
    {
        dwOct *= 8;

        cp2[0] = *cp;
        dwVal = atoi(cp2);

        if ( dwVal > 7 )
        {
            LSA_LOG_ERROR("Invalid Umask [%s]", pszValue);
            goto error;
        }

        dwOct += dwVal;
    }

    if ( dwCnt > 4 )
    {
        LSA_LOG_ERROR("Invalid Umask [%s]", pszValue);
        goto error;
    }

    // Disallow 07xx since the user should always have
    // access to his home directory.
    if ( (dwOct & 0700) == 0700 )
    {
        LSA_LOG_ERROR("Invalid Umask [%s]. User cannot access home directory.",
                pszValue);
        goto error;
    }
    else
    {
        pConfig->dwUmask = dwOct;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_ConvertMultiStringToStringArray(
    IN PCSTR pszMultiString,
    OUT PSTR** pppszStringArray,
    OUT PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    PSTR* ppszStringArray = NULL;
    DWORD dwCount = 0;
    PCSTR pszIter = NULL;
    DWORD dwIndex = 0;

    dwCount = 0;
    for (pszIter = pszMultiString;
         pszIter && *pszIter;
         pszIter += strlen(pszIter) + 1)
    {
        dwCount++;
    }

    if (dwCount)
    {
        dwError = LwAllocateMemory(
                        dwCount * sizeof(*ppszStringArray),
                        OUT_PPVOID(&ppszStringArray));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwIndex = 0;
    for (pszIter = pszMultiString;
         pszIter && *pszIter;
         pszIter += strlen(pszIter) + 1)
    {
        dwError = LwAllocateString(pszIter, &ppszStringArray[dwIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        dwIndex++;
    }

    LSA_ASSERT(dwIndex == dwCount);

cleanup:

    *pppszStringArray = ppszStringArray;
    *pdwCount = dwCount;

    return dwError;

error:

    LwFreeStringArray(ppszStringArray, dwCount);
    ppszStringArray = NULL;
    dwCount = 0;

    goto cleanup;
}

static
DWORD
AD_SetConfig_RequireMembershipOf(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PCSTR pszIter = pszValue;
    PSTR  pszMember = NULL;

    pszIter = pszValue;
    while (pszIter != NULL && *pszIter != '\0')
    {
        PSTR pszEnd;

        while (*pszIter == ' ')
        {
            ++pszIter;
        }

        dwError = LwStrDupOrNull(
                        pszIter,
                        &pszMember);
        BAIL_ON_LSA_ERROR(dwError);

        pszEnd = pszMember + strlen(pszMember);

        while (pszEnd > pszMember && pszEnd[-1] == ' ')
        {
            --pszEnd;
        }

        *pszEnd = '\0';

        dwError = LwDLinkedListAppend(
                        &pConfig->pUnresolvedMemberList,
                        pszMember);
        BAIL_ON_LSA_ERROR(dwError);

        pszMember = NULL;

        pszIter += strlen(pszIter) + 1;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszMember);

    return dwError;

error:

    goto cleanup;
}


static
DWORD
AD_SetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    DWORD          dwMachinePasswordSyncPwdLifetime
    )
{
    DWORD dwError = 0;

    if ((dwMachinePasswordSyncPwdLifetime != 0) &&
        (dwMachinePasswordSyncPwdLifetime < AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS))
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Minimum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwMachinePasswordSyncPwdLifetime > AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Maximum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwMachinePasswordSyncLifetime = dwMachinePasswordSyncPwdLifetime;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_DomainManager_TrustExceptionList(
    PLSA_AD_CONFIG pConfig,
    PCSTR pszTrustsListMultiString
    )
{
    return AD_ConvertMultiStringToStringArray(
                    pszTrustsListMultiString,
                    &pConfig->DomainManager.ppszTrustExceptionList,
                    &pConfig->DomainManager.dwTrustExceptionCount);
}

DWORD
AD_GetUnprovisionedModeShell(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszUnprovisionedModeShell
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszUnprovisionedModeShell = NULL;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    if (!LW_IS_NULL_OR_EMPTY_STR(pState->config.pszShell))
    {
        dwError = LwAllocateString(
                        pState->config.pszShell,
                        &pszUnprovisionedModeShell);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszUnprovisionedModeShell = pszUnprovisionedModeShell;

cleanup:

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwError;

error:

    *ppszUnprovisionedModeShell = NULL;

    goto cleanup;
}

DWORD
AD_GetHomedirPrefixPath(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR  pszHomedirPrefixPath = NULL;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    if (!LW_IS_NULL_OR_EMPTY_STR(pState->config.pszHomedirPrefix))
    {
        dwError = LwAllocateString(
                        pState->config.pszHomedirPrefix,
                        &pszHomedirPrefixPath
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszPath = pszHomedirPrefixPath;

cleanup:

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwError;

error:

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
AD_GetUserDomainPrefix(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR  pszValue = NULL;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    if (!LW_IS_NULL_OR_EMPTY_STR(pState->config.pszUserDomainPrefix))
    {
        dwError = LwAllocateString(
                        pState->config.pszUserDomainPrefix,
                        &pszValue
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pState->pProviderData &&
             !LW_IS_NULL_OR_EMPTY_STR(pState->pProviderData->szShortDomain))
    {
        dwError = LwAllocateString(
                        pState->pProviderData->szShortDomain,
                        &pszValue
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    LwStrToUpper(pszValue);

    *ppszPath = pszValue;

cleanup:

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwError;

error:

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
AD_GetUnprovisionedModeHomedirTemplate(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszUnprovisionedModeHomedirTemplate
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszUnprovisionedModeHomedirTemplate = NULL;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    if (!LW_IS_NULL_OR_EMPTY_STR(pState->config.pszHomedirTemplate))
    {
        dwError = LwAllocateString(
                        pState->config.pszHomedirTemplate,
                        &pszUnprovisionedModeHomedirTemplate
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszUnprovisionedModeHomedirTemplate = pszUnprovisionedModeHomedirTemplate;

cleanup:

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwError;

error:

    *ppszUnprovisionedModeHomedirTemplate = NULL;

    goto cleanup;
}

DWORD
AD_GetMachinePasswordSyncPwdLifetime(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwMachinePasswordSyncPwdLifetime = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);
    dwMachinePasswordSyncPwdLifetime = pState->config.dwMachinePasswordSyncLifetime;
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwMachinePasswordSyncPwdLifetime;
}

DWORD
AD_GetClockDriftSeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwClockDriftSecs = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);
    dwClockDriftSecs = pState->dwMaxAllowedClockDriftSeconds;
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwClockDriftSecs;
}

DWORD
AD_GetCacheEntryExpirySeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    dwResult = pState->config.dwCacheEntryExpirySecs;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwResult;
}

DWORD
AD_GetUmask(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);
    dwResult = pState->config.dwUmask;
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwResult;
}

DWORD
AD_GetSkelDirs(
    PLSA_AD_PROVIDER_STATE pState,
    PSTR* ppszSkelDirs
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszSkelDirs = NULL;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    if (!LW_IS_NULL_OR_EMPTY_STR(pState->config.pszSkelDirs))
    {
        dwError = LwAllocateString(
                        pState->config.pszSkelDirs,
                        &pszSkelDirs);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszSkelDirs = pszSkelDirs;

cleanup:

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwError;

error:

    *ppszSkelDirs = NULL;

    goto cleanup;
}

BOOLEAN
AD_GetLDAPSignAndSeal(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bResult = pState->config.bLDAPSignAndSeal;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bResult;
}

static
BOOLEAN
AD_IsInMembersList_InLock(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszMember
    )
{
    PLW_DLINKED_LIST pIter = NULL;
    BOOLEAN      bInList = FALSE;

    for (pIter = pState->config.pUnresolvedMemberList;
         pIter;
         pIter = pIter->pNext)
    {
        if (!strcmp(pszMember, (PSTR)pIter->pItem))
        {
            bInList = TRUE;
            break;
        }
    }

    return bInList;
}

static
VOID
AD_DeleteFromMembersList_InLock(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszMember
    )
{
    PLW_DLINKED_LIST pIter = NULL;
    PVOID        pItem = NULL;

    for (pIter = pState->config.pUnresolvedMemberList;
         pIter;
         pIter = pIter->pNext)
    {
        if (!strcmp(pszMember, (PSTR)pIter->pItem))
        {
            pItem = pIter->pItem;
            break;
        }
    }

    if (pItem)
    {
        LwDLinkedListDelete(&pState->config.pUnresolvedMemberList,
                             pItem);

        LwFreeMemory(pItem);
    }
}

VOID
AD_DeleteFromMembersList(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszMember
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_WRITER_LOCK(bInLock, pState);

    AD_DeleteFromMembersList_InLock(pState, pszMember);

    LEAVE_AD_CONFIG_RW_WRITER_LOCK(bInLock, pState);
}

static
void
AD_FreeHashStringKeyValue(
    const LW_HASH_ENTRY *pEntry)
{
    PSTR pszKeyCopy = (PSTR)pEntry->pKey;
    LW_SAFE_FREE_STRING(pszKeyCopy);
    PSTR pszValueCopy = (PSTR)pEntry->pValue;
    LW_SAFE_FREE_STRING(pszValueCopy);
}

static
DWORD
AD_CopyHashStringKeyValue(
    const LW_HASH_ENTRY *pEntry,
    LW_HASH_ENTRY       *pEntryCopy
    )
{
    DWORD dwError = 0;
    PSTR  pszKeyCopy = NULL;
    PSTR  pszValueCopy = NULL;

    dwError = LwAllocateString(
                    (PSTR)pEntry->pKey,
                    &pszKeyCopy);
    BAIL_ON_LSA_ERROR(dwError);
    dwError = LwAllocateString(
                    (PSTR)pEntry->pValue,
                    &pszValueCopy);
    BAIL_ON_LSA_ERROR(dwError);

    pEntryCopy->pKey = pszKeyCopy;
    pEntryCopy->pValue = pszValueCopy;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszKeyCopy);

    goto cleanup;
}

DWORD
AD_AddAllowedMember(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR               pszSID,
    IN PSTR                pszMember,
    IN OUT PLW_HASH_TABLE *ppAllowedMemberList
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR  pszValue = NULL;
    PSTR  pszSIDCopy = NULL;
    PSTR  pszMemberCopy = NULL;
    PLW_HASH_TABLE pAllowedMemberList = *ppAllowedMemberList;

    ENTER_AD_CONFIG_RW_WRITER_LOCK(bInLock, pState);

    if (!pState->pAllowedSIDs)
    {
        dwError = LwHashCreate(
                        11,
                        LwHashCaselessStringCompare,
                        LwHashCaselessStringHash,
                        AD_FreeHashStringKeyValue,
                        AD_CopyHashStringKeyValue,
                        &pState->pAllowedSIDs);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pAllowedMemberList)
    {
        dwError = LwHashCreate(
                        11,
                        LwHashCaselessStringCompare,
                        LwHashCaselessStringHash,
                        AD_FreeHashStringKeyValue,
                        AD_CopyHashStringKeyValue,
                        &pAllowedMemberList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(
                    pszSID,
                    &pszSIDCopy);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    pszMember,
                    &pszMemberCopy);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashSetValue(
                    pAllowedMemberList,
                    pszSIDCopy,
                    pszMemberCopy);
    BAIL_ON_LSA_ERROR(dwError);

    pszSIDCopy = NULL;
    pszMemberCopy = NULL;

    if ( AD_IsInMembersList_InLock(pState, pszMember) )
    {
        dwError = LwHashGetValue(
                      pState->pAllowedSIDs,
                      pszSID,
                      (PVOID*)&pszValue);
        if (dwError == ERROR_NOT_FOUND)
        {
            dwError = LwAllocateString(
                          pszSID,
                          &pszSIDCopy);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(
                          pszMember,
                          &pszMemberCopy);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwHashSetValue(
                          pState->pAllowedSIDs,
                          pszSIDCopy,
                          pszMemberCopy);
            BAIL_ON_LSA_ERROR(dwError);

            pszSIDCopy = NULL;
            pszMemberCopy = NULL;
        }

        AD_DeleteFromMembersList_InLock(pState, pszMember);
    }

    *ppAllowedMemberList = pAllowedMemberList;

cleanup:

    LW_SAFE_FREE_STRING(pszSIDCopy);
    LW_SAFE_FREE_STRING(pszMemberCopy);

    LEAVE_AD_CONFIG_RW_WRITER_LOCK(bInLock, pState);

    return dwError;

error:

    if ( ! *ppAllowedMemberList )
    {
        LwHashSafeFree(&pAllowedMemberList);
    }

    goto cleanup;
}

DWORD
AD_CheckIgnoreUserNameList(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszUserName,
    PBOOLEAN pbFoundIt
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bFoundIt = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    dwError = AD_CheckList(
                    pszUserName,
                    pState->config.pszaIgnoreUserNameList,
                    &bFoundIt);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    *pbFoundIt = bFoundIt;

    return dwError;

error:
    goto cleanup;
}

DWORD
AD_CheckIgnoreGroupNameList(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszGroupName,
    PBOOLEAN pbFoundIt
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bFoundIt = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    dwError = AD_CheckList(
                    pszGroupName,
                    pState->config.pszaIgnoreGroupNameList,
                    &bFoundIt);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    *pbFoundIt = bFoundIt;

    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_CheckList(
    PCSTR pszNeedle,
    PCSTR pszaHaystack,
    PBOOLEAN pbFoundIt
    )
{
    DWORD dwError = 0;
    BOOLEAN bFoundIt = FALSE;
    PCSTR pszCurrent = NULL;

    BAIL_ON_INVALID_STRING(pszNeedle);

    pszCurrent = pszaHaystack;
    if (pszCurrent)
    {
        while (*pszCurrent)
        {
            if (!strcasecmp(pszCurrent, pszNeedle))
            {
                bFoundIt = TRUE;
                goto cleanup;
            }
            pszCurrent = pszCurrent + strlen(pszCurrent) + 1;
        }
    }

cleanup:
    *pbFoundIt = bFoundIt;
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_GetMemberLists(
    IN PLSA_AD_PROVIDER_STATE pState,
    PSTR** pppszMembers,
    PDWORD pdwNumMembers,
    PLW_HASH_TABLE* ppAllowedMemberList
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumMembers = 0;
    PLW_DLINKED_LIST pIter = NULL;
    PSTR* ppszMembers = NULL;
    PLW_HASH_TABLE pAllowedMemberList = NULL;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    for (pIter = pState->config.pUnresolvedMemberList; pIter; pIter = pIter->pNext)
    {
        dwNumMembers++;
    }

    if (dwNumMembers)
    {
        DWORD iMember = 0;

        dwError = LwAllocateMemory(
                        dwNumMembers * sizeof(PSTR),
                        (PVOID*)&ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);

        for (pIter = pState->config.pUnresolvedMemberList;
             pIter;
             pIter = pIter->pNext, iMember++)
        {
            dwError = LwAllocateString(
                            (PSTR)pIter->pItem,
                            &ppszMembers[iMember]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pState->pAllowedSIDs)
    {
        dwError = LwHashCopy(
                      pState->pAllowedSIDs,
                      &pAllowedMemberList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppszMembers = ppszMembers;
    *pdwNumMembers = dwNumMembers;
    *ppAllowedMemberList = pAllowedMemberList;

cleanup:

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwError;

error:

    if (ppszMembers)
    {
        LwFreeStringArray(ppszMembers, dwNumMembers);
    }

    *pppszMembers = NULL;
    *pdwNumMembers = 0;
    *ppAllowedMemberList = NULL;

    LwHashSafeFree(&pAllowedMemberList);

    goto cleanup;
}

static
BOOLEAN
AD_ShouldFilterUserLoginsByGroup_InLock(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bFilter = FALSE;

    if (pState->pAllowedSIDs || pState->config.pUnresolvedMemberList)
    {
        bFilter = TRUE;
    }

    return bFilter;
}

BOOLEAN
AD_ShouldFilterUserLoginsByGroup(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bFilter = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bFilter = AD_ShouldFilterUserLoginsByGroup_InLock(pState);

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bFilter;
}

BOOLEAN
AD_IsMemberAllowed(
    IN PLSA_AD_PROVIDER_STATE pState,
    PCSTR           pszSID,
    PLW_HASH_TABLE pAllowedMemberList
    )
{
    BOOLEAN bAllowed = FALSE;
    PSTR    pszValue = NULL;

    if (!AD_ShouldFilterUserLoginsByGroup(pState) ||
        (pAllowedMemberList &&
         !LwHashGetValue(
                        pAllowedMemberList,
                        pszSID,
                        (PVOID*)&pszValue)))
    {
        bAllowed = TRUE;
    }

    return bAllowed;
}

VOID
AD_FreeAllowedSIDs_InLock(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    if (pState->pAllowedSIDs)
    {
        LwHashSafeFree(&pState->pAllowedSIDs);
    }
}

BOOLEAN
AD_ShouldAssumeDefaultDomain(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bAssumeDefaultDomain = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bAssumeDefaultDomain = pState->config.bAssumeDefaultDomain;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bAssumeDefaultDomain;
}

BOOLEAN
AD_ShouldSyncSystemTime(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bSyncSystemTime = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bSyncSystemTime = pState->config.bSyncSystemTime;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bSyncSystemTime;
}

BOOLEAN
AD_EventlogEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bResult = pState->config.bEnableEventLog;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bResult;
}

BOOLEAN
AD_ShouldLogNetworkConnectionEvents(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bResult = TRUE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bResult = pState->config.bShouldLogNetworkConnectionEvents;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bResult;
}

BOOLEAN
AD_ShouldCreateK5Login(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bResult = TRUE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bResult = pState->config.bCreateK5Login;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bResult;
}

BOOLEAN
AD_ShouldCreateHomeDir(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bCreateHomeDir = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bCreateHomeDir = pState->config.bCreateHomeDir;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bCreateHomeDir;
}

BOOLEAN
AD_ShouldRefreshUserCreds(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bRefreshUserCreds = FALSE;
    BOOLEAN bInLock          = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    bRefreshUserCreds = pState->config.bRefreshUserCreds;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return bRefreshUserCreds;
}

AD_CELL_SUPPORT
AD_GetCellSupport(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    AD_CELL_SUPPORT result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    result = pState->config.CellSupport;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

AD_CACHE_BACKEND
AD_GetCacheBackend(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    AD_CACHE_BACKEND result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    result = pState->config.CacheBackend;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

DWORD
AD_GetCacheSizeCap(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    dwResult = pState->config.dwCacheSizeCap;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwResult;
}

BOOLEAN
AD_GetTrimUserMembershipEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    result = pState->config.bTrimUserMembershipEnabled;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

BOOLEAN
AD_GetNssGroupMembersCacheOnlyEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    result = pState->config.bNssGroupMembersCacheOnlyEnabled;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

BOOLEAN
AD_GetNssUserMembershipCacheOnlyEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    result = pState->config.bNssUserMembershipCacheOnlyEnabled;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

BOOLEAN
AD_GetNssEnumerationEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    result = pState->config.bNssEnumerationEnabled;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

DWORD
AD_GetDomainManagerCheckDomainOnlineSeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);
    result = pState->config.DomainManager.dwCheckDomainOnlineSeconds;
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

DWORD
AD_GetDomainManagerUnknownDomainCacheTimeoutSeconds(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);
    result = pState->config.DomainManager.dwUnknownDomainCacheTimeoutSeconds;
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

DWORD
AD_GetDomainManagerTrustExceptionList(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT PBOOLEAN pbIgnoreAllTrusts,
    OUT PSTR** pppszTrustsList,
    OUT PDWORD pdwTrustsCount
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);
    *pbIgnoreAllTrusts = pState->config.DomainManager.bIgnoreAllTrusts;
    dwError = LwDuplicateStringArray(
                    pppszTrustsList,
                    pdwTrustsCount,
                    pState->config.DomainManager.ppszTrustExceptionList,
                    pState->config.DomainManager.dwTrustExceptionCount);
    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return dwError;
}

BOOLEAN
AD_GetAddDomainToLocalGroupsEnabled(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    result = pState->config.bAddDomainToLocalGroupsEnabled;

    LEAVE_AD_CONFIG_RW_READER_LOCK(bInLock, pState);

    return result;
}

VOID
AD_ConfigLockAcquireRead(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    int status = 0;

    status = pthread_rwlock_rdlock(pState->pConfigLock);
    LW_ASSERT(status == 0);
}

VOID
AD_ConfigLockAcquireWrite(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    int status = 0;

    status = pthread_rwlock_wrlock(pState->pConfigLock);
    LW_ASSERT(status == 0);
}

void
AD_ConfigLockRelease(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    int status = 0;

    status = pthread_rwlock_unlock(pState->pConfigLock);
    LW_ASSERT(status == 0);
}
