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
 *        lwps-provider.c
 * 
 *  Abstract:
 *  
 *        Likewise Password Storage (LWPS)
 *  
 *        Storage Provider API
 *  
 *  Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "lwps-utils.h"
#include "lwps/lwps.h"
#include "lwps-provider.h"
#include "lwps-provider_p.h"
#include <reg/reg.h>
#include <reg/regutil.h>

#define LWPS_CONFIG_PATH "registry"

#ifdef ENABLE_STATIC_PROVIDERS

extern DWORD LWPS_INITIALIZE_PROVIDER(regdb)(PCSTR, PSTR*, PLWPS_PROVIDER_FUNC_TABLE*);
extern DWORD LWPS_SHUTDOWN_PROVIDER(regdb)(PSTR, PLWPS_PROVIDER_FUNC_TABLE);

static LWPS_STATIC_PROVIDER gStaticProviders[] =
{
#ifdef ENABLE_REGDB
    LWPS_STATIC_PROVIDER_ENTRY(regdb, regdb),
#endif
    LWPS_STATIC_PROVIDER_END
};

#endif

static
DWORD
LwpsBuiltInProviders(
    PLWPS_STACK* ppProviderStack
    );

DWORD
LwpsOpenProvider(
    LwpsPasswordStoreType storeType,
    PLWPS_STORAGE_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = NULL;
    PLWPS_STACK pProviderStack = NULL;

    dwError = LwpsBuiltInProviders(&pProviderStack);
    BAIL_ON_LWPS_ERROR(dwError);

    if (storeType == LWPS_PASSWORD_STORE_DEFAULT) {

       dwError = LwpsFindDefaultProvider(
                       &pProviderStack,
                       &pProvider);
       BAIL_ON_LWPS_ERROR(dwError);

    }
    else {

       dwError = LwpsFindSpecificProvider(
                       storeType,
                       &pProviderStack,
                       &pProvider);
       BAIL_ON_LWPS_ERROR(dwError);

    }

    dwError = LwpsInitProvider(
                       LWPS_CONFIG_PATH,
                       pProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    *ppProvider = pProvider;

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

    *ppProvider = NULL;

    if (pProvider) {

        LwpsFreeProvider(pProvider);
    }

    goto cleanup;
}

DWORD
LwpsFindDefaultProvider(
    PLWPS_STACK* ppStack,
    PLWPS_STORAGE_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = NULL;
    PLWPS_STORAGE_PROVIDER pFirstProvider = NULL;
    int iProvider = 0;

    pProvider = (PLWPS_STORAGE_PROVIDER) LwpsStackPop(ppStack);
    while (pProvider) {

          if (pProvider->bDefault)
             break;

          if (!iProvider++) {
             pFirstProvider = pProvider;
          } else { 
             LwpsFreeProvider(pProvider);
          }
          pProvider = NULL;

          pProvider = (PLWPS_STORAGE_PROVIDER) LwpsStackPop(ppStack);
    }

    if (pProvider) {
       *ppProvider = pProvider;
    } else if (pFirstProvider) {
       *ppProvider = pFirstProvider;
       pFirstProvider = NULL;
    } else {
       dwError = LWPS_ERROR_NO_SUCH_PROVIDER;
       BAIL_ON_LWPS_ERROR(dwError);
    }

cleanup:

    if (pFirstProvider) {
        LwpsFreeProvider(pFirstProvider);
    }

    return dwError;

error:

    *ppProvider = NULL;

    goto cleanup;
}

DWORD
LwpsFindSpecificProvider(
    LwpsPasswordStoreType storeType,
    PLWPS_STACK* ppStack,
    PLWPS_STORAGE_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = NULL;

    pProvider = (PLWPS_STORAGE_PROVIDER) LwpsStackPop(ppStack);

    while (pProvider) {

        if (pProvider->storeType == storeType) {
            break;
        }

        LwpsFreeProvider(pProvider);
        pProvider = NULL;

        pProvider = (PLWPS_STORAGE_PROVIDER) LwpsStackPop(ppStack);
    }

    if (!pProvider) {
       dwError = LWPS_ERROR_NO_SUCH_PROVIDER;
       BAIL_ON_LWPS_ERROR(dwError);
    }

    *ppProvider = pProvider;

cleanup:

    return dwError;

error:

    *ppProvider = NULL;

    goto cleanup;
}

DWORD
LwpsFindAllProviders(
    PLWPS_STACK* ppStack
    )
{
    DWORD dwError = 0;
    PLWPS_STACK pProviderStack = NULL;

    dwError = LwpsBuiltInProviders(&pProviderStack);
    BAIL_ON_LWPS_ERROR(dwError);

    *ppStack = LwpsStackReverse(pProviderStack);

 cleanup:

    return dwError;

 error:

    if (pProviderStack) {
       LwpsStackForeach(
            pProviderStack,
            &LwpsConfigFreeProviderInStack,
            NULL);
       LwpsStackFree(pProviderStack);
    }

    goto cleanup;
}

DWORD
LwpsConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLWPS_STACK* ppProviderStack = (PLWPS_STACK*)pData;
    PLWPS_STORAGE_PROVIDER pProvider = NULL;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;
    PCSTR   pszLibName = NULL;

    BAIL_ON_INVALID_POINTER(ppProviderStack);

    if (IsNullOrEmptyString(pszSectionName) ||
        strncasecmp(pszSectionName,
                    "password storage:",
                    sizeof("password storage:") -1)) {
        bSkipSection = TRUE;
        goto done;
    }

    pszLibName = pszSectionName + sizeof("password storage:") - 1;
    if (IsNullOrEmptyString(pszLibName)) {
       bSkipSection = TRUE;
       goto done;
    }

    dwError = LwpsAllocateMemory(
                 sizeof(LWPS_STORAGE_PROVIDER),
                 (PVOID*)&pProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateString(
                 pszLibName,
                 &pProvider->pszId);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsStackPush(
                 pProvider,
                 ppProviderStack);
    BAIL_ON_LWPS_ERROR(dwError);

done:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    if (pProvider) {
       LwpsFreeProvider(pProvider);
    }

    *pbContinue = FALSE;
    *pbSkipSection = TRUE;

    goto cleanup;
}

DWORD
LwpsConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLWPS_STACK* ppProviderStack = (PLWPS_STACK*)pData;
    PLWPS_STORAGE_PROVIDER pProvider = NULL;
    PSTR pszProviderLibpath = NULL;

    BAIL_ON_INVALID_POINTER(ppProviderStack);

    pProvider = (PLWPS_STORAGE_PROVIDER)LwpsStackPeek(*ppProviderStack);

    if (!pProvider) {
       dwError = LWPS_ERROR_INTERNAL;
       BAIL_ON_LWPS_ERROR(dwError);
    }

    if (!strcasecmp(pszName, "path")) {

       if (!IsNullOrEmptyString(pszValue)) {
          dwError = LwpsAllocateString(
                          pszValue,
                          &pszProviderLibpath);
          BAIL_ON_LWPS_ERROR(dwError);
       }

       LWPS_SAFE_FREE_STRING(pProvider->pszLibPath);

       pProvider->pszLibPath = pszProviderLibpath;
       pszProviderLibpath = NULL;
    }
    else if (!strcasecmp(pszName, "type") && !IsNullOrEmptyString(pszValue)) {

       if (!strcasecmp(pszValue, "regdb")) {
           pProvider->storeType = LWPS_PASSWORD_STORE_REGDB;
       } else {
           pProvider->storeType = LWPS_PASSWORD_STORE_UNKNOWN;
       }
    }
    else if (!strcasecmp(pszName, "default"))
    {
       if (!IsNullOrEmptyString(pszValue) &&
           (*pszValue == 'Y') &&
           (*pszValue == 'y') &&
           (*pszValue == '1')) {
          pProvider->bDefault = TRUE; 
       } else {
          pProvider->bDefault = FALSE;
       }
    }

    *pbContinue = TRUE;

cleanup:

    return dwError;

error:

    LWPS_SAFE_FREE_STRING(pszProviderLibpath);

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LwpsConfigFreeProviderInStack(
    PVOID pItem,
    PVOID pData
    )
{
    if (pItem) {
       LwpsFreeProvider((PLWPS_STORAGE_PROVIDER)pItem);
    }

    return 0;
}

DWORD
LwpsInitProvider(
    PCSTR pszConfigPath,
    PLWPS_STORAGE_PROVIDER pProvider
    )
{
    DWORD dwError = 0;
    PFNLWPS_INITIALIZE_PROVIDER pfnInitProvider = NULL;
    PCSTR pszError = NULL;

#ifdef ENABLE_STATIC_PROVIDERS
    {
        int i = 0;

        /* First look for a static provider entry with the given name */
        for (i = 0; gStaticProviders[i].pszId; i++)
        {
            if (!strcmp(gStaticProviders[i].pszId, pProvider->pszId))
            {
                pfnInitProvider = gStaticProviders[i].pInitialize;
                pProvider->pFnShutdown = gStaticProviders[i].pShutdown;
                LWPS_LOG_DEBUG("Provider %s loaded from static list", pProvider->pszId);
                break;
            }
        }
    }
#endif    

    if (!pfnInitProvider)
    {
        if (IsNullOrEmptyString(pProvider->pszLibPath)) {
           dwError = ENOENT;
           BAIL_ON_LWPS_ERROR(dwError);
        }

        dlerror();

        pProvider->pLibHandle = dlopen(pProvider->pszLibPath,
                                       RTLD_NOW | RTLD_GLOBAL);
        if (pProvider->pLibHandle == NULL) {
           pszError = dlerror();

           if (!IsNullOrEmptyString(pszError)) {
               LWPS_LOG_ERROR("%s", pszError);
           }

           dwError = LWPS_ERROR_INVALID_PROVIDER;
           BAIL_ON_LWPS_ERROR(dwError);

        }

        dlerror();

        pfnInitProvider = (PFNLWPS_INITIALIZE_PROVIDER)dlsym(
                                  pProvider->pLibHandle,
                                  LWPS_SYMBOL_STORAGE_PROVIDER_INITIALIZE);
        if (pfnInitProvider == NULL) {
            dwError =  LWPS_ERROR_INVALID_PROVIDER;
            BAIL_ON_LWPS_ERROR(dwError);
        }

        dlerror();
        pProvider->pFnShutdown = (PFNLWPS_SHUTDOWN_PROVIDER)dlsym(
                                     pProvider->pLibHandle,
                                     LWPS_SYMBOL_STORAGE_PROVIDER_SHUTDOWN);
        if (pProvider->pFnShutdown == NULL) {
            dwError =  LWPS_ERROR_INVALID_PROVIDER;
            BAIL_ON_LWPS_ERROR(dwError);
        }
    }

    dwError = pfnInitProvider(
                  pszConfigPath,
                  &pProvider->pszName,
                  &pProvider->pFnTable);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
LwpsFreeProvider(
    PLWPS_STORAGE_PROVIDER pProvider
    )
{
    if (pProvider) {

       if (pProvider->pLibHandle) {
  
          if (pProvider->pFnShutdown) {

             pProvider->pFnShutdown(
                           pProvider->pszName,
                           pProvider->pFnTable); 

          }

          dlclose(pProvider->pLibHandle);

       }
  
       LWPS_SAFE_FREE_STRING(pProvider->pszLibPath);
       LWPS_SAFE_FREE_STRING(pProvider->pszId);

       LwpsFreeMemory(pProvider);
    }
}

DWORD
LwpsWritePasswordToStore(
    PVOID pItem,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)pItem;
    PLWPS_PASSWORD_INFO pInfo = (PLWPS_PASSWORD_INFO)pData;
    HANDLE hProvider = (HANDLE)NULL;

    BAIL_ON_INVALID_POINTER(pProvider);

    dwError = LwpsInitProvider(
                  LWPS_CONFIG_PATH,
		  pProvider);
    BAIL_ON_LWPS_ERROR(dwError);
  
    dwError = pProvider->pFnTable->pFnOpenProvider(&hProvider);
    BAIL_ON_LWPS_ERROR(dwError);
  
    dwError = pProvider->pFnTable->pFnWritePassword(hProvider, pInfo);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProvider && (hProvider != (HANDLE)NULL))
    {
        pProvider->pFnTable->pFnCloseProvider(hProvider); 
    }

    return dwError;

error:

    LWPS_LOG_ERROR("Failed to write password to provider: %s [Error code:%d]", ((pProvider && !IsNullOrEmptyString(pProvider->pszName)) ? pProvider->pszName : ""), dwError);

    goto cleanup;
}

