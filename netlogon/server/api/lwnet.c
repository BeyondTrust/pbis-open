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
LWNetSrvPingCLdapEnd(
    IN PLWNET_CLDAP_CONNECTION_CONTEXT pContext
    );

static
DWORD
LWNetSrvPingCLdapBegin(
    IN PLWNET_CLDAP_CONNECTION_CONTEXT pContext,
    IN PDNS_SERVER_INFO pServerInfo,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTimeout
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR szAttributeList[] = { NETLOGON_LDAP_ATTRIBUTE_NAME, NULL };
    struct timeval timeout = {0};
    LDAP *ld = NULL;

    pContext->hDirectory = NULL;
    pContext->pServerInfo = pServerInfo;

    dwError = LwAllocateStringPrintf(&pszQuery,
                                        "(&(DnsDomain=%s)(NtVer=\\06\\00\\00\\80))",
                                        pszDnsDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwCLdapOpenDirectory(pServerInfo->pszAddress, &pContext->hDirectory);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwLdapBindDirectoryAnonymous(pContext->hDirectory);
    BAIL_ON_LWNET_ERROR(dwError);

    timeout.tv_sec = 0;
    timeout.tv_usec = dwTimeout;

    dwError = LWNetGetSystemTimeInMs(&pContext->StartTime);
    BAIL_ON_LWNET_ERROR(dwError);

    ld = LwLdapGetSession(pContext->hDirectory);

    dwError = ldap_search_ext(
                  ld,
                  "",
                  LDAP_SCOPE_BASE,
                  pszQuery,
                  szAttributeList,
                  0,
                  NULL,
                  NULL,
                  &timeout,
                  0,
                  &pContext->msgid);
    dwError = LwMapLdapErrorToLwError(dwError);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = ldap_get_option(
                  ld,
                  LDAP_OPT_DESC,
                  &pContext->fd);
    dwError = LwMapLdapErrorToLwError(dwError);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        if (pContext->hDirectory)
        {
            LWNetSrvPingCLdapEnd(pContext);
        }
    }

    LWNET_SAFE_FREE_STRING(pszQuery);

    return dwError;
}

static
DWORD
LWNetSrvPingCLdapProcess(
    IN PLWNET_CLDAP_CONNECTION_CONTEXT pContext,
    IN DWORD dwDsFlags,
    IN LWNET_UNIX_MS_TIME_T StopTime,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PBOOLEAN pbFailedFindWritable
    )
{
    DWORD dwError = 0;
    DWORD dwResultType = 0;
    LDAPMessage* pMessage = NULL;
    PBYTE pNetlogonAttributeValue = NULL;
    DWORD dwNetlogonAttributeSize = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    BOOLEAN bFailedFindWritable = FALSE;
    struct timeval timeout = {0};
    LDAP *ld = NULL;

    ld = LwLdapGetSession(pContext->hDirectory);

    dwResultType =  ldap_result(
                        ld,
                        pContext->msgid,
                        0,
                        &timeout,
                        &pMessage);
    if (dwResultType == 0)
    {
        // timed out
        goto error;
    }
    else if (dwResultType == -1)
    {
        // -1 = problem
        dwError = LDAP_NO_SUCH_OBJECT;
        LWNET_LOG_VERBOSE("Caught LDAP_NO_SUCH_OBJECT Error on ldap search");
    }
    else
    {
        // returns result type
        if (dwResultType != LDAP_RES_SEARCH_ENTRY)
        {
            dwError = LDAP_NO_SUCH_OBJECT;
            LWNET_LOG_DEBUG("Caught incorrect result type on ldap search: %d", dwError);
        }
    }
    dwError = LwMapLdapErrorToLwError(dwError);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwLdapGetBytes(
                    pContext->hDirectory,
                    pMessage,
                    NETLOGON_LDAP_ATTRIBUTE_NAME,
                    &pNetlogonAttributeValue,
                    &dwNetlogonAttributeSize);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetBuildDCInfo(pNetlogonAttributeValue,
                               dwNetlogonAttributeSize,
                               &pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(pContext->pServerInfo->pszAddress, &pDcInfo->pszDomainControllerAddress);
    BAIL_ON_LWNET_ERROR(dwError);

    pDcInfo->dwPingTime = (DWORD)(StopTime - pContext->StartTime);
    if (StopTime < pContext->StartTime)
    {
        LWNET_LOG_ERROR("Stop time is earlier than start time");
    }

    if (!LWNetSrvIsMatchingDcInfo(pDcInfo, dwDsFlags))
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;

        if (LWNetSrvIsMatchingDcInfo(pDcInfo, dwDsFlags & ~DS_WRITABLE_REQUIRED))
        {
            // We found something, but it failed only because it did
            // not satisfy writability.
            bFailedFindWritable = TRUE;
        }
    }

error:
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    LWNET_SAFE_FREE_MEMORY(pNetlogonAttributeValue);

    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);

        LWNetSrvPingCLdapEnd(pContext);
    }

    *ppDcInfo = pDcInfo;
    *pbFailedFindWritable = bFailedFindWritable;

    return dwError;
}

