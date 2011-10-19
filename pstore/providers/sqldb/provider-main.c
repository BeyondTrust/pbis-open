/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        API to support SQL Password Storage
 *
 *  Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "lwps-utils.h"
#include "lwps/lwps.h"
#include "lwps-provider.h"
#include "provider-main_p.h"
#include "externs.h"
#include "db_p.h"

DWORD
LWPS_INITIALIZE_PROVIDER(sqldb)(
    PCSTR pszConfigFilePath,
    PSTR* ppszName,
    PLWPS_PROVIDER_FUNC_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;
    HANDLE hLock = (HANDLE)NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    dwError = LwpsCreateRWLock(
                LWPS_CONFIG_DIR "/pstore.conf",
                &hLock);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAcquireWriteLock(hLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = SqlDBDbInitGlobals();
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsReleaseWriteLock(hLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = FALSE;

    *ppszName = (PSTR)gpszSqlDBProviderName;
    *ppFnTable = &gSqlDBProviderAPITable;

cleanup:

    if (hLock != (HANDLE)NULL) {
       if (bUnlock) {
          LwpsReleaseWriteLock(hLock);
       }
       LwpsFreeRWLock(hLock);
    }

    return dwError;

error:

    *ppszName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
}

DWORD
SqlDB_OpenProvider(
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PSQLDB_PROVIDER_CONTEXT pContext = NULL;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(phProvider);

    dwError = LwpsAllocateMemory(
                  sizeof(SQLDB_PROVIDER_CONTEXT),
                  (PVOID*)&pContext);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsCreateRWLock(
                LWPS_CONFIG_DIR "/pstore.conf",
                &pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *phProvider = (HANDLE)NULL;

    if (pContext) {
       LwpsFreeProviderContext(pContext);
    }

    goto cleanup;
}

DWORD
SqlDB_ReadPasswordByHostName(
    HANDLE hProvider,
    PCSTR  pszHostname,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDB = (HANDLE)NULL;
    PMACHINE_ACCT_INFO pAcctInfo = NULL;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PSQLDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(ppInfo);

    pContext = (PSQLDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireReadLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBGetPwdEntryByHostName(
                  hDB,
                  pszHostname,
                  &pAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateMemory(
                  sizeof(LWPS_PASSWORD_INFO),
                  (PVOID*)&pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszDomainName,
                  &pInfo->pwszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszDomainDnsName,
                  &pInfo->pwszDnsDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszDomainSID,
                  &pInfo->pwszSID);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszHostName,
                  &pInfo->pwszHostname);
    BAIL_ON_LWPS_ERROR(dwError);

    if (pAcctInfo->pszHostDnsDomain)
    {
        dwError = LwpsMbsToWc16s(
                      pAcctInfo->pszHostDnsDomain,
                      &pInfo->pwszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszMachineAccountName,
                  &pInfo->pwszMachineAccount);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszMachineAccountPassword,
                  &pInfo->pwszMachinePassword);
    BAIL_ON_LWPS_ERROR(dwError);

    pInfo->last_change_time = pAcctInfo->tPwdClientModifyTimestamp;
    pInfo->dwSchannelType = pAcctInfo->dwSchannelType;

    *ppInfo = pInfo;
                
cleanup:

    if (hDB != (HANDLE)NULL) {
       SqlDBClose(hDB);
    }

    if (pAcctInfo) {
       SqlDBFreeMachineAcctInfo(pAcctInfo);
    }

    if (bUnlock) {
       LwpsReleaseReadLock(pContext->hRWLock);
    }

    return dwError;

error:

    *ppInfo = NULL;

    if (pInfo) {
       SqlDB_FreePassword(pInfo);
    }

    goto cleanup;
}

DWORD
SqlDB_ReadPasswordByDomainName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDB = (HANDLE)NULL;
    PMACHINE_ACCT_INFO pAcctInfo = NULL;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PSQLDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(ppInfo);

    pContext = (PSQLDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireReadLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBGetPwdEntryByDomainDnsName(
                  hDB,
                  pszDomainName,
                  &pAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateMemory(
                  sizeof(LWPS_PASSWORD_INFO),
                  (PVOID*)&pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszDomainName,
                  &pInfo->pwszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszDomainDnsName,
                  &pInfo->pwszDnsDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszDomainSID,
                  &pInfo->pwszSID);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszHostName,
                  &pInfo->pwszHostname);
    BAIL_ON_LWPS_ERROR(dwError);

    if (pAcctInfo->pszHostDnsDomain)
    {
        dwError = LwpsMbsToWc16s(
                      pAcctInfo->pszHostDnsDomain,
                      &pInfo->pwszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszMachineAccountName,
                  &pInfo->pwszMachineAccount);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(
                  pAcctInfo->pszMachineAccountPassword,
                  &pInfo->pwszMachinePassword);
    BAIL_ON_LWPS_ERROR(dwError);

    pInfo->last_change_time = pAcctInfo->tPwdClientModifyTimestamp;
    pInfo->dwSchannelType = pAcctInfo->dwSchannelType;

    *ppInfo = pInfo;
                
cleanup:

    if (hDB != (HANDLE)NULL) {
       SqlDBClose(hDB);
    }

    if (pAcctInfo) {
       SqlDBFreeMachineAcctInfo(pAcctInfo);
    }

    if (bUnlock) {
       LwpsReleaseReadLock(pContext->hRWLock);
    }

    return dwError;

error:

    *ppInfo = NULL;

    if (pInfo) {
       SqlDB_FreePassword(pInfo);
    }

    goto cleanup;
}

DWORD
SqlDB_ReadHostListByDomainName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PSTR **pppszHostnames,
    DWORD *pdwHostnames
    )
{
    DWORD dwError = 0;
    HANDLE hDB = (HANDLE)NULL;
    PMACHINE_ACCT_INFO *ppAcctInfo = NULL;
    PSTR *ppszHostNames = NULL;
    DWORD dwAccounts = 0;
    DWORD dwNumEntries = 0;
    DWORD iEntry = 0;
    PSQLDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_ON_INVALID_POINTER(pppszHostnames);
    BAIL_ON_INVALID_POINTER(pdwHostnames);
    
    pContext = (PSQLDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireReadLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBGetPwdEntries(hDB, &ppAcctInfo, &dwAccounts);
    BAIL_ON_LWPS_ERROR(dwError);

    if (dwAccounts == 0) {
        *pppszHostnames = NULL;
        *pdwHostnames = 0;
        goto cleanup;
    }

    /*
     * Make sure the entries match our hostname
     */
    for (iEntry = 0; iEntry < dwAccounts; iEntry++) {
        if ((strcasecmp(ppAcctInfo[iEntry]->pszDomainName,
                        pszDomainName) == 0) ||
                (strcasecmp(ppAcctInfo[iEntry]->pszDomainDnsName,
                        pszDomainName) == 0)) {
            dwNumEntries++;
        }
    }

    dwError = LwpsAllocateMemory(
                  sizeof(PSTR) * dwNumEntries,
                  (PVOID*)&ppszHostNames);
    BAIL_ON_LWPS_ERROR(dwError);

    memset(ppszHostNames, 0, sizeof(PSTR) * dwNumEntries);

    dwNumEntries = 0;
    for (iEntry = 0; iEntry < dwAccounts; iEntry++) {
        if ((strcasecmp(ppAcctInfo[iEntry]->pszDomainName,
                        pszDomainName) == 0) ||
                (strcasecmp(ppAcctInfo[iEntry]->pszDomainDnsName,
                        pszDomainName) == 0)) {
            dwError = LwpsAllocateString(ppAcctInfo[iEntry]->pszHostName,
                &ppszHostNames[dwNumEntries]);
            BAIL_ON_LWPS_ERROR(dwError);
            dwNumEntries++;
        }
    }
    
    *pppszHostnames = ppszHostNames;
    *pdwHostnames = dwNumEntries;
    ppszHostNames = NULL;

cleanup:

    if (hDB != (HANDLE)NULL) {
       SqlDBClose(hDB);
    }

    if (ppszHostNames) {
        for (iEntry = 0; iEntry < dwNumEntries; iEntry++) {
            LWPS_SAFE_FREE_STRING(ppszHostNames[iEntry]);
        }
        LWPS_SAFE_FREE_MEMORY(ppszHostNames);
    }
                
    if (ppAcctInfo) {
       SqlDBFreeEntryList(ppAcctInfo, dwAccounts);
    }

    if (bUnlock) {
       LwpsReleaseReadLock(pContext->hRWLock);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
SqlDB_WritePassword(
    HANDLE hProvider,
    PLWPS_PASSWORD_INFO pInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDB = (HANDLE)NULL;
    PSQLDB_PROVIDER_CONTEXT pContext = NULL;
    PMACHINE_ACCT_INFO pAcctInfo = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(pInfo);

    pContext = (PSQLDB_PROVIDER_CONTEXT)hProvider;
   
    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAllocateMemory(
                  sizeof(MACHINE_ACCT_INFO),
                  (PVOID*)&pAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsWc16sToMbs(
                  pInfo->pwszDomainName,
                  &pAcctInfo->pszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsWc16sToMbs(
                  pInfo->pwszDnsDomainName,
                  &pAcctInfo->pszDomainDnsName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsWc16sToMbs(
                  pInfo->pwszSID,
                  &pAcctInfo->pszDomainSID);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsWc16sToMbs(
                  pInfo->pwszHostname,
                  &pAcctInfo->pszHostName);
    BAIL_ON_LWPS_ERROR(dwError);

    if (pInfo->pwszHostDnsDomain)
    {
        dwError = LwpsWc16sToMbs(
                      pInfo->pwszHostDnsDomain,
                      &pAcctInfo->pszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = LwpsWc16sToMbs(
                  pInfo->pwszMachineAccount,
                  &pAcctInfo->pszMachineAccountName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsWc16sToMbs(
                  pInfo->pwszMachinePassword,
                  &pAcctInfo->pszMachineAccountPassword);
    BAIL_ON_LWPS_ERROR(dwError);

    pAcctInfo->tPwdClientModifyTimestamp = pInfo->last_change_time; 
    pAcctInfo->dwSchannelType = pInfo->dwSchannelType;

    dwError = LwpsAcquireWriteLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBSetPwdEntry(hDB, pAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
       SqlDBClose(hDB);
    }

    if (pAcctInfo) {
       SqlDBFreeMachineAcctInfo(pAcctInfo);
    }

    if (bUnlock) {
       LwpsReleaseWriteLock(pContext->hRWLock);
    }

    return dwError;

error:

    goto cleanup;
}


DWORD
SqlDB_DeleteAllEntries(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    HANDLE hDB = (HANDLE)NULL;
    PSQLDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    pContext = (PSQLDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireWriteLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBDeleteAllEntries(hDB);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
       SqlDBClose(hDB);
    }

    if (bUnlock) {
       LwpsReleaseWriteLock(pContext->hRWLock);
    }
    
    return dwError;

error:

    goto cleanup;
}

DWORD
SqlDB_DeleteHostEntry(
    HANDLE hProvider,
    PCSTR pszHostName
    )
{
    DWORD dwError = 0;
    HANDLE hDB = (HANDLE)NULL;
    PSQLDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    pContext = (PSQLDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireWriteLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = SqlDBOpen(&hDB);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SqlDBDeletePwdEntryByHostName(hDB, pszHostName);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (hDB != (HANDLE)NULL) {
       SqlDBClose(hDB);
    }

    if (bUnlock) {
       LwpsReleaseWriteLock(pContext->hRWLock);
    }
    
    return dwError;

error:

    goto cleanup;
}


DWORD
SqlDB_CloseProvider(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PSQLDB_PROVIDER_CONTEXT pContext = 
              (PSQLDB_PROVIDER_CONTEXT)hProvider;

    BAIL_IF_NOT_SUPERUSER(geteuid());

cleanup:

    if (pContext) {
       LwpsFreeProviderContext(pContext);
    }

    return dwError;

error:

    goto cleanup;
}

VOID
SqlDB_FreePassword(
    PLWPS_PASSWORD_INFO pInfo
    )
{
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszDomainName);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszDnsDomainName);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszSID);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszHostname);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszHostDnsDomain);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszMachineAccount);
    LWPS_SAFE_FREE_MEMORY(pInfo->pwszMachinePassword);
    LwpsFreeMemory(pInfo);
}

DWORD
LWPS_SHUTDOWN_PROVIDER(sqldb)(
    PSTR pszName,
    PLWPS_PROVIDER_FUNC_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    BAIL_IF_NOT_SUPERUSER(geteuid());

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
LwpsFreeProviderContext(
    PSQLDB_PROVIDER_CONTEXT pContext
    )
{
    if (pContext->hRWLock != (HANDLE)NULL)
       LwpsFreeRWLock(pContext->hRWLock);

    LwpsFreeMemory(pContext);
}

