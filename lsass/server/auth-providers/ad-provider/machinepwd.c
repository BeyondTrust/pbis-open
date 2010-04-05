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

typedef struct _AD_MACHINE_PASSWORD_SYNC_STATE {
    BOOLEAN bThreadShutdown;
    DWORD dwThreadWaitSecs;
    pthread_t Thread;
    pthread_mutex_t ThreadLock;
    pthread_cond_t ThreadCondition;
    pthread_t* pThread;
    HANDLE hPasswordStore;
    DWORD dwTgtExpiry;
    DWORD dwTgtExpiryGraceSeconds;
} AD_MACHINE_PASSWORD_SYNC_STATE, *PAD_MACHINE_PASSWORD_SYNC_STATE;

//
// Global Module State Variable
//

//
// ISSUE-2009/07/24-dalmeida -- Make the global state be
// dynamically initialized rather than be statically
// initialized as a global so we can easily restart.
//

static AD_MACHINE_PASSWORD_SYNC_STATE gAdMachinePasswordSyncState = {
    .bThreadShutdown = FALSE,
    .dwThreadWaitSecs = DEFAULT_THREAD_WAITSECS,
    .ThreadLock = PTHREAD_MUTEX_INITIALIZER,
    .ThreadCondition = PTHREAD_COND_INITIALIZER,
    .pThread = NULL,
    .hPasswordStore = NULL,
    .dwTgtExpiry = 0,
    .dwTgtExpiryGraceSeconds = 2 * DEFAULT_THREAD_WAITSECS,
};

//
// Static Function Prototypes
//

static
BOOLEAN
ADShouldRefreshMachineTGT(
    VOID
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
    DWORD dwGoodUntil,
    DWORD dwThreadWaitSecs
    );

//
// Function Implementations
//

