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
#include "provider-main.h"
#include "lsaum_p.h"

/// Minimum time interval to wait between runs.
#define LSA_UM_THREAD_MIN_PERIOD (5 * LSA_SECONDS_IN_MINUTE)

/// Minimum time interval to wait between checking user's
/// login status.
#define LSA_UM_USER_MIN_PERIOD (2 * LSA_SECONDS_IN_MINUTE)

#define LSA_UM_STATE_LOCK(bInLock)                   \
        do {                                         \
            if (!bInLock) {                          \
               LsaUmpAcquireMutex(Handle->pMutex);   \
               bInLock = TRUE;                       \
            }                                        \
        } while (0)

#define LSA_UM_STATE_UNLOCK(bInLock)                 \
        do {                                         \
            if (bInLock) {                           \
               LsaUmpReleaseMutex(Handle->pMutex);   \
               bInLock = FALSE;                      \
            }                                        \
        } while (0)

typedef struct _LSA_UM_USER_REFRESH_ITEM {
    uid_t uUid;
    DWORD dwTgtEndTime;
    DWORD dwLastActivity;
    LSA_CRED_HANDLE CredHandle;

    // Number of consecutive times TGT refresh failed.
    // If the number grows too high the user will be
    // dropped from the list.  TGT refresh can fail
    // permanently if the user's password is changed
    // via another host.
    DWORD                              dwFailedCount;
    struct _LSA_UM_USER_REFRESH_ITEM * pNext;
} LSA_UM_USER_REFRESH_ITEM, *PLSA_UM_USER_REFRESH_ITEM;

typedef struct _LSA_UM_USER_REFRESH_LIST {
    struct _LSA_UM_USER_REFRESH_ITEM * pFirst;
} LSA_UM_USER_REFRESH_LIST, *PLSA_UM_USER_REFRESH_LIST;

typedef DWORD LSA_UM_REQUEST_TYPE;

#define LSA_UM_REQUEST_TYPE_ADD    1
#define LSA_UM_REQUEST_TYPE_MODIFY 2
#define LSA_UM_REQUEST_TYPE_REMOVE 3

typedef struct _LSA_UM_REQUEST_ITEM {
    LSA_UM_REQUEST_TYPE dwType;
    uid_t uUid;
    DWORD dwTgtEndTime;
    DWORD dwLastActivity;
    LSA_CRED_HANDLE CredHandle;
} LSA_UM_REQUEST_ITEM, *PLSA_UM_REQUEST_ITEM;

typedef struct _LSA_UM_THREAD_INFO {
    pthread_t Thread;
    pthread_t* pThread;
    pthread_mutex_t* pMutex;
    pthread_cond_t* pCondition;
    BOOLEAN bIsDone;
    BOOLEAN bTrigger;
} LSA_UM_THREAD_INFO, *PLSA_UM_THREAD_INFO;

typedef struct _LSA_UM_KSCHEDULES {
    DES_key_schedule                   kSchedule[3];
} LSA_UM_KSCHEDULES, *PLSA_UM_KSCHEDULES;

///
/// Keeps track of all domain state.
///
typedef struct _LSA_UM_STATE {
    /// Provider state.
    PLSA_AD_PROVIDER_STATE pProviderState;

    /// Linked list of users.
    PLSA_UM_USER_REFRESH_LIST UserList;

    /// Linked list of requests;
    PLW_DLINKED_LIST RequestList;

    PLSA_UM_KSCHEDULES kSchedules;

    /// Lock for general state (UserList, etc).
    pthread_mutex_t* pMutex;

    /// Online detection thread info
    LSA_UM_THREAD_INFO CheckUsersThread;

    /// Number of seconds between checking user status
    DWORD dwCheckUsersSeconds;
} LSA_UM_STATE, *PLSA_UM_STATE;


static
DWORD
LsaUmpCheckUsers(
    PLSA_UM_STATE       Handle,
    PLSA_UM_THREAD_INFO pThreadInfo
    );

static
DWORD
LsaUmpCreateKeys(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_UM_KSCHEDULES kSchedules
    );

static
PVOID
LsaUmpThreadRoutine(
    IN PVOID pContext
    );

static
VOID
LsaUmpFreeUserList(
    PLSA_UM_USER_REFRESH_LIST pUserList
    );

static
VOID
LsaUmpFreeRequestList(
    PLW_DLINKED_LIST pRequestList
    );

static
DWORD
LsaUmpAddUserInternal(
    LSA_UM_STATE_HANDLE  Handle,
    PLSA_UM_REQUEST_ITEM pRequest
    );

static
DWORD
LsaUmpModifyUserInternal(
    LSA_UM_STATE_HANDLE  Handle,
    PLSA_UM_REQUEST_ITEM pRequest
    );

static
DWORD
LsaUmpRemoveUserInternal(
    LSA_UM_STATE_HANDLE  Handle,
    PLSA_UM_REQUEST_ITEM pRequest
    );

static
DWORD
LsaUmpRemoveUserFromList(
    PLSA_UM_USER_REFRESH_LIST pUserList,
    uid_t                     uUid
    );

static
DWORD
LsaUmpRefreshUserCreds(
    LSA_UM_STATE_HANDLE       Handle,
    PAD_PROVIDER_CONTEXT      pContext,
    PLSA_UM_USER_REFRESH_ITEM pItem
    );

VOID
LsaUmpFreeUserItem(
    PLSA_UM_USER_REFRESH_ITEM pUserItem
    );

VOID
LsaUmpLogUserTGTRefreshSuccessEvent(
    PSTR  pszUsername,
    uid_t uid,
    PSTR  pszDomainName,
    DWORD dwTgtEndTime
    );

