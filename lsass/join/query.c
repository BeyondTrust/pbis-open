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
 *        query.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Domain Information Query API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
LsaSrvJoinFindComputerDN(
    HANDLE hDirectory,
    PCSTR  pszHostName,
    PCSTR  pszDomain,
    PSTR*  ppszComputerDN
    );

DWORD
LsaNetGetShortDomainName(
    PCSTR pszDomainFQDN,
    PSTR* ppszShortDomainName
    )
{
    DWORD dwError = 0;
    PCSTR pszDomainFQDNLocal = NULL;
    PSTR  pszDefaultDomainFQDN = NULL;
    PSTR  pszShortDomainName = NULL;
    PLWNET_DC_INFO pDCInfo = NULL;
    PSTR  pszSiteName = NULL;
    DWORD dwFlags = 0;
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszDomainFQDN))
    {
        dwError = LWNetGetCurrentDomain(&pszDefaultDomainFQDN);
        if (dwError != 0) {
            dwError = LW_ERROR_NOT_JOINED_TO_AD;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        pszDomainFQDNLocal = pszDefaultDomainFQDN;
    }
    else
    {
        pszDomainFQDNLocal = pszDomainFQDN;
    }

    dwError = LWNetGetDCName(
                NULL,
                pszDomainFQDNLocal,
                pszSiteName,
                dwFlags,
                &pDCInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LW_IS_NULL_OR_EMPTY_STR(pDCInfo->pszNetBIOSDomainName))
    {
        dwError = LW_ERROR_NO_NETBIOS_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LwAllocateString(
                pDCInfo->pszNetBIOSDomainName,
                &pszShortDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszShortDomainName = pszShortDomainName;
    
cleanup:

    if (pszDefaultDomainFQDN)
    {
        LWNetFreeString(pszDefaultDomainFQDN);
    }
    
    LWNET_SAFE_FREE_DC_INFO(pDCInfo);

    return dwError;
    
error:

    *ppszShortDomainName = NULL;
    
    LW_SAFE_FREE_STRING(pszShortDomainName);

    goto cleanup;
}

DWORD
LsaNetGetRwDCName(
	PCSTR pszDomainFQDN,
	PSTR* ppszDCName
	)
{
    DWORD dwError = 0;
    PCSTR pszDomainFQDNLocal = NULL;
    PSTR  pszDefaultDomainFQDN = NULL;
    PSTR  pszDCName = NULL;
    PLWNET_DC_INFO pDcInfo = NULL;
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszDomainFQDN))
    {
        dwError = LWNetGetCurrentDomain(&pszDefaultDomainFQDN);
        if (dwError)
        {
        	dwError = LW_ERROR_NOT_JOINED_TO_AD;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        pszDomainFQDNLocal = pszDefaultDomainFQDN;
    }
    else
    {
        pszDomainFQDNLocal = pszDomainFQDN;
    }

    dwError = LWNetGetDCName(NULL,
                             pszDomainFQDN,
                             NULL,
                             DS_DIRECTORY_SERVICE_REQUIRED | DS_WRITABLE_REQUIRED,
                             &pDcInfo);
    if (dwError != 0) {
        dwError = LW_ERROR_FAILED_TO_LOOKUP_DC;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwAllocateString(pDcInfo->pszDomainControllerName, &pszDCName);
    BAIL_ON_LSA_ERROR(dwError);

error:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    if (pszDefaultDomainFQDN)
    {
    	LWNetFreeString(pszDefaultDomainFQDN);
    }

    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszDCName);
    }

    *ppszDCName = pszDCName;

    return dwError;
}

