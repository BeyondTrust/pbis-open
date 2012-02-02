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
 *        auth_provider.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

static
DWORD
LsaSrvAllocateProvider(
    PCSTR pszId,
    PCSTR pszPath,
    PLSA_AUTH_PROVIDER *ppProvider
    );

static
DWORD
LsaSrvAuthProviderAllocateProviderList(
    PLSA_AUTH_PROVIDER *ppProviderList
    );

static
VOID
LsaSrvFreeAuthProviderList(
    PLSA_AUTH_PROVIDER pProviderList
    );

static
VOID
LsaSrvAppendAuthProviderList(
    PLSA_AUTH_PROVIDER *ppProviderList,
    PLSA_AUTH_PROVIDER pProvider
    );

static
DWORD
LsaSrvAuthProviderRegGetLoadOrder(
    PSTR *ppszLoadOrder
    );

VOID
LsaSrvFreeAuthProvider(
    PLSA_AUTH_PROVIDER pProvider
    )
{
    if (pProvider)
    {
        if (pProvider->pFnTable && pProvider->pFnTable->pfnShutdownProvider)
        {
           pProvider->pFnTable->pfnShutdownProvider();
        }

        if (pProvider->pLibHandle)
        {
           dlclose(pProvider->pLibHandle);
        }

        LW_SAFE_FREE_STRING(pProvider->pszId);
        LW_SAFE_FREE_STRING(pProvider->pszProviderLibpath);

        LwFreeMemory(pProvider);
    }
}

DWORD
LsaSrvValidateProvider(
    PLSA_AUTH_PROVIDER pProvider
    )
{
    if (!pProvider || !pProvider->pFnTable)
    {
        return LW_ERROR_INVALID_AUTH_PROVIDER;
    }

    return 0;
}

