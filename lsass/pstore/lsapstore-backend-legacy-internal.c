/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *  
 * Module Name:
 *
 *     lsapstore-backend-legacy-internal.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Legacy Backend Internals Implementation
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *          Arlene Berry (aberry@likewise.com)
 *          Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-backend-legacy.h"

#include <reg/reg.h>
#include <reg/regutil.h>
#include <lw/base.h>
#include <lwmem.h>
#include <lwstr.h>
// Work around bug in lwstr.h:
#include <wc16str.h>
#include "lsapstore-includes.h"

#define PSTOREDB_REGISTRY_AD_KEY \
    "Services\\lsass\\Parameters\\Providers\\ActiveDirectory\\DomainJoin"
#define PSTOREDB_REGISTRY_PSTORE_SUBKEY \
    "Pstore"
#define PSTOREDB_REGISTRY_MACHINE_PWD_SUBKEY \
    "MachinePassword"
#define PSTOREDB_REGISTRY_DEFAULT_VALUE \
    "Default"

#define LWPS_REG_HOSTNAME           "HostName"
#define LWPS_REG_DOMAIN_SID         "DomainSID"
#define LWPS_REG_DOMAIN_NAME        "DomainName"
#define LWPS_REG_DOMAIN_DNS_NAME    "DomainDnsName"
#define LWPS_REG_HOST_DNS_DOMAIN    "HostDnsDomain"
#define LWPS_REG_MACHINE_ACCOUNT    "MachineAccount"
#define LWPS_REG_MACHINE_PWD        "MachinePassword"
#define LWPS_REG_MODIFY_TIMESTAMP   "ClientModifyTimestamp"
#define LWPS_REG_CREATION_TIMESTAMP "CreationTimestamp"
#define LWPS_REG_SCHANNEL_TYPE      "SchannelType"

struct _LWPS_LEGACY_STATE {
    HANDLE hReg;
    PSECURITY_DESCRIPTOR_ABSOLUTE pPasswordSecurityDescriptor;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAccountSecurityDescriptor;
};

//
// Prototypes
//

// This is only needed internally as the higher layer
// needs to use the read password call to ensure that
// the default joined domain has data.
static
DWORD
LwpsLegacyGetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR* ppszDomainName
    );

static
DWORD
LwpsLegacyCreateSecurityDescriptor(
    IN BOOLEAN bIsWorldReadable,
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    );

static
VOID
LwpsLegacyFreeSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    );

//
// Functions
//

