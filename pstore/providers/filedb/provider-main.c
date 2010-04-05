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
 *        API to support File Password Storage
 *
 *  Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LWPS_INITIALIZE_PROVIDER(filedb)(
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
    if (dwError)
    {
        dwError = LwpsCreateRWLock(
                      MACHINEPWD_DB,
                      &hLock);
    }
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAcquireWriteLock(hLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = FileDBDbInitGlobals();
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsReleaseWriteLock(hLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = FALSE;

    *ppszName = (PSTR)gpszFileDBProviderName;
    *ppFnTable = &gFileDBProviderAPITable;

cleanup:

    if (hLock != (HANDLE)NULL)
    {
        if (bUnlock)
        {
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
FileDB_OpenProvider(
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PFILEDB_PROVIDER_CONTEXT pContext = NULL;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(phProvider);

    dwError = LwpsAllocateMemory(
                  sizeof(FILEDB_PROVIDER_CONTEXT),
                  (PVOID*)&pContext);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsCreateRWLock(
                  LWPS_CONFIG_DIR "/pstore.conf",
                  &pContext->hRWLock);
    if (dwError)
    {
        dwError = LwpsCreateRWLock(
                      MACHINEPWD_DB,
                      &pContext->hRWLock);
    }
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
FileDB_ReadPassword(
    HANDLE hProvider,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PFILEDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;
    PVOID pData = NULL;
    size_t DataSize = 0;
    LWMsgContext * context = NULL;
    LWMsgDataContext * pDataContext = NULL;
    PLWPS_FILE_DB_MACHINE_ACCT_INFO pAcctData = NULL;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(ppInfo);

    pContext = (PFILEDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireReadLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = FileDBGetPwdEntry(
                  &pData,
                  &DataSize);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              FileDB_GetMachineAcctInfoSpec(),
                              pData,
                              DataSize,
                              (PVOID*)&pAcctData));
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateMemory(
                  sizeof(LWPS_PASSWORD_INFO),
                  (PVOID*)&pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    if (pAcctData->pwszDomainSID)
    {
        pInfo->pwszSID = wc16sdup(pAcctData->pwszDomainSID);
    }
    if (pAcctData->pwszDomainName)
    {
        pInfo->pwszDomainName = wc16sdup(pAcctData->pwszDomainName);
    }
    if (pAcctData->pwszDomainDnsName)
    {
        pInfo->pwszDnsDomainName = wc16sdup(pAcctData->pwszDomainDnsName);
    }
    if (pAcctData->pwszHostName)
    {
        pInfo->pwszHostname = wc16sdup(pAcctData->pwszHostName);
    }
    if (pAcctData->pwszHostDnsDomain)
    {
        pInfo->pwszHostDnsDomain = wc16sdup(pAcctData->pwszHostDnsDomain);
    }
    if (pAcctData->pwszMachineAccountName)
    {
        pInfo->pwszMachineAccount = wc16sdup(pAcctData->pwszMachineAccountName);
    }
    if (pAcctData->pwszMachineAccountPassword)
    {
        pInfo->pwszMachinePassword = wc16sdup(pAcctData->pwszMachineAccountPassword);
    }
    pInfo->last_change_time = pAcctData->pwdClientModifyTimestamp;
    pInfo->dwSchannelType = pAcctData->dwSchannelType;

    *ppInfo = pInfo;
                
cleanup:

    if (bUnlock)
    {
       LwpsReleaseReadLock(pContext->hRWLock);
    }

    if (pAcctData)
    {
        lwmsg_data_free_graph(
            pDataContext,
            FileDB_GetMachineAcctInfoSpec(),
            pAcctData);
    }
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (context)
    {
        lwmsg_context_delete(context);
    }

    LWPS_SAFE_FREE_MEMORY(pData);

    return dwError;

error:

    *ppInfo = NULL;

    if (pInfo)
    {
       FileDB_FreePassword(pInfo);
    }

    goto cleanup;
}

DWORD
FileDB_ReadPasswordByHostName(
    HANDLE hProvider,
    PCSTR  pszHostname,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PWSTR pwszHostname = NULL;

    dwError = FileDB_ReadPassword(
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
        FileDB_FreePassword(pInfo);
    }

    goto cleanup;
}

DWORD
FileDB_ReadPasswordByDomainName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    PWSTR pwszDomainName = NULL;

    dwError = FileDB_ReadPassword(
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
        FileDB_FreePassword(pInfo);
    }

    goto cleanup;
}

DWORD
FileDB_ReadHostListByDomainName(
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
    
    dwError = FileDB_ReadPassword(
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

    FileDB_FreePassword(pInfo);

    return dwError;

error:

    goto cleanup;
}

DWORD
FileDB_WritePassword(
    HANDLE hProvider,
    PLWPS_PASSWORD_INFO pInfo
    )
{
    DWORD dwError = 0;
    PFILEDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;
    LWMsgContext * context = NULL;
    LWMsgDataContext * pDataContext = NULL;
    LWPS_FILE_DB_MACHINE_ACCT_INFO acctData;
    PVOID pData = NULL;
    size_t DataSize = 0;

    memset(&acctData, 0, sizeof(acctData));

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(pInfo);

    acctData.pwszDomainSID = wc16sdup(pInfo->pwszSID);

    acctData.pwszDomainName = wc16sdup(pInfo->pwszDomainName);
    wc16supper(acctData.pwszDomainName);

    acctData.pwszDomainDnsName = wc16sdup(pInfo->pwszDnsDomainName);
    wc16supper(acctData.pwszDomainDnsName);

    acctData.pwszHostName = wc16sdup(pInfo->pwszHostname);
    wc16supper(acctData.pwszHostName);

    if (pInfo->pwszHostDnsDomain)
    {
        acctData.pwszHostDnsDomain = wc16sdup(pInfo->pwszHostDnsDomain);
    }
    else
    {
        acctData.pwszHostDnsDomain = wc16sdup(pInfo->pwszDnsDomainName);
    }

    acctData.pwszMachineAccountName = wc16sdup(pInfo->pwszMachineAccount);
    wc16supper(acctData.pwszMachineAccountName);

    acctData.pwszMachineAccountPassword = wc16sdup(pInfo->pwszMachinePassword);
    acctData.pwdCreationTimestamp = time(NULL);
    acctData.pwdClientModifyTimestamp = pInfo->last_change_time;
    acctData.dwSchannelType = pInfo->dwSchannelType;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              FileDB_GetMachineAcctInfoSpec(),
                              &acctData,
                              &pData,
                              &DataSize));
    BAIL_ON_LWPS_ERROR(dwError);


    pContext = (PFILEDB_PROVIDER_CONTEXT)hProvider;
   
    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireWriteLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = FileDBSetPwdEntry(pData, DataSize);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (bUnlock)
    {
        LwpsReleaseWriteLock(pContext->hRWLock);
    }

    LWPS_SAFE_FREE_MEMORY(acctData.pwszDomainSID);
    LWPS_SAFE_FREE_MEMORY(acctData.pwszDomainName);
    LWPS_SAFE_FREE_MEMORY(acctData.pwszDomainDnsName);
    LWPS_SAFE_FREE_MEMORY(acctData.pwszHostName);
    LWPS_SAFE_FREE_MEMORY(acctData.pwszHostDnsDomain);
    LWPS_SAFE_FREE_MEMORY(acctData.pwszMachineAccountName);
    LWPS_SAFE_FREE_MEMORY(acctData.pwszMachineAccountPassword);
    LWPS_SAFE_FREE_MEMORY(pData);

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (context)
    {
        lwmsg_context_delete(context);
    }

    return dwError;

error:

    goto cleanup;
}


DWORD
FileDB_DeleteAllEntries(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PFILEDB_PROVIDER_CONTEXT pContext = NULL;
    BOOLEAN bUnlock = FALSE;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    pContext = (PFILEDB_PROVIDER_CONTEXT)hProvider;

    BAIL_ON_INVALID_POINTER(pContext);

    dwError = LwpsAcquireWriteLock(pContext->hRWLock);
    BAIL_ON_LWPS_ERROR(dwError);

    bUnlock = TRUE;

    dwError = FileDBDeleteAllEntries();
    BAIL_ON_LWPS_ERROR(dwError);

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
FileDB_DeleteHostEntry(
    HANDLE hProvider,
    PCSTR pszHostName
    )
{
    return FileDB_DeleteAllEntries(hProvider);
}


DWORD
FileDB_CloseProvider(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PFILEDB_PROVIDER_CONTEXT pContext = 
              (PFILEDB_PROVIDER_CONTEXT)hProvider;

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
FileDB_FreePassword(
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
LWPS_SHUTDOWN_PROVIDER(filedb)(
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
    PFILEDB_PROVIDER_CONTEXT pContext
    )
{
    if (pContext->hRWLock != (HANDLE)NULL)
    {
        LwpsFreeRWLock(pContext->hRWLock);
    }

    LwpsFreeMemory(pContext);
}

