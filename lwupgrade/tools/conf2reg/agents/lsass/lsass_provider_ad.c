/*
 * Copyright (c) Likewise Software.  All rights reserved.
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

#include "includes.h"

static
BOOLEAN
UpAdGetBooleanConfigValue(
    PCSTR pszValue
    );

static
DWORD
UpAdSetConfig_EnableEventLog(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_LoginShellTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_HomeDirTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_CheckPunctuationChar(
    IN PCSTR pszName,
    IN PCSTR pszValue,
    IN BOOLEAN bAllowSpace,
    OUT PCHAR pchValue
    );

static
DWORD
UpAdSetConfig_SpaceReplacement(
    IN OUT PLSA_AD_CONFIG pConfig,
    IN PCSTR          pszName,
    IN PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_DomainSeparator(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_CachePurgeTimeout(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_CacheEntryExpiry(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_CacheSizeCap(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_LDAPSignAndSeal(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_RequireMembershipOf(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_AssumeDefaultDomain(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_SyncSystemTime(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_LogNetworkConnectionEvents(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_CreateK5Login(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_CreateHomeDir(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_RefreshUserCreds(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_SkelDirs(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_Umask(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_HomedirPrefix(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_CellSupport(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_CacheType(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_TrimUserMembershipEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_NssGroupMembersCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_NssUserMembershipCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_NssEnumerationEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
UpAdSetConfig_DomainManagerCheckDomainOnlineSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    );

static
DWORD
UpAdSetConfig_DomainManagerUnknownDomainCacheTimeoutSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    );

typedef DWORD (*PFN_AD_CONFIG_HANDLER)(
                    PLSA_AD_CONFIG pConfig,
                    PCSTR          pszName,
                    PCSTR          pszValue
                    );

typedef struct __AD_CONFIG_HANDLER
{
    PCSTR                 pszId;
    PFN_AD_CONFIG_HANDLER pfnHandler;
} AD_CONFIG_HANDLER, *PAD_CONFIG_HANDLER;

static AD_CONFIG_HANDLER gADConfigHandlers[] =
{
    {"enable-eventlog",               &UpAdSetConfig_EnableEventLog},
    {"login-shell-template",          &UpAdSetConfig_LoginShellTemplate},
    {"homedir-template",              &UpAdSetConfig_HomeDirTemplate},
    {"space-replacement",             &UpAdSetConfig_SpaceReplacement},
    {"domain-separator",              &UpAdSetConfig_DomainSeparator},
    {"cache-purge-timeout",           &UpAdSetConfig_CachePurgeTimeout},
    {"machine-password-lifespan",     &UpAdSetConfig_MachinePasswordLifespan},
    {"cache-entry-expiry",            &UpAdSetConfig_CacheEntryExpiry},
    {"memory-cache-size-cap",         &UpAdSetConfig_CacheSizeCap},
    {"ldap-sign-and-seal",            &UpAdSetConfig_LDAPSignAndSeal},
    {"require-membership-of",         &UpAdSetConfig_RequireMembershipOf},
    {"assume-default-domain",         &UpAdSetConfig_AssumeDefaultDomain},
    {"sync-system-time",              &UpAdSetConfig_SyncSystemTime},
    {"log-network-connection-events", &UpAdSetConfig_LogNetworkConnectionEvents},
    {"create-k5login",                &UpAdSetConfig_CreateK5Login},
    {"create-homedir",                &UpAdSetConfig_CreateHomeDir},
    {"skeleton-dirs",                 &UpAdSetConfig_SkelDirs},
    {"homedir-umask",                 &UpAdSetConfig_Umask},
    {"homedir-prefix",                &UpAdSetConfig_HomedirPrefix},
    {"refresh-user-credentials",      &UpAdSetConfig_RefreshUserCreds},
    {"cell-support",                  &UpAdSetConfig_CellSupport},
    {"trim-user-membership",          &UpAdSetConfig_TrimUserMembershipEnabled},
    {"nss-group-members-query-cache-only",   &UpAdSetConfig_NssGroupMembersCacheOnlyEnabled},
    {"nss-user-membership-query-cache-only", &UpAdSetConfig_NssUserMembershipCacheOnlyEnabled},
    {"nss-enumeration-enabled",              &UpAdSetConfig_NssEnumerationEnabled},
    {"domain-manager-check-domain-online-interval", &UpAdSetConfig_DomainManagerCheckDomainOnlineSeconds},
    {"domain-manager-unknown-domain-cache-timeout", &UpAdSetConfig_DomainManagerUnknownDomainCacheTimeoutSeconds},
    {"cache-type",                    &UpAdSetConfig_CacheType},
};


DWORD
UpAdInitializeConfig(
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
    /* Leave chSpaceReplacement and chDomainSeparator set as '\0' for now.
     * After the config file is parsed, they will be assigned default values
     * if they are still set to '\0'.
     *
     * This is done so that their values
     * can be swapped (chSpaceReplacement=\ chDomainSeparator=^), but not
     * assigned to the same value.
     */
    pConfig->dwCacheReaperTimeoutSecs = AD_CACHE_REAPER_TIMEOUT_DEFAULT_SECS;
    pConfig->dwCacheEntryExpirySecs   = AD_CACHE_ENTRY_EXPIRY_DEFAULT_SECS;
    pConfig->dwCacheSizeCap           = 0;
    pConfig->dwMachinePasswordSyncLifetime = AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS;
    dwError = LwAllocateString(AD_DEFAULT_UMASK, &pConfig->pszUmask);
    BAIL_ON_UP_ERROR(dwError);

    pConfig->bEnableEventLog = FALSE;
    pConfig->bShouldLogNetworkConnectionEvents = TRUE;
    pConfig->bRefreshUserCreds = TRUE;

    //pConfig->CellSupport = AD_CELL_SUPPORT_FULL;
    dwError = LwAllocateString("full", &pConfig->pszCellSupport);
    BAIL_ON_UP_ERROR(dwError);

    //pConfig->CacheBackend = AD_CACHE_SQLITE;
    dwError = LwAllocateString("sqlite", &pConfig->pszCacheBackend);
    BAIL_ON_UP_ERROR(dwError);

    pConfig->bTrimUserMembershipEnabled = TRUE;
    pConfig->bNssGroupMembersCacheOnlyEnabled = TRUE;
    pConfig->bNssUserMembershipCacheOnlyEnabled = FALSE;
    pConfig->bNssEnumerationEnabled = FALSE;

    pConfig->DomainManager.dwCheckDomainOnlineSeconds = 5 * UP_SECONDS_IN_MINUTE;
    pConfig->DomainManager.dwUnknownDomainCacheTimeoutSeconds = 1 * UP_SECONDS_IN_HOUR;

    dwError = LwAllocateString(
                    AD_DEFAULT_SHELL,
                    &pConfig->pszShell);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_HOMEDIR_PREFIX,
                    &pConfig->pszHomedirPrefix);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_HOMEDIR_TEMPLATE,
                    &pConfig->pszHomedirTemplate);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_SKELDIRS,
                    &pConfig->pszSkelDirs);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    return dwError;