DWORD
LsaSrvInitAuthProvider(
    IN PLSA_AUTH_PROVIDER pProvider,
    IN OPTIONAL PLSA_STATIC_PROVIDER pStaticProviders
    )
{
    DWORD dwError = 0;
    PFNINITIALIZEPROVIDER pfnInitProvider = NULL;
    PCSTR pszError = NULL;
    PSTR pszProviderLibpath = NULL;
    int i = 0;

    if (pStaticProviders)
    {
        /* First look for a static provider entry with the given name */
        for (i = 0; pStaticProviders[i].pszId; i++)
        {
            if (!strcmp(pStaticProviders[i].pszId, pProvider->pszId))
            {
                pfnInitProvider = pStaticProviders[i].pInitialize;
                LSA_LOG_DEBUG("Provider %s loaded from static list", pProvider->pszId);
                break;
            }
        }
    }

    if (!pfnInitProvider)
    {
        /* Try to load the provider dynamically */
        if (LW_IS_NULL_OR_EMPTY_STR(pProvider->pszProviderLibpath))
        {
            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        pszProviderLibpath = pProvider->pszProviderLibpath;

        dlerror();
        pProvider->pLibHandle = dlopen(pszProviderLibpath, RTLD_NOW | RTLD_LOCAL);
        if (!pProvider->pLibHandle)
        {
            LSA_LOG_ERROR("Failed to open auth provider at path '%s'", pszProviderLibpath);

            pszError = dlerror();
            if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
            {
                LSA_LOG_ERROR("%s", pszError);
            }

            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dlerror();
        pfnInitProvider = (PFNINITIALIZEPROVIDER) dlsym(
            pProvider->pLibHandle,
            LSA_SYMBOL_NAME_INITIALIZE_PROVIDER);
        if (!pfnInitProvider)
        {
            LSA_LOG_ERROR("Ignoring invalid auth provider at path '%s'", pszProviderLibpath);

            pszError = dlerror();
            if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
            {
                LSA_LOG_ERROR("%s", pszError);
            }

            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = pfnInitProvider(
                    &pProvider->pszName,
                    &pProvider->pFnTable);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvValidateProvider(pProvider);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvInitAuthProviders(
    IN OPTIONAL PLSA_STATIC_PROVIDER pStaticProviders
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pUninitializedProviderList = NULL;
    PLSA_AUTH_PROVIDER pProviderList = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;

    dwError = LsaSrvAuthProviderAllocateProviderList(
                &pUninitializedProviderList);
    BAIL_ON_LSA_ERROR(dwError);

    while(pUninitializedProviderList)
    {
        pProvider = pUninitializedProviderList;
        pUninitializedProviderList = pUninitializedProviderList->pNext;
        pProvider->pNext = NULL;

        dwError = LsaSrvInitAuthProvider(pProvider, pStaticProviders);
        if (dwError)
        {
            LSA_LOG_ERROR("Failed to load provider '%s' from '%s' - error %u (%s)",
                LSA_SAFE_LOG_STRING(pProvider->pszId),
                LSA_SAFE_LOG_STRING(pProvider->pszProviderLibpath),
                dwError,
                LwWin32ExtErrorToName(dwError));

            LsaSrvFreeAuthProvider(pProvider);
            pProvider = NULL;
            dwError = 0;
        }
        else
        {
            LsaSrvAppendAuthProviderList(&pProviderList, pProvider);
            pProvider = NULL;
        }
    }

    ENTER_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);

    LsaSrvFreeAuthProviderList(gpAuthProviderList);

    gpAuthProviderList = pProviderList;
    pProviderList = NULL;

    LEAVE_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);

cleanup:

    if (pUninitializedProviderList)
    {
        LsaSrvFreeAuthProviderList(pUninitializedProviderList);
    }

    return dwError;

error:

    if (pProviderList) {
        LsaSrvFreeAuthProviderList(pProviderList);
    }

    goto cleanup;
}

static
DWORD
LsaSrvAuthLoadProvider(
    PCSTR   pszProviderName,
    PCSTR   pszProviderKey,
    PLSA_AUTH_PROVIDER *ppProvider
    )
{
    DWORD dwError = 0;
    PSTR pszId = NULL;
    PSTR pszPath = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    LWREG_CONFIG_ITEM Config[] =
    {
        {
           "Id",
           FALSE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pszId,
           NULL
        },
        {
           "Path",
           FALSE,
           LwRegTypeString,
           0,
           MAXDWORD,
           NULL,
           &pszPath,
           NULL
        },
    };
    dwError = RegProcessConfig(
                pszProviderKey,
                pszProviderKey,
                Config,
                sizeof(Config)/sizeof(Config[0]));
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszId))
    {
        goto error;
    }
    if (LW_IS_NULL_OR_EMPTY_STR(pszPath))
    {
        goto error;
    }

    dwError = LsaSrvAllocateProvider(pszId, pszPath, &pProvider);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    *ppProvider = pProvider;

    LW_SAFE_FREE_STRING(pszId);
    LW_SAFE_FREE_STRING(pszPath);

    return dwError;

error:

    if (pProvider)
    {
        LsaSrvFreeAuthProvider(pProvider);
        pProvider = NULL;
    }

    goto cleanup;
}

static
DWORD
LsaSrvAuthProviderAllocateProviderList(
    PLSA_AUTH_PROVIDER *ppProviderList
    )
{
    DWORD dwError = 0;
    PSTR pszLoadOrder = NULL; // Multistring
    PCSTR pszProvider = NULL;
    PSTR pszProviderKey = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_AUTH_PROVIDER pProviderList = NULL;

    dwError = LsaSrvAuthProviderRegGetLoadOrder(&pszLoadOrder);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pszLoadOrder)
    {
        /* We should only get here if there is some problem with the
         * registry -- can't access it, the key isn't there, ...
         * -- so we will try a default set of providers.
         */
        LSA_LOG_ERROR("Problem accessing provider configuration in registry. Trying compiled defaults [ActiveDirectory, Local].");

        dwError = LsaSrvAllocateProvider(
                    LSA_PROVIDER_TAG_AD,
                    LSA_PROVIDER_PATH_AD,
                    &pProvider);
        BAIL_ON_LSA_ERROR(dwError);

        if (pProvider)
        {
            LsaSrvAppendAuthProviderList(&pProviderList, pProvider);
            pProvider = NULL;
        }

        dwError = LsaSrvAllocateProvider(
                    LSA_PROVIDER_TAG_LOCAL,
                    LSA_PROVIDER_PATH_LOCAL,
                    &pProvider);
        BAIL_ON_LSA_ERROR(dwError);

        if (pProvider)
        {
            LsaSrvAppendAuthProviderList(&pProviderList, pProvider);
            pProvider = NULL;
        }
    }
    else
    {
        pszProvider = pszLoadOrder;
        while ( pszProvider != NULL && *pszProvider != '\0' )
        {
            dwError = LwAllocateStringPrintf(
                        &pszProviderKey,
                        "Services\\lsass\\Parameters\\Providers\\%s",
                        pszProvider);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaSrvAuthLoadProvider(
                        pszProvider,
                        pszProviderKey,
                        &pProvider);
            BAIL_ON_LSA_ERROR(dwError);

            if (pProvider)
            {
                LsaSrvAppendAuthProviderList(&pProviderList, pProvider);
                pProvider = NULL;
            }

            LW_SAFE_FREE_STRING(pszProviderKey);
            pszProvider = pszProvider + strlen(pszProvider) + 1;
        }
    }

cleanup:

    *ppProviderList = pProviderList;

    LW_SAFE_FREE_MEMORY(pszLoadOrder);
    LW_SAFE_FREE_STRING(pszProviderKey);

    return dwError;

error:

    LsaSrvFreeAuthProviderList(pProviderList);
    pProviderList = NULL;

    goto cleanup;
}