DWORD
LwpsDeleteEntriesInStore(
    PVOID pItem,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)pItem;
    HANDLE hProvider = (HANDLE)NULL;

    BAIL_ON_INVALID_POINTER(pProvider);

    dwError = LwpsInitProvider(
                  LWPS_CONFIG_PATH,
		  pProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnOpenProvider(&hProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnDeleteAllEntries(hProvider);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProvider && (hProvider != (HANDLE)NULL))
    {
       pProvider->pFnTable->pFnCloseProvider(hProvider); 
    }

    return dwError;

error:

    LWPS_LOG_ERROR("Failed to delete all entries in provider: %s [Error code:%d]", ((pProvider && !IsNullOrEmptyString(pProvider->pszName)) ? pProvider->pszName : ""), dwError);

    goto cleanup;

}

DWORD
LwpsDeleteHostInStore(
    PVOID pItem,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)pItem;
    HANDLE hProvider = (HANDLE)NULL;
    PCSTR pszHostname = (PCSTR)pData;

    BAIL_ON_INVALID_POINTER(pProvider);
    BAIL_ON_INVALID_POINTER(pszHostname);

    dwError = LwpsInitProvider(
                  LWPS_CONFIG_PATH,
		  pProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnOpenProvider(&hProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnDeleteHostEntry(hProvider, pszHostname);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProvider && (hProvider != (HANDLE)NULL))
    {
       pProvider->pFnTable->pFnCloseProvider(hProvider); 
    }

    return dwError;

error:

    LWPS_LOG_ERROR("Failed to delete all entries in provider: %s [Error code:%d]", ((pProvider && !IsNullOrEmptyString(pProvider->pszName)) ? pProvider->pszName : ""), dwError);

    goto cleanup;

}