error:

    UpAdFreeConfigContents(pConfig);

    goto cleanup;
}

VOID
UpAdFreeConfigContents(
    PLSA_AD_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    LW_SAFE_FREE_STRING(pConfig->pszShell);
    LW_SAFE_FREE_STRING(pConfig->pszSkelDirs);
    LW_SAFE_FREE_STRING(pConfig->pszUmask);
    LW_SAFE_FREE_STRING(pConfig->pszCellSupport);
    LW_SAFE_FREE_STRING(pConfig->pszCacheBackend);

    if (pConfig->pszUnresolvedMemberList)
    {
        LW_SAFE_FREE_MEMORY(pConfig->pszUnresolvedMemberList);
        pConfig->pszUnresolvedMemberList = NULL;
    }
}


DWORD
UpAdSectionHandler(
    BOOLEAN         bStartOfSection,
    PCSTR           pszSectionName,
    PLSA_AD_CONFIG  pConfig,
    PBOOLEAN        pbContinue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if (!bStartOfSection)
    {
        if (pConfig->chSpaceReplacement == 0)
        {
            pConfig->chSpaceReplacement = AD_SPACE_REPLACEMENT_DEFAULT;
        }
        if (pConfig->chDomainSeparator == 0)
        {
            pConfig->chDomainSeparator = LSA_DOMAIN_SEPARATOR_DEFAULT;
        }

        if (pConfig->chSpaceReplacement == pConfig->chDomainSeparator)
        {
            LOG_ERROR("Error: space-replacement and domain-separator are set to '%c' in the config file. Their values must be unique.",
                            pConfig->chSpaceReplacement);
            dwError = LW_ERROR_INVALID_CONFIG;
            BAIL_ON_UP_ERROR(dwError);
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
UpAdConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PLSA_AD_CONFIG pConfig,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszName))
    {
        DWORD iHandler = 0;
        DWORD nHandlers = sizeof(gADConfigHandlers)/sizeof(gADConfigHandlers[0]);

        for (; iHandler < nHandlers; iHandler++)
        {
            if (!strcasecmp(gADConfigHandlers[iHandler].pszId, pszName))
            {
                gADConfigHandlers[iHandler].pfnHandler(
                                pConfig,
                                pszName,
                                pszValue);
                break;
            }
        }
    }

    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
UpAdSetConfig_EnableEventLog(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bEnableEventLog = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_LoginShellTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszShell = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    if (access(pszValue, X_OK) != 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    dwError = LwAllocateString(
                    pszValue,
                    &pszShell);
    BAIL_ON_UP_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszShell);

    pConfig->pszShell = pszShell;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszShell);

    goto cleanup;
}