static
DWORD
LsaSrvAuthProviderRegGetLoadOrder(
    PSTR *ppszLoadOrder
    )
{
    DWORD dwError = 0;
    PSTR pszLoadOrder = NULL;
    LWREG_CONFIG_ITEM Config[] =
    {
        {
           "LoadOrder",
           FALSE,
           LwRegTypeMultiString,
           0,
           MAXDWORD,
           NULL,
           &pszLoadOrder,
           NULL
        },
    };

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\Providers",
                "Policy\\Services\\lsass\\Parameters\\Providers",
                Config,
                sizeof(Config)/sizeof(Config[0]));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    *ppszLoadOrder = pszLoadOrder;

    return dwError;

error:

    if (pszLoadOrder)
    {
        LW_SAFE_FREE_MEMORY(pszLoadOrder);
        pszLoadOrder = NULL;
    }
    goto cleanup;
}

VOID
LsaSrvFreeAuthProviders(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);

    LsaSrvFreeAuthProviderList(gpAuthProviderList);
    gpAuthProviderList = NULL;

    LEAVE_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);
}

DWORD
LsaGetNumberOfProviders_inlock(
    VOID
    )
{
    DWORD dwCount = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwCount++;
    }

    return dwCount;
}

static
DWORD
LsaSrvAllocateProvider(
    PCSTR pszId,
    PCSTR pszPath,
    PLSA_AUTH_PROVIDER *ppProvider
    )
{
    DWORD dwError = 0;

    PLSA_AUTH_PROVIDER pProvider = NULL;

    dwError = LwAllocateMemory(
                sizeof(LSA_AUTH_PROVIDER),
                (PVOID*)&pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszPath, &(pProvider->pszProviderLibpath));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszId, &(pProvider->pszId));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    *ppProvider = pProvider;
    return dwError;

error:

    if (pProvider)
    {
        LsaSrvFreeAuthProvider(pProvider);
        pProvider = NULL;
    }
    goto cleanup;
}

static
VOID
LsaSrvAppendAuthProviderList(
    PLSA_AUTH_PROVIDER *ppProviderList,
    PLSA_AUTH_PROVIDER pProvider
    )
{
    if (ppProviderList)
    {
        if (!*ppProviderList)
        {
            *ppProviderList = pProvider;
        }
        else
        {
            PLSA_AUTH_PROVIDER pCurrent = *ppProviderList;
            while (pCurrent->pNext)
            {
                pCurrent = pCurrent->pNext;
            }
            pCurrent->pNext = pProvider;
        }
    }
}

static
VOID
LsaSrvFreeAuthProviderList(
    PLSA_AUTH_PROVIDER pProviderList
    )
{
    PLSA_AUTH_PROVIDER pProvider = NULL;

    while (pProviderList) {
        pProvider = pProviderList;
        pProviderList = pProviderList->pNext;
        LsaSrvFreeAuthProvider(pProvider);
    }
}