DWORD
ADInitMachinePasswordSync(
    VOID
    )
{
    DWORD dwError = 0;
    
    dwError = LwpsOpenPasswordStore(
                    LWPS_PASSWORD_STORE_DEFAULT,
                    &gAdMachinePasswordSyncState.hPasswordStore);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
ADStartMachinePasswordSync(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = pthread_create(&gAdMachinePasswordSyncState.Thread,
                             NULL,
                             ADSyncMachinePasswordThreadRoutine,
                             NULL);
    BAIL_ON_LSA_ERROR(dwError);

    gAdMachinePasswordSyncState.pThread = &gAdMachinePasswordSyncState.Thread;

cleanup:

    return dwError;

error:

    gAdMachinePasswordSyncState.pThread = NULL;

    goto cleanup;
}

static
PVOID
ADSyncMachinePasswordThreadRoutine(
    PVOID pData
    )
{
    DWORD dwError = 0;
    DWORD dwPasswordSyncLifetime = 0;
    struct timespec timeout = {0, 0};
    PLWPS_PASSWORD_INFO pAcctInfo = NULL;    
    PSTR pszHostname = NULL;
    PSTR pszDnsDomainName = NULL;
    DWORD dwGoodUntilTime = 0;

    LSA_LOG_INFO("Machine Password Sync Thread starting");

    pthread_mutex_lock(&gAdMachinePasswordSyncState.ThreadLock);

    for (;;)
    {
        DWORD dwReapingAge = 0;
        DWORD dwCurrentPasswordAge = 0;
        BOOLEAN bRefreshTGT = FALSE;

        dwError = LsaDnsGetHostInfo(&pszHostname);
        if (dwError)
        {
            LSA_LOG_ERROR("Error: Failed to find hostname (Error code: %ld)",
                          dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }

        ADSyncTimeToDC(gpADProviderData->szDomain);
        
        dwError = LwpsGetPasswordByHostName(
                        gAdMachinePasswordSyncState.hPasswordStore,
                        pszHostname,
                        &pAcctInfo);
        if (dwError)
        {
            LSA_LOG_ERROR("Error: Failed to re-sync machine account (Error code: %ld)", dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }

        dwCurrentPasswordAge = 
                         difftime(
                              time(NULL),
                              pAcctInfo->last_change_time);

        dwPasswordSyncLifetime = AD_GetMachinePasswordSyncPwdLifetime();
        dwReapingAge = dwPasswordSyncLifetime / 2;

        dwError = AD_MachineCredentialsCacheInitialize();
        if (dwError)
        {
            LSA_LOG_DEBUG("Failed to initialize credentials cache (error = %d)", dwError);
            dwError = 0;
            goto lsa_wait_resync;
        }

        if ((dwReapingAge > 0) && (dwCurrentPasswordAge >= dwReapingAge))
        {
            LSA_LOG_VERBOSE("Changing machine password");

            dwError = AD_SetSystemAccess(NULL);
            if (dwError)
            {
                LSA_LOG_ERROR("Error: Failed to acquire credentials, error = %u", dwError);

                dwError = 0;
                goto lsa_wait_resync;
            }

            dwError = NetMachineChangePassword();           
            if (dwError)
            {
                LSA_LOG_ERROR("Error: Failed to re-sync machine account [Error code: %ld]", dwError);                

                if (AD_EventlogEnabled())
                {
                    ADLogMachinePWUpdateFailureEvent(dwError);      
                }

                dwError = 0;
                goto lsa_wait_resync;
            }            

            if (AD_EventlogEnabled())
            {
                ADLogMachinePWUpdateSuccessEvent();      
            }            

            bRefreshTGT = TRUE;
        }
        else
        {
            bRefreshTGT = ADShouldRefreshMachineTGT();
        }

        if (bRefreshTGT)
        {
            dwError = LwKrb5RefreshMachineTGT(&dwGoodUntilTime);
            if (dwError)
            {
                if (AD_EventlogEnabled())
                {
                    ADLogMachineTGTRefreshFailureEvent(dwError);      
                }
                
                LSA_LOG_ERROR("Error: Failed to refresh machine TGT [Error code: %ld]", dwError);

                if (dwError == LW_ERROR_DOMAIN_IS_OFFLINE)
                {
                    LW_SAFE_FREE_STRING(pszDnsDomainName);

                    dwError = LsaWc16sToMbs(pAcctInfo->pwszDnsDomainName, &pszDnsDomainName);
                    BAIL_ON_LSA_ERROR(dwError);
                    
                    LsaDmTransitionOffline(pszDnsDomainName, FALSE);
                }

                ADSetMachineTGTExpiryError();
                dwError = 0;
                goto lsa_wait_resync;
            }

            ADSetMachineTGTExpiry(dwGoodUntilTime);

            LSA_LOG_VERBOSE("Machine TGT was refreshed successfully");

            if (AD_EventlogEnabled())
            {
                ADLogMachineTGTRefreshSuccessEvent();      
            }
        }
        
lsa_wait_resync:

        if (pAcctInfo)
        {
            LwpsFreePasswordInfo(gAdMachinePasswordSyncState.hPasswordStore, pAcctInfo);
            pAcctInfo = NULL;
        }

        LW_SAFE_FREE_STRING(pszHostname);

        timeout.tv_sec = time(NULL) + gAdMachinePasswordSyncState.dwThreadWaitSecs;
        timeout.tv_nsec = 0;

retry_wait:

        dwError = pthread_cond_timedwait(&gAdMachinePasswordSyncState.ThreadCondition,
                                         &gAdMachinePasswordSyncState.ThreadLock,
                                         &timeout);

        if (gAdMachinePasswordSyncState.bThreadShutdown)
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

    if (pAcctInfo)
    {
        LwpsFreePasswordInfo(gAdMachinePasswordSyncState.hPasswordStore, pAcctInfo);
    }

    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszDnsDomainName);

    pthread_mutex_unlock(&gAdMachinePasswordSyncState.ThreadLock);
    
    LSA_LOG_INFO("Machine Password Sync Thread stopping");

    return NULL;

error:

    LSA_LOG_ERROR("Machine password sync thread exiting due to error [code: %ld]", dwError);

    goto cleanup;
}

VOID
ADSyncTimeToDC(
    PCSTR pszDomainFQDN
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T dcTime = 0;
    time_t ttDcTime = 0;

    if ( !AD_ShouldSyncSystemTime() )
    {
        goto cleanup;
    }

    BAIL_ON_INVALID_STRING(pszDomainFQDN);

    if (LsaDmIsDomainOffline(pszDomainFQDN))
    {
        goto cleanup;
    }

    dwError = LWNetGetDCTime(
                    pszDomainFQDN,
                    &dcTime);
    BAIL_ON_LSA_ERROR(dwError);
    
    ttDcTime = (time_t) dcTime;
    
    if (labs(ttDcTime - time(NULL)) > AD_GetClockDriftSeconds()) {
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
ADShutdownMachinePasswordSync(
    VOID
    )
{
    if (gAdMachinePasswordSyncState.pThread)
    {   
        pthread_mutex_lock(&gAdMachinePasswordSyncState.ThreadLock);
        gAdMachinePasswordSyncState.bThreadShutdown = TRUE;
        pthread_cond_signal(&gAdMachinePasswordSyncState.ThreadCondition);
        pthread_mutex_unlock(&gAdMachinePasswordSyncState.ThreadLock);

        pthread_join(gAdMachinePasswordSyncState.Thread, NULL);
        gAdMachinePasswordSyncState.pThread = NULL;
    }

    if (gAdMachinePasswordSyncState.hPasswordStore)
    {
        LwpsClosePasswordStore(gAdMachinePasswordSyncState.hPasswordStore);
        gAdMachinePasswordSyncState.hPasswordStore = NULL;
    }
}

static
BOOLEAN
ADShouldRefreshMachineTGT(
    VOID
    )
{
    BOOLEAN bRefresh = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!gAdMachinePasswordSyncState.dwTgtExpiry ||
        (difftime(gAdMachinePasswordSyncState.dwTgtExpiry, time(NULL)) <= gAdMachinePasswordSyncState.dwTgtExpiryGraceSeconds))
    {
        bRefresh = TRUE;
    }

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bRefresh;
}

VOID
ADSetMachineTGTExpiry(
    DWORD dwGoodUntil
    )
{
    ADSetMachineTGTExpiryInternal(dwGoodUntil, DEFAULT_THREAD_WAITSECS);
}

VOID
ADSetMachineTGTExpiryError(
    VOID
    )
{
    ADSetMachineTGTExpiryInternal(0, ERROR_THREAD_WAITSECS);
}

static
VOID
ADSetMachineTGTExpiryInternal(
    DWORD dwGoodUntil,
    DWORD dwThreadWaitSecs
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD lifetime = 0;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (dwGoodUntil)
    {
        gAdMachinePasswordSyncState.dwTgtExpiry = dwGoodUntil;

        lifetime = difftime(
                       gAdMachinePasswordSyncState.dwTgtExpiry,
                       time(NULL));

        gAdMachinePasswordSyncState.dwTgtExpiryGraceSeconds = 
            LW_MAX(lifetime / 2, 2 * DEFAULT_THREAD_WAITSECS);
    }

    if (dwThreadWaitSecs)
    {
        gAdMachinePasswordSyncState.dwThreadWaitSecs = dwThreadWaitSecs;
    }
    else
    {
        gAdMachinePasswordSyncState.dwThreadWaitSecs = DEFAULT_THREAD_WAITSECS;
    }

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
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