static
DWORD
UpAdSetConfig_HomeDirTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszTemplate = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = LwAllocateString(
                    pszValue,
                    &pszTemplate);
    BAIL_ON_UP_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);

    pConfig->pszHomedirTemplate = pszTemplate;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszTemplate);

    goto cleanup;
}

static
DWORD
AD_CheckPunctuationChar(
    IN PCSTR pszName,
    IN PCSTR pszValue,
    IN BOOLEAN bAllowSpace,
    OUT PCHAR pchValue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CHAR chValue = 0;

    BAIL_ON_INVALID_STRING(pszValue);

    if ((pszValue[0] == 0) ||
        ((pszValue[0] == '\'' || pszValue[0] == '"') && 
            (pszValue[1] == 0 ||
             pszValue[2] != pszValue[0] ||
             pszValue[3] != 0)) ||
        (pszValue[0] != '\'' && pszValue[0] != '"' && pszValue[1] != 0))
    {
        LOG_ERROR(
                "Error: '%s' is an invalid setting for %s. "
                "%s may only be set to a single character.",
                pszValue,
                pszName,
                pszName);
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_UP_ERROR(dwError);
    }

    if (pszValue[0] == '\'' || pszValue[0] == '"')
    {
        chValue = pszValue[1];
    }
    else
    {
        chValue = pszValue[0];
    }

    if (!ispunct((int)chValue) && !(bAllowSpace && chValue == ' '))
    {
        LOG_ERROR(
                "Error: %s must be set to a punctuation character%s; "
                "the value provided is '%s'.",
                pszName,
                bAllowSpace ? " or space" : "",
                pszValue);
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_UP_ERROR(dwError);
    }

    if (chValue == '@')
    {
        LOG_ERROR(
                "Error: %s may not be set to @; the value provided is '%s'.",
                pszName,
                pszValue);
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_UP_ERROR(dwError);
    }

    *pchValue = chValue;

cleanup:

    return dwError;

error:

    *pchValue = 0;

    goto cleanup;
}

static
DWORD
UpAdSetConfig_SpaceReplacement(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    CHAR  result = 0;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = AD_CheckPunctuationChar(
                    pszName,
                    pszValue,
                    TRUE,
                    &result);
    BAIL_ON_UP_ERROR(dwError);

    pConfig->chSpaceReplacement = result;

error:

    return dwError;
}

static
DWORD
UpAdSetConfig_DomainSeparator(
    IN OUT PLSA_AD_CONFIG pConfig,
    IN PCSTR          pszName,
    IN PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    CHAR  result = 0;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = AD_CheckPunctuationChar(
                    pszName,
                    pszValue,
                    FALSE,
                    &result);
    BAIL_ON_UP_ERROR(dwError);

    pConfig->chDomainSeparator = result;

error:

    return dwError;
}