VOID
LsaUmpLogUserTGTRefreshFailureEvent(
    PSTR  pszUsername,
    uid_t uid,
    PSTR  pszDomainName,
    DWORD dwFailureNumber,
    DWORD dwErrCode
    );

static
VOID
LsaUmpDestroyMutex(
    IN OUT pthread_mutex_t ** ppMutex
    )
{
    if (*ppMutex)
    {
        pthread_mutex_destroy(*ppMutex);
        LwFreeMemory(*ppMutex);
        *ppMutex = NULL;
    }
}

static
DWORD
LsaUmpCreateMutex(
    OUT pthread_mutex_t ** ppMutex,
    IN  int                MutexType
    )
{
    DWORD                 dwError = 0;
    pthread_mutexattr_t   mutexAttr;
    pthread_mutexattr_t * pMutexAttr = NULL;
    pthread_mutex_t *     pMutex = NULL;

    if (MutexType)
    {
        dwError = pthread_mutexattr_init(&mutexAttr);
        BAIL_ON_LSA_ERROR(dwError);

        pMutexAttr = &mutexAttr;

        dwError = pthread_mutexattr_settype(
                      pMutexAttr,
                      PTHREAD_MUTEX_RECURSIVE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                  sizeof(*pMutex),
                  (PVOID*)&pMutex);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_mutex_init(
                  pMutex,
                  pMutexAttr);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pMutexAttr)
    {
        pthread_mutexattr_destroy(pMutexAttr);
    }

    *ppMutex = pMutex;
    return dwError;

error:
    // Note that we do not need to destroy as we failed to init.
    LW_SAFE_FREE_MEMORY(pMutex);
    goto cleanup;
}


static
VOID
LsaUmpAcquireMutex(
    IN pthread_mutex_t * pMutex
    )
{
    DWORD dwError = pthread_mutex_lock(pMutex);
    if (dwError)
    {
        LSA_LOG_ERROR("pthread_mutex_lock() failed: %u", dwError);
    }
}

static
VOID
LsaUmpReleaseMutex(
    IN pthread_mutex_t * pMutex
    )
{
    DWORD dwError = pthread_mutex_unlock(pMutex);
    if (dwError)
    {
        LSA_LOG_ERROR("pthread_mutex_unlock() failed: %u", dwError);
    }
}

static
VOID
LsaUmpDestroyCond(
    IN OUT pthread_cond_t ** ppCond
    )
{
    if (*ppCond)
    {
        pthread_cond_destroy(*ppCond);
        LwFreeMemory(*ppCond);
        *ppCond = NULL;
    }
}

static
DWORD
LsaUmpCreateCond(
    OUT pthread_cond_t ** ppCond
    )
{
    DWORD            dwError = 0;
    pthread_cond_t * pCond = NULL;

    dwError = LwAllocateMemory(
                  sizeof(*pCond),
                  (PVOID*)&pCond);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_cond_init(pCond, NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppCond = pCond;
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pCond);
    goto cleanup;
}

VOID
LsaUmpStateDestroy(
    IN OUT LSA_UM_STATE_HANDLE Handle
    )
///<
/// Destroy a state object for the offline manager.
///
/// This includes stopping the online detection thread.
///
/// @param[in, out] Handle - Offline state manager object to destroy.
///
/// @return N/A
///
{
    if (Handle)
    {
        if (Handle->CheckUsersThread.pThread)
        {
            void* threadResult = NULL;
            LsaUmpAcquireMutex(Handle->CheckUsersThread.pMutex);
            Handle->CheckUsersThread.bIsDone = TRUE;
            LsaUmpReleaseMutex(Handle->CheckUsersThread.pMutex);
            pthread_cond_signal(Handle->CheckUsersThread.pCondition);
            pthread_join(*Handle->CheckUsersThread.pThread, &threadResult);
            Handle->CheckUsersThread.pThread = NULL;
        }
        LsaUmpDestroyCond(&Handle->CheckUsersThread.pCondition);
        LsaUmpDestroyMutex(&Handle->CheckUsersThread.pMutex);
        LsaUmpDestroyMutex(&Handle->pMutex);

        LsaUmpFreeUserList( Handle->UserList );

        LsaUmpFreeRequestList( Handle->RequestList );

        LW_SAFE_FREE_MEMORY(Handle->kSchedules);

        LW_SAFE_FREE_MEMORY(Handle);
    }
}

DWORD
LsaUmpStateCreate(
    IN PLSA_AD_PROVIDER_STATE pProviderState,
    OUT PLSA_UM_STATE_HANDLE pHandle
    )
///<
/// Create an empty state object for the user credentials manager.
///
/// This includes starting up the user credentials refresh thread.
///
/// @param[out] pHandle - Returns new user manager state object.
///
/// @return LSA status code.
///  @arg LW_ERROR_SUCCESS on success
///  @arg !LW_ERROR_SUCCESS on failure
///
{
    DWORD         dwError = 0;
    PLSA_UM_STATE pState = NULL;
    BOOLEAN       bIsAcquired = FALSE;

    dwError = LwAllocateMemory(
                  sizeof(*pState),
                  (PVOID*)&pState);
    BAIL_ON_LSA_ERROR(dwError);

    pState->pProviderState = pProviderState;

    pState->dwCheckUsersSeconds = LSA_UM_THREAD_MIN_PERIOD;

    dwError = LsaUmpCreateMutex(
                  &pState->pMutex,
                  PTHREAD_MUTEX_RECURSIVE);
    BAIL_ON_LSA_ERROR(dwError);

    // Acquire to block the thread from checking online seconds.
    LsaUmpAcquireMutex(pState->pMutex);
    bIsAcquired = TRUE;

    dwError = LsaUmpCreateMutex(
                  &pState->CheckUsersThread.pMutex,
                  0);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmpCreateCond(&pState->CheckUsersThread.pCondition);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                  sizeof(*pState->UserList),
                  (PVOID*)&pState->UserList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                  sizeof(*pState->kSchedules),
                  (PVOID*)&pState->kSchedules);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmpCreateKeys(
                  pProviderState,
                  pState->kSchedules);
    BAIL_ON_LSA_ERROR(dwError);

    // Now that everything is set up, we need to initialize the thread.

    dwError = pthread_create(
                  &pState->CheckUsersThread.Thread,
                  NULL,
                  LsaUmpThreadRoutine,
                  pState);
    BAIL_ON_LSA_ERROR(dwError);

    // Indicate that the thread is created
    pState->CheckUsersThread.pThread = &pState->CheckUsersThread.Thread;

