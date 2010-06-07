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
 *        config.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Configuration API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "api.h"


static
DWORD
LsaSrvModifyMachineDC(
    PCSTR pszDN,
    PCSTR pszMachineName,
    PCSTR pszNewMachineName,
    PSTR  *ppszNewDN
    );


DWORD
LsaSrvRefreshConfiguration(
    HANDLE hServer
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    BOOLEAN bUnlockConfigLock = FALSE;
    LSA_SRV_API_CONFIG apiConfig;

    if (pServerState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvApiInitConfig(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiReadRegistry(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_lock(&gAPIConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    &gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_unlock(&gAPIConfigLock);
    bUnlockConfigLock = FALSE;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable2->pfnRefreshConfiguration(
                                        hProvider);
        if (dwError)
        {
            LSA_LOG_ERROR("Refreshing provider %s failed.",
                          pProvider->pszName ? pProvider->pszName : "");
        }

        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = (HANDLE)NULL;
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LsaSrvApiFreeConfigContents(&apiConfig);

    if (bUnlockConfigLock)
    {
        pthread_mutex_unlock(&gAPIConfigLock);
    }

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "refresh configuration");

    goto cleanup;

}

DWORD
LsaSrvApiInitConfig(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    LsaSrvApiFreeConfigContents(pConfig);

    pConfig->bEnableEventLog = FALSE;
    pConfig->bLogNetworkConnectionEvents = TRUE;

    return 0;
}

DWORD
LsaSrvApiReadRegistry(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    LSA_SRV_API_CONFIG StagingConfig;
    LSA_CONFIG Config[] =
    {
        {
           "EnableEventlog",
           TRUE,
           LsaTypeBoolean,
           0,
           MAXDWORD,
           NULL,
           &StagingConfig.bEnableEventLog
        },
        {
           "LogNetworkConnectionEvents",
           TRUE,
           LsaTypeBoolean,
           0,
           MAXDWORD,
           NULL,
           &StagingConfig.bLogNetworkConnectionEvents
        }
    };

    memset(&StagingConfig, 0, sizeof(StagingConfig));
    dwError = LsaSrvApiInitConfig(&StagingConfig);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaProcessConfig(
                "Services\\lsass\\Parameters",
                "Policy\\Services\\lsass\\Parameters",
                Config,
                sizeof(Config)/sizeof(Config[0]));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiTransferConfigContents(
                    &StagingConfig,
                    pConfig);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    
    LsaSrvApiFreeConfigContents(&StagingConfig);

    return dwError;

error:

    goto cleanup;
}

#if 0
static
DWORD
ValidateAndSetLogLevel(
    PCSTR               pszName,
    PCSTR               pszValue
    )
{
    DWORD dwError = 0;

    LSA_LOG_INFO LogInfo = {};
    
    if (!strcasecmp(pszValue, "error"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;
    }
    else if (!strcasecmp(pszValue, "warning"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_WARNING;
    }
    else if (!strcasecmp(pszValue, "info"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_INFO;
    }
    else if (!strcasecmp(pszValue, "verbose"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_VERBOSE;
    }
    else if (!strcasecmp(pszValue, "debug"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_DEBUG;
    }
    else if (!strcasecmp(pszValue, "trace"))
    {
        LogInfo.maxAllowedLogLevel = LSA_LOG_LEVEL_TRACE;
    }
    else
    {
        LSA_LOG_ERROR("Invalid value for %s.",
                      pszName);
        dwError = LW_ERROR_INVALID_LOG_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLogSetInfo_r(&LogInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}
#endif

DWORD
LsaSrvApiTransferConfigContents(
    PLSA_SRV_API_CONFIG pSrc,
    PLSA_SRV_API_CONFIG pDest
    )
{
    LsaSrvApiFreeConfigContents(pDest);

    *pDest = *pSrc;

    LsaSrvApiFreeConfigContents(pSrc);

    return 0;
}

VOID
LsaSrvApiFreeConfigContents(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    // Nothing to free right now
    memset(pConfig, 0, sizeof(*pConfig));
}

BOOLEAN
LsaSrvEventlogEnabled(
    VOID
    )
{
    BOOLEAN bResult = FALSE;

    pthread_mutex_lock(&gAPIConfigLock);

    bResult = gAPIConfig.bEnableEventLog;

    pthread_mutex_unlock(&gAPIConfigLock);

    return bResult;
}

VOID
LsaSrvEnableEventlog(
    BOOLEAN bValue
    )
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bEnableEventLog = bValue;

    pthread_mutex_unlock(&gAPIConfigLock);
}

BOOLEAN
LsaSrvShouldLogNetworkConnectionEvents(
    VOID
    )
{
    BOOLEAN bResult = TRUE;

    pthread_mutex_lock(&gAPIConfigLock);

    bResult = gAPIConfig.bLogNetworkConnectionEvents;

    pthread_mutex_unlock(&gAPIConfigLock);

    return bResult;
}

VOID
LsaSrvSetLogNetworkConnectionEvents(
    BOOLEAN bValue
    )
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bLogNetworkConnectionEvents = bValue;

    pthread_mutex_unlock(&gAPIConfigLock);
}


DWORD
LsaSrvSetMachineSid(
    HANDLE hServer,
    PCSTR pszSID
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d OR %ws=%d";
    const wchar_t wszAccountFilterFmt[] = L"%ws='%ws' AND (%ws=%d OR %ws=%d)";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_SRV_API_STATE pSrvState = (PLSA_SRV_API_STATE)hServer;
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
    DWORD iEntry = 0;
    DWORD dwObjectClass = DIR_OBJECT_CLASS_UNKNOWN;
    PWSTR pwszDomainSid = NULL;
    PSID pDomainSid = NULL;
    PWSTR pwszDomainObjectDN = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAccountFilter = NULL;
    DWORD dwAccountFilterLen = 0;
    PDIRECTORY_ENTRY pAccountEntries = NULL;
    PDIRECTORY_ENTRY pAccountEntry = NULL;
    DWORD dwNumAccountEntries = 0;
    PWSTR pwszAccountSid = NULL;
    PSID pAccountSid = NULL;
    PWSTR pwszAccountObjectDN = NULL;
    ULONG ulSidLength = 0;
    ULONG ulRid = 0;
    PSID pNewAccountSid = NULL;
    PWSTR pwszNewAccountSid = NULL;

    WCHAR wszAttrObjectDN[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DIRECTORY_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSID[] = DIRECTORY_ATTR_OBJECT_SID;
    WCHAR wszAttrDomainName[] = DIRECTORY_ATTR_DOMAIN_NAME;

    PWSTR wszAttributes[] = {
        &wszAttrObjectClass[0],
        &wszAttrObjectSID[0],
        &wszAttrDomainName[0],
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

    if (pSrvState->peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(pszSID,
                            &pwszNewDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateSidFromWC16String(&pNewDomainSid,
                                            pwszNewDomainSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = LwAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                                (PVOID*)&pwszDomainFilter);
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_DOMAIN,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_BUILTIN_DOMAIN);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszDomainFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pDomainEntries,
                              &dwNumDomainEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumDomainEntries != 2)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumDomainEntries; iEntry++)
    {
        pDomainEntry = &(pDomainEntries[iEntry]);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrObjectClass,
                                    DIRECTORY_ATTR_TYPE_INTEGER,
                                    &dwObjectClass);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwObjectClass != DIR_OBJECT_CLASS_DOMAIN)
        {
            continue;
        }

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrObjectSID,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = RtlAllocateSidFromWC16String(&pDomainSid,
                                                pwszDomainSid);
        if (ntStatus != STATUS_SUCCESS)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrObjectDN,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszDomainObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pDomainEntry,
                                    wszAttrDomainName,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewDomainSid;
    mods[0] = modObjectSID;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszDomainObjectDN,
                                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    dwAccountFilterLen = ((sizeof(wszAttrDomainName)/sizeof(WCHAR) - 1) +
                          wc16slen(pwszDomainName) +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAccountFilterFmt)/
                           sizeof(wszAccountFilterFmt[0])));
    dwError = LwAllocateMemory(dwAccountFilterLen * sizeof(WCHAR),
                                (PVOID*)&pwszAccountFilter);
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszAccountFilter, dwAccountFilterLen, wszAccountFilterFmt,
                &wszAttrDomainName[0],
                pwszDomainName,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_USER,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_LOCAL_GROUP);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszAccountFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pAccountEntries,
                              &dwNumAccountEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumAccountEntries == 0)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumAccountEntries; iEntry++)
    {
        pAccountEntry = &(pAccountEntries[iEntry]);

        dwError = DirectoryGetEntryAttrValueByName(
                                     pAccountEntry,
                                     wszAttrObjectSID,
                                     DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                     &pwszAccountSid);
        BAIL_ON_LSA_ERROR(dwError);

        /* Account SID has to be valid ... */
        ntStatus = RtlAllocateSidFromWC16String(&pAccountSid,
                                                pwszAccountSid);
        if (ntStatus != STATUS_SUCCESS)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        /* ... and it has to be in the same domain as machine SID */
        if (!RtlIsPrefixSid(pDomainSid,
                            pAccountSid))
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        ulSidLength = RtlLengthSid(pAccountSid);
        dwError = LwAllocateMemory(ulSidLength,
                                    (PVOID*)&pNewAccountSid);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = RtlCopySid(ulSidLength,
                              pNewAccountSid,
                              pNewDomainSid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        ulRid = pAccountSid->SubAuthority[pAccountSid->SubAuthorityCount - 1];
        ntStatus = RtlAppendRidSid(ulSidLength,
                                   pNewAccountSid,
                                   ulRid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        ntStatus = RtlAllocateWC16StringFromSid(&pwszNewAccountSid,
                                                pNewAccountSid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = DirectoryGetEntryAttrValueByName(
                                     pAccountEntry,
                                     wszAttrObjectDN,
                                     DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                     &pwszAccountObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        memset(&mods[0], 0, sizeof(mods));

        AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewAccountSid;
        mods[0] = modObjectSID;

        dwError = DirectoryModifyObject(hDirectory,
                                        pwszAccountObjectDN,
                                        mods);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAccountSid)
        {
            RTL_FREE(&pAccountSid);
        }

        if (pNewAccountSid)
        {
            RTL_FREE(&pNewAccountSid);
        }

        if (pwszNewAccountSid)
        {
            LW_SAFE_FREE_MEMORY(pwszNewAccountSid);
            pwszNewAccountSid = NULL;
        }
    }

cleanup:
    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (pAccountEntries)
    {
        DirectoryFreeEntries(pAccountEntries,
                             dwNumAccountEntries);
    }

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LW_SAFE_FREE_MEMORY(pwszDomainFilter);
    LW_SAFE_FREE_MEMORY(pwszAccountFilter);
    LW_SAFE_FREE_MEMORY(pwszNewDomainSid);
    LW_SAFE_FREE_MEMORY(pwszNewAccountSid);

    if (pDomainSid)
    {
        RTL_FREE(&pDomainSid);
    }

    if (pNewDomainSid)
    {
        RTL_FREE(&pNewDomainSid);
    }

    if (pAccountSid)
    {
        RTL_FREE(&pAccountSid);
    }

    if (pNewAccountSid)
    {
        RTL_FREE(&pNewAccountSid);
    }

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "set machine sid (new sid = '%s')", LSA_SAFE_LOG_STRING(pszSID));

    goto cleanup;
}


DWORD
LsaSrvSetMachineName(
    HANDLE hServer,
    PCSTR  pszNewMachineName
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d";
    const wchar_t wszAccountFilterFmt[] = L"%ws='%ws' AND (%ws=%d OR %ws=%d)";
    const wchar_t wszAnyObjectFilterFmt[] = L"%ws>0";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    PLSA_SRV_API_STATE pSrvState = (PLSA_SRV_API_STATE)hServer;
    PWSTR pwszNewMachineName = NULL;
    PWSTR pwszMachineName = NULL;
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
    DWORD iEntry = 0;
    DWORD iMod = 0;
    PWSTR pwszAccountFilter = NULL;
    DWORD dwAccountFilterLen = 0;
    PDIRECTORY_ENTRY pAccountEntries = NULL;
    PDIRECTORY_ENTRY pAccountEntry = NULL;
    DWORD dwNumAccountEntries = 0;
    PWSTR pwszAccountObjectDN = NULL;
    PWSTR pwszAnyObjectFilter = NULL;
    DWORD dwAnyObjectFilterLen = 0;
    PDIRECTORY_ENTRY pObjectEntries = NULL;
    PDIRECTORY_ENTRY pObjectEntry = NULL;
    DWORD dwNumObjectEntries = 0;
    PSTR pszMachineName = NULL;
    PWSTR pwszObjectDN = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszNewObjectDN = NULL;
    PWSTR pwszParentDN = NULL;
    PSTR pszParentDN = NULL;
    PSTR pszNewParentDN = NULL;

    WCHAR wszAttrRecordId[] = DIRECTORY_ATTR_RECORD_ID;
    WCHAR wszAttrObjectClass[] = DIRECTORY_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDN[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrParentDN[] = DIRECTORY_ATTR_PARENT_DN;
    WCHAR wszAttrDomainName[] = DIRECTORY_ATTR_DOMAIN_NAME;
    WCHAR wszAttrNetbiosName[] = DIRECTORY_ATTR_NETBIOS_NAME;
    WCHAR wszAttrCommonName[] = DIRECTORY_ATTR_COMMON_NAME;
    WCHAR wszAttrSamAccountName[] = DIRECTORY_ATTR_SAM_ACCOUNT_NAME;

    PWSTR wszAttributes[] = {
        &wszAttrObjectDN[0],
        &wszAttrParentDN[0],
        &wszAttrDomainName[0],
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_DOMAIN_NAME  = 0,
        ATTR_IDX_NETBIOS_NAME,
        ATTR_IDX_COMMON_NAME,
        ATTR_IDX_SAM_ACCOUNT_NAME,
        ATTR_IDX_OBJECT_DN,
        ATTR_IDX_PARENT_DN,
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
        {   /* ATTR_IDX_OBJECT_DN */
            .Type = DIRECTORY_ATTR_TYPE_ANSI_STRING,
            .data.pszStringValue = NULL
        },
        {   /* ATTR_IDX_PARENT_DN */
            .Type = DIRECTORY_ATTR_TYPE_ANSI_STRING,
            .data.pszStringValue = NULL
        }
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

    DIRECTORY_MOD modObjectDN = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrObjectDN[0],
        1,
        &AttrValues[ATTR_IDX_OBJECT_DN]
    };

    DIRECTORY_MOD modParentDN = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrParentDN[0],
        1,
        &AttrValues[ATTR_IDX_PARENT_DN]
    };

    DIRECTORY_MOD mods[ATTR_IDX_SENTINEL + 1];

    if (pSrvState->peerUID)
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(pszNewMachineName,
                            &pwszNewMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Search and modify MACHINE domain object first
     */

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = LwAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                               (PVOID*)&pwszDomainFilter);
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_DOMAIN,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_BUILTIN_DOMAIN);

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

    dwError = DirectoryGetEntryAttrValueByName(
                                pDomainEntry,
                                wszAttrDomainName,
                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                &pwszMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    iMod = 0;
    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_DOMAIN_NAME].data.pwszStringValue  = pwszNewMachineName;
    AttrValues[ATTR_IDX_NETBIOS_NAME].data.pwszStringValue = pwszNewMachineName;
    AttrValues[ATTR_IDX_COMMON_NAME].data.pwszStringValue  = pwszNewMachineName;
    AttrValues[ATTR_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue
        = pwszNewMachineName;

    mods[iMod++] = modDomainName;
    mods[iMod++] = modNetbiosName;
    mods[iMod++] = modCommonName;
    mods[iMod++] = modSamAccountName;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszDomainObjectDN,
                                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Search and modify local domain accounts
     */

    dwAccountFilterLen = ((sizeof(wszAttrDomainName)/sizeof(WCHAR) - 1) +
                          wc16slen(pwszMachineName) +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAccountFilterFmt)/
                           sizeof(wszAccountFilterFmt[0])));
    dwError = LwAllocateMemory(dwAccountFilterLen * sizeof(WCHAR),
                               (PVOID*)&pwszAccountFilter);
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszAccountFilter, dwAccountFilterLen, wszAccountFilterFmt,
                &wszAttrDomainName[0],
                pwszMachineName,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_USER,
                &wszAttrObjectClass[0],
                DIR_OBJECT_CLASS_LOCAL_GROUP);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszAccountFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pAccountEntries,
                              &dwNumAccountEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumAccountEntries == 0)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumAccountEntries; iEntry++)
    {
        pAccountEntry = &(pAccountEntries[iEntry]);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pAccountEntry,
                                    wszAttrObjectDN,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszAccountObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        iMod = 0;
        memset(&mods[0], 0, sizeof(mods));

        AttrValues[ATTR_IDX_DOMAIN_NAME].data.pwszStringValue
            = pwszNewMachineName;
        AttrValues[ATTR_IDX_NETBIOS_NAME].data.pwszStringValue
            = pwszNewMachineName;

        mods[iMod++] = modDomainName;
        mods[iMod++] = modNetbiosName;

        dwError = DirectoryModifyObject(hDirectory,
                                        pwszAccountObjectDN,
                                        mods);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*
     * Get all objects and update DN wherever it includes "DN=MACHINE"
     */

    dwAnyObjectFilterLen = ((sizeof(wszAttrRecordId)/sizeof(WCHAR) - 1) +
                            dwInt32StrSize +
                            (sizeof(wszAnyObjectFilterFmt)/
                             sizeof(wszAnyObjectFilterFmt[0])));
    dwError = LwAllocateMemory(dwAnyObjectFilterLen * sizeof(WCHAR),
                               (PVOID*)&pwszAnyObjectFilter);
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszAnyObjectFilter, dwAnyObjectFilterLen,
                wszAnyObjectFilterFmt,
                &wszAttrRecordId[0]);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              ulScope,
                              pwszAnyObjectFilter,
                              wszAttributes,
                              ulAttributesOnly,
                              &pObjectEntries,
                              &dwNumObjectEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumObjectEntries == 0)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumObjectEntries; iEntry++)
    {
        pObjectEntry = &(pObjectEntries[iEntry]);

        dwError = LwWc16sToMbs(pwszMachineName, &pszMachineName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pObjectEntry,
                                    wszAttrObjectDN,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwWc16sToMbs(pwszObjectDN, &pszObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaSrvModifyMachineDC(pszObjectDN,
                                        pszMachineName,
                                        pszNewMachineName,
                                        &pszNewObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pObjectEntry,
                                    wszAttrParentDN,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszParentDN);
        BAIL_ON_LSA_ERROR(dwError);

        if (pwszParentDN)
        {
            dwError = LwWc16sToMbs(pwszParentDN, &pszParentDN);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaSrvModifyMachineDC(pszParentDN,
                                            pszMachineName,
                                            pszNewMachineName,
                                            &pszNewParentDN);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pszNewObjectDN || pszNewParentDN)
        {
            iMod = 0;
            memset(&mods[0], 0, sizeof(mods));

            AttrValues[ATTR_IDX_OBJECT_DN].data.pszStringValue = pszNewObjectDN;
            AttrValues[ATTR_IDX_PARENT_DN].data.pszStringValue = pszNewParentDN;

            if (pszNewObjectDN)
            {
                mods[iMod++] = modObjectDN;
            }
            if (pszNewParentDN)
            {
                mods[iMod++] = modParentDN;
            }

            dwError = DirectoryModifyObject(hDirectory,
                                            pwszObjectDN,
                                            mods);
            BAIL_ON_LSA_ERROR(dwError);

        }

        if (pszMachineName)
        {
            LW_SAFE_FREE_STRING(pszMachineName);
            pszMachineName = NULL;
        }

        if (pszObjectDN)
        {
            LW_SAFE_FREE_STRING(pszObjectDN);
            pszObjectDN = NULL;
        }

        if (pszNewObjectDN)
        {
            LW_SAFE_FREE_STRING(pszNewObjectDN);
            pszNewObjectDN = NULL;
        }

        if (pszParentDN)
        {
            LW_SAFE_FREE_STRING(pszParentDN);
            pszParentDN = NULL;
        }

        if (pszNewParentDN)
        {
            LW_SAFE_FREE_STRING(pszNewParentDN);
            pszNewParentDN = NULL;
        }
    }

cleanup:
    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (pAccountEntries)
    {
        DirectoryFreeEntries(pAccountEntries,
                             dwNumAccountEntries);
    }

    if (pObjectEntries)
    {
        DirectoryFreeEntries(pObjectEntries,
                             dwNumObjectEntries);
    }

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LW_SAFE_FREE_MEMORY(pwszNewMachineName);
    LW_SAFE_FREE_MEMORY(pwszDomainFilter);
    LW_SAFE_FREE_MEMORY(pwszAccountFilter);
    LW_SAFE_FREE_MEMORY(pwszAnyObjectFilter);
    LW_SAFE_FREE_STRING(pszMachineName);
    LW_SAFE_FREE_STRING(pszObjectDN);
    LW_SAFE_FREE_STRING(pszNewObjectDN);
    LW_SAFE_FREE_STRING(pszParentDN);
    LW_SAFE_FREE_STRING(pszNewParentDN);

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "set machine name (new name = '%s')", LSA_SAFE_LOG_STRING(pszNewMachineName));
    goto cleanup;
}


