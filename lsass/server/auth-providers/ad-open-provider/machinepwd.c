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
 *        machinepwd.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Machine Password API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */

#include "adprovider.h"

#define DEFAULT_THREAD_WAITSECS  (30 * LSA_SECONDS_IN_MINUTE)
#define ERROR_THREAD_WAITSECS  (5 * LSA_SECONDS_IN_MINUTE)

//
// Global Module State Type
//

typedef struct _LSA_MACHINEPWD_STATE {
    BOOLEAN bThreadShutdown;
    DWORD dwThreadWaitSecs;
    pthread_t Thread;
    pthread_t* pThread;
    pthread_mutex_t ThreadLock;
    pthread_mutex_t *pThreadLock;
    pthread_cond_t ThreadCondition;
    pthread_cond_t *pThreadCondition;
    DWORD dwTgtExpiry;
    DWORD dwTgtExpiryGraceSeconds;
    // Datalock protects dwTgtExpiry and dwTgtExpiryGraceSeconds
    pthread_rwlock_t DataLock;
    pthread_rwlock_t* pDataLock;
} LSA_MACHINEPWD_STATE, *PLSA_MACHINEPWD_STATE;

static
BOOLEAN
ADShouldRefreshMachineTGT(
    IN PLSA_MACHINEPWD_STATE pMachinePwdState
    );

static
PVOID
ADSyncMachinePasswordThreadRoutine(
    PVOID pData
    );

static
VOID
ADLogMachinePWUpdateSuccessEvent(
    VOID
    );

static
VOID
ADLogMachinePWUpdateFailureEvent(
    DWORD dwErrCode
    );

static
VOID
ADLogMachineTGTRefreshSuccessEvent(
    VOID
    );

static
VOID
ADLogMachineTGTRefreshFailureEvent(
    DWORD dwErrCode
    );

static
VOID
ADSetMachineTGTExpiryInternal(
    PLSA_MACHINEPWD_STATE pMachinePwdState,
    DWORD dwGoodUntil,
    DWORD dwThreadWaitSecs
    );

//
// Function Implementations
//

