/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        connect.c
 *
 * Abstract:
 *
 *        Methods related to AD connection establishment.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**
 * Find the Windows domain this machine is joined to.
 *
 * @param domain Found domain.
 * @return 0 on success; error code on failure.
 */
static DWORD FindJoinedDomain(OUT PSTR *domain) {
    DWORD dwError = 0;
    PLSASTATUS pLsaStatus = NULL;
    HANDLE hLsaConnection = (HANDLE) NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LsaGetStatus(hLsaConnection, &pLsaStatus);
    ADT_BAIL_ON_ERROR_NP(dwError);

    if(
       !pLsaStatus ||
       !pLsaStatus->pAuthProviderStatusList ||
       !pLsaStatus->pAuthProviderStatusList[0].pszDomain
    ) {
       dwError = ADT_ERR_FAILED_TO_LOCATE_DOMAIN;
       ADT_BAIL_ON_ERROR_NP(dwError);
    }

    dwError = LwAllocateString((PCSTR) pLsaStatus->pAuthProviderStatusList[0].pszDomain, domain);
    ADT_BAIL_ON_ERROR_NP(dwError);

    cleanup:
        if (pLsaStatus) {
            LsaFreeStatus(pLsaStatus);
        }

        if (hLsaConnection != (HANDLE) NULL) {
            LsaCloseServer(hLsaConnection);
        }

        return (dwError);

    error:
        goto cleanup;
}

/**
 * Find locate AD server to connect to.
 *
 * @param domain Domain to look AD server in.
 * @param ipAddress AD server IP address.
 * @param dnsName AD server DNS name.
 * @return 0 on success; error code on failure.
 */
DWORD
FindAdServer(IN PSTR domain, OUT PSTR *ipAddress, OUT PSTR *dnsName) {
    DWORD dwError = 0;
    PLWNET_DC_ADDRESS pDcList = NULL;
    DWORD dwDcCount = 0;
    INT count = 0;
    INT timeout = 5;
    BOOL isFound = FALSE;

    /* First, trying to find PDC */
    dwError = LWNetGetDCList(
                domain,
                NULL,
                (DS_DIRECTORY_SERVICE_REQUIRED | DS_WRITABLE_REQUIRED | DS_PDC_REQUIRED),
                &pDcList,
                &dwDcCount);

    ADT_BAIL_ON_ERROR_NP(dwError);

    if(
       !pDcList ||
       !pDcList[count].pszDomainControllerAddress ||
       LwLdapPingTcp((PCSTR) pDcList[0].pszDomainControllerAddress, timeout)
    ) {
        /* Trying to find backup DC */
        if (pDcList) {
            LWNetFreeDCList(pDcList, dwDcCount);
        }

        dwError = LWNetGetDCList(
                    domain,
                    NULL,
                    (DS_DIRECTORY_SERVICE_REQUIRED | DS_WRITABLE_REQUIRED),
                    &pDcList,
                    &dwDcCount);

        dwError = ADT_ERR_FAILED_TO_LOCATE_AD;
        ADT_BAIL_ON_ERROR_NP(dwError);

        while(
           pDcList &&
           pDcList[count].pszDomainControllerAddress
        ) {
            if(LwLdapPingTcp((PCSTR) pDcList[count].pszDomainControllerAddress, timeout)) {
                ++count;
            }
            else {
                break;
            }
        }

        if(!isFound) {
            dwError = ADT_ERR_FAILED_TO_LOCATE_AD;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }

    dwError = LwAllocateString((PCSTR) pDcList[count].pszDomainControllerAddress, ipAddress);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwAllocateString((PCSTR) pDcList[count].pszDomainControllerName, dnsName);
    ADT_BAIL_ON_ERROR_NP(dwError);

    cleanup:
        if (pDcList) {
            LWNetFreeDCList(pDcList, dwDcCount);
        }
        return dwError;

    error:
        goto cleanup;
}

/**
 * Process connection arguments.
 *
 * @param appContext Application context reference.
 * @param ipAddr AD server IP address.
 * @param port AD server TCP/IP port.
 * @param domain Domain name.
 * @return 0 on success; error code on failure.
 */
DWORD
ProcessConnectArgs(IN AppContextTP appContext, IN PSTR ipAddr, IN INT port, IN PSTR domain)
{
    DWORD dwError = 0;

    if (ipAddr) {
        dwError = LwStrDupOrNull((PCSTR) ipAddr,
                                 &(appContext->workConn->serverAddress));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (domain) {
        dwError = LwStrDupOrNull((PCSTR) domain,
                                 &(appContext->workConn->domainName));
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (!port) {
        appContext->workConn->port = 389;
    }
    else {
        appContext->workConn->port = port;
    }

    if(!appContext->workConn->serverAddress) {

        if(!appContext->workConn->domainName) {
            dwError = FindJoinedDomain(&(appContext->workConn->domainName));
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

        PrintStderr(appContext, LogLevelTrace,
                    "%s: Will be using AD server from domain: %s\n",
                    appContext->actionName, appContext->workConn->domainName);

        dwError = FindAdServer(appContext->workConn->domainName,
                               &(appContext->workConn->serverAddress),
                               &(appContext->workConn->serverName));
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace,
                "%s: Will be connecting to AD server: %s:%d\n",
                appContext->actionName, appContext->workConn->serverAddress,
                appContext->workConn->port);

    cleanup:
        return dwError;

    error:
        goto cleanup;
}

/**
 * Process authentication arguments.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD ProcessAuthArgs(IN AppContextTP appContext) {
    DWORD dwError = 0;

    if(appContext->aopts.logonAs) {
        dwError = ProcessPassword(&(appContext->aopts));
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    cleanup:
        return dwError;

    error:
        goto cleanup;
}
