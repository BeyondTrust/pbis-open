/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
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
 *  Copyright (C) Likewise Software. All rights reserved.
 *  
 *  Module Name:
 *
 *     provider-main.c
 *
 *  Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        API to support Registry Password Storage
 *
 *  Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 *           Adam Bernstein (abernstein@likewise.com)
 */

#include "includes.h"

DWORD
LWPS_INITIALIZE_PROVIDER(regdb)(
    PCSTR pszConfigFilePath,
    PSTR* ppszName,
    PLWPS_PROVIDER_FUNC_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    *ppszName = (PSTR) gpszRegDBProviderName;
    *ppFnTable = &gRegDBProviderAPITable;

    dwError = LwNtStatusToWin32Error(LwMapSecurityInitialize());
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                  LwMapSecurityCreateContext(&gpRegLwMapSecurityCtx));
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:
    return dwError;

error:
    *ppszName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
}


DWORD
RegDB_OpenProvider(
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PREGDB_PROVIDER_CONTEXT pContext = NULL;
    HANDLE hReg = NULL;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(phProvider);

    dwError = LwpsAllocateMemory(
                  sizeof(REGDB_PROVIDER_CONTEXT),
                  (PVOID*)&pContext);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = RegOpenServer(&hReg);
    BAIL_ON_LWPS_ERROR(dwError);

    pContext->hReg = hReg;

    dwError = LwpsCreateRWLock(
                  LWPS_CONFIG_DIR "/" LWPS_LOCK_FILE,
                  &pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    *phProvider = (HANDLE)pContext;

cleanup:
    return dwError;

error:
    *phProvider = (HANDLE)NULL;

    if (pContext)
    {
        LwpsFreeProviderContext(pContext);
    }

    goto cleanup;
}


static
DWORD
RegDB_ReadPassword(
    HANDLE hProvider,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PREGDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;
    PSTR pszDomainDnsName = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszDomainSID = NULL;
    PSTR pszHostDnsDomain = NULL;
    PSTR pszHostNameValue = NULL;
    PSTR pszMachineAccountName = NULL;
    PSTR pszMachineAccountPassword = NULL;
    DWORD dwClientModifyTimestamp = 0;
    DWORD dwSchannelType = 0;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(ppInfo);

    pContext = (PREGDB_PROVIDER_CONTEXT)hProvider;
    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireReadLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);
    bUnlock = TRUE;

    dwError = RegUtilIsValidKey(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY);
    if (dwError)
    {
        /* This will fail when computer has never joined to a domain */
        dwError = LWPS_ERROR_INVALID_ACCOUNT;
        goto cleanup;
    }

    dwError = LwpsAllocateMemory(
                  sizeof(LWPS_PASSWORD_INFO),
                  (PVOID*)&pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_DOMAIN_DNS_NAME,
                  NULL,
                  (PVOID) &pszDomainDnsName,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_DOMAIN_NAME,
                  NULL,
                  (PVOID) &pszDomainName,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_DOMAIN_SID,
                  NULL,
                  (PVOID) &pszDomainSID,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_HOST_DNS_DOMAIN,
                  NULL,
                  (PVOID) &pszHostDnsDomain,
                  NULL);
    if (dwError != LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_HOSTNAME,
                  NULL,
                  (PVOID) &pszHostNameValue,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_MACHINE_ACCOUNT,
                  NULL,
                  (PVOID) &pszMachineAccountName,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_MACHINE_PWD_KEY,
                  NULL,
                  LWPS_REG_MACHINE_PWD,
                  NULL,
                  (PVOID) &pszMachineAccountPassword,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_MODIFY_TIMESTAMP,
                  NULL,
                  (PVOID) &dwClientModifyTimestamp,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

#if 0
    /* Not used */
    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_CREATION_TIMESTAMP,
                  NULL,
                  (PVOID) &dwCreationTimestamp,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);
#endif

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_SCHANNEL_TYPE,
                  NULL,
                  (PVOID) &dwSchannelType,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);


    dwError = LwpsMbsToWc16s(pszDomainDnsName, &pInfo->pwszDnsDomainName);
    BAIL_ON_LWPS_ERROR(dwError);
    wc16supper(pInfo->pwszDnsDomainName);

    dwError = LwpsMbsToWc16s(pszDomainName, &pInfo->pwszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);
    wc16supper(pInfo->pwszDomainName);

    dwError = LwpsMbsToWc16s(pszDomainSID, &pInfo->pwszSID);
    BAIL_ON_LWPS_ERROR(dwError);

    if (IsNullOrEmptyString(pszHostDnsDomain))
    {
        dwError = LwpsMbsToWc16s(pszDomainDnsName, &pInfo->pwszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    else
    {
        dwError = LwpsMbsToWc16s(pszHostDnsDomain, &pInfo->pwszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = LwpsMbsToWc16s(pszHostNameValue, &pInfo->pwszHostname);
    BAIL_ON_LWPS_ERROR(dwError);
    wc16supper(pInfo->pwszHostname);

    dwError = LwpsMbsToWc16s(pszMachineAccountName,
                             &pInfo->pwszMachineAccount);
    BAIL_ON_LWPS_ERROR(dwError);
    wc16supper(pInfo->pwszMachineAccount);

    dwError = LwpsMbsToWc16s(pszMachineAccountPassword,
                             &pInfo->pwszMachinePassword);
    BAIL_ON_LWPS_ERROR(dwError);

    pInfo->last_change_time = dwClientModifyTimestamp;
    pInfo->dwSchannelType = dwSchannelType;

    *ppInfo = pInfo;
                
cleanup:
    LWPS_SAFE_FREE_MEMORY(pszDomainDnsName);
    LWPS_SAFE_FREE_MEMORY(pszDomainName);
    LWPS_SAFE_FREE_MEMORY(pszDomainSID);
    LWPS_SAFE_FREE_MEMORY(pszHostDnsDomain);
    LWPS_SAFE_FREE_MEMORY(pszHostNameValue);
    LWPS_SAFE_FREE_MEMORY(pszMachineAccountName);
    LWPS_SECURE_FREE_STRING(pszMachineAccountPassword);
    if (bUnlock)
    {
       LwpsReleaseReadLock(pContext->hRWLock);
    }

    return dwError;

error:
    *ppInfo = NULL;

    if (pInfo)
    {
       RegDB_FreePassword(pInfo);
    }

    goto cleanup;
}


DWORD
RegDB_ReadPasswordByHostName(
    HANDLE hProvider,
    PCSTR  pszHostname,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PWSTR pwszHostname = NULL;

    dwError = RegDB_ReadPassword(
                  hProvider,
                  &pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pszHostname,
                  &pwszHostname);
    BAIL_ON_LWPS_ERROR(dwError);

    wc16supper(pwszHostname);

    if (wc16scmp(pwszHostname, pInfo->pwszHostname))
    {
        dwError = LWPS_ERROR_INVALID_ACCOUNT;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    *ppInfo = pInfo;
                
cleanup:
    LWPS_SAFE_FREE_MEMORY(pwszHostname);

    return dwError;

error:
    *ppInfo = NULL;

    if (pInfo)
    {
        RegDB_FreePassword(pInfo);
    }

    goto cleanup;
}


DWORD
RegDB_ReadPasswordByDomainName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PWSTR pwszDomainName = NULL;

    dwError = RegDB_ReadPassword(
                  hProvider,
                  &pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pszDomainName,
                  &pwszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    wc16supper(pwszDomainName);

    if (wc16scmp(pwszDomainName, pInfo->pwszDomainName) &&
        wc16scmp(pwszDomainName, pInfo->pwszDnsDomainName))
    {
        dwError = LWPS_ERROR_INVALID_ACCOUNT;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    *ppInfo = pInfo;
                
cleanup:
    LWPS_SAFE_FREE_MEMORY(pwszDomainName);

    return dwError;

error:
    *ppInfo = NULL;

    if (pInfo)
    {
        RegDB_FreePassword(pInfo);
    }

    goto cleanup;
}


DWORD
RegDB_ReadHostListByDomainName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PSTR **pppszHostnames,
    DWORD *pdwHostnames
    )
{
    DWORD dwError = 0;
    PSTR *ppszHostNames = NULL;
    DWORD dwNumEntries = 1;
    DWORD iEntry = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PWSTR pwszDomainName = NULL;

    BAIL_ON_INVALID_POINTER(pppszHostnames);
    BAIL_ON_INVALID_POINTER(pdwHostnames);
    
    dwError = RegDB_ReadPassword(
                  hProvider,
                  &pInfo);
    if (dwError == LWPS_ERROR_INVALID_ACCOUNT)
    {
        dwError = 0;
        *pppszHostnames = NULL;
        *pdwHostnames = 0;
        goto cleanup;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pszDomainName,
                  &pwszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    wc16supper(pwszDomainName);

    if (wc16scmp(pwszDomainName, pInfo->pwszDnsDomainName) &&
        wc16scmp(pwszDomainName, pInfo->pwszDomainName))
    {
        *pppszHostnames = NULL;
        *pdwHostnames = 0;
        goto cleanup;
    }

    dwError = LwpsAllocateMemory(
                  sizeof(PSTR) * dwNumEntries,
                  (PVOID*)&ppszHostNames);
    BAIL_ON_LWPS_ERROR(dwError);

    memset(ppszHostNames, 0, sizeof(PSTR) * dwNumEntries);

    dwError = LwpsWc16sToMbs(
                  pInfo->pwszHostname,
                  &ppszHostNames[0]);
    BAIL_ON_LWPS_ERROR(dwError);

    *pppszHostnames = ppszHostNames;
    *pdwHostnames = dwNumEntries;
    ppszHostNames = NULL;

cleanup:
    LWPS_SAFE_FREE_MEMORY(pwszDomainName);

    if (ppszHostNames)
    {
        for (iEntry = 0; iEntry < dwNumEntries; iEntry++)
        {
            LWPS_SAFE_FREE_STRING(ppszHostNames[iEntry]);
        }
        LWPS_SAFE_FREE_MEMORY(ppszHostNames);
    }

    RegDB_FreePassword(pInfo);

    return dwError;

error:
    goto cleanup;
}


DWORD
RegDB_WritePassword(
    HANDLE hProvider,
    PLWPS_PASSWORD_INFO pInfo
    )
{
    DWORD dwError = 0;
    PREGDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

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

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(hProvider);
    BAIL_ON_INVALID_POINTER(pInfo);
    pContext = (PREGDB_PROVIDER_CONTEXT)hProvider;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    /* This is all the values to be written to the registry. */
    dwError = LwpsWc16sToMbs(pInfo->pwszSID, &pszDomainSID);
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = LwpsWc16sToMbs(pInfo->pwszDomainName, &pszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);
    strupper(pszDomainName);
    dwError = LwpsWc16sToMbs(pInfo->pwszDnsDomainName, &pszDnsDomainName);
    BAIL_ON_LWPS_ERROR(dwError);
    strupper(pszDnsDomainName);
    dwError = LwpsWc16sToMbs(pInfo->pwszHostname, &pszHostname);
    BAIL_ON_LWPS_ERROR(dwError);
    strupper(pszHostname);

    if (pInfo->pwszHostDnsDomain)
    {
        dwError = LwpsWc16sToMbs(pInfo->pwszHostDnsDomain, &pszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    else
    {
        dwError = LwpsWc16sToMbs(pInfo->pwszDnsDomainName, &pszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    dwError = LwpsWc16sToMbs(pInfo->pwszMachineAccount, &pszMachineAccount);
    BAIL_ON_LWPS_ERROR(dwError);
    strupper(pszMachineAccount);
    dwError = LwpsWc16sToMbs(pInfo->pwszMachinePassword, &pszMachinePassword);
    BAIL_ON_LWPS_ERROR(dwError);
    time(&tCreationTimestamp);
    dwClientModifyTimestamp = pInfo->last_change_time;
    dwSchannelType = pInfo->dwSchannelType;


    dwError = LwpsAcquireWriteLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);
    bUnlock = TRUE;

    /* Add top-level pstore registry key */
    dwError = RegUtilAddKey(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

    /* 
     * Add "MachinePassword" registry key, needed to apply ACL restrictions
     * to restrict read access to value names (the machine password)
     * stored under this key.
     */
    dwError = LwNtStatusToWin32Error(
                  RegDB_CreateRestrictedSecDescAbs(&pSecDescAbs));
    BAIL_ON_LWPS_ERROR(status);

    dwError = RegUtilAddKeySecDesc(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_MACHINE_PWD_KEY,
                  NULL,
                  WRITE_OWNER | KEY_ALL_ACCESS,
                  pSecDescAbs);
    BAIL_ON_LWPS_ERROR(dwError);

    /* Write the data to the registry */
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_DOMAIN_SID,
                  REG_SZ,
                  pszDomainSID,
                  strlen(pszDomainSID));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_DOMAIN_NAME,
                  REG_SZ,
                  pszDomainName,
                  strlen(pszDomainName));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_DOMAIN_DNS_NAME,
                  REG_SZ,
                  pszDnsDomainName,
                  strlen(pszDnsDomainName));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_HOSTNAME,
                  REG_SZ,
                  pszHostname,
                  strlen(pszHostname));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_HOST_DNS_DOMAIN,
                  REG_SZ,
                  pszHostDnsDomain,
                  strlen(pszHostDnsDomain));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_MACHINE_ACCOUNT,
                  REG_SZ,
                  pszMachineAccount,
                  strlen(pszMachineAccount));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_MACHINE_PWD_KEY,
                  NULL,
                  LWPS_REG_MACHINE_PWD,
                  REG_SZ,
                  pszMachinePassword,
                  strlen(pszMachinePassword));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_MODIFY_TIMESTAMP,
                  REG_DWORD,
                  &dwClientModifyTimestamp,
                  sizeof(dwClientModifyTimestamp));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_CREATION_TIMESTAMP,
                  REG_DWORD,
                  &tCreationTimestamp,
                  sizeof(dwClientModifyTimestamp));
    BAIL_ON_LWPS_ERROR(dwError);
    dwError = RegUtilSetValue(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL,
                  LWPS_REG_SCHANNEL_TYPE,
                  REG_DWORD,
                  &dwSchannelType,
                  sizeof(dwSchannelType));
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:
    if (bUnlock)
    {
        LwpsReleaseWriteLock(pContext->hRWLock);
    }

    LWPS_SAFE_FREE_MEMORY(pszDomainSID);
    LWPS_SAFE_FREE_MEMORY(pszDomainName);
    LWPS_SAFE_FREE_MEMORY(pszDnsDomainName);
    LWPS_SAFE_FREE_MEMORY(pszHostname);
    LWPS_SAFE_FREE_MEMORY(pszHostDnsDomain);
    LWPS_SAFE_FREE_MEMORY(pszMachineAccount);
    LWPS_SECURE_FREE_STRING(pszMachinePassword);
    RegDB_FreeAbsoluteSecurityDescriptor(&pSecDescAbs);

    return dwError;

error:
    goto cleanup;
}