DWORD
LwpsLegacyOpenProvider(
    OUT PLWPS_LEGACY_STATE* ppContext
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLWPS_LEGACY_STATE pContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegOpenServer(&pContext->hReg);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyCreateSecurityDescriptor(
                    FALSE,
                    &pContext->pPasswordSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyCreateSecurityDescriptor(
                    TRUE,
                    &pContext->pAccountSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        if (pContext)
        {
            LwpsLegacyCloseProvider(pContext);
            pContext = NULL;
        }
    }

    *ppContext = pContext;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

VOID
LwpsLegacyCloseProvider(
    IN PLWPS_LEGACY_STATE pContext
    )
{
    if (pContext)
    {
        if (pContext->hReg)
        {
            RegCloseServer(pContext->hReg);
        }
        LwpsLegacyFreeSecurityDescriptor(&pContext->pPasswordSecurityDescriptor);
        LwpsLegacyFreeSecurityDescriptor(&pContext->pAccountSecurityDescriptor);
        LwFreeMemory(pContext);
    }
}

DWORD
LwpsLegacyReadPassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszQueryDomainName,
    OUT PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PSTR pszRegistryPath = NULL;
    PSTR pszDefaultDomain = NULL;
    PSTR pszDomainDnsName = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszDomainSID = NULL;
    PSTR pszHostDnsDomain = NULL;
    PSTR pszHostNameValue = NULL;
    PSTR pszMachineAccountName = NULL;
    PSTR pszMachineAccountPassword = NULL;
    DWORD dwClientModifyTimestamp = 0;
    DWORD dwSchannelType = 0;
    DWORD dwValueLen = 0;

    if (pszQueryDomainName)
    {
        dwError = LwAllocateStringPrintf(
                      &pszRegistryPath,
                      "%s\\%s\\%s",
                      PSTOREDB_REGISTRY_AD_KEY,
                      pszQueryDomainName,
                      PSTOREDB_REGISTRY_PSTORE_SUBKEY);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        dwError = RegUtilGetValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      PSTOREDB_REGISTRY_AD_KEY,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULT_VALUE,
                      NULL,
                      (PVOID) &pszDefaultDomain,
                      &dwValueLen);
        if (dwError)
        {
            /* This will fail when computer has never joined to a domain */
            dwError = NERR_SetupNotJoined;
        }
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = LwAllocateStringPrintf(
                      &pszRegistryPath,
                      "%s\\%s\\%s",
                      PSTOREDB_REGISTRY_AD_KEY,
                      pszDefaultDomain,
                      PSTOREDB_REGISTRY_PSTORE_SUBKEY);
        if (dwError)
        {
            /* This will fail when computer has never joined to a domain */
            dwError = NERR_SetupNotJoined;
        }
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = RegUtilIsValidKey(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath);
    if (dwError)
    {
        /* This will fail when computer has never joined to a domain */
        dwError = NERR_SetupNotJoined;
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwAllocateMemory(
                  sizeof(LWPS_PASSWORD_INFO),
                  (PVOID*)&pInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_DOMAIN_DNS_NAME,
                  NULL,
                  (PVOID) &pszDomainDnsName,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_DOMAIN_NAME,
                  NULL,
                  (PVOID) &pszDomainName,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    
    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_DOMAIN_SID,
                  NULL,
                  (PVOID) &pszDomainSID,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_HOST_DNS_DOMAIN,
                  NULL,
                  (PVOID) &pszHostDnsDomain,
                  NULL);
    if (dwError != LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_HOSTNAME,
                  NULL,
                  (PVOID) &pszHostNameValue,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_MACHINE_ACCOUNT,
                  NULL,
                  (PVOID) &pszMachineAccountName,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  PSTOREDB_REGISTRY_MACHINE_PWD_SUBKEY,
                  LWPS_REG_MACHINE_PWD,
                  NULL,
                  (PVOID) &pszMachineAccountPassword,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_MODIFY_TIMESTAMP,
                  NULL,
                  (PVOID) &dwClientModifyTimestamp,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_SCHANNEL_TYPE,
                  NULL,
                  (PVOID) &dwSchannelType,
                  NULL);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwMbsToWc16s(pszDomainDnsName, &pInfo->pwszDnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToUpper(pInfo->pwszDnsDomainName);

    dwError = LwMbsToWc16s(pszDomainName, &pInfo->pwszDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToUpper(pInfo->pwszDomainName);

    dwError = LwMbsToWc16s(pszDomainSID, &pInfo->pwszSID);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (LW_IS_NULL_OR_EMPTY_STR(pszHostDnsDomain))
    {
        dwError = LwMbsToWc16s(pszDomainDnsName, &pInfo->pwszHostDnsDomain);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        dwError = LwMbsToWc16s(pszHostDnsDomain, &pInfo->pwszHostDnsDomain);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwMbsToWc16s(pszHostNameValue, &pInfo->pwszHostname);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToUpper(pInfo->pwszHostname);

    dwError = LwMbsToWc16s(pszMachineAccountName,
                             &pInfo->pwszMachineAccount);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwWc16sToUpper(pInfo->pwszMachineAccount);

    dwError = LwMbsToWc16s(pszMachineAccountPassword,
                             &pInfo->pwszMachinePassword);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    pInfo->last_change_time = dwClientModifyTimestamp;
    pInfo->dwSchannelType = dwSchannelType;

cleanup:
    if (dwError)
    {
        if (pInfo)
        {
           LwpsLegacyFreePassword(pInfo);
           pInfo = NULL;
        }
    }

    LW_SAFE_FREE_MEMORY(pszRegistryPath);
    LW_SAFE_FREE_MEMORY(pszDefaultDomain);
    LW_SAFE_FREE_MEMORY(pszDomainDnsName);
    LW_SAFE_FREE_MEMORY(pszDomainName);
    LW_SAFE_FREE_MEMORY(pszDomainSID);
    LW_SAFE_FREE_MEMORY(pszHostDnsDomain);
    LW_SAFE_FREE_MEMORY(pszHostNameValue);
    LW_SAFE_FREE_MEMORY(pszMachineAccountName);
    LW_SECURE_FREE_STRING(pszMachineAccountPassword);
    LW_SAFE_FREE_MEMORY(pszMachineAccountPassword);

    *ppInfo = pInfo;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyWritePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PLWPS_PASSWORD_INFO pInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszRegistryPath = NULL;
    PSTR pszDefaultDomain = NULL;
    PSTR pszDomainSID = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszHostname = NULL;
    PSTR pszHostDnsDomain = NULL;
    PSTR pszMachineAccount = NULL;
    PSTR pszMachinePassword = NULL;
    DWORD dwClientModifyTimestamp = 0;
    time_t tCreationTimestamp = 0;
    DWORD dwSchannelType = 0;

    /* This is all the values to be written to the registry. */
    dwError = LwWc16sToMbs(pInfo->pwszSID, &pszDomainSID);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwWc16sToMbs(pInfo->pwszDomainName, &pszDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwStrToUpper(pszDomainName);

    dwError = LwWc16sToMbs(pInfo->pwszDnsDomainName, &pszDnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwStrToUpper(pszDnsDomainName);

    dwError = LwWc16sToMbs(pInfo->pwszHostname, &pszHostname);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwStrToUpper(pszHostname);

    if (pInfo->pwszHostDnsDomain)
    {
        dwError = LwWc16sToMbs(pInfo->pwszHostDnsDomain, &pszHostDnsDomain);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        dwError = LwWc16sToMbs(pInfo->pwszDnsDomainName, &pszHostDnsDomain);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwWc16sToMbs(pInfo->pwszMachineAccount, &pszMachineAccount);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LwStrToUpper(pszMachineAccount);

    dwError = LwWc16sToMbs(pInfo->pwszMachinePassword, &pszMachinePassword);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    time(&tCreationTimestamp);

    if ((pInfo->last_change_time < 0) ||
        (pInfo->last_change_time > MAXDWORD))
    {
        // This backend cannot currently deal with this.
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwClientModifyTimestamp = pInfo->last_change_time;
    dwSchannelType = pInfo->dwSchannelType;

    dwError = LwAllocateStringPrintf(
                  &pszRegistryPath,
                  "%s\\%s\\%s",
                  PSTOREDB_REGISTRY_AD_KEY,
                  pszDnsDomainName,
                  PSTOREDB_REGISTRY_PSTORE_SUBKEY);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    /* Add top-level pstore registry key */

    dwError = RegUtilAddKeySecDesc(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  KEY_ALL_ACCESS,
                  pContext->pAccountSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    /* 
     * Add "MachinePassword" registry key, needed to apply ACL restrictions
     * to restrict read access to value names (the machine password)
     * stored under this key.
     */
    dwError = RegUtilAddKeySecDesc(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  PSTOREDB_REGISTRY_MACHINE_PWD_SUBKEY,
                  KEY_ALL_ACCESS,
                  pContext->pPasswordSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    /* Write the data to the registry */
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_DOMAIN_SID,
                  REG_SZ,
                  pszDomainSID,
                  strlen(pszDomainSID));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_DOMAIN_NAME,
                  REG_SZ,
                  pszDomainName,
                  strlen(pszDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_DOMAIN_DNS_NAME,
                  REG_SZ,
                  pszDnsDomainName,
                  strlen(pszDnsDomainName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_HOSTNAME,
                  REG_SZ,
                  pszHostname,
                  strlen(pszHostname));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_HOST_DNS_DOMAIN,
                  REG_SZ,
                  pszHostDnsDomain,
                  strlen(pszHostDnsDomain));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_MACHINE_ACCOUNT,
                  REG_SZ,
                  pszMachineAccount,
                  strlen(pszMachineAccount));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  PSTOREDB_REGISTRY_MACHINE_PWD_SUBKEY,
                  LWPS_REG_MACHINE_PWD,
                  REG_SZ,
                  pszMachinePassword,
                  strlen(pszMachinePassword));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_MODIFY_TIMESTAMP,
                  REG_DWORD,
                  &dwClientModifyTimestamp,
                  sizeof(dwClientModifyTimestamp));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_CREATION_TIMESTAMP,
                  REG_DWORD,
                  &tCreationTimestamp,
                  sizeof(dwClientModifyTimestamp));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  LWPS_REG_SCHANNEL_TYPE,
                  REG_DWORD,
                  &dwSchannelType,
                  sizeof(dwSchannelType));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyGetDefaultJoinedDomain(
                    pContext,
                    &pszDefaultDomain);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = LwpsLegacySetDefaultJoinedDomain(
                        pContext,
                        pszDnsDomainName);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    LW_SAFE_FREE_MEMORY(pszRegistryPath);
    LW_SAFE_FREE_MEMORY(pszDefaultDomain);
    LW_SAFE_FREE_MEMORY(pszDomainSID);
    LW_SAFE_FREE_MEMORY(pszDomainName);
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pszHostname);
    LW_SAFE_FREE_MEMORY(pszHostDnsDomain);
    LW_SAFE_FREE_MEMORY(pszMachineAccount);
    LW_SECURE_FREE_STRING(pszMachinePassword);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyDeletePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszRegistryPath = NULL;
    PSTR pszDefaultDomain = NULL;
    DWORD dwSubKeysCount = 0;
    DWORD dwValuesCount = 0;

    LwpsLegacyGetDefaultJoinedDomain(pContext, &pszDefaultDomain);

    if (!pszDomainName && !pszDefaultDomain)
    {
        GOTO_CLEANUP_EE(EE);
    }

    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                      &pszRegistryPath,
                      "%s\\%s",
                      PSTOREDB_REGISTRY_AD_KEY,
                      pszDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                      &pszRegistryPath,
                      "%s\\%s",
                      PSTOREDB_REGISTRY_AD_KEY,
                      pszDefaultDomain);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    if (!pszDomainName || !pszDefaultDomain ||
        strcmp(pszDomainName, pszDefaultDomain) == 0)
    {
        LwpsLegacySetDefaultJoinedDomain(
                pContext,
                NULL);
        // TODO-What if there is an error?
    }

    RegUtilDeleteTree(
        pContext->hReg,
        NULL,
        pszRegistryPath,
        PSTOREDB_REGISTRY_PSTORE_SUBKEY);
    // TODO-What if there is an error?

    /* Delete domain key only if empty */
    dwError = RegUtilGetKeyObjectCounts(
                  pContext->hReg,
                  HKEY_THIS_MACHINE,
                  pszRegistryPath,
                  NULL,
                  &dwSubKeysCount,
                  &dwValuesCount);
    if (dwError)
    {
        dwError = 0;
    }
    else if (!dwSubKeysCount && !dwValuesCount)
    {
        RegUtilDeleteKey(
            pContext->hReg,
            HKEY_THIS_MACHINE,
            pszRegistryPath,
            NULL);
        // TODO-What if there is an error?
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pszRegistryPath);
    LW_SAFE_FREE_MEMORY(pszDefaultDomain);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
LwpsLegacyGetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR* ppszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszDomainName = NULL;
    DWORD dwValueLen = 0;

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  HKEY_THIS_MACHINE,
                  PSTOREDB_REGISTRY_AD_KEY,
                  NULL,
                  PSTOREDB_REGISTRY_DEFAULT_VALUE,
                  NULL,
                  (PVOID) &pszDomainName,
                  &dwValueLen);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        // TODO-INCONSISTENT-FREE-FUNCTION!!!
        LW_SAFE_FREE_MEMORY(pszDomainName);
    }

    *ppszDomainName = pszDomainName;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}


DWORD
LwpsLegacySetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;

    if (pszDomainName)
    {
        // TODO-Verify that actually joined to specified domain.

        dwError = RegUtilAddKey(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      PSTOREDB_REGISTRY_AD_KEY,
                      NULL);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = RegUtilSetValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      PSTOREDB_REGISTRY_AD_KEY,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULT_VALUE,
                      REG_SZ,
                      (PVOID)pszDomainName,
                      pszDomainName ? strlen(pszDomainName) : 0);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        dwError = RegUtilDeleteValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      PSTOREDB_REGISTRY_AD_KEY,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULT_VALUE);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyGetJoinedDomains(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR** pppszDomainList,
    OUT PDWORD pdwDomainCount
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR* ppwszSubKeys = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszSubKeyPtr = NULL;
    PSTR* ppszDomainList = NULL;
    DWORD dwSubKeysLen = 0;
    DWORD dwIndexKeys = 0;
    DWORD dwIndexDomains = 0;
    PLWPS_PASSWORD_INFO pPasswordInfo = NULL;
    DWORD dwDomainCount = 0;

    dwError = RegUtilIsValidKey(
                    pContext->hReg,
                    HKEY_THIS_MACHINE,
                    PSTOREDB_REGISTRY_AD_KEY);
    if (dwError)
    {
        dwError = 0;
        GOTO_CLEANUP_EE(EE);
    }

    dwError = RegUtilGetKeys(
                    pContext->hReg,
                    HKEY_THIS_MACHINE,
                    PSTOREDB_REGISTRY_AD_KEY,
                    NULL,
                    &ppwszSubKeys,
                    &dwSubKeysLen);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (dwSubKeysLen)
    {
        dwError = LwAllocateMemory(
                        sizeof(*ppszDomainList) * dwSubKeysLen,
                        (PVOID*)&ppszDomainList);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        for (dwIndexKeys = 0, dwIndexDomains = 0 ; dwIndexKeys < dwSubKeysLen; dwIndexKeys++)
        {
            dwError = LwWc16sToMbs(ppwszSubKeys[dwIndexKeys], &pszSubKey);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

            pszSubKeyPtr = strrchr(pszSubKey, '\\');
            if (pszSubKeyPtr)
            {
                dwError = LwAllocateString(
                              ++pszSubKeyPtr,
                              &ppszDomainList[dwIndexDomains]);
                GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

                LW_SAFE_FREE_STRING(pszSubKey);
            }
            else
            {
                ppszDomainList[dwIndexDomains] = pszSubKey;
                pszSubKey = NULL;
            }

            if (pPasswordInfo)
            {
                LwpsLegacyFreePassword(pPasswordInfo);
                pPasswordInfo = NULL;
            }

            dwError = LwpsLegacyReadPassword(
                            pContext,
                            ppszDomainList[dwIndexDomains],
                            &pPasswordInfo);
            if (dwError == NERR_SetupNotJoined)
            {
                // The domain is not joined.
                dwError = 0;
                LW_SAFE_FREE_STRING(ppszDomainList[dwIndexDomains]);
            }
            else
            {
                dwIndexDomains++;
            }
        }
    }

    dwDomainCount = dwIndexDomains;

cleanup:
    if (dwError)
    {
        if (ppszDomainList)
        {
            LwpsLegacyFreeStringArray(ppszDomainList, dwSubKeysLen);
            ppszDomainList = NULL;
        }
        dwDomainCount = 0;
    }

    if (pPasswordInfo)
    {
        LwpsLegacyFreePassword(pPasswordInfo);
    }

    for (dwIndexKeys = 0; dwIndexKeys < dwSubKeysLen; dwIndexKeys++)
    {
        LW_SAFE_FREE_MEMORY(ppwszSubKeys[dwIndexKeys]);
    }
    LW_SAFE_FREE_MEMORY(ppwszSubKeys);

    LW_SAFE_FREE_STRING(pszSubKey);

    *pppszDomainList = ppszDomainList;
    *pdwDomainCount = dwDomainCount;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

VOID
LwpsLegacyFreePassword(
    IN PLWPS_PASSWORD_INFO pInfo
    )
{
    if (pInfo)
    {
        LW_SAFE_FREE_MEMORY(pInfo->pwszDomainName);
        LW_SAFE_FREE_MEMORY(pInfo->pwszDnsDomainName);
        LW_SAFE_FREE_MEMORY(pInfo->pwszSID);
        LW_SAFE_FREE_MEMORY(pInfo->pwszHostname);
        LW_SAFE_FREE_MEMORY(pInfo->pwszHostDnsDomain);
        LW_SAFE_FREE_MEMORY(pInfo->pwszMachineAccount);
        LW_SECURE_FREE_WSTRING(pInfo->pwszMachinePassword);
        LwFreeMemory(pInfo);
    }
}

VOID
LwpsLegacyFreeStringArray(
    IN PSTR* ppszDomainList,
    IN DWORD dwCount
    )
{
    if (ppszDomainList)
    {
        LwFreeStringArray(ppszDomainList, dwCount);
    }
}

static
DWORD
LwpsLegacyBuildRestrictedDaclForKey(
    IN PSID pAllAccessSid,
    IN OPTIONAL PSID pReadAccessSid,
    OUT PACL *ppDacl
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    DWORD dwSidCount = 0;
    DWORD dwTotalSidSize = 0;
    DWORD dwDaclSize = 0;
    PACL pDacl = NULL;
    DWORD dwError = 0;

    dwSidCount++;
    dwTotalSidSize = RtlLengthSid(pAllAccessSid);

    if (pReadAccessSid)
    {
        dwSidCount++;
        dwTotalSidSize += RtlLengthSid(pReadAccessSid);
    }

    dwDaclSize = ACL_HEADER_SIZE +
        dwSidCount * LW_FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
        dwTotalSidSize;

    status = LW_RTL_ALLOCATE(&pDacl, VOID, dwDaclSize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlAddAccessAllowedAceEx(pDacl,
                                      ACL_REVISION,
                                      0,
                                      KEY_ALL_ACCESS,
                                      pAllAccessSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (pReadAccessSid)
    {
        status = RtlAddAccessAllowedAceEx(pDacl,
                                          ACL_REVISION,
                                          0,
                                          KEY_READ,
                                          pReadAccessSid);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        LW_RTL_FREE(&pDacl);
    }

    *ppDacl = pDacl;

    dwError = LwNtStatusToWin32Error(status);
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
LwpsLegacyCreateSecurityDescriptor(
    IN BOOLEAN bIsWorldReadable,
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PACL pDacl = NULL;
    PSID pRootSid = NULL;
    PSID pAdminsSid = NULL;
    PSID pEveryoneSid = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    DWORD dwError = 0;

    status = RtlAllocateWellKnownSid(WinLocalSystemSid, NULL, &pRootSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlAllocateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &pAdminsSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (bIsWorldReadable)
    {
        status = RtlAllocateWellKnownSid(WinWorldSid, NULL, &pEveryoneSid);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = LW_RTL_ALLOCATE(
                    &pSecDesc,
                    VOID,
                    SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCreateSecurityDescriptorAbsolute(
                    pSecDesc,
                    SECURITY_DESCRIPTOR_REVISION);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Owner: Root

    status = RtlDuplicateSid(&pOwnerSid, pRootSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlSetOwnerSecurityDescriptor(
                    pSecDesc,
                    pOwnerSid,
                    FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    pOwnerSid = NULL;

    // Group: Administrators

    status = RtlDuplicateSid(&pGroupSid, pAdminsSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlSetGroupSecurityDescriptor(
                    pSecDesc,
                    pGroupSid,
                    FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    pGroupSid = NULL;

    // Do not set Sacl currently

    // DACL
    status = LwpsLegacyBuildRestrictedDaclForKey(
                    pRootSid,
                    pEveryoneSid,
                    &pDacl);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlSetDaclSecurityDescriptor(
                    pSecDesc,
                    TRUE,
                    pDacl,
                    FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    pDacl = NULL;

    if (!RtlValidSecurityDescriptor(pSecDesc))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        if (pSecDesc)
        {
            LwpsLegacyFreeSecurityDescriptor(&pSecDesc);
        }
    }

    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pRootSid);
    LW_RTL_FREE(&pAdminsSid);
    LW_RTL_FREE(&pEveryoneSid);
    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);

    *ppSecurityDescriptor = pSecDesc;

    dwError = LwNtStatusToWin32Error(status);
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
VOID
LwpsLegacyFreeSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    )
{
    if (*ppSecurityDescriptor)
    {
        PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = *ppSecurityDescriptor;
        PSID pOwner = NULL;
        PSID pGroup = NULL;
        PACL pDacl = NULL;
        PACL pSacl = NULL;
        BOOLEAN bDefaulted = FALSE;
        BOOLEAN bPresent = FALSE;

        RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
        RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);
        RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
        RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

        LW_RTL_FREE(&pSecDesc);
        LW_RTL_FREE(&pOwner);
        LW_RTL_FREE(&pGroup);
        LW_RTL_FREE(&pDacl);
        LW_RTL_FREE(&pSacl);

        *ppSecurityDescriptor = NULL;
    }
}