DWORD
LsaGetDnsDomainName(
    PSTR* ppszDnsDomainName
    )
{
    DWORD dwError = 0;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PLSA_MACHINE_ACCT_INFO pAcct = NULL;
    PSTR pszHostname = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszDomain = NULL;

    dwError = LWNetGetCurrentDomain(&pszDomain);
    if (dwError != 0) {
        dwError = LW_ERROR_NOT_JOINED_TO_AD;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsOpenPasswordStore(
                LWPS_PASSWORD_STORE_DEFAULT,
                &hStore);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsGetPasswordByHostName(
                hStore,
                pszHostname,
                &pPassInfo);
    if (dwError != LWPS_ERROR_INVALID_ACCOUNT) {
        BAIL_ON_LSA_ERROR(dwError);
    } else {
        dwError = LW_ERROR_NOT_JOINED_TO_AD;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaBuildMachineAccountInfo(
                pPassInfo,
                &pAcct);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pAcct->pszDnsDomainName)) {
        dwError = LwAllocateString(
                        pAcct->pszDnsDomainName,
                        &pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppszDnsDomainName = pszDnsDomainName;
    
cleanup:

    if (pPassInfo) {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }
    
    if (pAcct) {
        LsaFreeMachineAccountInfo(pAcct);
    }
    
    LW_SAFE_FREE_STRING(pszHostname);
    
    if (pszDomain)
    {
    	LWNetFreeString(pszDomain);
    }
    
    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }

    return dwError;
    
error:

    *ppszDnsDomainName = NULL;
    
    LW_SAFE_FREE_STRING(pszDnsDomainName);

    goto cleanup;
}

DWORD
LsaGetComputerDN(
    PSTR* ppszComputerDN
    )
{
    DWORD dwError = 0;
    HANDLE hStore = (HANDLE)NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PLSA_MACHINE_ACCT_INFO pAcct = NULL;
    PSTR pszHostname = NULL;
    PSTR pszDomain = NULL;
    PSTR pszRootDN = NULL;
    PSTR pszComputerDN = NULL;
    PSTR pszUsername = NULL;
    PSTR pszKrb5CachePath = NULL;
    
    if (geteuid() != 0) {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LWNetGetCurrentDomain(&pszDomain);
    if (dwError != 0) {
        dwError = LW_ERROR_NOT_JOINED_TO_AD;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSyncTimeToDC(pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsOpenPasswordStore(
                    LWPS_PASSWORD_STORE_DEFAULT,
                    &hStore);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LwpsGetPasswordByHostName(
                    hStore,
                    pszHostname,
                    &pPassInfo);
    if (dwError != LWPS_ERROR_INVALID_ACCOUNT) {
        BAIL_ON_LSA_ERROR(dwError);
    } else {
        dwError = LW_ERROR_NOT_JOINED_TO_AD;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaBuildMachineAccountInfo(
                    pPassInfo,
                    &pAcct);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwAllocateStringPrintf(
                &pszUsername,
                "%s@%s",
                pAcct->pszMachineAccount,
                pAcct->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwKrb5GetUserCachePath(
                geteuid(),
                KRB5_InMemory_Cache,
                &pszKrb5CachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwKrb5GetTgt(
                pszUsername,
                pAcct->pszMachinePassword,
                pszKrb5CachePath,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_DEBUG("Switching default credentials path to get computer DN");
    dwError = LwKrb5SetDefaultCachePath(pszKrb5CachePath, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapOpenDirectoryDomain(pszDomain, NULL, 0, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LwLdapConvertDomainToDN(
                        pszDomain,
                        &pszRootDN);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaSrvJoinFindComputerDN(
                        hDirectory,
                        pAcct->pszMachineAccount,
                        pszDomain,
                        &pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppszComputerDN = pszComputerDN;
    
cleanup:

    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszRootDN);
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_FREE_STRING(pszKrb5CachePath);
    
    if (pszDomain)
    {
    	LWNetFreeString(pszDomain);
    }

    if (pPassInfo) {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }
    
    if (pAcct) {
        LsaFreeMachineAccountInfo(pAcct);
    }
    
    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }
    
    if (hDirectory != (HANDLE)NULL) {
        LwLdapCloseDirectory(hDirectory);
    }

    return dwError;
    
error:

    *ppszComputerDN = NULL;
    
    LW_SAFE_FREE_STRING(pszComputerDN);

    goto cleanup;
}

static
DWORD
LsaSrvJoinFindComputerDN(
    HANDLE hDirectory,
    PCSTR  pszHostName,
    PCSTR  pszDomain,
    PSTR*  ppszComputerDN
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeList[] = {"*", NULL};
    CHAR szQuery[1024];
    LDAPMessage *pMessage = NULL;
    int count = 0;
    PSTR pszComputerDN = NULL;
    PSTR pszEscapedUpperHostName = NULL;

    pLd = LwLdapGetSession(hDirectory);

    dwError = LwLdapConvertDomainToDN(
                    pszDomain,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapEscapeString(
                    &pszEscapedUpperHostName,
                    pszHostName);
    BAIL_ON_LSA_ERROR(dwError);
    
    LwStrToUpper(pszEscapedUpperHostName);
    
    sprintf(szQuery, "(sAMAccountName=%s)", pszEscapedUpperHostName);

    dwError = LwLdapDirectorySearch(
                    hDirectory,
                    pszDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    szQuery,
                    szAttributeList,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    count = ldap_count_entries(
                pLd,
                pMessage);
    if (count < 0) {
        dwError = LW_ERROR_LDAP_ERROR;
    } else if (count == 0) {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
    } else if (count > 1) {
        dwError = LW_ERROR_DUPLICATE_DOMAINNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);   
    
    dwError = LwLdapGetDN(
                    hDirectory,
                    pMessage,
                    &pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszComputerDN))
    {
        dwError = LW_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);        
    }

    *ppszComputerDN = pszComputerDN;

cleanup:
    
    LW_SAFE_FREE_STRING(pszDirectoryRoot);

    if (pMessage) {
        ldap_msgfree(pMessage);
    }
    
    LW_SAFE_FREE_STRING(pszEscapedUpperHostName);

    return dwError;
    
error:

    *ppszComputerDN = NULL;
    LW_SAFE_FREE_STRING(pszComputerDN);

    goto cleanup;    
}