DWORD
RegDB_DeleteAllEntries(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PREGDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    pContext = (PREGDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireWriteLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);
    bUnlock = TRUE;

    dwError = RegUtilDeleteTree(
                  pContext->hReg,
                  NULL,
                  PSTOREDB_REGISTRY_KEY,
                  NULL);
    if (dwError)
    {
        dwError = 0;
        goto cleanup;
    }

cleanup:
    if (bUnlock)
    {
        LwpsReleaseWriteLock(pContext->hRWLock);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
RegDB_DeleteHostEntry(
    HANDLE hProvider,
    PCSTR pszHostName
    )
{
    return RegDB_DeleteAllEntries(hProvider);
}


DWORD
RegDB_CloseProvider(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PREGDB_PROVIDER_CONTEXT pContext = 
              (PREGDB_PROVIDER_CONTEXT)hProvider;

    BAIL_IF_NOT_SUPERUSER(geteuid());

cleanup:
    if (pContext)
    {
        LwpsFreeProviderContext(pContext);
    }

    return dwError;

error:
    goto cleanup;
}


VOID
RegDB_FreePassword(
    PLWPS_PASSWORD_INFO pInfo
    )
{
    if (!pInfo)
    {
        return;
    }
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszDomainName);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszDnsDomainName);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszSID);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszHostname);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszHostDnsDomain);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszMachineAccount);
    LWPS_SECURE_FREE_WSTRING(pInfo->pwszMachinePassword);
    LwpsFreeMemory(pInfo);
}


DWORD
LWPS_SHUTDOWN_PROVIDER(regdb)(
    PSTR pszName,
    PLWPS_PROVIDER_FUNC_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    LwMapSecurityFreeContext(&gpRegLwMapSecurityCtx);
    LwMapSecurityCleanup();

cleanup:
    return dwError;

error:
    goto cleanup;
}


VOID
LwpsFreeProviderContext(
    PREGDB_PROVIDER_CONTEXT pContext
    )
{
    if (pContext->hRWLock != (HANDLE)NULL)
    {
        LwpsFreeRWLock(pContext->hRWLock);
    }
    RegCloseServer(pContext->hReg);
    LwpsFreeMemory(pContext);
}
