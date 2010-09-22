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
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

static
DWORD
LsaAdProviderStateCreate(
    OUT PLSA_AD_PROVIDER_STATE* ppState
    );

static
VOID
LsaAdProviderStateDestroy(
    IN OUT PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_ResolveProviderState(
    IN HANDLE hProvider,
    OUT PAD_PROVIDER_CONTEXT *ppContext
    );

static
DWORD
AD_MachineCredentialsCacheClear(
    IN PLSA_AD_PROVIDER_STATE pState
    );

static
BOOLEAN
AD_MachineCredentialsCacheIsInitialized(
    IN PLSA_AD_PROVIDER_STATE pState
    );

static
VOID
LsaAdProviderMediaSenseTransitionCallback(
    IN PVOID Context,
    IN BOOLEAN bIsOffline
    );

static
DWORD
AD_ResolveConfiguredLists(
    PAD_PROVIDER_CONTEXT pContext,
    PLSA_HASH_TABLE *ppAllowedMemberList
    );

static
VOID
LsaAdProviderLogServiceStartEvent(
    PCSTR   pszHostname,
    PCSTR   pszDomainDnsName,
    BOOLEAN bIsDomainOffline,
    DWORD   dwErrCode
    );

static
VOID
LsaAdProviderLogConfigReloadEvent(
    IN PLSA_AD_PROVIDER_STATE pState
    );

static
VOID
LsaAdProviderLogRequireMembershipOfChangeEvent(
    IN PAD_PROVIDER_CONTEXT pContext
    );

static
VOID
LsaAdProviderLogEventLogEnableChangeEvent(
    IN PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_JoinDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    );

static
DWORD
AD_TransitionJoined(
    PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_Deactivate(
    PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_TransitionNotJoined(
    PLSA_AD_PROVIDER_STATE pState
    );

static
VOID
InitADCacheFunctionTable(
    PLSA_AD_PROVIDER_STATE pState,
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheProviderTable
    );

static
VOID
LsaAdProviderStateAcquireRead(
    PLSA_AD_PROVIDER_STATE pState
    );

static
VOID
LsaAdProviderStateAcquireWrite(
    PLSA_AD_PROVIDER_STATE pState
    );

static
void
LsaAdProviderStateRelease(
    PLSA_AD_PROVIDER_STATE pState
    );

static
void*
LsaAdStartupThread(
    void* pData
    );

static
DWORD
AD_Activate(
    PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_GetUserGroupObjectMembership(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    );

static
DWORD
AD_InitializeProvider(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE_2* ppFunctionTable
    )
{
    DWORD dwError = 0;
    LSA_AD_CONFIG config = {0};
    pthread_t startThread;

    pthread_rwlock_init(&gADGlobalDataLock, NULL);

    dwError = LsaAdProviderStateCreate(&gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetCreateSchannelState(gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_InitializeConfig(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);
                
    dwError = AD_TransferConfigContents(
                    &config,
                    &gpLsaAdProviderState->config);
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdProviderLogConfigReloadEvent(gpLsaAdProviderState);

    InitADCacheFunctionTable(
        gpLsaAdProviderState,
        gpCacheProvider);

    dwError = LwKrb5SetProcessDefaultCachePath(LSASS_CACHE_PATH);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_OpenDb(
                &gpLsaAdProviderState->hStateConnection);
    BAIL_ON_LSA_ERROR(dwError);

    switch (gpLsaAdProviderState->config.CacheBackend)
    {
        default:
#ifdef AD_CACHE_ENABLE_SQLITE
        case AD_CACHE_SQLITE:
            dwError = ADCacheOpen(
                        LSASS_AD_SQLITE_CACHE_DB,
                        gpLsaAdProviderState,
                        &gpLsaAdProviderState->hCacheConnection);
            BAIL_ON_LSA_ERROR(dwError);
            break;
#endif
        case AD_CACHE_IN_MEMORY:
            dwError = ADCacheOpen(
                            LSASS_AD_MEMORY_CACHE_DB,
                            gpLsaAdProviderState,
                            &gpLsaAdProviderState->hCacheConnection);
            BAIL_ON_LSA_ERROR(dwError);
            dwError = MemCacheSetSizeCap(
                            gpLsaAdProviderState->hCacheConnection,
                            AD_GetCacheSizeCap(gpLsaAdProviderState));
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    dwError = ADUnprovPlugin_Initialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_create(
                                      &startThread,
                                      NULL,
                                      LsaAdStartupThread,
                                      gpLsaAdProviderState));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_detach(startThread));
    BAIL_ON_LSA_ERROR(dwError);

    *ppszProviderName = gpszADProviderName;
    *ppFunctionTable = &gADProviderAPITable2;

cleanup:

    AD_FreeConfigContents(&config);

    return dwError;

error:

    LsaAdProviderStateDestroy(gpLsaAdProviderState);
    gpLsaAdProviderState = NULL;

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}

static
void*
LsaAdStartupThread(
    void* pData
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pState = (PLSA_AD_PROVIDER_STATE) pData;

    LsaAdProviderStateAcquireWrite(pState);

    dwError = LwKrb5GetMachineCreds(NULL, NULL, NULL, NULL);
    if (dwError == 0)
    {
        dwError = AD_Activate(pState);
        if (dwError == 0)
        {
            pState->joinState = LSA_AD_JOINED;
        }
    }
    else
    {
        pState->joinState = LSA_AD_NOT_JOINED;
    }

    LsaSrvFlushSystemCache();

    LsaAdProviderStateRelease(pState);

    return NULL;
}

DWORD
AD_ShutdownProvider(
    VOID
    )
{
    DWORD dwError = 0;

    LsaAdProviderStateAcquireWrite(gpLsaAdProviderState);
    if (gpLsaAdProviderState->joinState == LSA_AD_JOINED)
    {
        AD_Deactivate(gpLsaAdProviderState);
    }
    LsaAdProviderStateRelease(gpLsaAdProviderState);
    
    ADUnprovPlugin_Cleanup();

    dwError = AD_NetShutdownMemory(gpLsaAdProviderState->hSchannelState);
    if (dwError)
    {
        LSA_LOG_DEBUG("AD Provider Shutdown: Failed to shutdown net memory (error = %u)", dwError);
        dwError = 0;
    }

    AD_NetDestroySchannelState(gpLsaAdProviderState);

    AD_FreeAllowedSIDs_InLock(gpLsaAdProviderState);

    // This will clean up media sense too.
    LsaAdProviderStateDestroy(gpLsaAdProviderState);
    gpLsaAdProviderState = NULL;

    return dwError;
}

static
DWORD
AD_Activate(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    PSTR  pszHostname = NULL;
    PSTR  pszUsername = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostDnsDomain = NULL;
    BOOLEAN bIsDomainOffline = FALSE;
    BOOLEAN bIgnoreAllTrusts = FALSE;
    PSTR* ppszTrustExceptionList = NULL;
    DWORD dwTrustExceptionCount = 0;

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pszHostname);

    dwError = LwKrb5GetMachineCreds(
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetDomainManagerTrustExceptionList(
                    pState,
                    &bIgnoreAllTrusts,
                    &ppszTrustExceptionList,
                    &dwTrustExceptionCount);
    BAIL_ON_LSA_ERROR(dwError);

    // Initialize domain manager before doing any network stuff.
    dwError = LsaDmInitialize(
                    pState,
                    TRUE,
                    AD_GetDomainManagerCheckDomainOnlineSeconds(pState),
                    AD_GetDomainManagerUnknownDomainCacheTimeoutSeconds(pState),
                    bIgnoreAllTrusts,
                    ppszTrustExceptionList,
                    dwTrustExceptionCount);
    BAIL_ON_LSA_ERROR(dwError);

    // Start media sense after starting up domain manager.
    dwError = MediaSenseStart(&pState->MediaSenseHandle,
                              LsaAdProviderMediaSenseTransitionCallback,
                              pState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmWrapLdapPingTcp(
                  pState->hDmState,
                  pszDomainDnsName);
    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        bIsDomainOffline = TRUE;
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADInitMachinePasswordSync(pState);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsDomainOffline)
    {
        dwError = AD_MachineCredentialsCacheInitialize(pState);
        if (dwError == LW_ERROR_CLOCK_SKEW)
        {
            bIsDomainOffline = TRUE;
            dwError = LW_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (bIsDomainOffline)
    {
        dwError = AD_MachineCredentialsCacheClear(pState);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_InitializeOperatingMode(
                pState,
                pszDomainDnsName,
                pszUsername,
                bIsDomainOffline);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmInitialize(pState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADStartMachinePasswordSync(pState);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_EventlogEnabled(pState))
    {
        LsaAdProviderLogServiceStartEvent(
                           pszHostname,
                           pszDomainDnsName,
                           bIsDomainOffline,
                           dwError);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SECURE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);
    LwFreeStringArray(ppszTrustExceptionList, dwTrustExceptionCount);

    return dwError;

error:

    if (AD_EventlogEnabled(pState))
    {
        LsaAdProviderLogServiceStartEvent(
                           pszHostname,
                           pszDomainDnsName,
                           bIsDomainOffline,
                           dwError);
    }

    ADShutdownMachinePasswordSync(pState);

    LsaDmCleanup(pState->hDmState);

    LsaUmCleanup();

    goto cleanup;
}

static
DWORD
AD_TransitionJoined(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;


    dwError = AD_Activate(pState);
    BAIL_ON_LSA_ERROR(dwError);

    pState->joinState = LSA_AD_JOINED;

error:

    return dwError;
}

static
DWORD
AD_Deactivate(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    ADShutdownMachinePasswordSync(pState);
    AD_MachineCredentialsCacheClear(pState);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (pState->pProviderData)
    {
        ADProviderFreeProviderData(pState->pProviderData);
        pState->pProviderData = NULL;
    }

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (pState->MediaSenseHandle)
    {
        MediaSenseStop(&pState->MediaSenseHandle);
        pState->MediaSenseHandle = NULL;
    }
    
    LsaUmCleanup();

    LsaDmCleanup(pState->hDmState);

    return dwError;
}

static
DWORD
AD_TransitionNotJoined(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    dwError = ADCacheEmptyCache(pState->hCacheConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_EmptyDb(pState->hStateConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_Deactivate(pState);
    BAIL_ON_LSA_ERROR(dwError);

    pState->joinState = LSA_AD_NOT_JOINED;

error:

    return dwError;
}

DWORD
AD_OpenHandle(
    HANDLE hServer,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = AD_CreateProviderContext(&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvGetClientId(
        hServer,
        &pContext->uid,
        &pContext->gid,
        &pContext->pid);

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *phProvider = (HANDLE)NULL;

    if (pContext)
    {
        AD_DereferenceProviderContext(pContext);
    }

    goto cleanup;
}

void
AD_CloseHandle(
    HANDLE hProvider
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;
    if (pContext)
    {
        AD_ClearProviderState(pContext);
        AD_DereferenceProviderContext(pContext);
    }
}

DWORD
AD_CreateProviderContext(
    OUT PAD_PROVIDER_CONTEXT *ppContext
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pContext),
                    (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    pContext->nRefCount = 1;

    *ppContext = pContext;

cleanup:

    return dwError;

error:

    *ppContext = NULL;

    if (pContext)
    {
        AD_DereferenceProviderContext(pContext);
    }

    goto cleanup;
}

VOID
AD_ReferenceProviderContext(
    IN PAD_PROVIDER_CONTEXT pContext
    )
{
    InterlockedIncrement(&pContext->nRefCount);
}

VOID
AD_DereferenceProviderContext(
    IN PAD_PROVIDER_CONTEXT pContext
    )
{
    if (pContext)
    {
        DWORD dwCount = 0;

        dwCount = InterlockedDecrement(&pContext->nRefCount);
        LW_ASSERT(dwCount >= 0);

        if (0 == dwCount)
        {
            LwFreeMemory(pContext);
        }
    }
}

static
DWORD
AD_ResolveProviderState(
    IN HANDLE hProvider,
    OUT PAD_PROVIDER_CONTEXT *ppContext
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    if (!pContext)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    else
    {
        pContext->pState = gpLsaAdProviderState;
        if (!pContext->pState)
        {
            dwError = LW_ERROR_NOT_HANDLED;
        }
    }

    *ppContext = pContext;

    return dwError;
}

VOID
AD_ClearProviderState(
    IN PAD_PROVIDER_CONTEXT pContext
    )
{
    if (pContext)
    {
        pContext->pState = NULL;
    }
}

DWORD
AD_ServicesDomain(
    PCSTR pszDomain,
    BOOLEAN* pbServicesDomain
    )
{
    *pbServicesDomain = FALSE;
    return LW_ERROR_NOT_HANDLED;
}

BOOLEAN
AD_ServicesDomainInternal(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        goto cleanup;
    }

    //
    // Added Trusted domains support
    //
    if (LW_IS_NULL_OR_EMPTY_STR(pszDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpLsaAdProviderState->pProviderData->szDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpLsaAdProviderState->pProviderData->szShortDomain)) {
       goto cleanup;
    }

    bResult = LsaDmIsDomainPresent(gpLsaAdProviderState->hDmState, pszDomain);
    if (!bResult)
    {
        LSA_LOG_INFO("AD_ServicesDomain was passed unknown domain '%s'", pszDomain);
    }

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return bResult;
}

DWORD
AD_AuthenticateUserPam(
    HANDLE hProvider,
    LSA_AUTH_USER_PAM_PARAMS* pParams,
    PLSA_AUTH_USER_PAM_INFO* ppPamAuthInfo
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    if (ppPamAuthInfo)
    {
        *ppPamAuthInfo = NULL;
    }

    if (pParams->dwFlags & LSA_AUTH_USER_PAM_FLAG_SMART_CARD)
    {
        BAIL_WITH_LSA_ERROR(LW_ERROR_NOT_HANDLED);
    }

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline(pContext->pState))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineAuthenticateUserPam(
            pContext,
            pParams,
            ppPamAuthInfo);
    }
   
    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineAuthenticateUserPam(
            pContext,
            pParams,
            ppPamAuthInfo);
    }

error:

    LsaAdProviderStateRelease(pContext->pState);
    return dwError;
}

DWORD
AD_AuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserParams->pszDomain)
    {
        BOOLEAN bFoundDomain = FALSE;

        dwError = AD_ServicesDomainWithDiscovery(
                        pContext->pState,
                        pUserParams->pszDomain,
                        &bFoundDomain);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bFoundDomain)
        {
            dwError = LW_ERROR_NOT_HANDLED;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LsaDmWrapAuthenticateUserEx(
                      pContext->pState->hDmState,
                      pContext->pState->pProviderData->szDomain,
                      pUserParams,
                      ppUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    return dwError;

error:
    /* On this one, it is a good idea to fallback to
       the local provider */

    if (dwError == LW_ERROR_RPC_NETLOGON_FAILED) {
        dwError = LW_ERROR_NOT_HANDLED;
    }

    goto cleanup;
}

DWORD
AD_ValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindUserObjectByName(
                pContext,
                pszLoginId,
                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    ADCacheSafeFreeObject(&pUserInfo);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CheckUserInList(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PCSTR  pszListName
    )
{
    DWORD  dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    size_t  sNumGroupsFound = 0;
    PLSA_SECURITY_OBJECT* ppGroupList = NULL;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    size_t  iGroup = 0;
    PLSA_HASH_TABLE pAllowedMemberList = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_ResolveConfiguredLists(
                  pContext,
                  &pAllowedMemberList);
    BAIL_ON_LSA_ERROR(dwError);

    if (!AD_ShouldFilterUserLoginsByGroup(pContext->pState))
    {
        goto cleanup;
    }

    dwError = AD_FindUserObjectByName(pContext, pszUserName, &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_IsMemberAllowed(pContext->pState,
                           pUserInfo->pszObjectSid,
                           pAllowedMemberList))
    {
        goto cleanup;
    }

    dwError = AD_GetUserGroupObjectMembership(
                    pContext,
                    pUserInfo,
                    FALSE,
                    &sNumGroupsFound,
                    &ppGroupList);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iGroup < sNumGroupsFound; iGroup++)
    {
        if (AD_IsMemberAllowed(pContext->pState,
                               ppGroupList[iGroup]->pszObjectSid,
                               pAllowedMemberList))
        {
            goto cleanup;
        }
    }

    dwError = LW_ERROR_ACCESS_DENIED;
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    ADCacheSafeFreeObjectList(sNumGroupsFound, &ppGroupList);
    ADCacheSafeFreeObject(&pUserInfo);
    LsaHashSafeFree(&pAllowedMemberList);

    return dwError;

error:

    if (dwError == LW_ERROR_ACCESS_DENIED)
    {
        LSA_LOG_INFO("Error: User [%s] not in restricted login list", pszUserName);
    }
    else
    {
        LSA_LOG_ERROR("Error: Failed to validate restricted membership. [Error code: %u]", dwError);
    }

    goto cleanup;
}

DWORD
AD_FindUserObjectById(
    IN HANDLE  hProvider,
    IN uid_t   uid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList = { 0 };
    DWORD dwUid = uid;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.pdwIds = &dwUid;

    dwError = AD_FindObjects(
        pContext,
        0,
        LSA_OBJECT_TYPE_USER,
	LSA_QUERY_TYPE_BY_UNIX_ID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = ppObjects[0];
    ppObjects[0] = 0;

cleanup:
    LsaUtilFreeSecurityObjectList(1, ppObjects);

    LsaAdProviderStateRelease(pContext->pState);

    return dwError;

error:
    goto cleanup;
}

DWORD
AD_EnumUsersFromCache(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD                 dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    DWORD                 dwObjectCount = 0;
    PLSA_SECURITY_OBJECT* ppUserObjectList = NULL;
    PVOID                 pBlob = NULL;
    size_t                BlobSize = 0;
    LWMsgContext*         context = NULL;
    LWMsgDataContext*      pDataContext = NULL;
    PLSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ request = NULL;
    LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP response = {0};

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetEnumUsersFromCacheReqSpec(),
                              pInputBuffer,
                              dwInputBufferSize,
                              (PVOID*)&request));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheEnumUsersCache(
                  pContext->pState->hCacheConnection,
                  request->dwMaxNumUsers,
                  request->pszResume,
                  &dwObjectCount,
                  &ppUserObjectList);
    if ( dwError == LW_ERROR_NOT_HANDLED )
    {
        // no more results found
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if ( dwObjectCount == request->dwMaxNumUsers )
    {
        dwError = LwAllocateString(
                      ppUserObjectList[dwObjectCount - 1]->pszObjectSid,
                      &response.pszResume);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // marshal the response data
    response.dwNumUsers = dwObjectCount;
    response.ppObjects = ppUserObjectList;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaAdIPCGetEnumUsersFromCacheRespSpec(),
                              &response,
                              &pBlob,
                              &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwOutputBufferSize = BlobSize;
    *ppOutputBuffer = pBlob;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    ADCacheSafeFreeObjectList(dwObjectCount, &ppUserObjectList);

    if ( request )
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetEnumUsersFromCacheReqSpec(),
            request);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if ( context )
    {
        lwmsg_context_delete(context);
    }

    LW_SAFE_FREE_STRING(response.pszResume);

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    if ( pBlob )
    {
        LwFreeMemory(pBlob);
    }

    goto cleanup;
}

DWORD
AD_RemoveUserByNameFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN PCSTR  pszLoginId
    )
{
    DWORD                dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_TYPE QueryType = 0;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pszLoginId, "root"))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvCrackDomainQualifiedName(
                    pszLoginId,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pLoginInfo->nameType)
    {
    case NameType_NT4:
        QueryType = LSA_QUERY_TYPE_BY_NT4;
        break;
    case NameType_Alias:
        QueryType = LSA_QUERY_TYPE_BY_ALIAS;
        break;
    case NameType_UPN:
        QueryType = LSA_QUERY_TYPE_BY_UPN;
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.ppszStrings = (PCSTR*) &pszLoginId;

    dwError = AD_FindObjects(
        pContext,
        LSA_FIND_FLAGS_CACHE_ONLY,
        LSA_OBJECT_TYPE_USER,
        QueryType,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheRemoveUserBySid(
                  pContext->pState->hCacheConnection,
                  ppObjects[0]->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LsaUtilFreeSecurityObjectList(1, ppObjects);
    LsaAdProviderStateRelease(pContext->pState);
    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_RemoveUserByIdFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN uid_t  uid
    )
{
    DWORD                dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList = {0};
    DWORD dwUid = uid;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (uid == 0)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.pdwIds = &dwUid;

    dwError = AD_FindObjects(
        pContext,
        LSA_FIND_FLAGS_CACHE_ONLY,
        LSA_OBJECT_TYPE_USER,
	LSA_QUERY_TYPE_BY_UNIX_ID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheRemoveUserBySid(
                  pContext->pState->hCacheConnection,
                  ppObjects[0]->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LsaUtilFreeSecurityObjectList(1, ppObjects);
    LsaAdProviderStateRelease(pContext->pState);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_PreJoinDomain(
    PAD_PROVIDER_CONTEXT pContext,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    
    switch (pState->joinState)
    {
    case LSA_AD_UNKNOWN:
    case LSA_AD_NOT_JOINED:
        break;

    case LSA_AD_JOINED:
        dwError = LsaDisableDomainGroupMembership();
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_TransitionNotJoined(pState);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

error:

    return dwError;
}

static
DWORD
AD_PostJoinDomain(
    PAD_PROVIDER_CONTEXT pContext,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    dwError = AD_TransitionJoined(pState);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
AD_JoinDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_AD_IPC_JOIN_DOMAIN_REQ pRequest = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN bLocked = FALSE;
    PSTR pszDC = NULL;
    
    if (peerUID != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetJoinDomainReqSpec(),
                                  pInputBuffer,
                                  (size_t) dwInputBufferSize,
                                  OUT_PPVOID(&pRequest)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_print_graph_alloc(
                                  pDataContext,
                                  LsaAdIPCGetJoinDomainReqSpec(),
                                  pRequest,
                                  &pszMessage));
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_TRACE("Domain join request: %s", pszMessage);

    dwError = LWNetGetDomainController(
                    pRequest->pszDomain,
                    &pszDC);
    if (dwError)
    {
        LSA_LOG_VERBOSE("Failed to find DC for domain %s", LSA_SAFE_LOG_STRING(pRequest->pszDomain));
        BAIL_ON_LSA_ERROR(dwError);
    }
    LSA_LOG_VERBOSE("Affinitized to DC '%s' for join request to domain '%s'",
            LSA_SAFE_LOG_STRING(pszDC),
            LSA_SAFE_LOG_STRING(pRequest->pszDomain));

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireWrite(pContext->pState);
    bLocked = TRUE;

    dwError = AD_PreJoinDomain(pContext, pContext->pState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaJoinDomain(
        pRequest->pszHostname,
        pRequest->pszHostDnsDomain,
        pRequest->pszDomain,
        pRequest->pszOU,
        pRequest->pszUsername,
        pRequest->pszPassword,
        pRequest->pszOSName,
        pRequest->pszOSVersion,
        pRequest->pszOSServicePack,
        pRequest->dwFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_PostJoinDomain(pContext, pContext->pState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaEnableDomainGroupMembership(pRequest->pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_INFO("Joined domain: %s", pRequest->pszDomain);

cleanup:

    if (bLocked)
    {
        LsaAdProviderStateRelease(pContext->pState);
    }

    LW_SAFE_FREE_MEMORY(pszMessage);

    if (pRequest)
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetJoinDomainReqSpec(),
            pRequest);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    LW_SAFE_FREE_STRING(pszDC);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_PreLeaveDomain(
    IN PAD_PROVIDER_CONTEXT pContext,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    
    switch (pState->joinState)
    {
    case LSA_AD_UNKNOWN:
    case LSA_AD_NOT_JOINED:
        dwError = LW_ERROR_NOT_JOINED_TO_AD;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_AD_JOINED:
        break;
    }

error:

    return dwError;
}

static
DWORD
AD_PostLeaveDomain(
    IN PAD_PROVIDER_CONTEXT pContext,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    return AD_TransitionNotJoined(pState);
}

static
DWORD
AD_LeaveDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_AD_IPC_LEAVE_DOMAIN_REQ pRequest = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN bLocked = FALSE;

    if (peerUID != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetLeaveDomainReqSpec(),
                                  pInputBuffer,
                                  (size_t) dwInputBufferSize,
                                  OUT_PPVOID(&pRequest)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_print_graph_alloc(
                                  pDataContext,
                                  LsaAdIPCGetLeaveDomainReqSpec(),
                                  pRequest,
                                  &pszMessage));
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_TRACE("Domain leave request: %s", pszMessage);

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireWrite(pContext->pState);
    bLocked = TRUE;

    dwError = LsaDisableDomainGroupMembership();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_PreLeaveDomain(pContext, pContext->pState);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLeaveDomain(
        pRequest->pszUsername,
        pRequest->pszPassword);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = AD_PostLeaveDomain(pContext, pContext->pState);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_INFO("Left domain\n");

cleanup:

    if (bLocked)
    {
        LsaAdProviderStateRelease(pContext->pState);
    }

    LW_SAFE_FREE_MEMORY(pszMessage);

    if (pRequest)
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetLeaveDomainReqSpec(),
            pRequest);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_EnumGroupsFromCache(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD                 dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    DWORD                 dwObjectCount = 0;
    PLSA_SECURITY_OBJECT* ppGroupObjectList = NULL;
    PVOID                 pBlob = NULL;
    size_t                BlobSize;
    LWMsgContext*         context = NULL;
    LWMsgDataContext*      pDataContext = NULL;
    PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ request = NULL;
    LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP response = {0};

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetEnumGroupsFromCacheReqSpec(),
                              pInputBuffer,
                              dwInputBufferSize,
                              (PVOID*)&request));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheEnumGroupsCache(
                  pContext->pState->hCacheConnection,
                  request->dwMaxNumGroups,
                  request->pszResume,
                  &dwObjectCount,
                  &ppGroupObjectList);
    if ( dwError == LW_ERROR_NOT_HANDLED )
    {
        // no more results found
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if ( dwObjectCount == request->dwMaxNumGroups )
    {
        dwError = LwAllocateString(
                      ppGroupObjectList[dwObjectCount - 1]->pszObjectSid,
                      &response.pszResume);
        BAIL_ON_LSA_ERROR(dwError);
    }

    response.dwNumGroups = dwObjectCount;
    response.ppObjects = ppGroupObjectList;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaAdIPCGetEnumGroupsFromCacheRespSpec(),
                              &response,
                              &pBlob,
                              &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwOutputBufferSize = BlobSize;
    *ppOutputBuffer = pBlob;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    ADCacheSafeFreeObjectList(dwObjectCount, &ppGroupObjectList);

    if ( request )
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetEnumGroupsFromCacheReqSpec(),
            request);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if ( context )
    {
        lwmsg_context_delete(context);
    }

    LW_SAFE_FREE_STRING(response.pszResume);

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    if ( pBlob )
    {
        LwFreeMemory(pBlob);
    }

    goto cleanup;
}

DWORD
AD_RemoveGroupByNameFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN PCSTR  pszGroupName
    )
{
    DWORD                dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_TYPE QueryType = 0;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }


    dwError = LsaSrvCrackDomainQualifiedName(
                    pszGroupName,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pLoginInfo->nameType)
    {
    case NameType_NT4:
        QueryType = LSA_QUERY_TYPE_BY_NT4;
        break;
    case NameType_Alias:
        QueryType = LSA_QUERY_TYPE_BY_ALIAS;
        break;
    case NameType_UPN:
        QueryType = LSA_QUERY_TYPE_BY_UPN;
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.ppszStrings = (PCSTR*) &pszGroupName;

    dwError = AD_FindObjects(
        pContext,
        LSA_FIND_FLAGS_CACHE_ONLY,
        LSA_OBJECT_TYPE_GROUP,
        QueryType,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheRemoveGroupBySid(
                  pContext->pState->hCacheConnection,
                  ppObjects[0]->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LsaUtilFreeSecurityObjectList(1, ppObjects);
    LsaAdProviderStateRelease(pContext->pState);
    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_RemoveGroupByIdFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN gid_t  gid
    )
{
    DWORD                dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList = {0};
    DWORD dwGid = gid;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.pdwIds = &dwGid;

    dwError = AD_FindObjects(
        pContext,
        LSA_FIND_FLAGS_CACHE_ONLY,
        LSA_OBJECT_TYPE_GROUP,
	LSA_QUERY_TYPE_BY_UNIX_ID,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheRemoveGroupBySid(
                  pContext->pState->hCacheConnection,
                  ppObjects[0]->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LsaUtilFreeSecurityObjectList(1, ppObjects);
    LsaAdProviderStateRelease(pContext->pState);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_GetUserGroupObjectMembership(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    )
{
    DWORD dwError = 0;

    if (AD_IsOffline(pContext->pState))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineGetUserGroupObjectMembership(
            pContext,
            pUserInfo,
            bIsCacheOnlyMode,
            psNumGroupsFound,
            pppResult);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineGetUserGroupObjectMembership(
            pContext,
            pUserInfo,
            psNumGroupsFound,
            pppResult);
    }

    return dwError;
}

DWORD
AD_ChangePassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline(pContext->pState))
    {
        dwError = AD_OfflineChangePassword(
            pContext,
            pszLoginId,
            pszPassword,
            pszOldPassword);
    }
    else
    {
        dwError = AD_OnlineChangePassword(
            pContext,
            pszLoginId,
            pszPassword,
            pszOldPassword);
    }

error:

    LsaAdProviderStateRelease(pContext->pState);

    return dwError;
}

DWORD
AD_SetPassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword
    )
{
    return LW_ERROR_NOT_HANDLED;
}


DWORD
AD_AddUser(
    HANDLE hProvider,
    PLSA_USER_ADD_INFO pUserAddInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_ModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO_2 pUserModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_DeleteObject(
    HANDLE hProvider,
    PCSTR pszSid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_AddGroup(
    HANDLE hProvider,
    PLSA_GROUP_ADD_INFO pGroupAddInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_ModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_EmptyCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheEmptyCache(
                  pContext->pState->hCacheConnection);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_OpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_TYPE QueryType = 0;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvCrackDomainQualifiedName(
                    pszLoginId,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pLoginInfo->nameType)
    {
    case NameType_NT4:
        QueryType = LSA_QUERY_TYPE_BY_NT4;
        break;
    case NameType_Alias:
        QueryType = LSA_QUERY_TYPE_BY_ALIAS;
        break;
    case NameType_UPN:
        QueryType = LSA_QUERY_TYPE_BY_UPN;
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.ppszStrings = (PCSTR*) &pszLoginId;

    dwError = AD_FindObjects(
        pContext,
        0,
        LSA_OBJECT_TYPE_USER,
        QueryType,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0] || !ppObjects[0]->enabled)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_CreateHomeDirectory(pContext->pState, ppObjects[0]);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldCreateK5Login(pContext->pState))
    {
        dwError = AD_CreateK5Login(ppObjects[0]);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    LsaAdProviderStateRelease(pContext->pState);

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_TYPE QueryType = 0;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvCrackDomainQualifiedName(
                    pszLoginId,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pLoginInfo->nameType)
    {
    case NameType_NT4:
        QueryType = LSA_QUERY_TYPE_BY_NT4;
        break;
    case NameType_Alias:
        QueryType = LSA_QUERY_TYPE_BY_ALIAS;
        break;
    case NameType_UPN:
        QueryType = LSA_QUERY_TYPE_BY_UPN;
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.ppszStrings = (PCSTR*) &pszLoginId;

    dwError = AD_FindObjects(
        pContext,
        0,
        LSA_OBJECT_TYPE_USER,
        QueryType,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0] || !ppObjects[0]->enabled)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaUmRemoveUser(ppObjects[0]->userInfo.uid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    LsaAdProviderStateRelease(pContext->pState);

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_FindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline(pContext->pState))
    {
        dwError = AD_OfflineFindNSSArtefactByKey(
                        pContext,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
    }
    else
    {
        dwError = AD_OnlineFindNSSArtefactByKey(
                        pContext,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
    }

error:

    LsaAdProviderStateRelease(pContext->pState);

    return dwError;
}

DWORD
AD_BeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PAD_ENUM_STATE pEnumState = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!dwFlags)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (pContext->pState->pProviderData->dwDirectoryMode)
    {
        case DEFAULT_MODE:
        case CELL_MODE:

            dwError = AD_CreateNSSArtefactState(
                                pContext,
                                dwInfoLevel,
                                pszMapName,
                                dwFlags,
                                &pEnumState);
            BAIL_ON_LSA_ERROR(dwError);

            LwInitCookie(&pEnumState->Cookie);

            break;

        case UNPROVISIONED_MODE:

            dwError = LW_ERROR_NOT_SUPPORTED;
            break;
    }

    *phResume = (HANDLE)pEnumState;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);
    AD_ClearProviderState(pContext);

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
AD_EnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PAD_ENUM_STATE pEnum = hResume;

    dwError = AD_ResolveProviderState(pEnum->pProviderContext, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }


    if (AD_IsOffline(pContext->pState))
    {
        dwError = AD_OfflineEnumNSSArtefacts(
            pContext,
            hResume,
            dwMaxNSSArtefacts,
            pdwNSSArtefactsFound,
            pppNSSArtefactInfoList);
    }
    else
    {
        dwError = AD_OnlineEnumNSSArtefacts(
            pContext,
            hResume,
            dwMaxNSSArtefacts,
            pdwNSSArtefactsFound,
            pppNSSArtefactInfoList);
    }

error:

    LsaAdProviderStateRelease(pContext->pState);
    AD_ClearProviderState(pContext);

    return dwError;
}

VOID
AD_EndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PAD_ENUM_STATE pEnum = hResume;

    AD_ResolveProviderState(pEnum->pProviderContext, &pContext);
    
    AD_FreeNSSArtefactState(pContext, pEnum);
}

DWORD
AD_GetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLWNET_DC_INFO pDCInfo = NULL;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED ||
        !pContext->pState->pProviderData)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    gpszADProviderName,
                    &pProviderStatus->pszId);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pContext->pState->pProviderData->dwDirectoryMode)
    {
        case DEFAULT_MODE:

            pProviderStatus->mode = LSA_PROVIDER_MODE_DEFAULT_CELL;
            break;

        case CELL_MODE:

            pProviderStatus->mode = LSA_PROVIDER_MODE_NON_DEFAULT_CELL;

            if (!LW_IS_NULL_OR_EMPTY_STR(pContext->pState->pProviderData->cell.szCellDN))
            {
                dwError = LwAllocateString(
                                pContext->pState->pProviderData->cell.szCellDN,
                                &pProviderStatus->pszCell);
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;

        case UNPROVISIONED_MODE:

            pProviderStatus->mode = LSA_PROVIDER_MODE_UNPROVISIONED;
            break;

        default:

            pProviderStatus->mode = LSA_PROVIDER_MODE_UNKNOWN;
            break;
    }

    switch (pContext->pState->pProviderData->adConfigurationMode)
    {
        case SchemaMode:

            pProviderStatus->subMode = LSA_AUTH_PROVIDER_SUBMODE_SCHEMA;
            break;

        case NonSchemaMode:

            pProviderStatus->subMode = LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA;
            break;

        default:

            pProviderStatus->subMode = LSA_AUTH_PROVIDER_SUBMODE_UNKNOWN;
            break;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pContext->pState->pProviderData->szDomain))
    {
        dwError = LwAllocateString(
                        pContext->pState->pProviderData->szDomain,
                        &pProviderStatus->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LWNetGetDCName(
                        NULL,
                        pContext->pState->pProviderData->szDomain,
                        NULL,
                        DS_BACKGROUND_ONLY,
                        &pDCInfo);
        if (ERROR_NO_SUCH_DOMAIN == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pDCInfo)
        {
            if (!LW_IS_NULL_OR_EMPTY_STR(pDCInfo->pszDnsForestName))
            {
                dwError = LwAllocateString(
                                pDCInfo->pszDnsForestName,
                                &pProviderStatus->pszForest);
                BAIL_ON_LSA_ERROR(dwError);
            }

            if (!LW_IS_NULL_OR_EMPTY_STR(pDCInfo->pszDCSiteName))
            {
                dwError = LwAllocateString(
                                pDCInfo->pszDCSiteName,
                                &pProviderStatus->pszSite);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    dwError = AD_GetTrustedDomainInfo(
                    pContext->pState->hDmState,
                    &pProviderStatus->pTrustedDomainInfoArray,
                    &pProviderStatus->dwNumTrustedDomains);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_IsOffline(pContext->pState))
    {
        pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_OFFLINE;
    }
    else
    {
        pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_ONLINE;
    }

    dwError = LsaDmQueryState(pContext->pState->hDmState, NULL, &pProviderStatus->dwNetworkCheckInterval, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderStatus = pProviderStatus;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    LWNET_SAFE_FREE_DC_INFO(pDCInfo);

    return dwError;

error:

    *ppProviderStatus = NULL;

    if (pProviderStatus)
    {
        AD_FreeStatus(pProviderStatus);
    }

    goto cleanup;
}

DWORD
AD_GetTrustedDomainInfo(
    LSA_DM_STATE_HANDLE hDmState,
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray,
    PDWORD pdwNumTrustedDomains
    )
{
    DWORD dwError = 0;
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray = NULL;
    DWORD dwCount = 0;
    PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo = NULL;

    dwError = LsaDmEnumDomainInfo(
                  hDmState,
                  NULL,
                  NULL,
                  &ppDomainInfo,
                  &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwCount)
    {
        DWORD iDomain = 0;

        dwError = LwAllocateMemory(
                        sizeof(pDomainInfoArray[0]) * dwCount,
                        (PVOID*)&pDomainInfoArray);
        BAIL_ON_LSA_ERROR(dwError);

        for (iDomain = 0; iDomain < dwCount; iDomain++)
        {
            dwError = AD_FillTrustedDomainInfo(
                        ppDomainInfo[iDomain],
                        &pDomainInfoArray[iDomain]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppDomainInfoArray = pDomainInfoArray;
    *pdwNumTrustedDomains = dwCount;

cleanup:
    LsaDmFreeEnumDomainInfoArray(ppDomainInfo);

    return dwError;

error:

    *ppDomainInfoArray = NULL;
    *pdwNumTrustedDomains = 0;

    if (pDomainInfoArray)
    {
        LsaFreeDomainInfoArray(dwCount, pDomainInfoArray);
    }

    goto cleanup;
}

VOID
AD_FreeTrustedDomainsInList(
    PVOID pItem,
    PVOID pUserData
    )
{
    if (pItem)
    {
        LsaFreeDomainInfo((PLSA_TRUSTED_DOMAIN_INFO)pItem);
    }
}

DWORD
AD_FillTrustedDomainInfo(
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo,
    OUT PLSA_TRUSTED_DOMAIN_INFO pTrustedDomainInfo
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    // Do not free dcInfo as it just points to other data.
    LSA_DM_DC_INFO dcInfo = { 0 };

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszDnsDomainName,
                    &pTrustedDomainInfo->pszDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszNetbiosDomainName,
                    &pTrustedDomainInfo->pszNetbiosDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (pDomainInfo->pSid)
    {
        dwError = LsaAllocateCStringFromSid(
                        &pTrustedDomainInfo->pszDomainSID,
                        pDomainInfo->pSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pDomainInfo->pGuid)
    {
        CHAR szGUID[37] = "";
        uuid_t uuid = {0};

        memcpy(&uuid, pDomainInfo->pGuid, sizeof(uuid));
        uuid_unparse(uuid, szGUID);

        dwError = LwAllocateString(
                        szGUID,
                        &pTrustedDomainInfo->pszDomainGUID);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszTrusteeDnsDomainName,
                    &pTrustedDomainInfo->pszTrusteeDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    pTrustedDomainInfo->dwTrustFlags = pDomainInfo->dwTrustFlags;
    pTrustedDomainInfo->dwTrustType = pDomainInfo->dwTrustType;
    pTrustedDomainInfo->dwTrustAttributes = pDomainInfo->dwTrustAttributes;
    pTrustedDomainInfo->dwTrustDirection = pDomainInfo->dwTrustDirection;
    pTrustedDomainInfo->dwTrustMode = pDomainInfo->dwTrustMode;

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszForestName,
                    &pTrustedDomainInfo->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszClientSiteName,
                    &pTrustedDomainInfo->pszClientSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pTrustedDomainInfo->dwDomainFlags = pDomainInfo->Flags;

    if (pDomainInfo->DcInfo)
    {
        dwError = AD_BuildDCInfo(
                        pDomainInfo->DcInfo,
                        &pTrustedDomainInfo->pDCInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LWNetGetDCName(
                    NULL,
                    pDomainInfo->pszDnsDomainName,
                    NULL,
                    DS_BACKGROUND_ONLY,
                    &pDcInfo);
        if (ERROR_NO_SUCH_DOMAIN == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pDcInfo)
        {
            dcInfo.dwDsFlags = pDcInfo->dwFlags;
            dcInfo.pszName = pDcInfo->pszDomainControllerName;
            dcInfo.pszAddress = pDcInfo->pszDomainControllerAddress;
            dcInfo.pszSiteName = pDcInfo->pszDCSiteName;

            dwError = AD_BuildDCInfo(
                            &dcInfo,
                            &pTrustedDomainInfo->pDCInfo);
            BAIL_ON_LSA_ERROR(dwError);

            if (!pTrustedDomainInfo->pszClientSiteName)
            {
                dwError = LwStrDupOrNull(
                            pDcInfo->pszClientSiteName,
                            &pTrustedDomainInfo->pszClientSiteName);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    if (pDomainInfo->GcInfo)
    {
        dwError = AD_BuildDCInfo(
                        pDomainInfo->GcInfo,
                        &pTrustedDomainInfo->pGCInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
        dwError = LWNetGetDCName(
                    NULL,
                    pDomainInfo->pszForestName,
                    NULL,
                    DS_GC_SERVER_REQUIRED | DS_BACKGROUND_ONLY,
                    &pDcInfo);
        if (ERROR_NO_SUCH_DOMAIN == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pDcInfo)
        {
            dcInfo.dwDsFlags = pDcInfo->dwFlags;
            dcInfo.pszName = pDcInfo->pszDomainControllerName;
            dcInfo.pszAddress = pDcInfo->pszDomainControllerAddress;
            dcInfo.pszSiteName = pDcInfo->pszDCSiteName;

            dwError = AD_BuildDCInfo(
                            &dcInfo,
                            &pTrustedDomainInfo->pGCInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    return dwError;

error:
    LsaFreeDomainInfoContents(pTrustedDomainInfo);
    goto cleanup;
}

DWORD
AD_BuildDCInfo(
    PLSA_DM_DC_INFO pDCInfo,
    PLSA_DC_INFO*   ppDCInfo
    )
{
    DWORD dwError = 0;
    PLSA_DC_INFO pDestDCInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LSA_DC_INFO),
                    (PVOID*)&pDestDCInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pDCInfo->pszName,
                    &pDestDCInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pDCInfo->pszAddress,
                    &pDestDCInfo->pszAddress);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pDCInfo->pszSiteName,
                    &pDestDCInfo->pszSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pDestDCInfo->dwFlags = pDCInfo->dwDsFlags;

    *ppDCInfo = pDestDCInfo;

cleanup:

    return dwError;

error:

    *ppDCInfo = NULL;

    if (pDestDCInfo)
    {
        LsaFreeDCInfo(pDestDCInfo);
    }

    goto cleanup;
}

VOID
AD_FreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    )
{
    LW_SAFE_FREE_STRING(pProviderStatus->pszId);
    LW_SAFE_FREE_STRING(pProviderStatus->pszDomain);
    LW_SAFE_FREE_STRING(pProviderStatus->pszForest);
    LW_SAFE_FREE_STRING(pProviderStatus->pszSite);
    LW_SAFE_FREE_STRING(pProviderStatus->pszCell);

    if (pProviderStatus->pTrustedDomainInfoArray)
    {
        LsaFreeDomainInfoArray(
                        pProviderStatus->dwNumTrustedDomains,
                        pProviderStatus->pTrustedDomainInfoArray);
    }

    LwFreeMemory(pProviderStatus);
}

DWORD
AD_RefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    LSA_AD_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;
    BOOLEAN bUpdateCap = FALSE;

    dwError = AD_InitializeConfig(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    

    dwError = AD_TransferConfigContents(
                    &config,
                    &pContext->pState->config);
    BAIL_ON_LSA_ERROR(dwError);

    AD_FreeAllowedSIDs_InLock(pContext->pState);

    dwError = LsaDmSetState(
                    pContext->pState->hDmState,
                    NULL,
                    &pContext->pState->config.DomainManager.dwCheckDomainOnlineSeconds,
                    &pContext->pState->config.DomainManager.dwUnknownDomainCacheTimeoutSeconds);
    BAIL_ON_LSA_ERROR(dwError);

    if (pContext->pState->config.CacheBackend == AD_CACHE_IN_MEMORY)
    {
        bUpdateCap = TRUE;
    }

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (bUpdateCap)
    {
        dwError = MemCacheSetSizeCap(
                        pContext->pState->hCacheConnection,
                        AD_GetCacheSizeCap(pContext->pState));
        BAIL_ON_LSA_ERROR(dwError);
    }
    LsaAdProviderLogConfigReloadEvent(pContext->pState);
    if (pContext->pState->joinState == LSA_AD_JOINED)
    {
        LsaAdProviderLogRequireMembershipOfChangeEvent(pContext);
    }
    LsaAdProviderLogEventLogEnableChangeEvent(pContext->pState);

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    AD_FreeConfigContents(&config);

    goto cleanup;

}

DWORD
AD_ProviderIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN uid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;

    switch (dwIoControlCode)
    {
        case LSA_AD_IO_EMPTYCACHE:
            dwError = AD_EmptyCache(
                          hProvider,
                          peerUID,
                          peerGID);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEUSERBYNAMECACHE:
            dwError = AD_RemoveUserByNameFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          (PCSTR)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEUSERBYIDCACHE:
            dwError = AD_RemoveUserByIdFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          *(uid_t *)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEGROUPBYNAMECACHE:
            dwError = AD_RemoveGroupByNameFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          (PCSTR)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEGROUPBYIDCACHE:
            dwError = AD_RemoveGroupByIdFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          *(gid_t *)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_ENUMUSERSCACHE:
            dwError = AD_EnumUsersFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        case LSA_AD_IO_ENUMGROUPSCACHE:
            dwError = AD_EnumGroupsFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        case LSA_AD_IO_JOINDOMAIN:
            dwError = AD_JoinDomain(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        case LSA_AD_IO_LEAVEDOMAIN:
            dwError = AD_LeaveDomain(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        default:
            dwError = LW_ERROR_NOT_HANDLED;
            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    *pdwOutputBufferSize=0;
    *ppOutputBuffer = NULL;

    goto cleanup;
}

DWORD
AD_FindUserObjectByName(
    IN HANDLE  hProvider,
    IN PCSTR   pszLoginId,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_TYPE QueryType = 0;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvCrackDomainQualifiedName(
                    pszLoginId,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pLoginInfo->nameType)
    {
    case NameType_NT4:
        QueryType = LSA_QUERY_TYPE_BY_NT4;
        break;
    case NameType_Alias:
        QueryType = LSA_QUERY_TYPE_BY_ALIAS;
        break;
    case NameType_UPN:
        QueryType = LSA_QUERY_TYPE_BY_UPN;
        break;
    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    QueryList.ppszStrings = (PCSTR*) &pszLoginId;

    dwError = AD_FindObjects(
        pContext,
        0,
        LSA_OBJECT_TYPE_USER,
        QueryType,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = ppObjects[0];
    ppObjects[0] = 0;

cleanup:
    LsaUtilFreeSecurityObjectList(1, ppObjects);

    LsaAdProviderStateRelease(pContext->pState);

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:
    goto cleanup;
}

static
VOID
AD_FilterBuiltinObjects(
    IN DWORD dwCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppObjects[dwIndex] &&
            AdIsSpecialDomainSidPrefix(ppObjects[dwIndex]->pszObjectSid))
        {
            ADCacheSafeFreeObject(&ppObjects[dwIndex]);
        }
    }
}

DWORD
AD_UpdateObject(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN OUT PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PAD_PROVIDER_DATA pProviderData = pState->pProviderData;
    struct timeval current_tv = {0};
    UINT64 u64current_NTtime = 0;

    switch(pObject->type)
    {
    case LSA_OBJECT_TYPE_USER:
        if (gettimeofday(&current_tv, NULL) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
        ADConvertTimeUnix2Nt(current_tv.tv_sec,
                             &u64current_NTtime);
        
        if (pObject->userInfo.bIsAccountInfoKnown)
        {
            if (pObject->userInfo.qwAccountExpires != 0LL &&
                pObject->userInfo.qwAccountExpires != 9223372036854775807LL &&
                u64current_NTtime >= pObject->userInfo.qwAccountExpires)
            {
                pObject->userInfo.bAccountExpired = TRUE;
            }

            pObject->userInfo.qwMaxPwdAge = pProviderData->adMaxPwdAge;

            if ((!pObject->userInfo.bPasswordNeverExpires &&
                  pObject->userInfo.qwPwdExpires != 0 &&
                  u64current_NTtime >= pObject->userInfo.qwPwdExpires) ||
                pObject->userInfo.qwPwdLastSet == 0)
            {
                //password is expired already
                pObject->userInfo.bPasswordExpired = TRUE;
            }
        }

        LW_SAFE_FREE_STRING(pObject->userInfo.pszUnixName);
        dwError = ADMarshalGetCanonicalName(
                      pState,
                      pObject,
                      &pObject->userInfo.pszUnixName);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_OBJECT_TYPE_GROUP:
        LW_SAFE_FREE_STRING(pObject->groupInfo.pszUnixName);
        dwError = ADMarshalGetCanonicalName(
                      pState,
                      pObject,
                      &pObject->groupInfo.pszUnixName);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        break;
    }


cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_UpdateObjects(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN DWORD dwCount,
    IN OUT PLSA_SECURITY_OBJECT* ppObjects)
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppObjects[dwIndex])
        {
            dwError = AD_UpdateObject(
                          pState,
                          ppObjects[dwIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

error:

    return dwError;
}

DWORD
AD_FindObjects(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline(pContext->pState) || (FindFlags & LSA_FIND_FLAGS_CACHE_ONLY))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindObjects(
            pContext,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindObjects(
            pContext,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (ppObjects)
    {
        dwError = AD_UpdateObjects(
                      pContext->pState,
                      dwCount,
                      ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        AD_FilterBuiltinObjects(dwCount, ppObjects);
    }

    *pppObjects = ppObjects;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    return dwError;

error:

    *pppObjects = NULL;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

DWORD
AD_OpenEnumObjects(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PAD_ENUM_HANDLE pEnum = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->Type = AD_ENUM_HANDLE_OBJECTS;
    pEnum->FindFlags = FindFlags;
    pEnum->ObjectType = ObjectType;

    if (ObjectType == LSA_OBJECT_TYPE_UNDEFINED)
    {
        pEnum->CurrentObjectType = LSA_OBJECT_TYPE_USER;
    }
    else
    {
        pEnum->CurrentObjectType = ObjectType;
    }

    LwInitCookie(&pEnum->Cookie);

    AD_ReferenceProviderContext(pContext);
    pEnum->pProviderContext = pContext;

    *phEnum = pEnum;
    pEnum = NULL;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);
    AD_ClearProviderState(pContext);

    if (pEnum)
    {
        AD_CloseEnum(pEnum);
    }

    return dwError;

error:

    *phEnum = NULL;

    goto cleanup;
}

DWORD
AD_EnumObjects(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PAD_ENUM_HANDLE pEnum = hEnum;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;

    dwError = AD_ResolveProviderState(pEnum->pProviderContext, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline(pContext->pState))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_OnlineEnumObjects(
            pContext,
            hEnum,
            dwMaxObjectsCount,
            &dwObjectsCount,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_UpdateObjects(
                  pContext->pState,
                  dwObjectsCount,
                  ppObjects);
    BAIL_ON_LSA_ERROR(dwError);


    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);
    AD_ClearProviderState(pContext);

    return dwError;

error:

    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwObjectsCount, ppObjects);
    }

    goto cleanup;
}

DWORD
AD_OpenEnumMembers(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PAD_ENUM_HANDLE pEnum = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AdIsSpecialDomainSidPrefix(pszSid))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->Type = AD_ENUM_HANDLE_MEMBERS;
    pEnum->FindFlags = FindFlags;

    LwInitCookie(&pEnum->Cookie);

    AD_ReferenceProviderContext(pContext);
    pEnum->pProviderContext = pContext;
   
    if (AD_IsOffline(pContext->pState))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineGetGroupMemberSids(
            pContext,
            FindFlags,
            pszSid,
            &pEnum->dwSidCount,
            &pEnum->ppszSids);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineGetGroupMemberSids(
            pContext,
            FindFlags,
            pszSid,
            &pEnum->dwSidCount,
            &pEnum->ppszSids);
    }
    BAIL_ON_LSA_ERROR(dwError);

    *phEnum = pEnum;
    pEnum = NULL;

cleanup:

    LsaAdProviderStateRelease(pContext->pState);
    AD_ClearProviderState(pContext);

    if (pEnum)
    {
        AD_CloseEnum(pEnum);
    }

    return dwError;

error:

    *phEnum = NULL;

    goto cleanup;
}

DWORD
AD_EnumMembers(
    IN HANDLE hEnum,
    IN DWORD dwMaxMemberSidCount,
    OUT PDWORD pdwMemberSidCount,
    OUT PSTR** pppszMemberSids
    )
{
   DWORD dwError = 0;
   PAD_ENUM_HANDLE pEnum = hEnum;
   DWORD dwMemberSidCount = dwMaxMemberSidCount;
   PSTR* ppszMemberSids = NULL;

   if (dwMemberSidCount > pEnum->dwSidCount - pEnum->dwSidIndex)
   {
       dwMemberSidCount = pEnum->dwSidCount - pEnum->dwSidIndex;
   }

   if (dwMemberSidCount == 0)
   {
       dwError = ERROR_NO_MORE_ITEMS;
       BAIL_ON_LSA_ERROR(dwError);
   }

   dwError = LwAllocateMemory(
       sizeof(*ppszMemberSids) * dwMemberSidCount,
       OUT_PPVOID(&ppszMemberSids));
   BAIL_ON_LSA_ERROR(dwError);

   memcpy(ppszMemberSids, &pEnum->ppszSids[pEnum->dwSidIndex], sizeof(*ppszMemberSids) * dwMemberSidCount);
   memset(&pEnum->ppszSids[pEnum->dwSidIndex], 0, sizeof(*ppszMemberSids) * dwMemberSidCount);

   pEnum->dwSidIndex += dwMemberSidCount;

   *pdwMemberSidCount = dwMemberSidCount;
   *pppszMemberSids = ppszMemberSids;

cleanup:

   return dwError;

error:

   if (ppszMemberSids)
   {
       LwFreeStringArray(ppszMemberSids, dwMemberSidCount);
   }

   goto cleanup;
}

DWORD
AD_QueryMemberOf(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = AD_ResolveProviderState(hProvider, &pContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaAdProviderStateAcquireRead(pContext->pState);

    if (pContext->pState->joinState != LSA_AD_JOINED ||
        FindFlags & LSA_FIND_FLAGS_LOCAL)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline(pContext->pState))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineQueryMemberOf(
            pContext,
            FindFlags,
            dwSidCount,
            ppszSids,
            pdwGroupSidCount,
            pppszGroupSids);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineQueryMemberOf(
            pContext,
            FindFlags,
            dwSidCount,
            ppszSids,
            pdwGroupSidCount,
            pppszGroupSids);
    }

cleanup:

    LsaAdProviderStateRelease(pContext->pState);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_GetSmartCardUserObject(
    IN HANDLE hProvider,
    OUT PLSA_SECURITY_OBJECT* ppObject,
    OUT PSTR* ppszSmartCardReader
    )
{
    *ppObject = NULL;
    *ppszSmartCardReader = NULL;

    return LW_ERROR_NOT_HANDLED;
}

VOID
AD_CloseEnum(
    IN OUT HANDLE hEnum
    )
{
    PAD_ENUM_HANDLE pEnum = hEnum;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    if (pEnum)
    {
        // joined to a domain.
        AD_ResolveProviderState(pEnum->pProviderContext, &pContext);

        if (pContext && pContext->pState)
        {
            LsaAdProviderStateAcquireRead(pContext->pState);
        }

        LwFreeCookieContents(&pEnum->Cookie);
        if (pEnum->ppszSids)
        {
            LwFreeStringArray(pEnum->ppszSids, pEnum->dwSidCount);
        }
        LwFreeMemory(pEnum);

        if (pContext && pContext->pState)
        {
            LsaAdProviderStateRelease(pContext->pState);
            AD_ClearProviderState(pContext);
            AD_DereferenceProviderContext(pContext);
        }
    }
}

DWORD
AD_InitializeOperatingMode(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName,
    IN BOOLEAN bIsDomainOffline
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PAD_PROVIDER_CONTEXT pContext = NULL;
    PAD_PROVIDER_DATA pProviderData = NULL;

    dwError = AD_CreateProviderContext(&pContext);
    pContext->pState = pState;

    if (bIsDomainOffline || AD_IsOffline(pState))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineInitializeOperatingMode(
                &pProviderData,
                pContext,
                pszDomain,
                pszHostName);
    }
    // If we are offline, do the offline case
    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineInitializeOperatingMode(
                &pProviderData,
                pContext,
                pszDomain,
                pszHostName);
        BAIL_ON_LSA_ERROR(dwError);

        if (bIsDomainOffline)
        {
            // The domain was originally offline, so we need to
            // tell the domain manager about it.
            // Note that we can only transition offline
            // now that we set up the domains in the domain manager.
            dwError = LsaDmTransitionOffline(
                          pState->hDmState,
                          pszDomain,
                          FALSE);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        // check whether we failed for some other reason.
        BAIL_ON_LSA_ERROR(dwError);
    }

    pState->pProviderData = pProviderData;

cleanup:

    AD_ClearProviderState(pContext);
    AD_DereferenceProviderContext(pContext);

    return dwError;

error:
    // Note that pContext->pState->pProviderData will already be NULL.

    if (pProviderData)
    {
        ADProviderFreeProviderData(pProviderData);
        pProviderData = NULL;
    }

    goto cleanup;
}

static
DWORD
LsaAdProviderStateCreate(
    OUT PLSA_AD_PROVIDER_STATE* ppState
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pState = NULL;

    dwError = LwAllocateMemory(sizeof(*pState), (PVOID)&pState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pState->MachineCreds.Mutex, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pState->MachineCreds.pMutex = &pState->MachineCreds.Mutex;

    dwError = AD_InitializeConfig(&pState->config);
    BAIL_ON_LSA_ERROR(dwError);

    pState->dwMaxAllowedClockDriftSeconds = AD_MAX_ALLOWED_CLOCK_DRIFT_SECONDS;

    dwError = LwMapErrnoToLwError(pthread_rwlock_init(&pState->stateLock, NULL));
    BAIL_ON_LSA_ERROR(dwError);
    
    pState->pStateLock = &pState->stateLock;

    *ppState = pState;

cleanup:
    return dwError;

error:

    LsaAdProviderStateDestroy(pState);
    *ppState = NULL;
    goto cleanup;
}

static
VOID
LsaAdProviderStateDestroy(
    IN OUT PLSA_AD_PROVIDER_STATE pState
    )
{
    if (pState)
    {
        ADCacheSafeClose(&pState->hCacheConnection);
        ADState_SafeCloseDb(&pState->hStateConnection);

        if (pState->MediaSenseHandle)
        {
            MediaSenseStop(&pState->MediaSenseHandle);
            pState->MediaSenseHandle = NULL;
        }

        if (pState->MachineCreds.pMutex)
        {
            pthread_mutex_destroy(pState->MachineCreds.pMutex);
            pState->MachineCreds.pMutex = NULL;
        }
        AD_FreeConfigContents(&pState->config);

        if (pState->pStateLock)
        {
            pthread_rwlock_destroy(pState->pStateLock);
        }

        if (pState->pProviderData)
        {
            ADProviderFreeProviderData(pState->pProviderData);
        }

        if (pState->hDmState)
        {
            LsaDmCleanup(pState->hDmState);
        }

        if (pState->hMachinePwdState)
        {
            ADShutdownMachinePasswordSync(pState);
        }

        if (pState->hSchannelState)
        {
            AD_NetDestroySchannelState(pState);
        }

        LwFreeMemory(pState);
    }
}

static
VOID
LsaAdProviderStateAcquireRead(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    int status = 0;

    status = pthread_rwlock_rdlock(pState->pStateLock);
    LW_ASSERT(status == 0);
}

static
VOID
LsaAdProviderStateAcquireWrite(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    int status = 0;

    status = pthread_rwlock_wrlock(pState->pStateLock);
    LW_ASSERT(status == 0);
}

static
void
LsaAdProviderStateRelease(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    int status = 0;

    status = pthread_rwlock_unlock(pState->pStateLock);
    LW_ASSERT(status == 0);
}

static
DWORD
AD_MachineCredentialsCacheClear(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    pthread_mutex_lock(pState->MachineCreds.pMutex);
    bInLock = TRUE;

    if (pState->MachineCreds.bIsInitialized)
    {
        dwError = LwKrb5CleanupMachineSession();
        BAIL_ON_LSA_ERROR(dwError);
        pState->MachineCreds.bIsInitialized = FALSE;
    }

error:

    if (bInLock)
    {
        pthread_mutex_unlock(pState->MachineCreds.pMutex);
    }

    return dwError;
}

static
BOOLEAN
AD_MachineCredentialsCacheIsInitialized(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    BOOLEAN bIsInitialized = FALSE;

    pthread_mutex_lock(pState->MachineCreds.pMutex);
    bIsInitialized = pState->MachineCreds.bIsInitialized;
    pthread_mutex_unlock(pState->MachineCreds.pMutex);

    return bIsInitialized;
}

DWORD
AD_MachineCredentialsCacheInitialize(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PSTR pszHostname = NULL;
    PSTR pszUsername = NULL;
    PSTR pszPassword = NULL;
    PSTR pszDomainDnsName = NULL;
    PSTR pszHostDnsDomain = NULL;
    DWORD dwGoodUntilTime = 0;

    // Check before doing any work.
    if (AD_MachineCredentialsCacheIsInitialized(pState))
    {
        goto cleanup;
    }

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pszHostname);

    // Read password info before acquiring the lock.
    dwError = LwKrb5GetMachineCreds(
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaDmIsDomainOffline(pState->hDmState, pszDomainDnsName))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pthread_mutex_lock(pState->MachineCreds.pMutex);
    bIsAcquired = TRUE;

    // Verify that state did not change now that we have the lock.
    if (pState->MachineCreds.bIsInitialized)
    {
        goto cleanup;
    }

    ADSyncTimeToDC(pState, pszDomainDnsName);

    dwError = LwKrb5SetProcessDefaultCachePath(LSASS_CACHE_PATH);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwSetupMachineSession(
                    pszUsername,
                    pszPassword,
                    pszDomainDnsName,
                    pszHostDnsDomain,
                    &dwGoodUntilTime);
    if (dwError)
    {
        if (dwError == LW_ERROR_DOMAIN_IS_OFFLINE)
        {
            LsaDmTransitionOffline(
                pState->hDmState,
                pszDomainDnsName,
                FALSE);
        }

        ADSetMachineTGTExpiryError(pState->hMachinePwdState);
    }
    BAIL_ON_LSA_ERROR(dwError);

    ADSetMachineTGTExpiry(pState->hMachinePwdState, dwGoodUntilTime);

    pState->MachineCreds.bIsInitialized = TRUE;

cleanup:
    if (bIsAcquired)
    {
        pthread_mutex_unlock(pState->MachineCreds.pMutex);
    }

    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SECURE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);

    return dwError;

error:

    goto cleanup;
}

static
VOID
LsaAdProviderMediaSenseTransitionCallback(
    IN PVOID Context,
    IN BOOLEAN bIsOffline
    )
{
    PLSA_AD_PROVIDER_STATE pState = (PLSA_AD_PROVIDER_STATE)Context;

    if (bIsOffline)
    {
        LsaDmMediaSenseOffline(pState->hDmState);
    }
    else
    {
        LsaDmMediaSenseOnline(pState->hDmState);
    }
}

static
VOID
LsaAdProviderLogServiceStartEvent(
    PCSTR   pszHostname,
    PCSTR   pszDomainDnsName,
    BOOLEAN bIsDomainOffline,
    DWORD   dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;
    PLWNET_DC_INFO pDCInfo = NULL;
    PLWNET_DC_INFO pGCDCInfo = NULL;

    if (!bIsDomainOffline)
    {
        dwError = LWNetGetDCName(
                      NULL,
                      pszDomainDnsName,
                      NULL,
                      DS_BACKGROUND_ONLY,
                      &pDCInfo);

        if (pDCInfo)
        {
            dwError = LWNetGetDCName(
                          NULL,
                          pDCInfo->pszDnsForestName,
                          NULL,
                          DS_GC_SERVER_REQUIRED,
                          &pGCDCInfo);
        }
    }

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider initialization %s.\r\n\r\n" \
                 "     Authentication provider:   %s\r\n\r\n" \
                 "     Hostname:                  %s\r\n" \
                 "     Domain:                    %s\r\n" \
                 "     Current Domain Controller: %s\r\n" \
                 "     Current Global Catalog:    %s\r\n" \
                 "     Offline Startup:           %s",
                 dwErrCode ? "failed" : "succeeded",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 LSA_SAFE_LOG_STRING(pszHostname),
                 LSA_SAFE_LOG_STRING(pszDomainDnsName),
                 (pDCInfo)   ? LSA_SAFE_LOG_STRING(pDCInfo->pszDomainControllerName)   : "(Unknown)" ,
                 (pGCDCInfo) ? LSA_SAFE_LOG_STRING(pGCDCInfo->pszDomainControllerName) : "(Unknown)" ,
                 bIsDomainOffline ? "Yes" : "No");
    BAIL_ON_LSA_ERROR(dwError);

    if (dwErrCode)
    {
        dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
        BAIL_ON_LSA_ERROR(dwError);

        LsaSrvLogServiceFailureEvent(
                 LSASS_EVENT_FAILED_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 pszData);
    }
    else
    {
        LsaSrvLogServiceSuccessEvent(
                 LSASS_EVENT_SUCCESSFUL_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 NULL);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    LWNET_SAFE_FREE_DC_INFO(pDCInfo);
    LWNET_SAFE_FREE_DC_INFO(pGCDCInfo);

    return;

error:

    goto cleanup;
}

static
VOID
LsaAdProviderLogConfigReloadEvent(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszMemberList = NULL;
    PDLINKEDLIST pIter = NULL;

    for (pIter = pState->config.pUnresolvedMemberList;
         pIter;
         pIter = pIter->pNext)
    {
        PSTR pszNewMemberList = NULL;

        dwError = LwAllocateStringPrintf(
                     &pszNewMemberList,
                     "%s        %s\r\n",
                     pszMemberList ? pszMemberList : "",
                     LSA_SAFE_LOG_STRING((PSTR)pIter->pItem));
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_STRING(pszMemberList);
        pszMemberList = pszNewMemberList;
        pszNewMemberList = NULL;
    }

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider configuration settings have been reloaded.\r\n\r\n" \
                 "     Authentication provider:           %s\r\n" \
                 "     Current settings are...\r\n" \
                 "     Cache reaper timeout (secs):       %u\r\n" \
                 "     Cache entry expiry (secs):         %u\r\n" \
                 "     Space replacement character:       '%c'\r\n" \
                 "     Domain separator character:        '%c'\r\n" \
                 "     Enable event log:                  %s\r\n" \
                 "     Logon membership requirements:     \r\n%s" \
                 "     Log network connection events:     %s\r\n" \
                 "     Create K5Login file:               %s\r\n" \
                 "     Create home directory:             %s\r\n" \
                 "     Sign and seal LDAP traffic:        %s\r\n" \
                 "     Assume default domain:             %s\r\n" \
                 "     Sync system time:                  %s\r\n" \
                 "     Refresh user credentials:          %s\r\n" \
                 "     Machine password sync lifetime:    %u\r\n" \
                 "     Default Shell:                     %s\r\n" \
                 "     Default home directory prefix:     %s\r\n" \
                 "     Home directory template:           %s\r\n" \
                 "     Umask:                             %u\r\n" \
                 "     Skeleton directory:                %s\r\n" \
                 "     Cell support:                      %s\r\n" \
                 "     Trim user membership:              %s\r\n" \
                 "     NSS group members from cache only: %s\r\n" \
                 "     NSS user members from cache only:  %s\r\n" \
                 "     NSS enumeration enabled:           %s\r\n"
                 "     Domain Manager check domain online (secs):          %u\r\n"
                 "     Domain Manager unknown domain cache timeout (secs): %u",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 pState->config.dwCacheReaperTimeoutSecs,
                 pState->config.dwCacheEntryExpirySecs,
                 LsaSrvSpaceReplacement(),
                 LsaSrvDomainSeparator(),
                 pState->config.bEnableEventLog ? "true" : "false",
                 pszMemberList ? pszMemberList : "        <No login restrictions specified>\r\n",
                 pState->config.bShouldLogNetworkConnectionEvents ? "true" : "false",
                 pState->config.bCreateK5Login ? "true" : "false",
                 pState->config.bCreateHomeDir ? "true" : "false",
                 pState->config.bLDAPSignAndSeal ? "true" : "false",
                 pState->config.bAssumeDefaultDomain ? "true" : "false",
                 pState->config.bSyncSystemTime ? "true" : "false",
                 pState->config.bRefreshUserCreds ? "true" : "false",
                 pState->config.dwMachinePasswordSyncLifetime,
                 LSA_SAFE_LOG_STRING(pState->config.pszShell),
                 LSA_SAFE_LOG_STRING(pState->config.pszHomedirPrefix),
                 LSA_SAFE_LOG_STRING(pState->config.pszHomedirTemplate),
                 pState->config.dwUmask,
                 LSA_SAFE_LOG_STRING(pState->config.pszSkelDirs),
                 pState->config.CellSupport == AD_CELL_SUPPORT_UNINITIALIZED ? "Uninitialized" :
                 pState->config.CellSupport == AD_CELL_SUPPORT_FULL ? "Full" :
                 pState->config.CellSupport == AD_CELL_SUPPORT_FILE ? "File" :
                 pState->config.CellSupport == AD_CELL_SUPPORT_UNPROVISIONED ? "Unprovisioned" : "Invalid",
                 pState->config.bTrimUserMembershipEnabled ? "true" : "false",
                 pState->config.bNssGroupMembersCacheOnlyEnabled ? "true" : "false",
                 pState->config.bNssUserMembershipCacheOnlyEnabled ? "true" : "false",
                 pState->config.bNssEnumerationEnabled ? "true" : "false",
                 pState->config.DomainManager.dwCheckDomainOnlineSeconds,
                 pState->config.DomainManager.dwUnknownDomainCacheTimeoutSeconds);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             LSASS_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszMemberList);

    return;

error:

    goto cleanup;
}

static
VOID
LsaAdProviderLogRequireMembershipOfChangeEvent(
    IN PAD_PROVIDER_CONTEXT pContext
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PLSA_HASH_TABLE pAllowedMemberList = NULL;
    LSA_HASH_ITERATOR hashIterator = {0};
    LSA_HASH_ENTRY *pHashEntry = NULL;
    PSTR pszMemberList = NULL;
    DWORD i = 0;

    dwError = AD_ResolveConfiguredLists(
                  pContext,
                  &pAllowedMemberList);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAllowedMemberList != NULL)
    {
        dwError = LsaHashGetIterator(pAllowedMemberList, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; i++)
        {
            PSTR pszNewMemberList = NULL;

            dwError = LwAllocateStringPrintf(
                         &pszNewMemberList,
                         "%s        %s\r\n",
                         pszMemberList ? pszMemberList : "",
                         LSA_SAFE_LOG_STRING(pHashEntry->pValue));
            BAIL_ON_LSA_ERROR(dwError);

            LW_SAFE_FREE_STRING(pszMemberList);
            pszMemberList = pszNewMemberList;
            pszNewMemberList = NULL;
        }
    }
    else
    {
            dwError = LwAllocateStringPrintf(
                         &pszMemberList,
                         "        <No login restrictions specified>\r\n");
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider login restriction settings have been reloaded.\r\n\r\n" \
                 "     Authentication provider:           %s\r\n" \
                 "     Current settings are...\r\n" \
                 "     require-membership-of:\r\n%s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 LSA_SAFE_LOG_STRING(pszMemberList));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             LSASS_EVENT_INFO_REQUIRE_MEMBERSHIP_OF_UPDATED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszMemberList);
    LsaHashSafeFree(&pAllowedMemberList);

    return;

error:

    goto cleanup;
}

static
VOID
LsaAdProviderLogEventLogEnableChangeEvent(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider auditing settings have been updated.\r\n\r\n" \
                 "     Authentication provider:           %s\r\n" \
                 "     Current settings are...\r\n" \
                 "     Enable event log:                  %s\r\n",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 pState->config.bEnableEventLog ? "true" : "false");
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             pState->config.bEnableEventLog ?
                 LSASS_EVENT_INFO_AUDITING_CONFIGURATION_ENABLED : 
                 LSASS_EVENT_INFO_AUDITING_CONFIGURATION_DISABLED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

static
DWORD
AD_ResolveConfiguredLists(
    PAD_PROVIDER_CONTEXT pContext,
    PLSA_HASH_TABLE *ppAllowedMemberList
    )
{
    DWORD dwError = 0;
    DWORD iMember = 0;
    PSTR* ppszMembers = 0;
    DWORD dwNumMembers = 0;
    PLSA_HASH_TABLE pAllowedMemberList = NULL;
    PLSA_SECURITY_OBJECT pGroupInfo = NULL;
    PLSA_SECURITY_IDENTIFIER pSID = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_QUERY_LIST QueryList;
    LSA_QUERY_TYPE QueryType = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = AD_GetMemberLists(
                    pContext->pState,
                    &ppszMembers,
                    &dwNumMembers,
                    &pAllowedMemberList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iMember = 0; iMember < dwNumMembers; iMember++)
    {
        PSTR pszMember = *(ppszMembers + iMember);

        LSA_LOG_VERBOSE("Resolving entry [%s] for restricted login", pszMember);

        if (AD_STR_IS_SID(pszMember))
        {
            dwError = LsaAllocSecurityIdentifierFromString(
                            pszMember,
                            &pSID);
            if (dwError)
            {
                LSA_LOG_ERROR("Removing invalid SID entry [%s] from required membership list", pszMember);

                AD_DeleteFromMembersList(
                    pContext->pState,
                    pszMember);
            }
            else
            {
                LSA_LOG_VERBOSE("Adding entry to allow login for SID [%s]", pszMember);

                dwError = AD_AddAllowedMember(
                              pContext->pState,
                              pszMember,
                              pszMember,
                              &pAllowedMemberList);
                BAIL_ON_LSA_ERROR(dwError);

                if (pSID)
                {
                    LsaFreeSecurityIdentifier(pSID);
                    pSID = NULL;
                }
            }
        }
        else // User or Group Name
        {
            dwError = LsaSrvCrackDomainQualifiedName(
                pszMember,
                &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            switch (pLoginInfo->nameType)
            {
            case NameType_NT4:
                QueryType = LSA_QUERY_TYPE_BY_NT4;
                break;
            case NameType_Alias:
                QueryType = LSA_QUERY_TYPE_BY_ALIAS;
                break;
            case NameType_UPN:
                QueryType = LSA_QUERY_TYPE_BY_UPN;
                break;
            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }
            
            LsaSrvFreeNameInfo(pLoginInfo);
            pLoginInfo = NULL;

            QueryList.ppszStrings = (PCSTR*) &pszMember;
            
            dwError = AD_FindObjects(
                pContext,
                0,
                LSA_OBJECT_TYPE_UNDEFINED,
                QueryType,
                1,
                QueryList,
                &ppObjects);
            BAIL_ON_LSA_ERROR(dwError);
            
            if (!ppObjects[0] ||
                (ppObjects[0]->type != LSA_OBJECT_TYPE_USER &&
                 ppObjects[0]->type != LSA_OBJECT_TYPE_GROUP))
            {
                LSA_LOG_WARNING("Restricted login list - couldn't resolve %s [%u]",
                                pszMember, LW_ERROR_NO_SUCH_OBJECT);
            }
            else if (ppObjects[0]->type == LSA_OBJECT_TYPE_USER)
            {
                LSA_LOG_VERBOSE("Adding entry to allow login for user [%s]", pszMember);

                dwError = AD_AddAllowedMember(
                    pContext->pState,
                    ppObjects[0]->pszObjectSid,
                    pszMember,
                    &pAllowedMemberList);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else if (ppObjects[0]->type == LSA_OBJECT_TYPE_GROUP)
            {
                LSA_LOG_VERBOSE("Adding entry to allow login for group [%s]", pszMember);
                
                dwError = AD_AddAllowedMember(
                    pContext->pState,
                    ppObjects[0]->pszObjectSid,
                    pszMember,
                    &pAllowedMemberList);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    *ppAllowedMemberList = (PVOID)pAllowedMemberList;

cleanup:

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    if (pLoginInfo)
    {
        LsaSrvFreeNameInfo(pLoginInfo);
    }

    if (ppszMembers)
    {
        LwFreeStringArray(ppszMembers, dwNumMembers);
    }

    if (pSID)
    {
        LsaFreeSecurityIdentifier(pSID);
    }

    ADCacheSafeFreeObject(&pGroupInfo);

    return dwError;

error:

    *ppAllowedMemberList = NULL;

    LsaHashSafeFree(&pAllowedMemberList);

    goto cleanup;
}

static
VOID
InitADCacheFunctionTable(
    PLSA_AD_PROVIDER_STATE pState,
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheProviderTable
    )
{
    switch (pState->config.CacheBackend)
    {
        default:
            LSA_LOG_DEBUG("Unknown cache backend. Switching to default");
#ifdef AD_CACHE_ENABLE_SQLITE
        case AD_CACHE_SQLITE:
            InitializeDbCacheProvider(
                pCacheProviderTable
                );
            break;
#endif
        case AD_CACHE_IN_MEMORY:
            InitializeMemCacheProvider(
                pCacheProviderTable
                );
            break;
    }
}

DWORD
LsaInitializeProvider2(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE_2* ppFunctionTable
    )
{
    return AD_InitializeProvider(ppszProviderName, ppFunctionTable);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
