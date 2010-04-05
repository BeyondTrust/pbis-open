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
 *        lwnet.h
 *
 * Abstract:
 *
 *        Likewise Netlogon
 * 
 *        Active Directory Site API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#include "includes.h"

static
DWORD
LWNetFindServersInDomain(
    IN PDNS_SERVER_INFO pServerArray,
    IN DWORD dwServerCount,
    IN PCSTR pszDomainName,
    OUT PDNS_SERVER_INFO* ppServersInDomain,
    OUT PDWORD pdwServersInDomainCount
    );

static
BOOL
LWNetServerIsInDomain(
    IN PDNS_SERVER_INFO pServerInfo,
    IN PCSTR pszDomainName
    );

static
DWORD
LWNetSrvPingCLdap(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszAddress,
    OUT PLWNET_DC_INFO* ppDcInfo
    )
{
    DWORD dwError = 0;
    // ISSUE-2008/07/01-dalmeida -- A HANDLE cannot be an integer -- must be ptr type!!!
    HANDLE hDirectory = 0;
    PSTR pszQuery = NULL;
    PSTR szAttributeList[] = { NETLOGON_LDAP_ATTRIBUTE_NAME, NULL };
    LDAPMessage* pMessage = NULL;
    PBYTE pNetlogonAttributeValue = NULL;
    DWORD dwNetlogonAttributeSize = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    LWNET_UNIX_MS_TIME_T startTime = 0;
    LWNET_UNIX_MS_TIME_T stopTime = 0;

    dwError = LwAllocateStringPrintf(&pszQuery,
                                        "(&(DnsDomain=%s)(NtVer=\\06\\00\\00\\80))",
                                        pszDnsDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwCLdapOpenDirectory(pszAddress, &hDirectory);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwLdapBindDirectoryAnonymous(hDirectory);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetSystemTimeInMs(&startTime);
    BAIL_ON_LWNET_ERROR(dwError);

    /* TODO: May need to do retries with shorter timeout if UDP does not retry */
    dwError = LwLdapDirectorySearchEx(
                    hDirectory,
                    "",
                    LDAP_SCOPE_BASE,
                    pszQuery,
                    szAttributeList,
                    NULL,
                    0,
                    &pMessage);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetSystemTimeInMs(&stopTime);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwLdapGetBytes(
                    hDirectory,
                    pMessage,
                    NETLOGON_LDAP_ATTRIBUTE_NAME,
                    &pNetlogonAttributeValue,
                    &dwNetlogonAttributeSize);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetBuildDCInfo(pNetlogonAttributeValue,
                               dwNetlogonAttributeSize,
                               &pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(pszAddress, &pDcInfo->pszDomainControllerAddress);
    BAIL_ON_LWNET_ERROR(dwError);

    pDcInfo->dwPingTime = (DWORD)(stopTime - startTime);
    if (stopTime < startTime)
    {
        LWNET_LOG_ERROR("Stop time is earlier than start time");
    }

error:
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    if (hDirectory)
    {
        LwLdapCloseDirectory(hDirectory);
    }

    LWNET_SAFE_FREE_MEMORY(pNetlogonAttributeValue);
    LWNET_SAFE_FREE_STRING(pszQuery);

    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    }

    *ppDcInfo = pDcInfo;

    return dwError;
}

static
VOID
LWNetSrvPingCLdapDerefenceThreadContext(
    IN OUT PLWNET_CLDAP_THREAD_CONTEXT pContext
    )
{
    BOOLEAN bNeedCleanup = FALSE;
    // Acquire; decrement; if 0, will cleanup; release; cleanup if needed.
    LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext);
    pContext->dwRefCount--;
    if (pContext->dwRefCount <= 0)
    {
        bNeedCleanup = TRUE;
    }
    LWNET_CLDAP_THREAD_CONTEXT_RELEASE(pContext);
    if (bNeedCleanup)
    {
        // TODO: Replace with ASSERT
        if (pContext->pDcInfo)
        {
            LWNET_LOG_ERROR("DC info in CLDAP thread context should be NULL.");
        }
        // Clean up lock and condition variables; free context
        if (pContext->pMutex)
        {
            pthread_mutex_destroy(pContext->pMutex);
        }
        if (pContext->pCondition)
        {
            pthread_cond_destroy(pContext->pCondition);
        }
        LWNET_SAFE_FREE_STRING(pContext->pszDnsDomainName);
        LWNetFreeMemory(pContext);
    }
}