static
DWORD
UpAdSetConfig_CachePurgeTimeout(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwCacheReaperTimeoutSecs = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = UpParseDateString(
                        pszValue,
                        &dwCacheReaperTimeoutSecs);
        BAIL_ON_UP_ERROR(dwError);
    }

    if (dwCacheReaperTimeoutSecs < AD_CACHE_REAPER_TIMEOUT_MINIMUM_SECS)
    {
        LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Minimum is %u.",
                      dwCacheReaperTimeoutSecs,
                      AD_CACHE_REAPER_TIMEOUT_MINIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    if (dwCacheReaperTimeoutSecs > AD_CACHE_REAPER_TIMEOUT_MAXIMUM_SECS)
    {
        LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Maximum is %u.",
                      dwCacheReaperTimeoutSecs,
                      AD_CACHE_REAPER_TIMEOUT_MAXIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    pConfig->dwCacheReaperTimeoutSecs = dwCacheReaperTimeoutSecs;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
UpAdSetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwMachinePasswordSyncPwdLifetime = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = UpParseDateString(
                        pszValue,
                        &dwMachinePasswordSyncPwdLifetime);
        BAIL_ON_UP_ERROR(dwError);
    }

    if ((dwMachinePasswordSyncPwdLifetime != 0) &&
        (dwMachinePasswordSyncPwdLifetime < AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS))
    {
        LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Minimum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    if (dwMachinePasswordSyncPwdLifetime > AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS)
    {
        LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Maximum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    pConfig->dwMachinePasswordSyncLifetime = dwMachinePasswordSyncPwdLifetime;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
UpAdSetConfig_CacheEntryExpiry(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwExpirySecs = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = UpParseDateString(
                    pszValue,
                    &dwExpirySecs);
        BAIL_ON_UP_ERROR(dwError);
    }

    if (dwExpirySecs < AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS)
    {
        LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Minimum is %u.",
                        dwExpirySecs,
                        AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    if (dwExpirySecs > AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS)
    {
        LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Maximum is %u.",
                        dwExpirySecs,
                        AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    pConfig->dwCacheEntryExpirySecs = dwExpirySecs;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
UpAdSetConfig_CacheSizeCap(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwValue = (DWORD) atoi(pszValue);
    }

    pConfig->dwCacheSizeCap = dwValue;

    return dwError;
}

static
DWORD
UpAdSetConfig_LDAPSignAndSeal(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bLDAPSignAndSeal = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_RequireMembershipOf(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto cleanup;
    }

    dwError = UpStringToMultiString(
                pszValue,
                ",",
                &pConfig->pszUnresolvedMemberList);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
UpAdSetConfig_AssumeDefaultDomain(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bAssumeDefaultDomain = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_SyncSystemTime(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bSyncSystemTime = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_LogNetworkConnectionEvents(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bShouldLogNetworkConnectionEvents = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_CreateK5Login(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bCreateK5Login = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_CreateHomeDir(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bCreateHomeDir = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_RefreshUserCreds(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bRefreshUserCreds = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_SkelDirs(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszSkelDirs = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(
                      pszValue,
                      &pszSkelDirs);
        BAIL_ON_UP_ERROR(dwError);
    }

    LW_SAFE_FREE_STRING(pConfig->pszSkelDirs);

    pConfig->pszSkelDirs = pszSkelDirs;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszSkelDirs);

    goto cleanup;
}

static
DWORD
UpAdSetConfig_Umask(
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
    PSTR pszUmask = NULL;

    // Convert the umask octal string to a decimal number

    cp2[1] = 0;

    for (cp = pszValue, dwCnt = 0; isdigit((int)*cp); cp++, dwCnt++)
    {
        dwOct *= 8;

        cp2[0] = *cp;
        dwVal = atoi(cp2);

        if (dwVal > 7)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_UP_ERROR(dwError);

        dwOct += dwVal;
    }

    if (dwCnt > 4)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_UP_ERROR(dwError);

    // Disallow 07xx since the user should always have
    // access to his home directory.
    if ((dwOct & 0700) == 0700)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(pszValue, &pszUmask);
    BAIL_ON_UP_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszUmask);
    pConfig->pszUmask = pszUmask;
    pszUmask = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszUmask);
    return dwError;

error:

    goto cleanup;
}

static
DWORD
UpAdSetConfig_HomedirPrefix(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR pszHomedirPrefix = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = LwAllocateString(
                pszValue,
                &pszHomedirPrefix);
    BAIL_ON_UP_ERROR(dwError);

    LwStripWhitespace(pszHomedirPrefix, TRUE, TRUE);

    BAIL_ON_INVALID_STRING(pszHomedirPrefix);

    if (*pszHomedirPrefix != '/')
    {
        LOG_ERROR("Invalid home directory prefix [%s]", pszHomedirPrefix);
        goto error;
    }

    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    pConfig->pszHomedirPrefix = pszHomedirPrefix;

cleanup:

    return 0;

error:

    LW_SAFE_FREE_STRING(pszHomedirPrefix);

    goto cleanup;
}

static
DWORD
UpAdSetConfig_CellSupport(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    if (!strcasecmp(pszValue, "unprovisioned") ||
        !strcasecmp(pszValue, "full") ||
        !strcasecmp(pszValue, "default-schema") )
    {
        dwError = LwAllocateString(pszValue, &pConfig->pszCellSupport);
        BAIL_ON_UP_ERROR(dwError);
    }
    else
    {
        LOG_ERROR("Invalid value for cell-support parameter");
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
UpAdSetConfig_TrimUserMembershipEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bTrimUserMembershipEnabled = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_NssGroupMembersCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bNssGroupMembersCacheOnlyEnabled = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_NssUserMembershipCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bNssUserMembershipCacheOnlyEnabled = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_NssEnumerationEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bNssEnumerationEnabled = UpAdGetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
UpAdSetConfig_CacheType(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;

    if (!strcasecmp(pszValue, "sqlite") ||
        !strcasecmp(pszValue, "memory") )
    {
        dwError = LwAllocateString(
                        pszValue,
                        &pConfig->pszCacheBackend);
        BAIL_ON_UP_ERROR(dwError);
    }
    else
    {
        LOG_ERROR("Invalid value for cache-type parameter");
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
UpAdSetConfig_DomainManagerCheckDomainOnlineSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    )
{
    DWORD dwError = 0;
    DWORD result = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = UpParseDateString(pszValue, &result);
        BAIL_ON_UP_ERROR(dwError);
    }

    pConfig->DomainManager.dwCheckDomainOnlineSeconds = result;

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
UpAdSetConfig_DomainManagerUnknownDomainCacheTimeoutSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    )
{
    DWORD dwError = 0;
    DWORD result = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = UpParseDateString(pszValue, &result);
        BAIL_ON_UP_ERROR(dwError);
    }

    pConfig->DomainManager.dwUnknownDomainCacheTimeoutSeconds = result;

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
BOOLEAN
UpAdGetBooleanConfigValue(
    PCSTR pszValue
    )
{
    BOOLEAN bResult = FALSE;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
        (!strcasecmp(pszValue, "true") ||
         !strcasecmp(pszValue, "1") ||
         (*pszValue == 'y') ||
         (*pszValue == 'Y')))
    {
        bResult = TRUE;
    }

    return bResult;
}



DWORD
UpAdPrintConfig(
    FILE *fp,
    PLSA_AD_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    char szString[2]; // Used to help print CHAR through UpPrintString.

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters\\Providers\\ActiveDirectory]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    //dwError = UpPrintBoolean(fp, "EnableEventlog", pConfig->bEnableEventLog);
    //BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "LogNetworkConnectionEvents", pConfig->bShouldLogNetworkConnectionEvents);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "LoginShellTemplate", pConfig->pszShell);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "HomeDirPrefix", pConfig->pszHomedirPrefix);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "HomeDirTemplate", pConfig->pszHomedirTemplate);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "LdapSignAndSeal", pConfig->bLDAPSignAndSeal);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "CacheEntryExpiry", pConfig->dwCacheEntryExpirySecs);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "MachinePasswordLifespan", pConfig->dwMachinePasswordSyncLifetime); 
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "AssumeDefaultDomain", pConfig->bAssumeDefaultDomain);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "SyncSystemTime",    pConfig->bSyncSystemTime);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "CachePurgeTimeout", pConfig->dwCacheReaperTimeoutSecs);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintMultiString(fp, "RequireMembershipOf", pConfig->pszUnresolvedMemberList);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "CreateK5Login", pConfig->bCreateK5Login);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "CreateHomeDir", pConfig->bCreateHomeDir);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "HomeDirUmask", pConfig->pszUmask);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "SkeletonDirs", pConfig->pszSkelDirs);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "RefreshUserCredentials", pConfig->bRefreshUserCreds);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "CellSupport", pConfig->pszCellSupport);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "TrimUserMembership", pConfig->bTrimUserMembershipEnabled);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "NssGroupMembersQueryCacheOnly", pConfig->bNssGroupMembersCacheOnlyEnabled);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "NssUserMembershipQueryCacheOnly", pConfig->bNssUserMembershipCacheOnlyEnabled);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "NssEnumerationEnabled", pConfig->bNssEnumerationEnabled);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "DomainManagerCheckDomainOnlineInterval", pConfig->DomainManager.dwCheckDomainOnlineSeconds);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "DomainManagerUnknownDomainCacheTimeout", pConfig->DomainManager.dwUnknownDomainCacheTimeoutSeconds);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "CacheType", pConfig->pszCacheBackend);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "MemoryCacheSizeCap", pConfig->dwCacheSizeCap);
    BAIL_ON_UP_ERROR(dwError);

    fputs("\n", fp);

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    szString[0] = pConfig->chSpaceReplacement;
    szString[1] = '\0';
    dwError = UpPrintString(fp, "SpaceReplacement", szString);
    BAIL_ON_UP_ERROR(dwError);

    szString[0] = pConfig->chDomainSeparator;
    szString[1] = '\0';
    dwError = UpPrintString(fp, "DomainSeparator", szString);
    BAIL_ON_UP_ERROR(dwError);

    fputs("\n", fp);

error:
    return dwError;
}