static
DWORD
LsaSrvModifyMachineDC(
    PCSTR pszDN,
    PCSTR pszMachineName,
    PCSTR pszNewMachineName,
    PSTR  *ppszNewDN
    )
{
    DWORD dwError = 0;
    PSTR pszToken = NULL;
    PSTR pszPreToken = NULL;
    PSTR pszPostToken = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszNewObjectDN = NULL;
    DWORD dwTokenLen = 0;
    DWORD dwPreTokenLen = 0;

    dwError = LwStrDupOrNull(pszDN, &pszObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrStr(pszObjectDN, pszMachineName, &pszToken);

    if (pszToken)
    {
        pszToken[0]   = '\0';
        pszPreToken   = pszObjectDN;
        dwPreTokenLen = strlen(pszPreToken);
        dwTokenLen    = strlen(pszMachineName);
        pszPostToken  = &pszObjectDN[dwPreTokenLen + dwTokenLen];

        dwError = LwAllocateStringPrintf(&pszNewObjectDN,
                                         "%s%s%s",
                                         pszPreToken,
                                         pszNewMachineName,
                                         pszPostToken);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszNewDN = pszNewObjectDN;

cleanup:
    LW_SAFE_FREE_STRING(pszObjectDN);

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszNewObjectDN);

    *ppszNewDN = NULL;

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
