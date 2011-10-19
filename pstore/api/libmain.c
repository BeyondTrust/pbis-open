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
 *     libmain.c
 *
 *  Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        Password Storage API
 *
 *  Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "lwps-utils.h"
#include "lwps/lwps.h"
#include "lwps-provider.h"
#include "lwps-provider_p.h"

DWORD
LwpsAPIInitialize()
{
    return 0;
}

DWORD
LwpsOpenPasswordStore(
    LwpsPasswordStoreType storeType,
    PHANDLE phStore
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = NULL;

    BAIL_ON_INVALID_POINTER(phStore);

    dwError = LwpsOpenProvider(
                  storeType,
                  &pProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    *phStore = (HANDLE)pProvider;

cleanup:

    return dwError;

error:

    *phStore = (HANDLE)NULL;

    if (pProvider) {
       LwpsFreeProvider(pProvider);
    }

    goto cleanup;
}

DWORD
LwpsGetPasswordByHostName(
    HANDLE hStore,
    PCSTR  pszHostname,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    HANDLE hProvider = (HANDLE)NULL;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)hStore;

    BAIL_ON_INVALID_HANDLE(hStore);

    dwError = pProvider->pFnTable->pFnOpenProvider(&hProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnReadPasswordByHostName(
                                        hProvider,
                                        pszHostname,
                                        ppInfo);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProvider && (hProvider != (HANDLE)NULL))
    {
       pProvider->pFnTable->pFnCloseProvider(hProvider); 
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwpsGetPasswordByCurrentHostName(
    HANDLE hStore,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    CHAR szBuffer[256];
    PSTR pszLocal = NULL;
    PSTR pszDot = NULL;
    int len = 0;

    if ( gethostname(szBuffer, sizeof(szBuffer)) != 0 )
    {
        dwError = errno;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    len = strlen(szBuffer);
    if ( len > strlen(".local") )
    {
        pszLocal = &szBuffer[len - strlen(".local")];
        if ( !strcasecmp( pszLocal, ".local" ) )
        {
            pszLocal[0] = '\0';
        }
    }
    
    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    pszDot = strchr(szBuffer, '.');
    if ( pszDot )
    {
        pszDot[0] = '\0';
    }

    dwError = LwpsGetPasswordByHostName(
                    hStore,
                    szBuffer,
                    ppInfo);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    return dwError;

error:
    *ppInfo = NULL;
    
    goto cleanup;
}

DWORD
LwpsGetPasswordByDomainName(
    HANDLE hStore,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    HANDLE hProvider = (HANDLE)NULL;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)hStore;

    BAIL_ON_INVALID_HANDLE(hStore);

    dwError = pProvider->pFnTable->pFnOpenProvider(&hProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnReadPasswordByDomainName(
                                        hProvider,
                                        pszDomainName,
                                        ppInfo);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProvider && (hProvider != (HANDLE)NULL))
    {
       pProvider->pFnTable->pFnCloseProvider(hProvider); 
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwpsGetHostListByDomainName(
    HANDLE hStore,
    PCSTR  pszDomainName,
    PSTR **pppszHostnames,
    DWORD *pdwNumEntries
    )
{
    DWORD dwError = 0;
    HANDLE hProvider = (HANDLE)NULL;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)hStore;

    BAIL_ON_INVALID_HANDLE(hStore);

    dwError = pProvider->pFnTable->pFnOpenProvider(&hProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnReadHostListByDomainName(
                                        hProvider,
                                        pszDomainName,
                                        pppszHostnames,
                                        pdwNumEntries);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProvider && (hProvider != (HANDLE)NULL))
    {
        pProvider->pFnTable->pFnCloseProvider(hProvider);
    }

    return dwError;

error:
    
    goto cleanup;
}

DWORD
LwpsWritePasswordToAllStores(
    PLWPS_PASSWORD_INFO pInfo
    )
{
    DWORD dwError = 0;
    PLWPS_STACK pProviderStack = NULL;

    dwError = LwpsFindAllProviders(&pProviderStack);
    BAIL_ON_LWPS_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pProviderStack);

    dwError = LwpsStackForeach(
	          pProviderStack,
                  &LwpsWritePasswordToStore,
                  pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProviderStack) {
       LwpsStackForeach(
            pProviderStack,
            &LwpsConfigFreeProviderInStack,
            NULL);
       LwpsStackFree(pProviderStack);
    }


    return dwError;

error:

    LWPS_LOG_ERROR("Failed to write password to all stores. [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LwpsDeleteEntriesInAllStores(
    VOID
    )
{
    DWORD dwError = 0;
    PLWPS_STACK pProviderStack = NULL;

    dwError = LwpsFindAllProviders(&pProviderStack);
    BAIL_ON_LWPS_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pProviderStack);

    dwError = LwpsStackForeach(
	          pProviderStack,
                  &LwpsDeleteEntriesInStore,
                  NULL);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProviderStack) {
       LwpsStackForeach(
            pProviderStack,
            &LwpsConfigFreeProviderInStack,
            NULL);
       LwpsStackFree(pProviderStack);
    }

    return dwError;

error:

    LWPS_LOG_ERROR("Failed to delete password entries in all stores. [Error code: %d]", dwError);

    goto cleanup;
}

DWORD
LwpsDeleteHostInAllStores(
    PCSTR pszHostname
    )
{
    DWORD dwError = 0;
    PLWPS_STACK pProviderStack = NULL;

    dwError = LwpsFindAllProviders(&pProviderStack);
    BAIL_ON_LWPS_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pProviderStack);
    BAIL_ON_INVALID_POINTER(pszHostname);

    dwError = LwpsStackForeach(
	          pProviderStack,
                  &LwpsDeleteHostInStore,
                  (PVOID)pszHostname);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProviderStack) {
       LwpsStackForeach(
            pProviderStack,
            &LwpsConfigFreeProviderInStack,
            NULL);
       LwpsStackFree(pProviderStack);
    }

    return dwError;

error:

    LWPS_LOG_ERROR("Failed to delete password entries in all stores. [Error code: %d]", dwError);

    goto cleanup;
}



VOID
LwpsFreePasswordInfo(
    HANDLE hStore,
    PLWPS_PASSWORD_INFO pInfo
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)hStore;

    BAIL_ON_INVALID_HANDLE(hStore);

    if (pInfo) {
       pProvider->pFnTable->pfnFreePassword(pInfo);
    }

error:

    return;
}

DWORD
LwpsClosePasswordStore(
    HANDLE hStore
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)hStore;

    BAIL_ON_INVALID_HANDLE(hStore);

    LwpsFreeProvider(pProvider);

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
LwpsFreeHostList(
    PSTR *ppszHostnames,
    DWORD dwNumEntries
    )
{
    LwpsFreeStringArray(ppszHostnames, dwNumEntries);
}

DWORD
LwpsAPIShutdown()
{
    return 0;
}