static
DWORD
LWNetSrvPingCLdapEnd(
    IN PLWNET_CLDAP_CONNECTION_CONTEXT pContext
    )
{
    DWORD dwError = 0;

    if (pContext->hDirectory)
    {
        LwLdapCloseDirectory(pContext->hDirectory);
        pContext->hDirectory = NULL;
    }

    return dwError;
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
VOID
LWNetSrvPingCLdapNewConnections(
    IN PCSTR pszDnsDomainName,
    IN DWORD dwSingleConnTimeoutMilliseconds,
    IN DWORD dwMaxConnCount,
    IN DWORD dwServerCount,
    IN DWORD dwIncrementalConnCount,
    IN PDNS_SERVER_INFO pServerArray,
    IN OUT PLWNET_CLDAP_CONNECTION_CONTEXT pConnections,
    IN OUT struct pollfd *Readfds,
    IN OUT DWORD* pdwActualConnCount,
    IN OUT DWORD* pdwUsedServers
    )
{
    DWORD dwError = 0;
    DWORD dwNewConn = 0;
    DWORD dwActualConnCount = *pdwActualConnCount;
    DWORD dwUsedServers = *pdwUsedServers;

    while (dwNewConn < dwIncrementalConnCount &&
           dwActualConnCount < dwMaxConnCount &&
           dwUsedServers < dwServerCount)
    {

        dwError = LWNetSrvPingCLdapBegin(
                      &pConnections[dwActualConnCount],
                      &pServerArray[dwUsedServers],
                      pszDnsDomainName,
                      dwSingleConnTimeoutMilliseconds);
        if (dwError)
        {
            dwError = 0;
        }
        else
        {
            Readfds[dwActualConnCount].fd = pConnections[dwActualConnCount].fd;
            Readfds[dwActualConnCount].events = POLLIN | POLLERR;
            pConnections[dwActualConnCount++].pServerInfo = &pServerArray[dwUsedServers];
            dwNewConn++;
        }
        dwUsedServers++;
    }

    *pdwActualConnCount = dwActualConnCount;
    *pdwUsedServers = dwUsedServers;
}

static
DWORD
LWNetSrvPingCLdapPoll(
    IN DWORD dwTimeoutMilliseconds,
    IN DWORD dwFdCount,
    IN struct pollfd *Readfds
    )
{
    DWORD dwError = 0;
    int sret = 0;
    LWNET_UNIX_MS_TIME_T CurrentTime = 0;
    LWNET_UNIX_MS_TIME_T StopTime = 0;
    DWORD dwRemainder = 0;

    dwError = LWNetGetSystemTimeInMs(&StopTime);
    BAIL_ON_LWNET_ERROR(dwError);

    StopTime += dwTimeoutMilliseconds;

    do
    {
        dwError = LWNetGetSystemTimeInMs(&CurrentTime);
        BAIL_ON_LWNET_ERROR(dwError);

        if (CurrentTime < StopTime)
        {
            dwRemainder = StopTime - CurrentTime;
            sret = poll(Readfds, dwFdCount, dwRemainder);
        }
        else
        {
            sret = 0;
        }
    } while (sret < 0 && errno == EINTR);

    if (sret < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_LWNET_ERROR(dwError);

error:

    return dwError;
}

static
VOID
LWNetSrvPingCLdapProcessConnections(
    IN DWORD dwDsFlags,
    IN LWNET_UNIX_MS_TIME_T CurrentTime,
    IN DWORD dwSingleConnTimeoutMilliseconds,
    IN DWORD dwActualConnCount,
    IN struct pollfd *Readfds,
    IN OUT PLWNET_CLDAP_CONNECTION_CONTEXT pConnections,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PBOOLEAN pbFailedFindWritable
    )
{
    DWORD dwError = 0;
    DWORD dwIndexConn = dwActualConnCount;
    BOOLEAN bFailedFindWritable = FALSE;
    BOOLEAN bFailed = FALSE;
    PLWNET_DC_INFO pDcInfo = NULL;

    while (dwIndexConn > 0)
    {
        dwIndexConn--;

        if (Readfds[dwIndexConn].revents)
        {
            dwError = LWNetSrvPingCLdapProcess(
                          &pConnections[dwIndexConn],
                          dwDsFlags,
                          CurrentTime,
                          &pDcInfo,
                          &bFailed);
            bFailedFindWritable = bFailed ? TRUE : bFailedFindWritable;
            if (dwError)
            {
                LWNET_LOG_VERBOSE("CLDAP error: %d, %s", dwError, pConnections[dwIndexConn].pServerInfo->pszName);
                dwError = 0;
            }
            else if (pDcInfo)
            {
                break;
            }
        } else if (pConnections[dwIndexConn].StartTime + dwSingleConnTimeoutMilliseconds < CurrentTime)
        {
            LWNET_LOG_VERBOSE("CLDAP timed out: %s", pConnections[dwIndexConn].pServerInfo->pszName);

            LWNetSrvPingCLdapEnd(&pConnections[dwIndexConn]);
        }
    }

    *ppDcInfo = pDcInfo;
}

static
DWORD
LWNetSrvPingCLdapPackArrays(
    IN DWORD dwConnectionCount,
    IN PLWNET_CLDAP_CONNECTION_CONTEXT pConnections,
    IN struct pollfd *Readfds
    )
{
    DWORD dwSourceConn = 0;
    DWORD dwTargetConn = 0;

    // Eliminate the invalid connections while preserving
    // the order so that the newest connections remain at
    // the end of the list.
    for (dwTargetConn = 0, dwSourceConn = 0 ; 
         dwSourceConn < dwConnectionCount ;
         dwSourceConn++)
    {
        if (pConnections[dwTargetConn].hDirectory == NULL &&
            pConnections[dwSourceConn].hDirectory != NULL)
        {
            Readfds[dwTargetConn] = Readfds[dwSourceConn];
            pConnections[dwTargetConn] = pConnections[dwSourceConn];
            pConnections[dwSourceConn].hDirectory = NULL;
        }

        if (pConnections[dwTargetConn].hDirectory != NULL)
        {
            dwTargetConn++;
        }
    }

    return dwTargetConn;
}

DWORD
LWNetSrvPingCLdapArray(
    IN PCSTR pszDnsDomainName,
    IN DWORD dwDsFlags,
    IN PDNS_SERVER_INFO pServerArray,
    IN DWORD dwServerCount,
    IN OPTIONAL DWORD dwTimeoutSeconds,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PBOOLEAN pbFailedFindWritable
    )
{
    DWORD dwError = 0;
    DWORD dwMaxConnCount = 0;
    DWORD dwIncrementalConnCount = 0;
    DWORD dwActualTimeoutSeconds = 0;
    DWORD dwSingleConnTimeoutSeconds = 0;
    DWORD dwSingleConnTimeoutMilliseconds = 0;
    DWORD dwTimeoutMilliseconds = 0;
    LWNET_UNIX_MS_TIME_T StopTime = 0;
    LWNET_UNIX_MS_TIME_T CurrentTime = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    BOOLEAN bFailedFindWritable = FALSE;
    PLWNET_CLDAP_CONNECTION_CONTEXT pConnections = NULL;
    DWORD dwActualConnCount = 0;
    DWORD dwUsedServers = 0;
    DWORD dwIndexConn = 0;
    struct pollfd *Readfds = NULL;

    // The basic scheme is a big loop over:
    //
    //     1. starting some CLDAP searches
    //     2. polling for responses
    //     3. processing responses
    //
    // When starting CLDAP searches, a portion of the 
    // available connections will be started followed
    // by a short poll until the maximum number of
    // connections has been used.  Once the maximum
    // number of connections are in use, the polling
    // timeout will be the single search timeout for
    // the oldest connection.  This will start the
    // maximum number of connections reasonably fast
    // yet prevent excess network traffic if a server
    // responds quickly.
    //
    // Active connections will be tracked in a list
    // that is ordered by start time.  If several
    // servers respond at the same time, the one
    // nearest the end of the list will be the one
    // that responded the fastest.

    dwMaxConnCount = CT_MIN(LWNetConfigGetCLdapMaximumConnections(), dwServerCount);
    dwIncrementalConnCount = CT_MAX(dwMaxConnCount / 5, LWNET_CLDAP_MINIMUM_INCREMENTAL_CONNECTIONS);

    if (dwTimeoutSeconds > 0)
    {
        dwActualTimeoutSeconds = dwTimeoutSeconds;
    }
    else
    {
        dwActualTimeoutSeconds = LWNetConfigGetCLdapSearchTimeoutSeconds();
    }

    dwSingleConnTimeoutSeconds = CT_MIN(LWNetConfigGetCLdapSingleConnectionTimeoutSeconds(), dwActualTimeoutSeconds);
    dwSingleConnTimeoutMilliseconds = dwSingleConnTimeoutSeconds * 1000;

    dwError = LWNetAllocateMemory(
                  dwMaxConnCount * sizeof(*pConnections),
                  OUT_PPVOID(&pConnections));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateMemory(
                  dwMaxConnCount * sizeof(*Readfds),
                  OUT_PPVOID(&Readfds));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetSystemTimeInMs(&CurrentTime);
    BAIL_ON_LWNET_ERROR(dwError);

    StopTime = CurrentTime + (dwActualTimeoutSeconds * 1000);

    for (;;)
    {
        LWNetSrvPingCLdapNewConnections(
            pszDnsDomainName,
            dwSingleConnTimeoutMilliseconds,
            dwMaxConnCount,
            dwServerCount,
            dwIncrementalConnCount,
            pServerArray,
            pConnections,
            Readfds,
            &dwActualConnCount,
            &dwUsedServers);

        if (dwActualConnCount == 0)
        {
            dwError = NERR_DCNotFound;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        dwError = LWNetGetSystemTimeInMs(&CurrentTime);
        BAIL_ON_LWNET_ERROR(dwError);

        if (CurrentTime > StopTime)
        {
            dwError = NERR_DCNotFound;
            BAIL_ON_LWNET_ERROR(dwError);
        }

        if (dwActualConnCount < dwMaxConnCount &&
            dwUsedServers < dwServerCount)
        {
            // If there are more connections available,
            // use a short timeout
            dwTimeoutMilliseconds = LWNET_CLDAP_SHORT_POLL_TIMEOUT_MILLISECONDS;
        }
        else
        {
            if (pConnections[0].StartTime +
                dwSingleConnTimeoutMilliseconds > CurrentTime)
            {
                // Use the remaining time for the oldest connection
                dwTimeoutMilliseconds = pConnections[0].StartTime + 
                                        dwSingleConnTimeoutMilliseconds -
                                        CurrentTime;
                dwTimeoutMilliseconds = CT_MIN(dwSingleConnTimeoutMilliseconds, StopTime - CurrentTime);
            }
            else
            {
                // The oldest connection has exceeded its limit so
                // use the shortest possible timeout to check which
                // connections have responded now.
                dwTimeoutMilliseconds = 1;
            }
        }

        dwError = LWNetSrvPingCLdapPoll(
                      dwTimeoutMilliseconds,
                      dwActualConnCount,
                      Readfds);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetGetSystemTimeInMs(&CurrentTime);
        BAIL_ON_LWNET_ERROR(dwError);

        LWNetSrvPingCLdapProcessConnections(
            dwDsFlags,
            CurrentTime,
            dwSingleConnTimeoutMilliseconds,
            dwActualConnCount,
            Readfds,
            pConnections,
            &pDcInfo,
            &bFailedFindWritable);

        if (pDcInfo)
        {
            break;
        }

        dwActualConnCount = LWNetSrvPingCLdapPackArrays(
                                dwActualConnCount,
                                pConnections,
                                Readfds);
    }

error:
    if (pConnections)
    {
        for (dwIndexConn = 0 ; dwIndexConn < dwActualConnCount ; dwIndexConn++)
        {
            LWNetSrvPingCLdapEnd(&pConnections[dwIndexConn]);
        }
        LW_SAFE_FREE_MEMORY(pConnections);
    }
    LW_SAFE_FREE_MEMORY(Readfds);
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
                                             0, &pDcInfo, &bFailedFindWritable);
        }

        if (dwServersInPrimaryDomainCount == 0 ||
            dwError == NERR_DCNotFound)
        {
            dwError = LWNetSrvPingCLdapArray(pszDnsDomainName,
                                             dwDsFlags,
                                             pServerArray, dwServerCount,
                                             0, &pDcInfo, &bFailedFindWritable);
            BAIL_ON_LWNET_ERROR(dwError);
        }
    }
    else
    {
        // Do CLDAP
        dwError = LWNetSrvPingCLdapArray(pszDnsDomainName,
                                         dwDsFlags,
                                         pServerArray, dwServerCount,
                                         0, &pDcInfo, &bFailedFindWritable);
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
        if (LWNetIsUpdateKrb5AffinityEnabled(dwDsFlags, pszSiteName, pDcInfo) &&
            !pszSiteName &&
            pDcInfo->pszClientSiteName)
        {
            dwError = pfnDCListQuery(
                          pszDnsDomainName,
                          pDcInfo->pszClientSiteName,
                          dwDsFlags,
                          &pSiteServerArray,
                          &dwSiteServerCount);
            if (dwError == 0)
            {
                // Use the site-specific DC.
                LWNET_SAFE_FREE_MEMORY(pServerArray);
                dwServerCount = 0;

                pServerArray = pSiteServerArray;
                dwServerCount = dwSiteServerCount;

                pSiteServerArray = NULL;
                dwSiteServerCount = 0;
            }
        }

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

BOOLEAN
LWNetIsUpdateKrb5AffinityEnabled(
    IN DWORD DsFlags,
    IN PCSTR SiteName,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    BOOLEAN enabled = FALSE;

    // Will only affinitize for KDC/LDAP (i.e., not PDC/GC), if the site
    // is the same

    if (!(DsFlags & (DS_PDC_REQUIRED | DS_GC_SERVER_REQUIRED)) &&
        (IsNullOrEmptyString(SiteName) || 
         (strcasecmp(SiteName, pDcInfo->pszClientSiteName) == 0)))
    {
        enabled = TRUE;
    }

    return enabled;
}