BOOLEAN
LWNetSrvIsMatchingDcInfo(
    IN PLWNET_DC_INFO pDcInfo,
    IN DWORD dwDsFlags
    )
{
    BOOLEAN isMatching = TRUE;

    // If any of these conditions are satisfied,
    // then our match is no good.
    if (FALSE
        ||
        ((dwDsFlags & DS_DIRECTORY_SERVICE_REQUIRED) &&
         !(pDcInfo->dwFlags & DS_DS_FLAG))
        ||
        ((dwDsFlags & DS_GC_SERVER_REQUIRED) &&
         !(pDcInfo->dwFlags & DS_GC_FLAG))
        ||
        ((dwDsFlags & DS_PDC_REQUIRED) &&
         !(pDcInfo->dwFlags & DS_PDC_FLAG))
        ||
        ((dwDsFlags & DS_KDC_REQUIRED) &&
         !(pDcInfo->dwFlags & DS_KDC_FLAG))
        ||
        ((dwDsFlags & DS_TIMESERV_REQUIRED) &&
         !(pDcInfo->dwFlags & DS_TIMESERV_FLAG))
        ||
        ((dwDsFlags & DS_WRITABLE_REQUIRED) &&
         !(pDcInfo->dwFlags & DS_WRITABLE_FLAG))
        ||
        ((dwDsFlags & DS_GOOD_TIMESERV_REQUIRED) &&
         !(pDcInfo->dwFlags & DS_GOOD_TIMESERV_FLAG))
        ||
        FALSE)
    {
        isMatching = FALSE;
    }

    return isMatching;
}

BOOLEAN
LWNetSrvIsAffinitizableRequestFlags(
    IN DWORD dwDsFlags
    )
{
    BOOLEAN isCacheable = TRUE;

    // If any of these conditions are satisfied,
    // then caching should not be done.
    if (dwDsFlags & (DS_TIMESERV_REQUIRED | DS_GOOD_TIMESERV_REQUIRED))
    {
        isCacheable = FALSE;
    }

    return isCacheable;
}

static
BOOLEAN
LWNetSrvIsInSameSite(
    IN PLWNET_DC_INFO pDcInfo
    )
{
    BOOLEAN isInSameSite = FALSE;

    if (pDcInfo->pszDCSiteName && pDcInfo->pszClientSiteName &&
        !strcasecmp(pDcInfo->pszClientSiteName, pDcInfo->pszDCSiteName))
    {
        isInSameSite = TRUE;
    }

    return isInSameSite;
}

