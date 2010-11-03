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
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */

#include "adprovider.h"

//
// Password Cache Type
//

typedef struct _LSA_MACHINEPWD_CACHE{
    BOOLEAN bIsLoaded;
    PSTR pszDomainName;
    pthread_rwlock_t stateLock;
    pthread_rwlock_t* pStateLock;
    PLWPS_PASSWORD_INFO pPasswordInfo;
    PLWPS_PASSWORD_INFO_A pPasswordInfoA;
} LSA_MACHINEPWD_CACHE, *PLSA_MACHINEPWD_CACHE;

static
DWORD
LsaPcacheSetPasswordInfoInLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE hPcache
    );

DWORD
LsaPcacheCreate(
    IN PCSTR pszDomainName,
    OUT PLSA_MACHINEPWD_CACHE_HANDLE phPcache
    )
{
    DWORD dwError = 0;
    PLSA_MACHINEPWD_CACHE pPcache = NULL;

    dwError = LwAllocateMemory(
                  sizeof(*pPcache),
                  (PVOID*)&pPcache);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszDomainName)
    {
        dwError = LwAllocateString(
                      pszDomainName,
                      &pPcache->pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwMapErrnoToLwError(pthread_rwlock_init(&pPcache->stateLock, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pPcache->pStateLock = &pPcache->stateLock;

    pPcache->bIsLoaded = FALSE;
    pPcache->pPasswordInfo = NULL;
    pPcache->pPasswordInfoA = NULL;

    *phPcache = pPcache;
    pPcache = NULL;

error:

    if (dwError)
    {
        *phPcache = NULL;
    }

    LsaPcacheDestroy(pPcache);

    return dwError;
}

VOID
LsaPcacheDestroy(
    IN LSA_MACHINEPWD_CACHE_HANDLE hPcache
    )
{
    PLSA_MACHINEPWD_CACHE pPcache = (PLSA_MACHINEPWD_CACHE)hPcache;

    if (pPcache)
    {
        if (pPcache->pStateLock)
        {
            pthread_rwlock_destroy(pPcache->pStateLock);
        }

        LwFreePasswordInfo(pPcache->pPasswordInfo);
        LwFreePasswordInfoA(pPcache->pPasswordInfoA);
        LW_SAFE_FREE_STRING(pPcache->pszDomainName);
        LW_SAFE_FREE_MEMORY(pPcache);
    }

    return;
}

DWORD
LsaPcacheGetPasswordInfo(
    IN LSA_MACHINEPWD_CACHE_HANDLE hPcache,
    OUT OPTIONAL PLWPS_PASSWORD_INFO* ppPasswordInfo,
    OUT OPTIONAL PLWPS_PASSWORD_INFO_A* ppPasswordInfoA
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_MACHINEPWD_CACHE pPcache = (PLSA_MACHINEPWD_CACHE)hPcache;
    PLWPS_PASSWORD_INFO pPasswordInfo = NULL;
    PLWPS_PASSWORD_INFO_A pPasswordInfoA = NULL;

    if (!pPcache)
    {
        dwError = LW_ERROR_INVALID_ACCOUNT; 
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = pthread_rwlock_rdlock(pPcache->pStateLock);
    LW_ASSERT(dwError == 0);
    bInLock = TRUE;

    if (!pPcache->bIsLoaded)
    {
        dwError = pthread_rwlock_unlock(pPcache->pStateLock);
        LW_ASSERT(dwError == 0);
        bInLock = FALSE;

        dwError = pthread_rwlock_wrlock(pPcache->pStateLock);
        LW_ASSERT(dwError == 0);
        bInLock = TRUE;

        if (!pPcache->bIsLoaded)
        {
            dwError = LsaPcacheSetPasswordInfoInLock(pPcache);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (ppPasswordInfo && pPcache->pPasswordInfo)
    {
        dwError = LwDuplicatePasswordInfo(
                      pPcache->pPasswordInfo,
                      &pPasswordInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppPasswordInfoA && pPcache->pPasswordInfoA)
    {
        dwError = LwDuplicatePasswordInfoA(
                      pPcache->pPasswordInfoA,
                      &pPasswordInfoA);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    if (bInLock)
    {
        int status = 0;
        status = pthread_rwlock_unlock(pPcache->pStateLock);
        LW_ASSERT(status == 0);
    }

    if (dwError)
    {
        LwFreePasswordInfo(pPasswordInfo);
        pPasswordInfo = NULL;
        LwFreePasswordInfoA(pPasswordInfoA);
        pPasswordInfoA = NULL;
    }

    if (ppPasswordInfo)
    {
        *ppPasswordInfo = pPasswordInfo;
    }

    if (ppPasswordInfoA)
    {
        *ppPasswordInfoA = pPasswordInfoA;
    }

    return dwError;
}

VOID
LsaPcacheClearPasswordInfo(
    IN LSA_MACHINEPWD_CACHE_HANDLE hPcache
    )
{
    DWORD dwError = 0;
    PLSA_MACHINEPWD_CACHE pPcache = (PLSA_MACHINEPWD_CACHE)hPcache;

    if (pPcache)
    {
        dwError = pthread_rwlock_wrlock(pPcache->pStateLock);
        LW_ASSERT(dwError == 0);

        pPcache->bIsLoaded = FALSE;
        LwFreePasswordInfo(pPcache->pPasswordInfo);
        pPcache->pPasswordInfo = NULL;
        LwFreePasswordInfoA(pPcache->pPasswordInfoA);
        pPcache->pPasswordInfoA = NULL;

        dwError = pthread_rwlock_unlock(pPcache->pStateLock);
        LW_ASSERT(dwError == 0);
    }

    return;
}

static
DWORD
LsaPcacheSetPasswordInfoInLock(
    IN LSA_MACHINEPWD_CACHE_HANDLE hPcache
    )
{
    DWORD dwError = 0;
    PLSA_MACHINEPWD_CACHE pPcache = (PLSA_MACHINEPWD_CACHE)hPcache;
    HANDLE hPasswordStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPasswordInfo = NULL;

    dwError = LwpsOpenPasswordStore(
                  LWPS_PASSWORD_STORE_DEFAULT,
                  &hPasswordStore);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsGetPasswordByDomainName(
                  hPasswordStore,
                  pPcache->pszDomainName,
                  &pPasswordInfo);
    if (dwError == LWPS_ERROR_INVALID_ACCOUNT)
    {
        dwError = LW_ERROR_INVALID_ACCOUNT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pPcache->bIsLoaded)
    {
        goto error;
    }

    dwError = LwDuplicatePasswordInfo(
                  pPasswordInfo,
                  &pPcache->pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwDuplicatePasswordInfoWToA(
                  pPasswordInfo,
                  &pPcache->pPasswordInfoA);
    BAIL_ON_LSA_ERROR(dwError);

    pPcache->bIsLoaded = TRUE;

error:

    if (pPasswordInfo)
    {
        LwpsFreePasswordInfo(hPasswordStore, pPasswordInfo);
    }
    if (hPasswordStore != (HANDLE)NULL)
    {
        LwpsClosePasswordStore(hPasswordStore);
    }

    return dwError;
}
