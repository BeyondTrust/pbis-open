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
 *        pwdcache.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Password Info Cache API
 *
 * Authors: Arlene Berry (aberry@likewise.com)
 *          Danilo Almeida (dalmeida@likewise.com)
 */

#include "adprovider.h"
#include <lsa/lsapstore-api.h>
#include "machinepwdinfo-impl.h"

typedef struct _LSA_MACHINEPWD_CACHE_ENTRY {
    LONG RefCount;
    LSA_MACHINE_PASSWORD_INFO_A PasswordInfoA;
    LSA_MACHINE_PASSWORD_INFO_W PasswordInfoW;
} LSA_MACHINEPWD_CACHE_ENTRY, *PLSA_MACHINEPWD_CACHE_ENTRY;

//
// Password Cache Type
//

typedef struct _LSA_MACHINEPWD_CACHE {
    PSTR pszDomainName;
    PWSTR pwszDomainName;
    pthread_rwlock_t StateLock;
    pthread_rwlock_t* pStateLock;
    BOOLEAN bIsLoaded;
    PLSA_MACHINEPWD_CACHE_ENTRY pEntry;
} LSA_MACHINEPWD_CACHE, *PLSA_MACHINEPWD_CACHE;

static
DWORD
LsaPcachepEnsurePasswordInfoAndLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE hPcache
    );

static
DWORD
LsaPcachepLoadPasswordInfoInLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE hPcache
    );

static
VOID
LsaPcachepClearPasswordInfoInLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    );

static
VOID
LsaPcachepReleaseEntry(
    IN PLSA_MACHINEPWD_CACHE_ENTRY pEntry
    );

DWORD
LsaPcacheCreate(
    IN PCSTR pszDomainName,
    OUT PLSA_MACHINEPWD_CACHE_HANDLE ppPcache
    )
{
    DWORD dwError = 0;
    PLSA_MACHINEPWD_CACHE pPcache = NULL;

    dwError = LwAllocateMemory(
                  sizeof(*pPcache),
                  (PVOID*)&pPcache);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszDomainName, &pPcache->pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pszDomainName, &pPcache->pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_rwlock_init(&pPcache->StateLock, NULL));
    BAIL_ON_LSA_ERROR(dwError);
    pPcache->pStateLock = &pPcache->StateLock;

    pPcache->bIsLoaded = FALSE;
    pPcache->pEntry = NULL;

error:
    if (dwError)
    {
        if (pPcache)
        {
            LsaPcacheDestroy(pPcache);
            pPcache = NULL;
        }
    }

    *ppPcache = pPcache;

    return dwError;
}

VOID
LsaPcacheDestroy(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    )
{
    if (pPcache)
    {
        if (pPcache->pStateLock)
        {
            pthread_rwlock_destroy(pPcache->pStateLock);
        }

        LsaPcachepReleaseEntry(pPcache->pEntry);
        LW_SAFE_FREE_STRING(pPcache->pszDomainName);
        LW_SAFE_FREE_MEMORY(pPcache->pwszDomainName);
        LW_SAFE_FREE_MEMORY(pPcache);
    }

    return;
}

VOID
LsaPcacheClearPasswordInfo(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    )
{
    if (pPcache)
    {
        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_wrlock(pPcache->pStateLock));
        LsaPcachepClearPasswordInfoInLock(pPcache);
        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_unlock(pPcache->pStateLock));
    }
}