DWORD
ADInitMachinePasswordSync(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    PLSA_MACHINEPWD_STATE pMachinePwdState = NULL;

    dwError = LwAllocateMemory(
                  sizeof(*pMachinePwdState),
                  (PVOID*)&pMachinePwdState);
    BAIL_ON_LSA_ERROR(dwError);

    pMachinePwdState->bThreadShutdown = FALSE;
    pMachinePwdState->dwThreadWaitSecs = DEFAULT_THREAD_WAITSECS;
    pMachinePwdState->dwTgtExpiryGraceSeconds = 2 * DEFAULT_THREAD_WAITSECS;

    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pMachinePwdState->ThreadLock, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pMachinePwdState->pThreadLock = &pMachinePwdState->ThreadLock;

    dwError = LwMapErrnoToLwError(pthread_cond_init(&pMachinePwdState->ThreadCondition, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pMachinePwdState->pThreadCondition = &pMachinePwdState->ThreadCondition;

    dwError = LwMapErrnoToLwError(pthread_rwlock_init(&pMachinePwdState->DataLock, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pMachinePwdState->pDataLock = &pMachinePwdState->DataLock;

    pState->hMachinePwdState = pMachinePwdState;

cleanup:

    return dwError;

error:

    ADShutdownMachinePasswordSync(&pMachinePwdState);

    goto cleanup;
}

DWORD
ADStartMachinePasswordSync(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    PLSA_MACHINEPWD_STATE pMachinePwdState = pState->hMachinePwdState;

    dwError = pthread_create(&pMachinePwdState->Thread,
                             NULL,
                             ADSyncMachinePasswordThreadRoutine,
                             pState);
    BAIL_ON_LSA_ERROR(dwError);

    pMachinePwdState->pThread = &pMachinePwdState->Thread;

cleanup:

    return dwError;

error:

    pMachinePwdState->pThread = NULL;

    goto cleanup;
}

static
DWORD
ADChangeMachinePasswordInThreadLock(
    IN PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    LSA_LOG_VERBOSE("Changing machine password for %s", pState->pszDomainName);

    dwError = AD_SetSystemAccess(pState, NULL);
    if (dwError)
    {
        LSA_LOG_ERROR("Error: Failed to acquire credentials (error = %u)", dwError);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMachineChangePassword(pState->pszDomainName);
    if (dwError)
    {
        LSA_LOG_ERROR("Error: Failed to change machine password for %s (error = %u)", pState->pszDomainName, dwError);

        if (AD_EventlogEnabled(pState))
        {
            ADLogMachinePWUpdateFailureEvent(dwError);
        }

        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaPcacheClearPasswordInfo(pState->pPcache);

    if (AD_EventlogEnabled(pState))
    {
        ADLogMachinePWUpdateSuccessEvent();
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
PVOID
ADSyncMachinePasswordThreadRoutine(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pState = (PLSA_AD_PROVIDER_STATE)pData;
    PLSA_MACHINEPWD_STATE pMachinePwdState = (PLSA_MACHINEPWD_STATE)pState->hMachinePwdState;
    DWORD dwPasswordSyncLifetime = 0;
    struct timespec timeout = {0, 0};
    DWORD dwGoodUntilTime = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;
    UINT64 lastChangeTime = 0;

    LSA_LOG_INFO("Machine Password Sync Thread starting");

    pthread_mutex_lock(pMachinePwdState->pThreadLock);

    dwError = LwKrb5SetThreadDefaultCachePath(
                  pState->MachineCreds.pszCachePath,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

    for (;;)
    {
        DWORD dwReapingAge = 0;
        DWORD dwCurrentPasswordAge = 0;
        BOOLEAN bRefreshTGT = FALSE;

        if (pMachinePwdState->bThreadShutdown)
        {
           break;
        }

        if (!pState->pProviderData)
        {
            dwError = 0;
            goto lsa_wait_resync;
        }
        ADSyncTimeToDC(pState, pState->pProviderData->szDomain);

        dwError = LsaPcacheGetMachineAccountInfoA(
                      pState->pPcache,
                      &pAccountInfo);
        if (dwError)
        {
            LSA_LOG_ERROR("Error: Failed to get machine account information (error = %u)", dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }

        dwError = ADConvertTimeNt2Unix(
                        (UINT64) pAccountInfo->LastChangeTime,
                        &lastChangeTime);
        if (dwError)
        {
            LSA_LOG_DEBUG("Failed to convert time (error = %u)", dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }

        if (lastChangeTime != (time_t) lastChangeTime)
        {
            LSA_LOG_DEBUG("Overflow in converted time");
            dwError = 0;
            goto lsa_wait_resync;
        }

        dwCurrentPasswordAge = 
                         difftime(
                              time(NULL),
                              (time_t) lastChangeTime);

        dwPasswordSyncLifetime = AD_GetMachinePasswordSyncPwdLifetime(pState);
        dwReapingAge = dwPasswordSyncLifetime / 2;

        dwError = AD_MachineCredentialsCacheInitialize(pState);
        if (dwError)
        {
            LSA_LOG_DEBUG("Failed to initialize credentials cache (error = %u)", dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }

        if ((dwReapingAge > 0) && (dwCurrentPasswordAge >= dwReapingAge))
        {
            PTHREAD_CALL_MUST_SUCCEED(
                    pthread_rwlock_wrlock(pMachinePwdState->pDataLock));

            dwError = ADChangeMachinePasswordInThreadLock(pState);

            PTHREAD_CALL_MUST_SUCCEED(
                    pthread_rwlock_unlock(pMachinePwdState->pDataLock));

            if (dwError)
            {
                dwError = 0;
            }
            else
            {
                bRefreshTGT = TRUE;
            }
        }

        if (!bRefreshTGT)
        {
            bRefreshTGT = ADShouldRefreshMachineTGT(pMachinePwdState);
        }

        if (bRefreshTGT)
        {
            dwError = ADRefreshMachineTGT(pState, &dwGoodUntilTime);
            if (dwError)
            {
                if (AD_EventlogEnabled(pState))
                {
                    ADLogMachineTGTRefreshFailureEvent(dwError);
                }

                LSA_LOG_ERROR("Error: Failed to refresh machine TGT for %s (error = %u)", pState->pszDomainName, dwError);

                if (dwError == LW_ERROR_DOMAIN_IS_OFFLINE)
                {
                    LsaDmTransitionOffline(
                        pState->hDmState,
                        pState->pszDomainName,
                        FALSE);
                }

                ADSetMachineTGTExpiryError(pMachinePwdState);
                dwError = 0;
                goto lsa_wait_resync;
            }

            ADSetMachineTGTExpiry(pMachinePwdState, dwGoodUntilTime);

            LSA_LOG_VERBOSE("Machine TGT was refreshed successfully for %s", pState->pszDomainName);

            if (AD_EventlogEnabled(pState))
            {
                ADLogMachineTGTRefreshSuccessEvent();
            }
        }

lsa_wait_resync:

        LsaPcacheReleaseMachineAccountInfoA(pAccountInfo);
        pAccountInfo = NULL;

        timeout.tv_sec = time(NULL) + pMachinePwdState->dwThreadWaitSecs;
        timeout.tv_nsec = 0;

retry_wait:

        dwError = pthread_cond_timedwait(pMachinePwdState->pThreadCondition,
                                         pMachinePwdState->pThreadLock,
                                         &timeout);

        if (pMachinePwdState->bThreadShutdown)
        {
           break;
        }

        if (dwError == ETIMEDOUT)
        {
            dwError = 0;
            if (time(NULL) < timeout.tv_sec)
            {
                // It didn't really timeout. Something else happened
                goto retry_wait;
            }
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    LsaPcacheReleaseMachineAccountInfoA(pAccountInfo);

    pthread_mutex_unlock(pMachinePwdState->pThreadLock);
    
    LSA_LOG_INFO("Machine Password Sync Thread stopping");

    return NULL;

error:

    LSA_LOG_ERROR("Machine password sync thread exiting due to error [code: %ld]", dwError);

    goto cleanup;
}

VOID
ADSyncTimeToDC(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszDomainFQDN
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T dcTime = 0;
    time_t ttDcTime = 0;

    if ( !pState->bIsDefault || !AD_ShouldSyncSystemTime(pState) )
    {
        goto cleanup;
    }

    BAIL_ON_INVALID_STRING(pszDomainFQDN);

    if (LsaDmIsDomainOffline(pState->hDmState, pszDomainFQDN))
    {
        goto cleanup;
    }

    dwError = LWNetGetDCTime(
                    pszDomainFQDN,
                    &dcTime);
    BAIL_ON_LSA_ERROR(dwError);
    
    ttDcTime = (time_t) dcTime;
    
    if (labs(ttDcTime - time(NULL)) > AD_GetClockDriftSeconds(pState)) {
        dwError = LsaSetSystemTime(ttDcTime);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    return;
    
error:

    LSA_LOG_ERROR("Failed to sync system time [error code: %u]", dwError);

    goto cleanup;
}

VOID
ADLockMachinePassword(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState
    )
{
    PTHREAD_CALL_MUST_SUCCEED(
            pthread_rwlock_rdlock(hMachinePwdState->pDataLock));
}

VOID
ADUnlockMachinePassword(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState
    )
{
    PTHREAD_CALL_MUST_SUCCEED(
            pthread_rwlock_unlock(hMachinePwdState->pDataLock));
}

VOID
ADShutdownMachinePasswordSync(
    IN OUT LSA_MACHINEPWD_STATE_HANDLE* phMachinePwdState
    )
{
    if (phMachinePwdState && *phMachinePwdState)
    {
        PLSA_MACHINEPWD_STATE pMachinePwdState = (PLSA_MACHINEPWD_STATE)*phMachinePwdState;

        if (pMachinePwdState->pThread)
        {   
            pthread_mutex_lock(pMachinePwdState->pThreadLock);
            pMachinePwdState->bThreadShutdown = TRUE;
            pthread_cond_signal(pMachinePwdState->pThreadCondition);
            pthread_mutex_unlock(pMachinePwdState->pThreadLock);

            pthread_join(pMachinePwdState->Thread, NULL);
            pMachinePwdState->pThread = NULL;
            pMachinePwdState->bThreadShutdown = FALSE;
        }

        if (pMachinePwdState->pThreadCondition)
        {
            pthread_cond_destroy(pMachinePwdState->pThreadCondition);
        }

        if (pMachinePwdState->pThreadLock)
        {
            pthread_mutex_destroy(pMachinePwdState->pThreadLock);
        }

        if (pMachinePwdState->pDataLock)
        {
            pthread_rwlock_destroy(pMachinePwdState->pDataLock);
        }

        LwFreeMemory(pMachinePwdState);
        *phMachinePwdState = NULL;
    }
}

DWORD
ADRefreshMachineTGT(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT OPTIONAL PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    DWORD dwGoodUntilTime = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;
    PSTR pszUserPrincipalName = NULL;

    LSA_LOG_VERBOSE("Refreshing machine TGT");

    dwError = LsaPcacheGetMachinePasswordInfoA(
                    pState->pPcache,
                    &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszUserPrincipalName,
                    "%s@%s",
                    pPasswordInfo->Account.SamAccountName,
                    pPasswordInfo->Account.DnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwKrb5InitializeCredentials(
                    pszUserPrincipalName,
                    pPasswordInfo->Password,
                    pState->MachineCreds.pszCachePath,
                    &dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (dwError)
    {
        dwGoodUntilTime = 0;
    }

    LW_SAFE_FREE_STRING(pszUserPrincipalName);
    LsaPcacheReleaseMachinePasswordInfoA(pPasswordInfo);

    if (pdwGoodUntilTime)
    {
        *pdwGoodUntilTime = dwGoodUntilTime;
    }

    return dwError;
}

static
BOOLEAN
ADShouldRefreshMachineTGT(
    IN PLSA_MACHINEPWD_STATE pMachinePwdState
    )
{
    BOOLEAN bRefresh = FALSE;
    int status = 0;

    status = pthread_rwlock_rdlock(pMachinePwdState->pDataLock);
    LW_ASSERT(status == 0);

    if (!pMachinePwdState->dwTgtExpiry ||
        (difftime(pMachinePwdState->dwTgtExpiry, time(NULL)) <= pMachinePwdState->dwTgtExpiryGraceSeconds))
    {
        bRefresh = TRUE;
    }

    status = pthread_rwlock_unlock(pMachinePwdState->pDataLock);
    LW_ASSERT(status == 0);

    return bRefresh;
}

VOID
ADSetMachineTGTExpiry(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState,
    IN DWORD dwGoodUntil
    )
{
    ADSetMachineTGTExpiryInternal(
        hMachinePwdState,
        dwGoodUntil,
        DEFAULT_THREAD_WAITSECS);
}

VOID
ADSetMachineTGTExpiryError(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState
    )
{
    ADSetMachineTGTExpiryInternal(
        hMachinePwdState,
        0,
        ERROR_THREAD_WAITSECS);
}

static
VOID
ADSetMachineTGTExpiryInternal(
    PLSA_MACHINEPWD_STATE pMachinePwdState,
    DWORD dwGoodUntil,
    DWORD dwThreadWaitSecs
    )
{
    DWORD lifetime = 0;
    int status = 0;

    status = pthread_rwlock_wrlock(pMachinePwdState->pDataLock);
    LW_ASSERT(status == 0);

    if (dwGoodUntil)
    {
        pMachinePwdState->dwTgtExpiry = dwGoodUntil;

        lifetime = difftime(
                       pMachinePwdState->dwTgtExpiry,
                       time(NULL));

        pMachinePwdState->dwTgtExpiryGraceSeconds = 
            LW_MAX(lifetime / 2, 2 * DEFAULT_THREAD_WAITSECS);
    }

    if (dwThreadWaitSecs)
    {
        pMachinePwdState->dwThreadWaitSecs = dwThreadWaitSecs;
    }
    else
    {
        pMachinePwdState->dwThreadWaitSecs = DEFAULT_THREAD_WAITSECS;
    }

    status = pthread_rwlock_unlock(pMachinePwdState->pDataLock);
    LW_ASSERT(status == 0);
}

static
VOID
ADLogMachinePWUpdateSuccessEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Updated Active Directory machine password.\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_PASSWORD_UPDATE,
            PASSWORD_EVENT_CATEGORY,
            pszDescription,
            NULL);
    
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;
    
error:

    goto cleanup;
}

static
VOID
ADLogMachinePWUpdateFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Active Directory machine password failed to update.\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
      
    LsaSrvLogServiceFailureEvent(
            LSASS_EVENT_FAILED_MACHINE_ACCOUNT_PASSWORD_UPDATE,
            PASSWORD_EVENT_CATEGORY,
            pszDescription,
            pszData);
    
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;
    
error:

    goto cleanup;
}

static
VOID
ADLogMachineTGTRefreshSuccessEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Refreshed Active Directory machine account TGT (Ticket Granting Ticket).\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_SUCCESSFUL_MACHINE_ACCOUNT_TGT_REFRESH,
            KERBEROS_EVENT_CATEGORY,
            pszDescription,
            NULL);
    
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;
    
error:

    goto cleanup;
}

static
VOID
ADLogMachineTGTRefreshFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Active Directory machine account TGT (Ticket Granting Ticket) failed to refresh.\r\n\r\n" \
                 "     Authentication provider:   %s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName));
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
      
    LsaSrvLogServiceFailureEvent(
            LSASS_EVENT_FAILED_MACHINE_ACCOUNT_TGT_REFRESH,
            KERBEROS_EVENT_CATEGORY,
            pszDescription,
            pszData);
    
cleanup:

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

    return;
    
error:

    goto cleanup;
}