static
PVOID
LWNetSrvPingCLdapThread(
    PVOID pThreadContext
    )
{
    DWORD dwError = 0;
    PLWNET_CLDAP_THREAD_CONTEXT pContext = (PLWNET_CLDAP_THREAD_CONTEXT) pThreadContext;
    BOOLEAN isAcquired = FALSE;
    PSTR pszAddress = NULL;
    DWORD dwServerIndex = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    BOOLEAN shouldSignal = FALSE;

    for (;;)
    {
        LWNET_SAFE_FREE_STRING(pszAddress);

        //
        // Lock the context to grab next item.
        //
        if (!isAcquired)
        {
            LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext);
            isAcquired = TRUE;
        }

        //
        // Check whether there is a next item.
        //
        dwServerIndex = pContext->dwServerIndex;
        if (dwServerIndex >= pContext->dwServerCount)
        {
            break;
        }

        //
        // Capture the item before we unlock.
        //
        pContext->dwServerIndex++;
        dwError = LWNetAllocateString(pContext->pServerArray[dwServerIndex].pszAddress,
                                      &pszAddress);
        if (dwError)
        {
            LWNET_LOG_ERROR("Failed to allocate address string in %s() at %s:%d (error %d/0x%08x)", __FUNCTION__, __FILE__, __LINE__, dwError, dwError);
            continue;
        }

        //
        // Unlock before we do the ping.
        //
        LWNET_CLDAP_THREAD_CONTEXT_RELEASE(pContext);
        isAcquired = FALSE;

        //
        // Do the ping.
        //
        dwError = LWNetSrvPingCLdap(pContext->pszDnsDomainName, pszAddress, &pDcInfo);
        if (dwError)
        {
            LWNET_LOG_INFO("Failed CLDAP ping %s (%s) in %s() at %s:%d (error %d/0x%08x)", pContext->pszDnsDomainName, pszAddress, __FUNCTION__, __FILE__, __LINE__, dwError, dwError);
            continue;
        }
        if (!LWNetSrvIsMatchingDcInfo(pDcInfo, pContext->dwDsFlags))
        {
            if (LWNetSrvIsMatchingDcInfo(pDcInfo, pContext->dwDsFlags & ~DS_WRITABLE_REQUIRED))
            {
                // We found something, but it failed only because it did
                // not satisfy writability.  We mark this in the context.
                if (!isAcquired)
                {
                    LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext);
                    isAcquired = TRUE;
                }
                pContext->bFailedFindWritable = TRUE;
                LWNET_CLDAP_THREAD_CONTEXT_RELEASE(pContext);
                isAcquired = FALSE;
            }
            LWNET_SAFE_FREE_DC_INFO(pDcInfo);
            continue;
        }
        break;
    }

    if (pDcInfo)
    {
        //
        // Save the result, if appropriate.
        //
        if (!isAcquired)
        {
            LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext);
            isAcquired = TRUE;
        }

        if (!pContext->bIsDone && !pContext->pDcInfo)
        {
            shouldSignal = TRUE;

            pContext->pDcInfo = pDcInfo;
            pDcInfo = NULL;

            // Stop processing any more
            pContext->bIsDone = TRUE;
            pContext->dwServerIndex = pContext->dwServerCount;
        }
    }

    if (!isAcquired)
    {
        LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext);
        isAcquired = TRUE;
    }
    pContext->dwActiveThreadCount--;
    if (pContext->dwActiveThreadCount <= 0)
    {
        shouldSignal = TRUE;
    }

    if (shouldSignal)
    {
        // We really cannot do anything about an error here.
        dwError = pthread_cond_signal(pContext->pCondition);
        if (dwError)
        {
            LWNET_LOG_ERROR("Failed to signal condition in %s() at %s:%d (error %d/0x%08x)", __FUNCTION__, __FILE__, __LINE__, dwError, dwError);
        }
    }

    if (isAcquired)
    {
        LWNET_CLDAP_THREAD_CONTEXT_RELEASE(pContext);
        isAcquired = FALSE;
    }

    LWNET_SAFE_FREE_STRING(pszAddress);
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    LWNetSrvPingCLdapDerefenceThreadContext(pContext);

    return NULL;
}

DWORD
LWNetSrvPingCLdapArray(
    IN PCSTR pszDnsDomainName,
    IN DWORD dwDsFlags,
    IN PDNS_SERVER_INFO pServerArray,
    IN DWORD dwServerCount,
    IN OPTIONAL DWORD dwThreadCount,
    IN OPTIONAL DWORD dwTimeoutSeconds,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PBOOLEAN pbFailedFindWritable
    )