static
DWORD
LsaPcachepEnsurePasswordInfoAndLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    )
///<
/// Ensure that there is password information and lock the pcache.
///
/// This function ensures that the password information is locked
/// and acquires a lock on the pcache.  Note that the lock will
/// be at least a read lock but may be a write lock.  However,
/// callers cannot rely on it being any more than a read lock.
///
/// Note that making this always return a read lock is not optimal
/// as it must acquire a write lock in the case where the password
/// information is not already loaded.  To make it return a read
/// lock in that case would require dropping the write lock and
/// re-checking that the password information is still loaded.
/// This could result in looping and unnecessarily complex code.
///
/// @return Windows error code
///
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (!pPcache)
    {
        dwError = NERR_SetupNotJoined;
        BAIL_ON_LSA_ERROR(dwError);
    }

    PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_rdlock(pPcache->pStateLock));
    bInLock = TRUE;

    if (!pPcache->bIsLoaded)
    {
        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_unlock(pPcache->pStateLock));
        bInLock = FALSE;

        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_wrlock(pPcache->pStateLock));
        bInLock = TRUE;

        if (!pPcache->bIsLoaded)
        {
            dwError = LsaPcachepLoadPasswordInfoInLock(pPcache);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

error:
    if (dwError)
    {
        if (bInLock)
        {
            PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_unlock(pPcache->pStateLock));
            bInLock = FALSE;
        }
    }

    LW_ASSERT(LW_IS_BOTH_OR_NEITHER(0 == dwError, bInLock));

    return dwError;
}

static
DWORD
LsaPcachepLoadPasswordInfoInLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfoA = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfoW = NULL;

    LSA_ASSERT(!pPcache->bIsLoaded);
    LSA_ASSERT(!pPcache->pEntry);

    //
    // Read information from LSA Pstore
    //

    dwError = LsaPstoreGetPasswordInfoA(
                    pPcache->pszDomainName,
                    &pPasswordInfoA);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPstoreGetPasswordInfoW(
                    pPcache->pwszDomainName,
                    &pPasswordInfoW);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_ASSERT(pPasswordInfoA->Account.KeyVersionNumber == pPasswordInfoW->Account.KeyVersionNumber);
    LSA_ASSERT(pPasswordInfoA->Account.LastChangeTime == pPasswordInfoW->Account.LastChangeTime);

    //
    // Stash information from LSA Pstore
    //

    dwError = LwAllocateMemory(
                    sizeof(*pPcache->pEntry),
                    OUT_PPVOID(&pPcache->pEntry));
    BAIL_ON_LSA_ERROR(dwError);
    pPcache->pEntry->RefCount = 1;

    dwError = LsaImplFillMachinePasswordInfoA(
                    pPasswordInfoA,
                    &pPcache->pEntry->PasswordInfoA);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaImplFillMachinePasswordInfoW(
                    pPasswordInfoW,
                    &pPcache->pEntry->PasswordInfoW);
    BAIL_ON_LSA_ERROR(dwError);

    pPcache->bIsLoaded = TRUE;

error:
    if (dwError)
    {
        LsaPcachepClearPasswordInfoInLock(pPcache);
    }

    if (pPasswordInfoW)
    {
        LsaPstoreFreePasswordInfoW(pPasswordInfoW);
    }
    if (pPasswordInfoA)
    {
        LsaPstoreFreePasswordInfoA(pPasswordInfoA);
    }

    return dwError;
}

static
VOID
LsaPcachepClearPasswordInfoInLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache
    )
{
    pPcache->bIsLoaded = FALSE;
    LsaPcachepReleaseEntry(pPcache->pEntry);
    pPcache->pEntry = NULL;
}

static
VOID
LsaPcachepReleaseEntry(
    IN PLSA_MACHINEPWD_CACHE_ENTRY pEntry
    )
{
    if (pEntry)
    {
        LONG count = LwInterlockedDecrement(&pEntry->RefCount);
        LW_ASSERT(count >= 0);
        if (0 == count)
        {
            LsaImplFreeMachinePasswordInfoContentsA(&pEntry->PasswordInfoA);
            LsaImplFreeMachinePasswordInfoContentsW(&pEntry->PasswordInfoW);
            LwFreeMemory(pEntry);
        }
    }
}

// Get Functions

DWORD
LsaPcacheGetMachineAccountInfoA(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;

    dwError = LsaPcachepEnsurePasswordInfoAndLock(pPcache);
    BAIL_ON_LSA_ERROR(dwError);
    bInLock = TRUE;

    pAccountInfo = &pPcache->pEntry->PasswordInfoA.Account;
    LwInterlockedIncrement(&pPcache->pEntry->RefCount);

error:
    if (bInLock)
    {
        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_unlock(pPcache->pStateLock));
    }

    if (dwError)
    {
        if (pAccountInfo)
        {
            LsaPcacheReleaseMachineAccountInfoA(pAccountInfo);
            pAccountInfo = NULL;
        }
    }

    *ppAccountInfo = pAccountInfo;

    return dwError;
}

