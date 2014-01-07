/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        dcinfo.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Domain Controller Info API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

static
DWORD
LWNetSrvGetDCTimeFromDC(
    IN PCSTR pszDCAddress,
    OUT PLWNET_UNIX_TIME_T pDCTime,
    OUT PBOOLEAN pbIsNetworkError
    );

DWORD
LWNetSrvGetDCName(
    IN PCSTR pszServerName,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN PCSTR pszPrimaryDomain,
    IN DWORD dwDsFlags,
    IN DWORD dwBlackListCount,
    IN PSTR* ppszAddressBlackList,
    OUT PLWNET_DC_INFO* ppDcInfo
    )
//
// TODO: Remove server name param.  Technically, the server-size should not
//       take a server name.  That's for the client to use to figure out
//       where to dispatch the request (as an RPC, in that case).
//
// TODO: Add parameter for GUID-based lookups.
//
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T now = 0;
    LWNET_UNIX_TIME_T lastPinged = 0;
    LWNET_UNIX_TIME_T lastDiscovered = 0;
    BOOLEAN isBackoffToWritableDc = FALSE;
    LWNET_UNIX_TIME_T lastBackoffToWritableDc = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    PLWNET_DC_INFO pNewDcInfo = NULL;
    PDNS_SERVER_INFO pServerArray = NULL;
    DWORD dwServerCount = 0;
    DWORD dwIndex = 0;
    BOOLEAN bFailedFindWritable = FALSE;
    BOOLEAN bUpdateCache = FALSE;
    BOOLEAN bUpdateKrb5Affinity = FALSE;

    LWNET_LOG_INFO("Looking for a DC in domain '%s', site '%s' with flags %X",
            LWNET_SAFE_LOG_STRING(pszDnsDomainName),
            LWNET_SAFE_LOG_STRING(pszSiteName),
            dwDsFlags);

    if (!IsNullOrEmptyString(pszServerName))
    {
        LWNET_LOG_ERROR("Cannot specify computer name: '%s'", pszServerName);
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    for (dwIndex = 0; dwIndex < dwBlackListCount; dwIndex++)
    {
        if (!ppszAddressBlackList[dwIndex])
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }

    // Look up in cache
    if (!(dwDsFlags & DS_FORCE_REDISCOVERY))
    {
        dwError = LWNetCacheQuery(pszDnsDomainName, pszSiteName, dwDsFlags,
                                  &pDcInfo, &lastDiscovered, &lastPinged,
                                  &isBackoffToWritableDc,
                                  &lastBackoffToWritableDc);
        BAIL_ON_LWNET_ERROR(dwError);

        // If in background mode, we do not care about any expiration
        if (dwDsFlags & DS_BACKGROUND_ONLY)
        {
            dwError = pDcInfo ? 0 : ERROR_NO_SUCH_DOMAIN;
            BAIL_ON_LWNET_ERROR(dwError);
            goto error;
        }

        dwError = LWNetGetSystemTime(&now);
        BAIL_ON_LWNET_ERROR(dwError);

        // Check whether a negative cache applies
        if (!pDcInfo && (lastDiscovered > 0))
        {
            if ((now - lastDiscovered) <= LWNetConfigGetNegativeCacheTimeoutSeconds())
            {
                dwError = ERROR_NO_SUCH_DOMAIN;
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }

        // ISSUE-2008/07/03-Perhaps cache on pszClientSiteName too...
        // Actually, it may be worthwhile to have that take place
        // under the hood inside the caching code itself.

        // If we found something in the cache, we may need to ping it.
        if (pDcInfo)
        {
            if (isBackoffToWritableDc &&
                (now - lastBackoffToWritableDc) > LWNetConfigGetWritableRediscoveryTimeoutSeconds())
            {
                // Need to re-affinitize
                LWNET_SAFE_FREE_DC_INFO(pDcInfo);
            }
            // Note that we only explicitly verify writability wrt re-affinitization.
            else if (!LWNetSrvIsMatchingDcInfo(pDcInfo, dwDsFlags))
            {
                // Cannot use these results.
                LWNET_SAFE_FREE_DC_INFO(pDcInfo);
            }
            else if ((now - lastPinged) > LWNetConfigGetPingAgainTimeoutSeconds())
            {
                DNS_SERVER_INFO serverInfo;

                serverInfo.pszName = pDcInfo->pszDomainControllerName;
                serverInfo.pszAddress = pDcInfo->pszDomainControllerAddress;

                while (serverInfo.pszName && serverInfo.pszName[0] == '\\')
                    serverInfo.pszName++;
                while (serverInfo.pszAddress && serverInfo.pszAddress[0] == '\\')
                    serverInfo.pszAddress++;

                dwError = LWNetSrvPingCLdapArray(pszDnsDomainName,
                                                 dwDsFlags,
                                                 &serverInfo, 1,
                                                 0, &pNewDcInfo,
                                                 &bFailedFindWritable);
                if (!dwError)
                {
                    // We need to update the cache to reflect the ping time
                    // and perhaps extend the life of the backoff to writable
                    // DC flag.

                    bUpdateCache = TRUE;

                    dwError = LWNetGetSystemTime(&now);
                    BAIL_ON_LWNET_ERROR(dwError);

                    lastPinged = now;

                    if ((dwDsFlags & DS_WRITABLE_REQUIRED) && isBackoffToWritableDc)
                    {
                        // update the time
                        lastBackoffToWritableDc = now;
                    }

                    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

                    pDcInfo = pNewDcInfo;
                    pNewDcInfo = 0;

                }
                else
                {
                    // now we need to bottom out and find us new info.
                    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
                }
            }
            else
            {
                // cached data is fine
                dwError = 0;

                // check whether we need to update the backoff time
                if ((dwDsFlags & DS_WRITABLE_REQUIRED) &&
                    isBackoffToWritableDc &&
                    (now - lastBackoffToWritableDc) > LWNetConfigGetWritableTimestampMinimumChangeSeconds())
                {
                    // We need to update cache to extend the life of the
                    // backoff to writable DC flag.

                    bUpdateCache = TRUE;

                    dwError = LWNetGetSystemTime(&now);
                    BAIL_ON_LWNET_ERROR(dwError);

                    lastBackoffToWritableDc = now;
                }
            }
        }
    }

    for (dwIndex = 0; pDcInfo && dwIndex < dwBlackListCount; dwIndex++)
    {
        if (!strcmp(pDcInfo->pszDomainControllerAddress,
                    ppszAddressBlackList[dwIndex]))
        {
            LWNET_SAFE_FREE_DC_INFO(pDcInfo);
        }
    }

    if (!pDcInfo)
    {
        dwError = LWNetSrvGetDCNameDiscover(pszDnsDomainName,
                                            pszSiteName,
                                            pszPrimaryDomain,
                                            dwDsFlags,
                                            dwBlackListCount,
                                            ppszAddressBlackList,
                                            &pDcInfo,
                                            &pServerArray,
                                            &dwServerCount,
                                            &bFailedFindWritable);
        BAIL_ON_LWNET_ERROR(dwError);

        // Do not update the cache if a black list is passed in. Otherwise,
        // callers would have too much control over which DC is affinitized.
        // Also do not update if looking for a writeable DC since Windows
        // (Vista) does not appear to update its cache in that case either.
        if ((dwBlackListCount == 0) && LWNetSrvIsAffinitizableRequestFlags(dwDsFlags))
        {
            // We need to update the cache with this entry.
            
            bUpdateCache = TRUE;
            
            dwError = LWNetGetSystemTime(&now);
            BAIL_ON_LWNET_ERROR(dwError);

            lastDiscovered = now;
            lastPinged = now;
            isBackoffToWritableDc = bFailedFindWritable;
            lastBackoffToWritableDc = isBackoffToWritableDc ? now : 0;

            bUpdateKrb5Affinity = LWNetIsUpdateKrb5AffinityEnabled(
                                      dwDsFlags,
                                      pszSiteName, 
                                      pDcInfo);
        }
    }

    // Handle updates

    if (bUpdateCache)
    {
        dwError = LWNetCacheUpdate(pszDnsDomainName,
                                   pszSiteName,
                                   dwDsFlags,
                                   lastDiscovered,
                                   lastPinged,
                                   isBackoffToWritableDc,
                                   lastBackoffToWritableDc,
                                   pDcInfo);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (bUpdateKrb5Affinity)
    {
        dwError = LWNetKrb5UpdateAffinity(
                        pszDnsDomainName,
                        pDcInfo,
                        pServerArray,
                        dwServerCount);
        BAIL_ON_LWNET_ERROR(dwError);
    }


error:
    LWNET_SAFE_FREE_MEMORY(pServerArray);
    LWNET_SAFE_FREE_DC_INFO(pNewDcInfo);
    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    }
    *ppDcInfo = pDcInfo;
    return dwError;
}

DWORD
LWNetSrvGetDCList(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_ADDRESS* ppDcList,
    OUT PDWORD pdwDcCount
    )
{
    DWORD dwError = 0;
    PDNS_SERVER_INFO pServerArray = NULL;
    DWORD dwServerCount = 0;
    PLWNET_DC_ADDRESS pDcList = NULL;
    DWORD i = 0;

    dwError = LWNetDnsSrvQuery(
                    pszDnsDomainName,
                    pszSiteName,
                    dwDsFlags,
                    &pServerArray,
                    &dwServerCount);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateMemory(
                    sizeof(pDcList[0]) * dwServerCount,
                    (PVOID*)&pDcList);
    BAIL_ON_LWNET_ERROR(dwError);

    for (i = 0; i < dwServerCount; i++)
    {
        if (pServerArray[i].pszName)
        {
            dwError = LWNetAllocateString(pServerArray[i].pszName, &pDcList[i].pszDomainControllerName);
            BAIL_ON_LWNET_ERROR(dwError);
        }
        if (pServerArray[i].pszAddress)
        {
            dwError = LWNetAllocateString(pServerArray[i].pszAddress, &pDcList[i].pszDomainControllerAddress);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }

error:
    if (dwError)
    {
        if (pDcList)
        {
            LWNetFreeDCList(pDcList, dwServerCount);
            pDcList = NULL;
        }
        dwServerCount = 0;
    }

    LWNET_SAFE_FREE_MEMORY(pServerArray);

    *ppDcList = pDcList;
    *pdwDcCount = dwServerCount;

    return dwError;
}

DWORD
LWNetSrvGetDomainController(
    IN PCSTR pszDomainFQDN,
    OUT PSTR* ppszDomainControllerFQDN
    )
{
    DWORD dwError = 0;
    PSTR pszDomainControllerFQDN = NULL;
    PLWNET_DC_INFO pDcInfo = NULL;
    
    dwError = LWNetSrvGetDCName(NULL,
                                pszDomainFQDN,
                                NULL,
                                NULL,
                                DS_DIRECTORY_SERVICE_REQUIRED,
                                0,
                                NULL,
                                &pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(pDcInfo->pszDomainControllerName,
                                  &pszDomainControllerFQDN);
    BAIL_ON_LWNET_ERROR(dwError);
    
error:    
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    if (dwError)
    {
        LWNET_SAFE_FREE_STRING(pszDomainControllerFQDN);
    }
    *ppszDomainControllerFQDN = pszDomainControllerFQDN;
    return dwError;
}

DWORD
LWNetSrvGetDCTime(
    IN PCSTR pszDomainFQDN,
    OUT PLWNET_UNIX_TIME_T pDCTime
    )
{
    DWORD dwError   = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    PSTR  pszDCTime = NULL;
    LWNET_UNIX_TIME_T result = 0;
    BOOLEAN bIsNetworkError = FALSE;

    BAIL_ON_INVALID_POINTER(pDCTime);

    LWNET_LOG_INFO("Determining the current time for domain '%s'",
            LWNET_SAFE_LOG_STRING(pszDomainFQDN));

    dwError = LWNetSrvGetDCName(
                  NULL,
                  pszDomainFQDN,
                  NULL,
                  NULL,
                  DS_DIRECTORY_SERVICE_REQUIRED,
                  0,
                  NULL,
                  &pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvGetDCTimeFromDC(
        pDcInfo->pszDomainControllerAddress,
        &result,
        &bIsNetworkError);
    if (bIsNetworkError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);

        dwError = LWNetSrvGetDCName(
                      NULL,
                      pszDomainFQDN,
                      NULL,
                      NULL,
                      DS_DIRECTORY_SERVICE_REQUIRED | DS_FORCE_REDISCOVERY,
                      0,
                      NULL,
                      &pDcInfo);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetSrvGetDCTimeFromDC(
            pDcInfo->pszDomainControllerAddress,
            &result,
            &bIsNetworkError);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else
    {
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    LWNET_SAFE_FREE_STRING(pszDCTime);

    if (dwError)
    {
        memset(&result, 0, sizeof(result));
    }

    *pDCTime = result;

    return dwError;
}

static
DWORD
LWNetSrvGetDCTimeFromDC(
    IN PCSTR pszDCAddress,
    OUT PLWNET_UNIX_TIME_T pDCTime,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError   = 0;
    BOOLEAN bIsNetworkError = FALSE;
    PSTR  pszDCTime = NULL;
    struct tm dcTime = {0};
    time_t ttDcTimeUTC = 0;
    LDAPMessage* pMessage = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR  ppszAttributeList[] = 
        {
             "currentTime",
             NULL
        };
    LWNET_UNIX_TIME_T result = 0;
#ifndef HAVE_TIMEGM
    struct tm utcTm = {0};
    struct tm utcTmAdjusted = {0};
    struct tm dcTimeCopy = {0};
    time_t now = -1;
#endif

    BAIL_ON_INVALID_POINTER(pDCTime);

    dwError = LwCLdapOpenDirectory(pszDCAddress, &hDirectory);
    if (dwError)
    {
        LWNET_LOG_ERROR(
            "Failed ldap open on %s error=%d\n",
            pszDCAddress,
            dwError);
    }
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LwLdapBindDirectoryAnonymous(hDirectory);
    if (dwError)
    {
        LWNET_LOG_ERROR(
            "Failed ldap bind on %s error=%d\n",
            pszDCAddress,
            dwError);
    }
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwLdapDirectorySearchEx(
                  hDirectory,
                  "",
                  LDAP_SCOPE_BASE,
                  "(objectclass=*)",
                  ppszAttributeList,
                  NULL,
                  0,
                  &pMessage);
    if (dwError)
    {
        LWNET_LOG_ERROR(
            "Failed ldap search on %s error=%d\n",
            pszDCAddress,
            dwError);
    }
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LwLdapGetString(hDirectory, pMessage, "currentTime",
                              &pszDCTime);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetCrackLdapTime(pszDCTime, &dcTime);
    if (dwError)
    {
        if (dwError == EINVAL)
        {
            dwError = ERROR_INVALID_TIME;
        }
        BAIL_ON_LWNET_ERROR(dwError);
    }
     
#ifdef HAVE_TIMEGM
    ttDcTimeUTC = timegm(&dcTime);
#else
    // We need to convert a universal time in a time structure to 
    // seconds since the epoch but the only function that is available
    // assumes it's a local time and applies the timezone offset.  We
    // have to determine what the offset is for today's date which could
    // vary depending on whether daylight savings time is in effect and
    // adjust the results.  It is possible that the local time and the DC
    // time are on opposite sides of the daylight savings time change
    // boundary so the result could be off by as much as an hour and may
    // may need to be adjusted again.  Note that we're saving a copy of
    // dcTime because mktime has a tendency to modify the structure.
    now = time(NULL);
    gmtime_r(&now, &utcTm);
    dcTimeCopy = dcTime;
    ttDcTimeUTC = mktime(&dcTime) + now - mktime(&utcTm);

    gmtime_r(&ttDcTimeUTC, &utcTmAdjusted);

    if (utcTmAdjusted.tm_hour == 0 && dcTimeCopy.tm_hour != 0)
    {
        ttDcTimeUTC += (dcTimeCopy.tm_hour - 24) * 60 * 60;
    }
    else if (utcTmAdjusted.tm_hour != 0 && dcTimeCopy.tm_hour == 0)
    {
        ttDcTimeUTC += (24 - utcTmAdjusted.tm_hour) * 60 * 60;
    }
    else
    {
        ttDcTimeUTC += (dcTimeCopy.tm_hour - utcTmAdjusted.tm_hour) * 60 * 60;
    }
    ttDcTimeUTC += (dcTimeCopy.tm_min - utcTmAdjusted.tm_min) * 60;
#endif

    result = ttDcTimeUTC;

error:
    LWNET_SAFE_FREE_STRING(pszDCTime);

    if (hDirectory)
    {
        LwLdapCloseDirectory(hDirectory);
    }

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    if (dwError)
    {
        memset(&result, 0, sizeof(result));

        switch (dwError)
        {
            case LW_ERROR_LDAP_SERVER_DOWN:
            case LW_ERROR_LDAP_TIMEOUT:
            case LW_ERROR_LDAP_SERVER_UNAVAILABLE:
            case LW_ERROR_LDAP_CONNECT_ERROR:
                bIsNetworkError = TRUE;
                break;
        }
    }

    *pDCTime = result;
    *pbIsNetworkError = bIsNetworkError;

    return dwError;
}
