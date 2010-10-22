/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        loader.c
 *
 * Abstract:
 *
 *        Service loader functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwSmLoaderLoadPlugin(
    PCSTR pszPath
    );

typedef struct _SM_PLUGIN
{
    PVOID hLibrary;
    PLW_SERVICE_LOADER_PLUGIN pPlugin;
    SM_LINK link;
} SM_PLUGIN, *PSM_PLUGIN;

static SM_LINK gPluginList =
{
    .pPrev = &gPluginList,
    .pNext = &gPluginList
};

static PSM_LOADER_CALLS gpCalls = NULL;

PVOID
LwSmGetServiceObjectData(
    PLW_SERVICE_OBJECT pObject
    )
{
    return gpCalls->pfnGetServiceObjectData(pObject);
}

VOID
LwSmRetainServiceObject(
    PLW_SERVICE_OBJECT pObject
    )
{
    gpCalls->pfnRetainServiceObject(pObject);
}

VOID
LwSmReleaseServiceObject(
    PLW_SERVICE_OBJECT pObject
    )
{
    gpCalls->pfnReleaseServiceObject(pObject);
}

VOID
LwSmNotifyServiceObjectStateChange(
    PLW_SERVICE_OBJECT pObject,
    LW_SERVICE_STATE newState
    )
{
    gpCalls->pfnNotifyServiceObjectStateChange(pObject, newState);
}

DWORD
LwSmLoaderInitialize(
    PSM_LOADER_CALLS pCalls
    )
{
    DWORD dwError = 0;
    DIR* pLoaderDir = NULL;
    struct dirent* pEntry = NULL;
    size_t len = 0;
    size_t modLen = strlen(MOD_EXT);
    PSTR pszPath = NULL;

    if ((pLoaderDir = opendir(LOADERDIR)) == NULL)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    while ((pEntry = readdir(pLoaderDir)) != NULL)
    {
        len = strlen(pEntry->d_name);

        if (len >= modLen &&
            !strcmp(pEntry->d_name + len - modLen, MOD_EXT))
        {
            dwError = LwAllocateStringPrintf(
                &pszPath,
                "%s/%s",
                LOADERDIR,
                pEntry->d_name);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmLoaderLoadPlugin(pszPath);
            BAIL_ON_ERROR(dwError);

            LW_SAFE_FREE_MEMORY(pszPath);
        }
    }

    gpCalls = pCalls;

error:

    if (pLoaderDir)
    {
        closedir(pLoaderDir);
    }
    
    LW_SAFE_FREE_MEMORY(pszPath);

    return dwError;
}

static
DWORD
LwSmLoaderLoadPlugin(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;
    PSM_PLUGIN pPlugin = NULL;
    DWORD (*pfnInit)(DWORD dwInterfaceVersion, PLW_SERVICE_LOADER_PLUGIN* ppPlugin);

    dwError = LwAllocateMemory(sizeof(*pPlugin), OUT_PPVOID(&pPlugin));
    BAIL_ON_ERROR(dwError);

    LwSmLinkInit(&pPlugin->link);

    (void) dlerror();
    if ((pPlugin->hLibrary = dlopen(pszPath, RTLD_NOW | RTLD_LOCAL)) == NULL)
    {
        fprintf(stderr, "Could not load %s: %s\n", pszPath, dlerror());
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
    }

    (void) dlerror();
    if ((pfnInit = dlsym(pPlugin->hLibrary, "ServiceLoaderInit")) == NULL)
    {
        fprintf(stderr, "Could not load symbol ServiceLoaderInit: %s\n", dlerror());
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
    }

    dwError = pfnInit(LW_SERVICE_LOADER_INTERFACE_VERSION, &pPlugin->pPlugin);
    BAIL_ON_ERROR(dwError);

    LwSmLinkInsertBefore(&gPluginList, &pPlugin->link);

cleanup:

    return dwError;

error:

    if (pPlugin)
    {
        if (pPlugin->hLibrary)
        {
            dlclose(pPlugin->hLibrary);
        }

        LwFreeMemory(pPlugin);
    }

    goto cleanup;
}

DWORD
LwSmLoaderGetVtbl(
    PCWSTR pwszLoaderName,
    PLW_SERVICE_LOADER_VTBL* ppVtbl
    )
{
    DWORD dwError = 0;
    PSM_LINK pLink = NULL;
    PSM_PLUGIN pPlugin = NULL;
    PSTR pszLoaderName = NULL;

    dwError = LwWc16sToMbs(pwszLoaderName, &pszLoaderName);
    BAIL_ON_ERROR(dwError);

    while ((pLink = SM_LINK_ITERATE(&gPluginList, pLink)))
    {
        pPlugin = STRUCT_FROM_MEMBER(pLink, SM_PLUGIN, link);

        if (!strcmp(pPlugin->pPlugin->pszName, pszLoaderName))
        {
            *ppVtbl = pPlugin->pPlugin->pVtbl;
            goto error;
        }
    }

    dwError = LW_ERROR_INTERNAL;
    BAIL_ON_ERROR(dwError);

error:

    LW_SAFE_FREE_MEMORY(pszLoaderName);

    return dwError;
}

VOID
LwSmLoaderShutdown(
    VOID
    )
{
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_PLUGIN pPlugin = NULL;

    for (pLink = LwSmLinkBegin(&gPluginList);
         LwSmLinkValid(&gPluginList, pLink);
         pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pPlugin = STRUCT_FROM_MEMBER(pLink, SM_PLUGIN, link);

        if (pPlugin->pPlugin && pPlugin->pPlugin->pVtbl->pfnShutdown)
        {
            pPlugin->pPlugin->pVtbl->pfnShutdown(pPlugin->pPlugin);
        }

        if (pPlugin->hLibrary)
        {
            dlclose(pPlugin->hLibrary);
        }

        LwFreeMemory(pPlugin);
    }
}