DWORD
LwpsDeleteDomainInStore(
    PVOID pItem,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = (PLWPS_STORAGE_PROVIDER)pItem;
    HANDLE hProvider = (HANDLE)NULL;
    PCSTR pszDomainName = (PCSTR)pData;

    BAIL_ON_INVALID_POINTER(pProvider);
    BAIL_ON_INVALID_POINTER(pszDomainName);

    dwError = LwpsInitProvider(
                  LWPS_CONFIG_PATH,
		  pProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnOpenProvider(&hProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pProvider->pFnTable->pFnDeleteDomainEntry(
                                       hProvider,
                                       pszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pProvider && (hProvider != (HANDLE)NULL))
    {
       pProvider->pFnTable->pFnCloseProvider(hProvider); 
    }

    return dwError;

error:

    LWPS_LOG_ERROR("Failed to delete all entries in provider: %s [Error code:%d]", ((pProvider && !IsNullOrEmptyString(pProvider->pszName)) ? pProvider->pszName : ""), dwError);

    goto cleanup;

}

static
DWORD
LwpsBuiltInProviders(
    PLWPS_STACK* ppProviderStack
    )
{
    DWORD dwError = 0;
    PLWPS_STORAGE_PROVIDER pProvider = NULL;
    PLWPS_STACK pProviderStack = NULL;
    HANDLE hReg = NULL;
    DWORD dwStorageType = 0;
    DWORD dwProviderDefault = 0;
    PSTR pszStorageType = NULL;
    PSTR pszProviderPath = NULL;
    BOOLEAN bHasStorageType = FALSE;
    BOOLEAN bHasProviderPath = FALSE;
    BOOLEAN bHasProviderDefault = FALSE;


    dwError = LwpsAllocateMemory(
                  sizeof(LWPS_STORAGE_PROVIDER),
                  (PVOID*)&pProvider);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsStackPush(
                  pProvider,
                  &pProviderStack);
    BAIL_ON_LWPS_ERROR(dwError);

#if defined(ENABLE_REGDB)
    dwError = RegOpenServer(&hReg);
    if (dwError == 0)
    {
        dwError = RegUtilIsValidKey(
                      hReg,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULTS);
    }
    if (dwError == 0)
    {
        dwError = RegUtilGetValue(
                      hReg,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULTS,
                      NULL,
                      LWPS_REG_STORAGE_TYPE,
                      NULL,
                      (PVOID) &dwStorageType,
                      NULL);
        if (dwError == 0)
        {
            bHasStorageType = TRUE;
            pProvider->storeType = dwStorageType;
        }

        dwError = RegUtilGetValue(
                      hReg,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULTS,
                      NULL,
                      LWPS_REG_PROVIDER_PATH,
                      NULL,
                      (PVOID) &pszProviderPath,
                      NULL);
        if (dwError == 0)
        {
            bHasProviderPath = TRUE;
            pProvider->pszLibPath = pszProviderPath;
        }

        dwError = RegUtilGetValue(
                      hReg,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULTS,
                      NULL,
                      LWPS_REG_PROVIDER_DEFAULT,
                      NULL,
                      (PVOID) &dwProviderDefault,
                      NULL);
        if (dwError == 0)
        {
            bHasProviderDefault = TRUE;
            pProvider->bDefault = dwProviderDefault ? TRUE : FALSE;
        }
    }

    /* Set defaults when values are not found in registry */
    if (!bHasProviderPath)
    {
        dwError = LwpsAllocateString(
                      LIBDIR "/liblwps-regdb" MOD_EXT,
                      &pProvider->pszLibPath);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    if (!bHasStorageType)
    {
        pProvider->storeType = LWPS_PASSWORD_STORE_REGDB;
    }
    if (!bHasProviderDefault)
    {
        pProvider->bDefault = TRUE;
    }

    switch (pProvider->storeType)
    {
        case LWPS_PASSWORD_STORE_UNKNOWN:   /* 0 */
            pszStorageType = "unknown";
            break;
        case LWPS_PASSWORD_STORE_DEFAULT:   /* 1 */
            pszStorageType = "unknown";
            break;
        case LWPS_PASSWORD_STORE_REGDB:     /* 5 */
        default:
            pszStorageType = "regdb";
            break;
    }
    dwError = LwpsAllocateString(
                  pszStorageType,
                  &pProvider->pszId);
    BAIL_ON_LWPS_ERROR(dwError);

    if (hReg)
    {
        RegCloseServer(hReg);
    }

#else
    dwError = LWPS_ERROR_NO_SUCH_PROVIDER;
    BAIL_ON_LWPS_ERROR(dwError);
#endif

    *ppProviderStack = pProviderStack;

cleanup:

    return dwError;

error:

    *ppProviderStack = NULL;

    if (ppProviderStack) {
       LwpsStackForeach(
            pProviderStack,
            &LwpsConfigFreeProviderInStack,
            NULL);
       LwpsStackFree(pProviderStack);
    }

    goto cleanup;
}

