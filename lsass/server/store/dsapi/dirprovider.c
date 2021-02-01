/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */



/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        dirprovider.c
 *
 * Abstract:
 *
 *
 *      BeyondTrust Directory Wrapper Interface
 *
 *      Directory Provider Management
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
DirectoryGetProvider(
    PDIRECTORY_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PDIRECTORY_PROVIDER_INFO pProviderInfo = NULL;

    DIRECTORY_LOCK_MUTEX(bInLock, &gDirGlobals.mutex);

    if (!gDirGlobals.pProvider)
    {
        dwError = DirectoryGetProviderInfo(&pProviderInfo);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        dwError = DirectoryLoadProvider(
                        pProviderInfo,
                        &gDirGlobals.pProvider);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        gDirGlobals.pProvider->pProviderInfo = pProviderInfo;

        pProviderInfo = NULL;
    }

    InterlockedIncrement(&gDirGlobals.pProvider->refCount);

    *ppProvider = gDirGlobals.pProvider;

cleanup:

    DIRECTORY_UNLOCK_MUTEX(bInLock, &gDirGlobals.mutex);

    if (pProviderInfo)
    {
        DirectoryFreeProviderInfo(pProviderInfo);
    }

    return dwError;

error:

    *ppProvider = NULL;

    goto cleanup;
}

DWORD
DirectoryGetProviderInfo(
    PDIRECTORY_PROVIDER_INFO* ppProviderInfo
    )
{
    DWORD dwError = 0;
    CHAR  szProviderPath[] = SAM_DB_PROVIDER_PATH;
    PDIRECTORY_PROVIDER_INFO pProviderInfo = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(DIRECTORY_PROVIDER_INFO),
                    (PVOID*)&pProviderInfo);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    pProviderInfo->dirType = LOCAL_SAM;

    dwError = DirectoryAllocateString(
                    &szProviderPath[0],
                    &pProviderInfo->pszProviderPath);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    *ppProviderInfo = pProviderInfo;

cleanup:

    return dwError;

error:

    *ppProviderInfo = NULL;

    if (pProviderInfo)
    {
        DirectoryFreeProviderInfo(pProviderInfo);
    }

    goto cleanup;
}

DWORD
DirectoryLoadProvider(
    PDIRECTORY_PROVIDER_INFO pProviderInfo,
    PDIRECTORY_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    PFNINITIALIZEDIRPROVIDER pfnInitProvider = NULL;
    PDIRECTORY_PROVIDER pProvider = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(DIRECTORY_PROVIDER),
                    (PVOID*)&pProvider);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    pProvider->refCount = 1;

    dlerror();

#ifdef DSAPI_INTERNAL_PROVIDER
    pProvider->pLibHandle = NULL;
    pfnInitProvider = DirectoryInitializeProvider;
    pProvider->pfnShutdown = DirectoryShutdownProvider;