// TODO: Potentially have a minimum amount of time so we can evaluate multiple
// ping results (i.e., in a case where the ping starts later due to thread
// scheduling).  In such a case, we would check the ping time when setting
// the result in the context.  We would also defer trigerring the cond
// until time minimum time has elapsed.
{
    DWORD dwError = 0;
    PLWNET_CLDAP_THREAD_CONTEXT pContext = NULL;
    DWORD dwActualThreadCount = LWNET_CLDAP_DEFAULT_THREAD_COUNT;
    DWORD dwActualTimeoutSeconds = LWNET_CLDAP_DEFAULT_TIMEOUT_SECONDS;
    pthread_attr_t ThreadAttr;
    pthread_attr_t* pThreadAttr = NULL;
    BOOLEAN isAcquired = FALSE;
    struct timespec stopTime = { 0 };
    LWNET_UNIX_MS_TIME_T now = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    DWORD i;
    BOOLEAN bFailedFindWritable = FALSE;

    if (dwThreadCount > 0)
    {
        dwActualThreadCount = dwThreadCount;
    }
    dwActualThreadCount = CT_MIN(dwActualThreadCount, dwServerCount);

    if (dwTimeoutSeconds > 0)
    {
        dwActualTimeoutSeconds = dwTimeoutSeconds;
    }

    // TODO: Error code conversion
    dwError = pthread_attr_init(&ThreadAttr);
    BAIL_ON_LWNET_ERROR(dwError);
    pThreadAttr = &ThreadAttr;

    // TODO: Error code conversion
    dwError = pthread_attr_setdetachstate(pThreadAttr, PTHREAD_CREATE_DETACHED);
    BAIL_ON_LWNET_ERROR(dwError);

    // TODO: Global tracking of contexts (for debugging)? */
    dwError = LWNetAllocateMemory(sizeof(LWNET_CLDAP_THREAD_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_LWNET_ERROR(dwError);

    pContext->dwRefCount = 1;
    pContext->pServerArray = pServerArray;
    pContext->dwServerCount = dwServerCount;
    pContext->dwDsFlags = dwDsFlags;
    pContext->dwActiveThreadCount = 0;

    dwError = LWNetAllocateString(pszDnsDomainName, &pContext->pszDnsDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    // TODO: Error code conversion
    dwError = pthread_mutex_init(&pContext->Mutex, NULL);
    BAIL_ON_LWNET_ERROR(dwError);
    pContext->pMutex = &pContext->Mutex;

    // TODO: Error code conversion
    dwError = pthread_cond_init(&pContext->Condition, NULL);
    BAIL_ON_LWNET_ERROR(dwError);
    pContext->pCondition = &pContext->Condition;

    LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext);
    isAcquired = TRUE;

    for (i = 0; i < dwActualThreadCount; i++)
    {
        pthread_t thread;

        // TODO: Error code conversion
        dwError = pthread_create(&thread,
                                 pThreadAttr,
                                 LWNetSrvPingCLdapThread,
                                 pContext);
        BAIL_ON_LWNET_ERROR(dwError);

        pContext->dwActiveThreadCount++;
        pContext->dwRefCount++;
    }

    // Wait on condition variable up to some time limit

    dwError = LWNetGetSystemTimeInMs(&now);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetTimeInMsToTimespec(now + (dwActualTimeoutSeconds * LWNET_MILLISECONDS_IN_SECOND),
                                      &stopTime);
    BAIL_ON_LWNET_ERROR(dwError);

    // We are about to release by doing the wait and re-acquire afterwards
    isAcquired = FALSE;
    // TODO: Error code conversion
    dwError = pthread_cond_timedwait(pContext->pCondition, pContext->pMutex, &stopTime);
    isAcquired = TRUE;
    if (ETIMEDOUT == dwError)
    {
        dwError = NERR_DCNotFound;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    if (!pContext->pDcInfo)
    {
        dwError = NERR_DCNotFound;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    pDcInfo = pContext->pDcInfo;
    pContext->pDcInfo = NULL;

error:
    if (pContext)
    {
        // Need to make sure that threads will stop ASAP
        if (pContext->pMutex)
        {
            if (!isAcquired)
            {
                LWNET_CLDAP_THREAD_CONTEXT_ACQUIRE(pContext);
                isAcquired = TRUE;
            }
            // Stop processing any more
            pContext->bIsDone = TRUE;
            pContext->dwServerIndex = pContext->dwServerCount;
            bFailedFindWritable = pContext->bFailedFindWritable;
        }
        if (isAcquired)
        {
            LWNET_CLDAP_THREAD_CONTEXT_RELEASE(pContext);
            isAcquired = FALSE;
        }
        LWNetSrvPingCLdapDerefenceThreadContext(pContext);
    }
    if (pThreadAttr)
    {
        pthread_attr_destroy(pThreadAttr);
    }
    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    }
    *ppDcInfo = pDcInfo;
    *pbFailedFindWritable = pDcInfo ? FALSE : bFailedFindWritable;
    return dwError;
}

VOID
LWNetFilterFromBlackList(
    IN DWORD dwBlackListCount,
    IN OPTIONAL PSTR* ppszAddressBlackList,
    IN OUT PDWORD pdwServerCount,
    IN OUT PDNS_SERVER_INFO pServerArray
    )
{
    DWORD dwServerRead = 0;
    DWORD dwServerWrote = 0;
    DWORD dwBlackIndex = 0;
    BOOLEAN bBlackListed = FALSE;

    LWNET_LOG_INFO("Filtering list of %d servers with list of %d black listed servers", *pdwServerCount, dwBlackListCount);

    if (!dwBlackListCount)
    {
        return;
    }

    for (dwServerRead = 0; dwServerRead < *pdwServerCount; dwServerRead++)
    {
        bBlackListed = FALSE;
        for (dwBlackIndex = 0;
             !bBlackListed && dwBlackIndex < dwBlackListCount;
             dwBlackIndex++)
        {
            if (!strcmp(pServerArray[dwServerRead].pszAddress,
                        ppszAddressBlackList[dwBlackIndex]))
            {
                bBlackListed = TRUE;
                LWNET_LOG_INFO("Filtering server %s since it is black listed",
                        pServerArray->pszAddress);
            }
        }
        /* If bBlackListed is true, this server array entry will get
         * overwritten with the next non-blacklisted entry. The address and
         * name strings inside of the entry do not need to be freed because
         * they are allocated in the same memory block as the array.
         */
        if (!bBlackListed)
        {
            pServerArray[dwServerWrote++] = pServerArray[dwServerRead];
        }
    }
    *pdwServerCount = dwServerWrote;
}

typedef DWORD (*PLWNET_DC_LIST_QUERY_METHOD)(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PDNS_SERVER_INFO* ppServerArray,
    OUT PDWORD pdwServerCount
    );

static
DWORD
LWNetSrvGetDCNameDiscoverInternal(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN OPTIONAL PCSTR pszPrimaryDomain,
    IN DWORD dwDsFlags,
    IN DWORD dwBlackListCount,
    IN OPTIONAL PSTR* ppszAddressBlackList,
    IN PLWNET_DC_LIST_QUERY_METHOD pfnDCListQuery,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT OPTIONAL PDNS_SERVER_INFO* ppServerArray,
    OUT OPTIONAL PDWORD pdwServerCount,
    OUT PBOOLEAN bFailedFindWritable
    );

DWORD
LWNetSrvGetDCNameDiscover(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN OPTIONAL PCSTR pszPrimaryDomain,
    IN DWORD dwDsFlags,
    IN DWORD dwBlackListCount,
    IN OPTIONAL PSTR* ppszAddressBlackList,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT OPTIONAL PDNS_SERVER_INFO* ppServerArray,
    OUT OPTIONAL PDWORD pdwServerCount,
    OUT PBOOLEAN pbFailedFindWritable
    )
{
    DWORD dwError = 0;

    /* First try with the "Preferred DC list" */

    dwError = LWNetSrvGetDCNameDiscoverInternal(
                  pszDnsDomainName,
                  pszSiteName,
                  pszPrimaryDomain,
                  dwDsFlags,
                  dwBlackListCount,
                  ppszAddressBlackList,
                  LWNetGetPreferredDcList,
                  ppDcInfo,
                  ppServerArray,
                  pdwServerCount,
                  pbFailedFindWritable);
    if (dwError == ERROR_SUCCESS)
    {
        goto cleanup;
    }

    /* Try again using the standard DNS SRV queries */

    dwError = LWNetSrvGetDCNameDiscoverInternal(
                  pszDnsDomainName,
                  pszSiteName,
                  pszPrimaryDomain,
                  dwDsFlags,
                  dwBlackListCount,
                  ppszAddressBlackList,
                  LWNetDnsSrvQuery,
                  ppDcInfo,
                  ppServerArray,
                  pdwServerCount,
                  pbFailedFindWritable);
    BAIL_ON_LWNET_ERROR(dwError);    

cleanup:

    return dwError;

error:

    goto cleanup;    
}

static
DWORD
LWNetSrvGetDCNameDiscoverInternal(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN OPTIONAL PCSTR pszPrimaryDomain,
    IN DWORD dwDsFlags,
    IN DWORD dwBlackListCount,
    IN OPTIONAL PSTR* ppszAddressBlackList,
    IN PLWNET_DC_LIST_QUERY_METHOD pfnDCListQuery,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT OPTIONAL PDNS_SERVER_INFO* ppServerArray,
    OUT OPTIONAL PDWORD pdwServerCount,
    OUT PBOOLEAN pbFailedFindWritable
    )
//
// Algorithm:
//
//    - DNS query for desired site & required DC type (pdc, kdc, gc).
//      - note that if no site is specified, use "un-sited" lookup.
//    - If no site specified:
//      - CLDAP to one DC to get actual site
//      - use new site info that to do DNS query for updated DC list
//    - CLDAP to DCs in parallel to find the first responder
//      (meeting any additional criteria -- writable, etc)
//
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    PDNS_SERVER_INFO pServerArray = NULL;
    PDNS_SERVER_INFO pServersInPrimaryDomain = NULL;
    DWORD dwServersInPrimaryDomainCount = 0;
    DWORD dwServerCount = 0;
    PLWNET_DC_INFO pSiteDcInfo = NULL;
    PDNS_SERVER_INFO pSiteServerArray = NULL;
    DWORD dwSiteServerCount = 0;
    BOOLEAN bFailedFindWritable = FALSE;

    // Get server list
    dwError = pfnDCListQuery(pszDnsDomainName,
                             pszSiteName,
                             dwDsFlags,
                             &pServerArray,
                             &dwServerCount);
    BAIL_ON_LWNET_ERROR(dwError);

    LWNetFilterFromBlackList(
        dwBlackListCount,
        ppszAddressBlackList,
        &dwServerCount,
        pServerArray);
    if (!dwServerCount)
    {
        dwError = DNS_ERROR_BAD_PACKET;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (pszPrimaryDomain)
    {
        dwError = LWNetFindServersInDomain(
            pServerArray,
            dwServerCount, 
            pszPrimaryDomain,
            &pServersInPrimaryDomain,
            &dwServersInPrimaryDomainCount);
        BAIL_ON_LWNET_ERROR(dwError);

        if (dwServersInPrimaryDomainCount > 0)
        {
            dwError = LWNetSrvPingCLdapArray(pszDnsDomainName,
                                             dwDsFlags,
                                             pServersInPrimaryDomain,
                                             dwServersInPrimaryDomainCount,
                                             0, 0, &pDcInfo, &bFailedFindWritable);
        }

        if (dwServersInPrimaryDomainCount == 0 ||
            dwError == NERR_DCNotFound)
        {
            dwError = LWNetSrvPingCLdapArray(pszDnsDomainName,
                                             dwDsFlags,
                                             pServerArray, dwServerCount,
                                             0, 0, &pDcInfo, &bFailedFindWritable);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }
    else
    {
        // Do CLDAP
        dwError = LWNetSrvPingCLdapArray(pszDnsDomainName,
                                         dwDsFlags,
                                         pServerArray, dwServerCount,
                                         0, 0, &pDcInfo, &bFailedFindWritable);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    // If there is no client site, then we are done (though we do not
    // expect this to ever happen).
    if (IsNullOrEmptyString(pDcInfo->pszClientSiteName))
    {
        LWNET_LOG_ALWAYS("Missing client site name from "
                "DC response from %s (%s)",
                LWNET_SAFE_LOG_STRING(pDcInfo->pszDomainControllerName),
                LWNET_SAFE_LOG_STRING(pDcInfo->pszDomainControllerAddress));
        goto cleanup;
    }

    // If a site was passed in, there is nothing more to do.
    if (!IsNullOrEmptyString(pszSiteName))
    {
        goto cleanup;
    }

    // There was no site passed in, so we need to look at the
    // CLDAP response to get the client site.

    // If we got the correct site already, we are done.
    if (LWNetSrvIsInSameSite(pDcInfo))
    {
        dwError = 0;
        goto cleanup;
    }

    // Now we need to use the client site to find a site-specific DC.
    dwError = LWNetSrvGetDCNameDiscover(pszDnsDomainName,
                                        pDcInfo->pszClientSiteName,
                                        pszPrimaryDomain,
                                        dwDsFlags,
                                        dwBlackListCount,
                                        ppszAddressBlackList,
                                        &pSiteDcInfo,
                                        &pSiteServerArray, &dwSiteServerCount,
                                        &bFailedFindWritable);
    if (NERR_DCNotFound == dwError)
    {
        if (bFailedFindWritable)
        {
            LWNET_LOG_WARNING("No writable DC in client site '%s' for domain '%s'",
                              pDcInfo->pszClientSiteName,
                              pszDnsDomainName);
        }
        // Count not find site-specific DC, so use the original DC.
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    // Use the site-specific DC.
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    LWNET_SAFE_FREE_MEMORY(pServerArray);
    dwServerCount = 0;

    pDcInfo = pSiteDcInfo;
    pServerArray = pSiteServerArray;
    dwServerCount = dwSiteServerCount;

    pSiteDcInfo = NULL;
    pSiteServerArray = NULL;
    dwSiteServerCount = 0;

error:
cleanup:
    LWNET_SAFE_FREE_DC_INFO(pSiteDcInfo);
    LWNET_SAFE_FREE_MEMORY(pSiteServerArray);
    LWNET_SAFE_FREE_MEMORY(pServersInPrimaryDomain);
    dwSiteServerCount = 0;

    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
        LWNET_SAFE_FREE_MEMORY(pServerArray);
        dwServerCount = 0;
    }

    *ppDcInfo = pDcInfo;
    if (ppServerArray)
    {
        *ppServerArray = pServerArray;
        *pdwServerCount = dwServerCount;
    }
    else
    {
        LWNET_SAFE_FREE_MEMORY(pServerArray);
    }
    *pbFailedFindWritable = bFailedFindWritable;

    return dwError;
}

static
DWORD
LWNetFindServersInDomain(
    IN PDNS_SERVER_INFO pServerArray,
    IN DWORD dwServerCount,
    IN PCSTR pszDomainName,
    OUT PDNS_SERVER_INFO* ppServersInDomain,
    OUT PDWORD pdwServersInDomainCount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwServersInDomainCount = 0;
    DWORD dwServerIndex = 0;
    PDNS_SERVER_INFO pServersInDomain = NULL;

    for (i = 0; i < dwServerCount; i++)
    {
        if (LWNetServerIsInDomain(&pServerArray[i], pszDomainName))
        {
            dwServersInDomainCount++;
        }
    }

    if (dwServersInDomainCount)
    {
        dwError = LWNetAllocateMemory(dwServersInDomainCount * sizeof(*pServerArray), OUT_PPVOID(&pServersInDomain));
        BAIL_ON_LWNET_ERROR(dwError);
        
        for (i = 0; i < dwServerCount; i++)
        {
            if (LWNetServerIsInDomain(&pServerArray[i], pszDomainName))
            {
                pServersInDomain[dwServerIndex++] = pServerArray[i];
            }
        }
    }
        
    *ppServersInDomain = pServersInDomain;
    *pdwServersInDomainCount = dwServersInDomainCount;

cleanup:

    return dwError;

error:

    *ppServersInDomain = NULL;
    *pdwServersInDomainCount = 0;

    LWNET_SAFE_FREE_MEMORY(pServersInDomain);

    goto cleanup;
}

static
BOOL
LWNetServerIsInDomain(
    IN PDNS_SERVER_INFO pServerInfo,
    IN PCSTR pszDomainName
    )
{
    PSTR pszDot = strchr(pServerInfo->pszName, '.');

    if (!pszDot)
    {
        return FALSE;
    }
    else
    {
        return !strcasecmp(pszDot + 1, pszDomainName);
    }
}

DWORD
LWNetBuildDCInfo(
    IN PBYTE pBuffer,
    IN DWORD dwBufferSize,
    OUT PLWNET_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDCInfo = NULL;
    PACKED_ARRAY netLogon = { 0 };
    
    dwError = LWNetAllocateMemory(sizeof(LWNET_DC_INFO), (PVOID*)&pDCInfo);
    BAIL_ON_LWNET_ERROR(dwError);    
    
    netLogon.pStart = pBuffer;
    netLogon.pCur = pBuffer;
    netLogon.totalSize = dwBufferSize;

    dwError = LWNetReadLEDword(&pDCInfo->dwDomainControllerAddressType, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadLEDword(&pDCInfo->dwFlags, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadGUID((PBYTE)&pDCInfo->pucDomainGUID[0], &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszDnsForestName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszFullyQualifiedDomainName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszDomainControllerName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszNetBIOSDomainName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszNetBIOSHostName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszUserName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszDCSiteName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadString(&pDCInfo->pszClientSiteName, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadLEDword(&pDCInfo->dwVersion, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadLEWord(&pDCInfo->wLMToken, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetReadLEWord(&pDCInfo->wNTToken, &netLogon);
    BAIL_ON_LWNET_ERROR(dwError);

error:    
    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDCInfo);
    }

    *ppDCInfo = pDCInfo;

    return dwError;
}

static
DWORD
LWNetReadUnalignedDword(
    IN const void* ptr
    )
{
    DWORD dwResult;

    memcpy(&dwResult, ptr, sizeof(dwResult));

    return dwResult;
}

DWORD
LWNetReadLEDword(
    OUT PDWORD pdwDest,
    IN PACKED_ARRAY* pArray
    )
{
    if (pArray->totalSize + pArray->pStart - pArray->pCur < sizeof(DWORD))
    {
        return DNS_ERROR_BAD_PACKET;
    }
#if defined(WORDS_BIGENDIAN)
    *pdwDest = LW_ENDIAN_SWAP32(LWNetReadUnalignedDword(pArray->pCur));
#else
    *pdwDest = LWNetReadUnalignedDword(pArray->pCur);
#endif
    pArray->pCur += sizeof(DWORD);
    return ERROR_SUCCESS;
}

static
WORD
LWNetReadUnalignedWord(
    IN const void* ptr
    )
{
    WORD wResult;

    memcpy(&wResult, ptr, sizeof(wResult));

    return wResult;
}

DWORD
LWNetReadLEWord(
    OUT PWORD pwDest,
    IN PACKED_ARRAY* pArray
    )
{
    if (pArray->totalSize + pArray->pStart - pArray->pCur < sizeof(WORD))
    {
        return DNS_ERROR_BAD_PACKET;
    }
#if defined(WORDS_BIGENDIAN)
    *pwDest = LW_ENDIAN_SWAP16(LWNetReadUnalignedWord(pArray->pCur));
#else
    *pwDest = LWNetReadUnalignedWord(pArray->pCur);
#endif
    pArray->pCur += sizeof(WORD);
    return ERROR_SUCCESS;
}

DWORD
LWNetReadGUID(
    OUT PBYTE pbtDest,
    IN PACKED_ARRAY* pArray
    )
{
    DWORD dwError = 0;
    
    if (BYTES_REMAINING((*pArray)) < LWNET_GUID_SIZE)
    {
        dwError = DNS_ERROR_BAD_PACKET;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    memcpy(pbtDest, pArray->pCur, LWNET_GUID_SIZE);
    pArray->pCur += LWNET_GUID_SIZE;    

error:
    return dwError;
}

DWORD
LWNetReadString(
    OUT PSTR *ppszDest,
    IN PACKED_ARRAY *pArray
    )
{
    // Decode a string according to RFC1035
    DWORD dwError = 0;
    PSTR pszOut = NULL;
    size_t outOffset = 0;
    BYTE *followingCur;
    size_t followOffset;
    size_t copyLen;
    BYTE **pCur = &pArray->pCur;

    // A valid string can't be longer than the packet it comes from
    dwError = LWNetAllocateMemory(pArray->totalSize, (PVOID*)&pszOut);
    BAIL_ON_LWNET_ERROR(dwError);

    while (**pCur != '\0')
    {
        switch (**pCur & 0xC0)
        {
            case 0xC0:
                // This is a string reference
                followOffset = (((WORD)(*pCur)[0] << 8) | (*pCur)[1]) & 0x3FFF;
                if (followOffset >= pArray->totalSize)
                {
                    //Offset out of bounds
                    dwError = DNS_ERROR_BAD_PACKET;
                    goto error;
                }
                *pCur += 2;
                pCur = &followingCur;
                followingCur = pArray->pStart +  followOffset;
                if ((**pCur & 0xC0) == 0xC0)
                {
                    // This is definitely redundant, and it could be an infinite
                    // loop.
                    dwError = DNS_ERROR_BAD_PACKET;
                    goto error;
                }
                break;
            case 0x00:
                // This is a string component
                copyLen = (*pCur)[0];
                (*pCur)++;
                if (copyLen + 2 + outOffset > pArray->totalSize)
                {
                    // This goes out of bounds of the output buffer. There must be
                    // an infinite loop in the encoding.
                    dwError = DNS_ERROR_BAD_PACKET;
                    goto error;
                }
                if (copyLen + 1 + *pCur - pArray->pStart > pArray->totalSize)
                {
                    // The label goes out of bounds of the message
                    dwError = DNS_ERROR_BAD_PACKET;
                    goto error;
                }
                if (outOffset > 0)
                {
                    //Add a dot to separate the components
                    pszOut[outOffset++] = '.';
                }
                memcpy(pszOut + outOffset, *pCur, copyLen);
                *pCur += copyLen;
                outOffset += copyLen;
                break;
            default:
                // Illegal reserved value
                dwError = DNS_ERROR_BAD_PACKET;
                goto error;
        }
    }
    *pCur += 1;
    pszOut[outOffset++] = '\0';
    
error:
    if (dwError)
    {
        LWNET_SAFE_FREE_STRING(pszOut);
    }
    *ppszDest = pszOut;
    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