DWORD
LsaPcacheGetMachineAccountInfoW(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = NULL;

    dwError = LsaPcachepEnsurePasswordInfoAndLock(pPcache);
    BAIL_ON_LSA_ERROR(dwError);
    bInLock = TRUE;

    pAccountInfo = &pPcache->pEntry->PasswordInfoW.Account;
    LwInterlockedIncrement(&pPcache->pEntry->RefCount);

error:
    if (bInLock)
    {
        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_unlock(pPcache->pStateLock));
    }

    if (dwError)
    {
        if (pAccountInfo)
        {
            LsaPcacheReleaseMachineAccountInfoW(pAccountInfo);
            pAccountInfo = NULL;
        }
    }

    *ppAccountInfo = pAccountInfo;

    return dwError;
}

DWORD
LsaPcacheGetMachinePasswordInfoA(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = LsaPcachepEnsurePasswordInfoAndLock(pPcache);
    BAIL_ON_LSA_ERROR(dwError);
    bInLock = TRUE;

    pPasswordInfo = &pPcache->pEntry->PasswordInfoA;
    LwInterlockedIncrement(&pPcache->pEntry->RefCount);

error:
    if (bInLock)
    {
        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_unlock(pPcache->pStateLock));
    }

    if (dwError)
    {
        if (pPasswordInfo)
        {
            LsaPcacheReleaseMachinePasswordInfoA(pPasswordInfo);
            pPasswordInfo = NULL;
        }
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;
}

DWORD
LsaPcacheGetMachinePasswordInfoW(
    IN LSA_MACHINEPWD_CACHE_HANDLE pPcache,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;

    dwError = LsaPcachepEnsurePasswordInfoAndLock(pPcache);
    BAIL_ON_LSA_ERROR(dwError);
    bInLock = TRUE;

    pPasswordInfo = &pPcache->pEntry->PasswordInfoW;
    LwInterlockedIncrement(&pPcache->pEntry->RefCount);

error:
    if (bInLock)
    {
        PTHREAD_CALL_MUST_SUCCEED(pthread_rwlock_unlock(pPcache->pStateLock));
    }

    if (dwError)
    {
        if (pPasswordInfo)
        {
            LsaPcacheReleaseMachinePasswordInfoW(pPasswordInfo);
            pPasswordInfo = NULL;
        }
    }

    *ppPasswordInfo = pPasswordInfo;

    return dwError;
}

// Release Functions

VOID
LsaPcacheReleaseMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    if (pAccountInfo)
    {
        PLSA_MACHINEPWD_CACHE_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pAccountInfo, LSA_MACHINEPWD_CACHE_ENTRY, PasswordInfoA.Account);
        LsaPcachepReleaseEntry(pEntry);
    }
}

VOID
LsaPcacheReleaseMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    )
{
    if (pAccountInfo)
    {
        PLSA_MACHINEPWD_CACHE_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pAccountInfo, LSA_MACHINEPWD_CACHE_ENTRY, PasswordInfoW.Account);
        LsaPcachepReleaseEntry(pEntry);
    }
}

VOID
LsaPcacheReleaseMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        PLSA_MACHINEPWD_CACHE_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pPasswordInfo, LSA_MACHINEPWD_CACHE_ENTRY, PasswordInfoA);
        LsaPcachepReleaseEntry(pEntry);
    }
}

VOID
LsaPcacheReleaseMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    if (pPasswordInfo)
    {
        PLSA_MACHINEPWD_CACHE_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pPasswordInfo, LSA_MACHINEPWD_CACHE_ENTRY, PasswordInfoW);
        LsaPcachepReleaseEntry(pEntry);
    }
}