#else
    pProvider->pLibHandle = dlopen(pProviderInfo->pszProviderPath,
        RTLD_NOW | RTLD_LOCAL);
    if (pProvider->pLibHandle == NULL)
    {
        PCSTR pszError = NULL;

        DIRECTORY_LOG_ERROR("Failed to open directory provider at path [%s]",
            pProviderInfo->pszProviderPath);

        pszError = dlerror();
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
        {
            DIRECTORY_LOG_ERROR("%s", pszError);
        }

        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dlerror();
    pfnInitProvider = (PFNINITIALIZEDIRPROVIDER)dlsym(
        pProvider->pLibHandle,
        DIRECTORY_SYMBOL_NAME_INITIALIZE_PROVIDER);
    if (pfnInitProvider == NULL)
    {
        PCSTR pszError = NULL;

        DIRECTORY_LOG_ERROR("Invalid directory provider at path [%s]",
            pProviderInfo->pszProviderPath);

        pszError = dlerror();
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
        {
            DIRECTORY_LOG_ERROR("%s", pszError);
        }

        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dlerror();
    pProvider->pfnShutdown = (PFNSHUTDOWNDIRPROVIDER)dlsym(
        pProvider->pLibHandle,
        DIRECTORY_SYMBOL_NAME_SHUTDOWN_PROVIDER);
    if (pProvider->pfnShutdown == NULL)
    {
        PCSTR pszError = NULL;

        DIRECTORY_LOG_ERROR("Invalid directory provider at path [%s]",
            pProviderInfo->pszProviderPath);

        pszError = dlerror();
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
        {
            DIRECTORY_LOG_ERROR("%s", pszError);
        }

        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }
#endif

    dwError = pfnInitProvider(
                    &pProvider->pszProviderName,
                    &pProvider->pProviderFnTbl);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwError = DirectoryValidateProvider(pProvider);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    *ppProvider = pProvider;

cleanup:

    return dwError;

error:

    *ppProvider = NULL;

    if (pProvider)
    {
        DirectoryReleaseProvider(pProvider);
    }

    goto cleanup;
}

DWORD
DirectoryValidateProvider(
    PDIRECTORY_PROVIDER pProvider
    )
{
    DWORD dwError = 0;

    if (!pProvider ||
        !pProvider->pfnShutdown ||
        !pProvider->pProviderFnTbl ||
        !pProvider->pProviderFnTbl->pfnDirectoryAdd ||
        !pProvider->pProviderFnTbl->pfnDirectoryBind ||
        !pProvider->pProviderFnTbl->pfnDirectoryClose ||
        !pProvider->pProviderFnTbl->pfnDirectoryDelete ||
        !pProvider->pProviderFnTbl->pfnDirectoryModify ||
        !pProvider->pProviderFnTbl->pfnDirectorySetPassword ||
        !pProvider->pProviderFnTbl->pfnDirectoryChangePassword ||
        !pProvider->pProviderFnTbl->pfnDirectoryVerifyPassword ||
        !pProvider->pProviderFnTbl->pfnDirectoryGetGroupMembers ||
        !pProvider->pProviderFnTbl->pfnDirectoryGetMemberships ||
        !pProvider->pProviderFnTbl->pfnDirectoryAddToGroup ||
        !pProvider->pProviderFnTbl->pfnDirectoryRemoveFromGroup ||
        !pProvider->pProviderFnTbl->pfnDirectoryOpen ||
        !pProvider->pProviderFnTbl->pfnDirectorySearch ||
        !pProvider->pProviderFnTbl->pfnDirectoryGetUserCount ||
        !pProvider->pProviderFnTbl->pfnDirectoryGetGroupCount)
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
    }

    return dwError;
}

VOID
DirectoryReleaseProvider(
    PDIRECTORY_PROVIDER pProvider
    )
{
    if (InterlockedDecrement(&pProvider->refCount) == 0)
    {
        DirectoryFreeProvider(pProvider);
    }
}

VOID
DirectoryFreeProvider(
    PDIRECTORY_PROVIDER pProvider
    )
{
    if (pProvider->pLibHandle)
    {
        if (pProvider->pfnShutdown)
        {
            DWORD dwError = 0;

            dwError = pProvider->pfnShutdown(
                            pProvider->pszProviderName,
                            pProvider->pProviderFnTbl);
            if (dwError)
            {
                DIRECTORY_LOG_ERROR("Failed to shutdown provider [Name:%s][code: %u]",
                                    (pProvider->pszProviderName ? pProvider->pszProviderName : ""),
                                    dwError);
            }
        }

        dlclose(pProvider->pLibHandle);
    }

    if (pProvider->pProviderInfo)
    {
        DirectoryFreeProviderInfo(pProvider->pProviderInfo);
    }

    DirectoryFreeMemory(pProvider);
}

VOID
DirectoryFreeProviderInfo(
    PDIRECTORY_PROVIDER_INFO pProviderInfo
    )
{
    if (pProviderInfo->pszProviderPath)
    {
        DirectoryFreeMemory(pProviderInfo->pszProviderPath);
    }

    DirectoryFreeMemory(pProviderInfo);
}