cleanup:
    if (bIsAcquired)
    {
        LsaUmpReleaseMutex(pState->pMutex);
    }
    *pHandle = pState;

    return dwError;

error:
    // ISSUE-2008/09/10-dalmeida -- Good example of why error label is bad.
    // We end up having to duplicate this code under error and cleanup.
    if (bIsAcquired)
    {
        LsaUmpReleaseMutex(pState->pMutex);
        bIsAcquired = FALSE;
    }
    if (pState)
    {
        LsaUmpStateDestroy(pState);
        pState = NULL;
    }

    goto cleanup;
}

VOID
LsaUmpTriggerCheckUsersThread(
    IN LSA_UM_STATE_HANDLE Handle
    )
{
    if (!Handle)
    {
        return;
    }
    LsaUmpAcquireMutex(Handle->CheckUsersThread.pMutex);
    Handle->CheckUsersThread.bTrigger = TRUE;
    pthread_cond_signal(Handle->CheckUsersThread.pCondition);
    LsaUmpReleaseMutex(Handle->CheckUsersThread.pMutex);
}

static
PVOID
LsaUmpThreadRoutine(
    IN PVOID pContext
    )
{
    DWORD               dwError = 0;
    PLSA_UM_STATE       pState = (PLSA_UM_STATE) pContext;
    PLSA_UM_THREAD_INFO pThreadInfo = &pState->CheckUsersThread;
    time_t              lastCheckTime = 0;

    LSA_LOG_VERBOSE("Started user manager credentials refresh thread");

    dwError = LwKrb5SetThreadDefaultCachePath(
                  pState->pProviderState->MachineCreds.pszCachePath,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

    for (;;)
    {
        DWORD   dwCheckUsersSeconds = 0;
        BOOLEAN bIsDone = FALSE;
        time_t  nextCheckTime = 0;
        struct  timespec wakeTime = { 0 };
        BOOLEAN bIsTriggered = FALSE;

        // Get the checking interval first.  This also synchronizes
        // thread startup wrt thread creation.
        LsaUmpAcquireMutex(pState->pMutex);
        dwCheckUsersSeconds = pState->dwCheckUsersSeconds;
        LsaUmpReleaseMutex(pState->pMutex);

        // Note that we assume nobody is setting the date
        // to before 1970...
        if (!lastCheckTime)
        {
            nextCheckTime = time(NULL) + LSA_UM_THREAD_MIN_PERIOD;
        }
        else
        {
            //
            // We compute the next time to check based on the last
            // time we checked, making sure that we are not going
            // too often (e.g., if checking is taking a long time
            // compared to the interval).  This also handles
            // someone setting very short interval (e.g., 0 seconds).
            //

            time_t minNextCheckTime = time(NULL) + LSA_UM_THREAD_MIN_PERIOD;
            nextCheckTime = lastCheckTime + dwCheckUsersSeconds;
            // Make sure that we are not checking more often than allowed.
            nextCheckTime = LSA_MAX(nextCheckTime, minNextCheckTime);
        }

        memset(&wakeTime, 0, sizeof(wakeTime));
        wakeTime.tv_sec = nextCheckTime;

retry_wait:

        LsaUmpAcquireMutex(pThreadInfo->pMutex);
        bIsDone = pThreadInfo->bIsDone;
        bIsTriggered = pThreadInfo->bTrigger;
        if (!bIsDone && !bIsTriggered)
        {
            // TODO: Error code conversion
            dwError = pthread_cond_timedwait(
                          pThreadInfo->pCondition,
                          pThreadInfo->pMutex,
                          &wakeTime);
            bIsDone = pThreadInfo->bIsDone;
            bIsTriggered = pThreadInfo->bTrigger;
        }
        pThreadInfo->bTrigger = FALSE;
        LsaUmpReleaseMutex(pThreadInfo->pMutex);
        if (bIsDone)
        {
            break;
        }
        if (ETIMEDOUT == dwError && !bIsTriggered)
        {
            if (time(NULL) < wakeTime.tv_sec)
            {
                // It didn't really timeout. Something else happened
                dwError = 0;
                goto retry_wait;
            }
        }
        if (ETIMEDOUT == dwError || bIsTriggered)
        {
            // Mark the time so we don't try to check again too soon.
            lastCheckTime = time(NULL);

            // Check user entries
            dwError = LsaUmpCheckUsers(
                          pState,
                          pThreadInfo);
            if (dwError)
            {
                LSA_LOG_ERROR("Error while checking user refresh credentials list: %u", dwError);
                dwError = 0;
            }
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LSA_LOG_VERBOSE("Stopped user manager credentials refresh thread");
    return NULL;

error:
    LSA_LOG_ERROR("Unexpected error in user manager credentials refresh thread (%u)", dwError);
    goto cleanup;
}

static
DWORD
LsaUmpCheckUsers(
    PLSA_UM_STATE       Handle,
    PLSA_UM_THREAD_INFO pThreadInfo
    )
{
    DWORD                     dwError = 0;
    BOOLEAN                   bInLock = FALSE;
    BOOLEAN                   bDomainIsOffline = FALSE;
    BOOLEAN                   bShouldRefreshCreds = FALSE;
    BOOLEAN                   bUserIsActive = TRUE;
    PLW_DLINKED_LIST              pRequestList = NULL;
    PLSA_UM_USER_REFRESH_LIST pUserList = NULL;
    PLSA_UM_USER_REFRESH_ITEM pItem = NULL;
    PLSA_UM_USER_REFRESH_ITEM pNextItem = NULL;
    DWORD                     dwTime = 0;
    PAD_PROVIDER_CONTEXT      pProviderContext = NULL;

    LSA_LOG_VERBOSE("Lsa User Manager - checking user credentials refresh list");

    LSA_UM_STATE_LOCK(bInLock);
    pUserList = Handle->UserList;
    pRequestList = Handle->RequestList;
    Handle->RequestList = NULL;
    LSA_UM_STATE_UNLOCK(bInLock);

    if ( pRequestList )
    {
        PLW_DLINKED_LIST         pList = pRequestList;
        PLSA_UM_REQUEST_ITEM pRequest = NULL;

        // Find the last item so we can process the
        // list in reverse order since that is the
        // order that requests were submitted.

        while (pList->pNext)
        {
            pList = pList->pNext;
        }

        while (pList)
        {
            pRequest = (PLSA_UM_REQUEST_ITEM)pList->pItem;

            switch (pRequest->dwType)
            {
                case LSA_UM_REQUEST_TYPE_ADD:
                    dwError = LsaUmpAddUserInternal(
                                  Handle,
                                  pRequest);
                    break;
                case LSA_UM_REQUEST_TYPE_MODIFY:
                    dwError = LsaUmpModifyUserInternal(
                                  Handle,
                                  pRequest);
                    break;
                case LSA_UM_REQUEST_TYPE_REMOVE:
                    dwError = LsaUmpRemoveUserInternal(
                                  Handle,
                                  pRequest);
                    break;
            }

            pList = pList->pPrev;
        }
    }

    ADSyncTimeToDC(
        Handle->pProviderState,
        Handle->pProviderState->pProviderData->szDomain);

    bDomainIsOffline = LsaDmIsDomainOffline(
                           Handle->pProviderState->hDmState,
                           Handle->pProviderState->pProviderData->szDomain);
    bShouldRefreshCreds = AD_ShouldRefreshUserCreds(Handle->pProviderState);

    if ( bDomainIsOffline )
    {
        LSA_LOG_DEBUG("LSA User Manager - domain is offline");
    }

    if (bShouldRefreshCreds)
    {
        dwError = AD_CreateProviderContext(
                      Handle->pProviderState->pszDomainName,
                      Handle->pProviderState,
                      &pProviderContext);
        if (dwError)
        {
            bShouldRefreshCreds = FALSE;
        }
    }

    if ( pUserList )
    {
        for ( pItem = pUserList->pFirst ; pItem ; pItem = pNextItem )
        {
            pNextItem = pItem->pNext;

            LSA_LOG_DEBUG("LSA User Manager - checking user %u", pItem->uUid);

            dwTime = time(NULL);

            if ( (dwTime - pItem->dwLastActivity) > LSA_UM_USER_MIN_PERIOD )
            {
                dwError = LsaUmpIsUserActive(
                              pItem->uUid,
                              &bUserIsActive);
                if ( bUserIsActive )
                {
                    pItem->dwLastActivity = dwTime;
                }
                else
                {
                    LsaUmpRemoveUserFromList(
                        pUserList,
                        pItem->uUid);
                    continue;
                }
            }

            if ( (bShouldRefreshCreds && !bDomainIsOffline) &&
                 (((pItem->dwTgtEndTime - pItem->dwLastActivity) < (Handle->dwCheckUsersSeconds * 3)) ||
                  pItem->dwTgtEndTime < dwTime ))
            {
                // ignore errors because we want to process all users
                LsaUmpRefreshUserCreds(
                    Handle,
                    pProviderContext,
                    pItem);
            }
        }
    }

    LsaUmpFreeRequestList(pRequestList);
    AD_DereferenceProviderContext(pProviderContext);

    return dwError;
}

static
DWORD
LsaUmpCreateKeys(
    PLSA_AD_PROVIDER_STATE pState,
    PLSA_UM_KSCHEDULES kSchedules
    )
{
    DWORD      dwError = 0;
    DWORD      dwCount = 0;
    DES_cblock Key;
    DWORD      dwTime = 0;
    pid_t      Pid = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    if ( !RAND_status() )
    {
        LSA_LOG_DEBUG("LSA User Manager - randomness not seeded automatically");

        dwError = LsaPcacheGetMachineAccountInfoA(
                      pState->pPcache,
                      &pAccountInfo);
        BAIL_ON_LSA_ERROR(dwError);

        RAND_seed(
            pAccountInfo->DnsDomainName,
            strlen(pAccountInfo->DnsDomainName));
        RAND_seed(
            pAccountInfo->NetbiosDomainName,
            strlen(pAccountInfo->NetbiosDomainName));
        RAND_seed(
            pAccountInfo->DomainSid,
            strlen(pAccountInfo->DomainSid));
        RAND_seed(
            pAccountInfo->SamAccountName,
            strlen(pAccountInfo->SamAccountName));
        RAND_seed(
            &pAccountInfo->AccountFlags,
            sizeof(pAccountInfo->AccountFlags));
        RAND_seed(
            &pAccountInfo->KeyVersionNumber,
            sizeof(pAccountInfo->KeyVersionNumber));
        RAND_seed(
            pAccountInfo->Fqdn,
            strlen(pAccountInfo->Fqdn));
        RAND_seed(
            &pAccountInfo->LastChangeTime,
            sizeof(pAccountInfo->LastChangeTime));

        dwTime = time(NULL);
        RAND_seed(
            &dwTime,
            sizeof(dwTime));

        Pid = getpid();
        RAND_seed(
            &Pid,
            sizeof(Pid));

        Pid = getppid();
        RAND_seed(
            &Pid,
            sizeof(Pid));

        LSA_LOG_DEBUG("LSA User Manager - randomness is %s seeded", RAND_status() ? "" : "not");
    }

    for ( dwCount = 0 ; dwCount < 3 ; dwCount++ )
    {

        dwError = DES_random_key(&Key);
        if ( dwError == 0 )
        {
            LSA_LOG_DEBUG("LSA User Manager - failed to create random key");
            dwError = LW_ERROR_CREATE_KEY_FAILED;
            goto error;
        }
        else
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        DES_set_key_unchecked(
            &Key,
            &kSchedules->kSchedule[dwCount]);
    }

cleanup:

    memset(&Key, 0, sizeof(Key));

    LsaPcacheReleaseMachineAccountInfoA(pAccountInfo);

    return dwError;

error:

    goto cleanup;
}

#if 0
static
DWORD
LsaUmpEncryptString(
    LSA_UM_STATE_HANDLE Handle,
    PCSTR               pszString,
    PVOID *             ppEncString,
    PDWORD              pdwStringLen
    )
{
    DWORD              dwError = 0;
    BOOLEAN            bInLock = FALSE;
    PLSA_UM_KSCHEDULES pKSchedules = NULL;
    DWORD              dwStringLen = 0;
    PSTR               pszDataString = NULL;
    DWORD              dwEncryptLen = 0;
    PVOID              pEncString = NULL;
    DES_cblock         iv;

    memset(&iv ,0 ,sizeof(iv));

    // add 1 for NULL and 8 for some random data,
    // then round up to nearest multiple of 9
    dwStringLen = strlen(pszString);
    dwEncryptLen = dwStringLen + 9;
    if ( dwEncryptLen % 8 > 0 )
    {
        dwEncryptLen = ((dwEncryptLen / 8) + 1) * 8;
    }

    dwError = LwAllocateMemory(
                  dwEncryptLen,
                  (PVOID *)&pszDataString);
    BAIL_ON_LSA_ERROR(dwError);

    RAND_bytes((unsigned char *)pszDataString, 8);
    memcpy(&pszDataString[8],
           pszString,
           dwStringLen);

    dwError = LwAllocateMemory(
                  dwEncryptLen,
                  &pEncString);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_UM_STATE_LOCK(bInLock);
    pKSchedules = Handle->kSchedules;
    LSA_UM_STATE_UNLOCK(bInLock);

    DES_ede3_cbc_encrypt(
        (const unsigned char *)pszDataString,
        pEncString,
        dwEncryptLen,
        &pKSchedules->kSchedule[0],
        &pKSchedules->kSchedule[1],
        &pKSchedules->kSchedule[2],
        &iv,
        DES_ENCRYPT);

    *ppEncString = pEncString;
    *pdwStringLen = dwEncryptLen;

cleanup:

    LW_SAFE_FREE_MEMORY(pszDataString);

    return dwError;

error:

    *ppEncString = NULL;
    *pdwStringLen = 0;

    if ( pEncString )
    {
        memset(pEncString, 0, dwEncryptLen);
        LW_SAFE_FREE_MEMORY(pEncString);
    }

    goto cleanup;
}

static
DWORD
LsaUmpDecryptString(
    LSA_UM_STATE_HANDLE Handle,
    PVOID               pEncString,
    DWORD               dwDecryptLen,
    PSTR *              ppszString
    )
{
    DWORD              dwError = 0;
    BOOLEAN            bInLock = FALSE;
    PLSA_UM_KSCHEDULES pKSchedules = NULL;
    PVOID              pDecString = NULL;
    PSTR               pszString = NULL;
    DES_cblock         iv;

    memset(&iv ,0 ,sizeof(iv));

    dwError = LwAllocateMemory(
                  dwDecryptLen,
                  &pDecString);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_UM_STATE_LOCK(bInLock);
    pKSchedules = Handle->kSchedules;
    LSA_UM_STATE_UNLOCK(bInLock);

    DES_ede3_cbc_encrypt(
        pEncString,
        pDecString,
        dwDecryptLen,
        &pKSchedules->kSchedule[0],
        &pKSchedules->kSchedule[1],
        &pKSchedules->kSchedule[2],
        &iv,
        DES_DECRYPT);

    // remove the random prefix
    dwError = LwAllocateString(
                  &((CHAR *)pDecString)[8],
                  &pszString);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszString = pszString;

cleanup:

    if ( pDecString )
    {
        memset(pDecString, 0, dwDecryptLen);
        LW_SAFE_FREE_MEMORY(pDecString);
    }

    return dwError;

error:

    *ppszString = NULL;

    LW_SECURE_FREE_STRING(pszString);

    goto cleanup;
}

static
VOID
LsaUmpFreePassword(
    PVOID pPassword,
    DWORD dwPasswordLen
    )
{
    LW_SECURE_FREE_MEMORY(pPassword, dwPasswordLen);
}
#endif

static
VOID
LsaUmpFreeUserList(
    PLSA_UM_USER_REFRESH_LIST pUserList
    )
{
    PLSA_UM_USER_REFRESH_ITEM pItem = NULL;
    PLSA_UM_USER_REFRESH_ITEM pNextItem = NULL;

    pNextItem = pUserList->pFirst;

    while ( pNextItem )
    {
        pItem = pNextItem;
        pNextItem = pItem->pNext;

        LsaUmpFreeUserItem(pItem);
    }

    LW_SAFE_FREE_MEMORY(pUserList);

    return;
}

static
VOID
LsaUmpFreeRequest(
    PLSA_UM_REQUEST_ITEM pRequest
    )
{
    if ( pRequest )
    {
        LsaReleaseCredential(&pRequest->CredHandle);
        LW_SAFE_FREE_MEMORY(pRequest);
    }

    return;
}

static
VOID
LsaUmpForEachRequestDestroy(
    IN PVOID pData,
    IN PVOID pContext
    )
{
    PLSA_UM_REQUEST_ITEM pRequest = (PLSA_UM_REQUEST_ITEM)pData;
    LsaUmpFreeRequest(pRequest);
}

static
VOID
LsaUmpFreeRequestList(
    PLW_DLINKED_LIST pRequestList
    )
{
    if ( pRequestList )
    {
        LwDLinkedListForEach(
            pRequestList,
            LsaUmpForEachRequestDestroy,
            NULL);

        LwDLinkedListFree(pRequestList);
    }

    return;
}

static
DWORD
LsaUmpAddRequest(
    LSA_UM_STATE_HANDLE  Handle,
    PLSA_UM_REQUEST_ITEM pRequest
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;

    LSA_UM_STATE_LOCK(bInLock);

    dwError = LwDLinkedListPrepend(
                  &Handle->RequestList,
                  pRequest);

    LSA_UM_STATE_UNLOCK(bInLock);

    return dwError;
}

DWORD
LsaUmpAddUser(
    LSA_UM_STATE_HANDLE Handle,
    uid_t               uUid,
    PCSTR               pszUserName,
    PCSTR               pszPassword,
    DWORD               dwTgtEndTime
    )
{
    DWORD                dwError = 0;
    PLSA_UM_REQUEST_ITEM pRequest = NULL;

    LSA_LOG_DEBUG("LSA User Manager - requesting user addition %u", uUid);

    dwError = LwAllocateMemory(
                  sizeof(*pRequest),
                  (PVOID*)&pRequest);
    BAIL_ON_LSA_ERROR(dwError);

    pRequest->dwType = LSA_UM_REQUEST_TYPE_ADD;
    pRequest->uUid = uUid;

    pRequest->uUid = uUid;

#if 0
    dwError = LsaUmpEncryptString(
                  Handle,
                  pszPassword,
                  &pRequest->pPassword,
                  &pRequest->dwPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);
#endif

    pRequest->dwTgtEndTime = dwTgtEndTime;
    pRequest->dwLastActivity = time(NULL);

    dwError = LsaAddCredential(
        pszUserName,
        pszPassword,
        &uUid,
        &pRequest->CredHandle);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmpAddRequest(
                  Handle,
                  pRequest);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    if ( pRequest )
    {
        LsaUmpFreeRequest(pRequest);
    }

    goto cleanup;
}

DWORD
LsaUmpModifyUser(
    LSA_UM_STATE_HANDLE Handle,
    uid_t               uUid,
    PCSTR               pszPassword
    )
{
    DWORD                dwError = 0;
    PLSA_UM_REQUEST_ITEM pRequest = NULL;
    PCSTR pUserName = NULL;
    LSA_CRED_HANDLE OldCredHandle = NULL;
    LSA_CRED_HANDLE NewCredHandle = NULL;

    LSA_LOG_DEBUG("LSA User Manager - requesting user modify %u", uUid);

    // We're going to update our credential now instead of waiting on the
    // thread.  We have the new uid and/or password... get the username by
    // finding the current (now old) credential.
    OldCredHandle = LsaGetCredential(uUid);

    if(OldCredHandle)
    {
        // This is to get the username to pass into the new cred
        LsaGetCredentialInfo(OldCredHandle, &pUserName, NULL, NULL);

        // This creates a new updated credential
        dwError = LsaAddCredential(
            pUserName,
            pszPassword,
            &uUid,
            &NewCredHandle);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateMemory(
                      sizeof(*pRequest),
                      (PVOID*)&pRequest);
        BAIL_ON_LSA_ERROR(dwError);

        pRequest->CredHandle = NewCredHandle;
        NewCredHandle = NULL;

        pRequest->dwType = LSA_UM_REQUEST_TYPE_MODIFY;
        pRequest->uUid = uUid;

        pRequest->dwLastActivity = time(NULL);

        dwError = LsaUmpAddRequest(
            Handle,
            pRequest);
        BAIL_ON_LSA_ERROR(dwError);

        pRequest = NULL;
    }
    else
    {
        LSA_LOG_DEBUG("LSA User Manager - cred not found while modifying user (id: %u)", uUid);
    }

cleanup:
    // This release is to cleanup the reference from the LsaGetCredential above.
    LsaReleaseCredential(&OldCredHandle);

    LsaReleaseCredential(&NewCredHandle);

    if ( pRequest )
    {
        LsaUmpFreeRequest(pRequest);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaUmpRemoveUser(
    LSA_UM_STATE_HANDLE Handle,
    uid_t               uUid
    )
{
    DWORD                dwError = 0;
    PLSA_UM_REQUEST_ITEM pRequest = NULL;

    LSA_LOG_DEBUG("LSA User Manager - requesting user removal %u", uUid);

    dwError = LwAllocateMemory(
                  sizeof(*pRequest),
                  (PVOID*)&pRequest);
    BAIL_ON_LSA_ERROR(dwError);

    pRequest->dwType = LSA_UM_REQUEST_TYPE_REMOVE;
    pRequest->uUid = uUid;

    dwError = LsaUmpAddRequest(
                  Handle,
                  pRequest);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    if ( pRequest )
    {
        LsaUmpFreeRequest(pRequest);
    }

    goto cleanup;
}

static
VOID
LsaUmpFindUserPtr(
    PLSA_UM_USER_REFRESH_LIST    pUserList,
    uid_t                        uUid,
    PLSA_UM_USER_REFRESH_ITEM ** pUserItemPtr
    )
{
    PLSA_UM_USER_REFRESH_ITEM pItem = NULL;
    PLSA_UM_USER_REFRESH_ITEM pNextItem = NULL;

    pItem = pUserList->pFirst;

    if ( !pItem || pItem->uUid >= uUid )
    {
        *pUserItemPtr = &pUserList->pFirst;
    }
    else
    {
        while ( pItem )
        {
            pNextItem = pItem->pNext;

            if ( !pNextItem || pNextItem->uUid >= uUid )
                break;

            pItem = pNextItem;
        }

        *pUserItemPtr = &pItem->pNext;
    }

    return;
}

static
DWORD
LsaUmpAddUserInternal(
    LSA_UM_STATE_HANDLE  Handle,
    PLSA_UM_REQUEST_ITEM pRequest
    )
{
    DWORD                       dwError = 0;
    BOOLEAN                     bInLock = FALSE;
    PLSA_UM_USER_REFRESH_LIST   pUserList = NULL;
    PLSA_UM_USER_REFRESH_ITEM   pNewUserItem = NULL;
    PLSA_UM_USER_REFRESH_ITEM * pUserItemPtr = NULL;
    PLSA_UM_USER_REFRESH_ITEM   pUserItem = NULL;

    LSA_LOG_DEBUG("LSA User Manager - adding user %u", pRequest->uUid);

    LSA_UM_STATE_LOCK(bInLock);
    pUserList = Handle->UserList;
    LSA_UM_STATE_UNLOCK(bInLock);

    LsaUmpFindUserPtr(
        pUserList,
        pRequest->uUid,
        &pUserItemPtr);

    pUserItem = *pUserItemPtr;
    if ( pUserItem && pUserItem->uUid == pRequest->uUid )
    {
        pUserItem->dwTgtEndTime = pRequest->dwTgtEndTime;
        pUserItem->dwLastActivity = time(NULL);
        pUserItem->dwFailedCount = 0;

        LsaReleaseCredential(&pUserItem->CredHandle);

        pUserItem->CredHandle= pRequest->CredHandle;
        pRequest->CredHandle = NULL;
    }
    else
    {
        dwError = LwAllocateMemory(
                      sizeof(LSA_UM_USER_REFRESH_ITEM),
                      (PVOID*)&pNewUserItem);
        BAIL_ON_LSA_ERROR(dwError);

        pNewUserItem->uUid = pRequest->uUid;

        pNewUserItem->dwTgtEndTime = pRequest->dwTgtEndTime;
        pNewUserItem->dwLastActivity = pRequest->dwLastActivity;

        pNewUserItem->CredHandle = pRequest->CredHandle;
        pRequest->CredHandle = NULL;

        pNewUserItem->pNext = *pUserItemPtr;

        *pUserItemPtr = pNewUserItem;
        pNewUserItem = NULL;
    }

cleanup:

    return dwError;

error:
    if (pNewUserItem)
    {
        LsaUmpFreeUserItem(pNewUserItem);
    }

    goto cleanup;
}

static
DWORD
LsaUmpModifyUserInternal(
    LSA_UM_STATE_HANDLE  Handle,
    PLSA_UM_REQUEST_ITEM pRequest
    )
{
    DWORD                       dwError = 0;
    BOOLEAN                     bInLock = FALSE;
    PLSA_UM_USER_REFRESH_LIST   pUserList = NULL;
    PLSA_UM_USER_REFRESH_ITEM * pUserItemPtr = NULL;
    PLSA_UM_USER_REFRESH_ITEM   pUserItem = NULL;

    LSA_LOG_DEBUG("LSA User Manager - modifying user %u", pRequest->uUid);

    LSA_UM_STATE_LOCK(bInLock);
    pUserList = Handle->UserList;
    LSA_UM_STATE_UNLOCK(bInLock);

    LsaUmpFindUserPtr(
        pUserList,
        pRequest->uUid,
        &pUserItemPtr);

    pUserItem = *pUserItemPtr;
    if ( pUserItem && pUserItem->uUid == pRequest->uUid )
    {
        if ( pRequest->dwLastActivity > pUserItem->dwLastActivity )
        {
            pUserItem->dwLastActivity = pRequest->dwLastActivity;
        }

        LsaReleaseCredential(&pUserItem->CredHandle);
        pUserItem->CredHandle = pRequest->CredHandle;
        pRequest->CredHandle = NULL;

        pUserItem->dwFailedCount = 0;
    }
    else
    {
        LSA_LOG_DEBUG("LSA User Manager - user not found while modifying user (id: %u)", pRequest->uUid);
    }

    return dwError;
}

static
DWORD
LsaUmpRemoveUserInternal(
    LSA_UM_STATE_HANDLE  Handle,
    PLSA_UM_REQUEST_ITEM pRequest
    )
{
    DWORD                       dwError = 0;
    BOOLEAN                     bInLock = FALSE;
    PLSA_UM_USER_REFRESH_LIST   pUserList = NULL;
    PLSA_UM_USER_REFRESH_ITEM * pUserItemPtr = NULL;
    PLSA_UM_USER_REFRESH_ITEM   pUserItem = NULL;

    LSA_LOG_DEBUG("LSA User Manager - removing user %u", pRequest->uUid);

    LSA_UM_STATE_LOCK(bInLock);
    pUserList = Handle->UserList;
    LSA_UM_STATE_UNLOCK(bInLock);

    LsaUmpFindUserPtr(
        pUserList,
        pRequest->uUid,
        &pUserItemPtr);

    pUserItem = *pUserItemPtr;
    if ( pUserItem && pUserItem->uUid == pRequest->uUid )
    {
        pUserItem->dwLastActivity = 0;
    }

    return dwError;
}

static
DWORD
LsaUmpRemoveUserFromList(
    PLSA_UM_USER_REFRESH_LIST pUserList,
    uid_t                     uUid
    )
{
    DWORD                       dwError = 0;
    PLSA_UM_USER_REFRESH_ITEM * pUserItemPtr = NULL;
    PLSA_UM_USER_REFRESH_ITEM   pUserItem = NULL;

    LSA_LOG_DEBUG("LSA User Manager - removing user from list %u", uUid);

    LsaUmpFindUserPtr(
        pUserList,
        uUid,
        &pUserItemPtr);

    pUserItem = *pUserItemPtr;
    if ( pUserItem && pUserItem->uUid == uUid )
    {
        *pUserItemPtr = pUserItem->pNext;

        LsaUmpFreeUserItem(pUserItem);
    }

    return dwError;
}

static
DWORD
LsaUmpRefreshUserCreds(
    LSA_UM_STATE_HANDLE       Handle,
    PAD_PROVIDER_CONTEXT      pProviderContext,
    PLSA_UM_USER_REFRESH_ITEM pUserItem
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    PCSTR pszPassword = NULL;

    if ( pUserItem->dwFailedCount > 5 )
    {
        // The user's TGT cannot be refreshed if the
        // password is changed from another host.  Allow
        // a few failures in case of temporary problems
        // but stop if the condition appears to be
        // permanent.
        LSA_LOG_DEBUG("LSA User Manager - failed refresh count too high for %u", pUserItem->uUid);
        goto cleanup;
    }
    else
    {
        LSA_LOG_DEBUG("LSA User Manager - refreshing user credentials %u", pUserItem->uUid);
    }

    dwError = AD_FindUserObjectById(
                  pProviderContext,
                  pUserItem->uUid,
                  &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    LsaGetCredentialInfo(
        pUserItem->CredHandle,
        NULL,
        &pszPassword,
        NULL);

    dwError = AD_OnlineCheckUserPassword(
                    pProviderContext,
                    pUserInfo,
                    pszPassword,
                    &pUserItem->dwTgtEndTime,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    // At this point the user's TGT has been refreshed.
    // For purposes of determining whether to continue
    // to refresh the user's TGT, we don't care whether
    // post-processing fails.
    pUserItem->dwFailedCount = 0;

    if (AD_EventlogEnabled(Handle->pProviderState))
    {
        LsaUmpLogUserTGTRefreshSuccessEvent(pUserInfo->userInfo.pszUPN,
                                            pUserItem->uUid,
                                            pUserInfo->pszNetbiosDomainName,
                                            pUserItem->dwTgtEndTime);
    }

cleanup:

    ADCacheSafeFreeObject(&pUserInfo);

    return dwError;

error:

    if ( dwError != LW_ERROR_DOMAIN_IS_OFFLINE &&
         dwError != LW_ERROR_LDAP_SERVER_UNAVAILABLE )
    {
        pUserItem->dwFailedCount++;

        if (AD_EventlogEnabled(Handle->pProviderState))
        {
            if (pUserInfo)
            {
                LsaUmpLogUserTGTRefreshFailureEvent(
                        pUserInfo->userInfo.pszUPN,
                        pUserItem->uUid,
                        pUserInfo->pszNetbiosDomainName,
                        pUserItem->dwFailedCount,
                        dwError);
            }
            else
            {
                LsaUmpLogUserTGTRefreshFailureEvent(
                        "<null>",
                        pUserItem->uUid,
                        "<null>",
                        pUserItem->dwFailedCount,
                        dwError);
            }
        }
    }

    goto cleanup;
}

VOID
LsaUmpFreeUserItem(
    PLSA_UM_USER_REFRESH_ITEM pUserItem
    )
{
    LsaReleaseCredential(&pUserItem->CredHandle);
    LwFreeMemory(pUserItem);
}

VOID
LsaUmpLogUserTGTRefreshSuccessEvent(
    PSTR pszUsername,
    uid_t uid,
    PSTR  pszDomainName,
    DWORD dwTgtEndTime
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "Refreshed Active Directory user account Kerberos credentials.\r\n\r\n" \
                 "     Authentication provider:   %s\r\n\r\n" \
                 "     User name:                 %s\r\n" \
                 "     UID:                       %u\r\n" \
                 "     Domain name:               %s\r\n" \
                 "     TGT end time:              %u\r\n",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 LSA_SAFE_LOG_STRING(pszUsername),
                 uid,
                 LSA_SAFE_LOG_STRING(pszDomainName),
                 dwTgtEndTime);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_SUCCESSFUL_USER_ACCOUNT_KERB_REFRESH,
            KERBEROS_EVENT_CATEGORY,
            pszDescription,
            NULL);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LsaUmpLogUserTGTRefreshFailureEvent(
    PSTR  pszUsername,
    uid_t uid,
    PSTR  pszDomainName,
    DWORD dwFailureNumber,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "The Active Directory user account Kerberos credentials failed to refresh.\r\n\r\n" \
                 "     Authentication provider:   %s\r\n\r\n" \
                 "     User name:                 %s\r\n" \
                 "     UID:                       %u\r\n" \
                 "     Domain name:               %s\r\n" \
                 "     Failure number:            %u\r\n",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 LSA_SAFE_LOG_STRING(pszUsername),
                 uid,
                 LSA_SAFE_LOG_STRING(pszDomainName),
                 dwFailureNumber);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceFailureEvent(
            LSASS_EVENT_FAILED_USER_ACCOUNT_KERB_REFRESH,
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
