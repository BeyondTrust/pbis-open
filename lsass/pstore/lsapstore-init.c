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
 *     lsapstore-init.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Initialization code
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"

typedef struct _LSA_PSTORE_PLUGIN_INFO {
    PSTR Name;
    PSTR Path;
    PVOID LibraryHandle;
    PLSA_PSTORE_PLUGIN_DISPATCH Dispatch;
    PLSA_PSTORE_PLUGIN_CONTEXT Context;
} LSA_PSTORE_PLUGIN_INFO, *PLSA_PSTORE_PLUGIN_INFO;

typedef struct _LSA_PSTORE_PLUGIN_LIST {
    DWORD Count;
    LSA_PSTORE_PLUGIN_INFO PluginInfo[1];
} LSA_PSTORE_PLUGIN_LIST, *PLSA_PSTORE_PLUGIN_LIST;

typedef struct _LSA_PSTORE_STATE {
    struct {
        pthread_once_t OnceControl;
        DWORD Error;
        BOOLEAN Done;
    } Init;
    LONG RefCount;
    PLSA_PSTORE_PLUGIN_LIST Plugins;
    BOOLEAN NeedClose;
} LSA_PSTORE_STATE, *PLSA_PSTORE_STATE;

static LSA_PSTORE_STATE LsaPstoreState = { { ONCE_INIT } };

//
// Prototypes
//

static
VOID
LsaPstorepCleanupLibraryInternal(
    VOID
    );

//
// Functions
//

__attribute__((constructor))
VOID
LsaPstoreInitializeLibrary(
    VOID
    )
{
    LwInterlockedIncrement(&LsaPstoreState.RefCount);
}

__attribute__((destructor))
VOID
LsaPstoreCleanupLibrary(
    VOID
    )
{
    if (0 == LwInterlockedDecrement(&LsaPstoreState.RefCount))
    {
        LsaPstorepCleanupLibraryInternal();
    }
}

static
DWORD
LsaPstorepGetPluginNames(
    OUT PSTR** Names,
    OUT PDWORD Count
    )
{
    DWORD dwError = 0;
    int EE = 0;
    HANDLE registryConnection = NULL;
    HKEY keyHandle = NULL;
    PSTR* loadOrder = NULL;
    DWORD loadOrderCount = 0;

    dwError = LwRegOpenServer(&registryConnection);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    registryConnection,
                    NULL,
                    LSA_PSTORE_REG_KEY_PATH_PLUGINS,
                    0,
                    GENERIC_READ,
                    &keyHandle);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        GOTO_CLEANUP_EE(EE);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetMultiStringA(
                    registryConnection,
                    keyHandle,
                    LSA_PSTORE_REG_VALUE_NAME_PLUGINS_LOAD_ORDER,
                    &loadOrder,
                    &loadOrderCount);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_STRING_ARRAY_A(&loadOrder, &loadOrderCount);
    }

    *Names = loadOrder;
    *Count = loadOrderCount;

    return dwError;
}

static
DWORD
LsaPstorepGetPluginPath(
    IN PCSTR pszName,
    OUT PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    int EE = 0;
    HANDLE registryConnection = NULL;
    HKEY keyHandle = NULL;
    PSTR pszKeyPath = NULL;
    PSTR pszPath = NULL;

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocatePrintf(
                    &pszKeyPath,
                    "%s\\%s",
                    LSA_PSTORE_REG_KEY_PATH_PLUGINS,
                    pszName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenServer(&registryConnection);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    registryConnection,
                    NULL,
                    pszKeyPath,
                    0,
                    GENERIC_READ,
                    &keyHandle);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        LW_RTL_LOG_ERROR("LSA pstore plugin '%s' is missing its configuration registry key '%s'",
                pszName, pszKeyPath);
        dwError = ERROR_DLL_INIT_FAILED;
        GOTO_CLEANUP_EE(EE);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    registryConnection,
                    keyHandle,
                    LSA_PSTORE_REG_VALUE_NAME_PLUGINS_PATH,
                    &pszPath);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        LW_RTL_LOG_ERROR("LSA pstore plugin '%s' is missing the '%s' configuration value from its configuration registry key '%s'",
                pszName, LSA_PSTORE_REG_VALUE_NAME_PLUGINS_PATH, pszKeyPath);
        dwError = ERROR_DLL_INIT_FAILED;
        GOTO_CLEANUP_EE(EE);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE(&pszPath);
    }

    if (keyHandle)
    {
        LwRegCloseKey(registryConnection, keyHandle);
    }

    if (registryConnection)
    {
        LwRegCloseServer(registryConnection);
    }

    LSA_PSTORE_FREE(&pszKeyPath);

    *ppszPath = pszPath;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
VOID
LsaPstorepCleanupPlugin(
    IN PLSA_PSTORE_PLUGIN_INFO PluginInfo
    )
{
    if (PluginInfo->Dispatch && PluginInfo->Dispatch->Cleanup)
    {
        PluginInfo->Dispatch->Cleanup(PluginInfo->Context);
    }

    PluginInfo->Dispatch = NULL;
    PluginInfo->Context = NULL;
    
    if (PluginInfo->LibraryHandle)
    {
        LsaPstorepClosePlugin(PluginInfo->LibraryHandle);
        PluginInfo->LibraryHandle = NULL;
    }

    LSA_PSTORE_FREE(&PluginInfo->Name);
    LSA_PSTORE_FREE(&PluginInfo->Path);
}

static
DWORD
LsaPstorepInitializePlugin(
    OUT PLSA_PSTORE_PLUGIN_INFO PluginInfo,
    IN PCSTR PluginName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    LSA_PSTORE_PLUGIN_INITIALIZE_FUNCTION initFunction = NULL;

    dwError = LwNtStatusToWin32Error(LwRtlCStringDuplicate(&PluginInfo->Name, PluginName));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepGetPluginPath(PluginInfo->Name, &PluginInfo->Path);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    
    dwError = LsaPstorepOpenPlugin(
                    PluginInfo->Path,
                    LSA_PSTORE_PLUGIN_INITIALIZE_FUNCTION_NAME,
                    &PluginInfo->LibraryHandle,
                    OUT_PPVOID(&initFunction));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = initFunction(LSA_PSTORE_PLUGIN_VERSION,
                           PluginInfo->Name,
                           &PluginInfo->Dispatch,
                           &PluginInfo->Context);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (!PluginInfo->Dispatch)
    {
        LW_RTL_LOG_ERROR("LSA pstore plugin %s is missing a dispatch table",
                         PluginInfo->Name);
        dwError = ERROR_DLL_INIT_FAILED;
        GOTO_CLEANUP_EE(EE);
    }

    if (!PluginInfo->Dispatch->Cleanup)
    {
        LW_RTL_LOG_ERROR("LSA pstore plugin %s is missing the Cleanup function",
                         PluginInfo->Name);
        dwError = ERROR_DLL_INIT_FAILED;
        GOTO_CLEANUP_EE(EE);
    }

    LW_RTL_LOG_VERBOSE("Loaded LSA pstore plugin %s from %s",
                       PluginInfo->Name, PluginInfo->Path);

cleanup:
    if (dwError)
    {
        LsaPstorepCleanupPlugin(PluginInfo);
    }

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
VOID
LsaPstorepDestroyPluginList(
    IN PLSA_PSTORE_PLUGIN_LIST PluginList
    )
{
    if (PluginList)
    {
        DWORD i = 0;
        for (i = 0; i < PluginList->Count; i++)
        {
            LsaPstorepCleanupPlugin(&PluginList->PluginInfo[i]);
        }
        LSA_PSTORE_FREE(&PluginList);
    }
}

static
DWORD
LsaPstorepCreatePluginList(
    OUT PLSA_PSTORE_PLUGIN_LIST* PluginList,
    IN PSTR* Names,
    IN DWORD Count
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_PLUGIN_LIST pluginList = NULL;
    DWORD i = 0;

    dwError = LSA_PSTORE_ALLOCATE(
                    OUT_PPVOID(&pluginList),
                    (LW_FIELD_OFFSET(LSA_PSTORE_PLUGIN_LIST, PluginInfo) +
                     (Count * sizeof(*pluginList))));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    for (i = 0; i < Count; i++)
    {
        dwError = LsaPstorepInitializePlugin(
                        &pluginList->PluginInfo[pluginList->Count],
                        Names[i]);
        if (dwError)
        {
            LW_RTL_LOG_ERROR("Failed to load plugin %s with error = %u (%s)",
                    Names[i], dwError,
                    LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError)));
            dwError = 0;
        }

        pluginList->Count++;
    }

cleanup:
    if (dwError)
    {
        LsaPstorepDestroyPluginList(pluginList);
        pluginList = NULL;
    }

    *PluginList = pluginList;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
LsaPstorepLoadPlugins(
    OUT PLSA_PSTORE_PLUGIN_LIST* PluginList
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_PLUGIN_LIST pluginList = NULL;
    PSTR* names = NULL;
    DWORD count = 0;

    dwError = LsaPstorepGetPluginNames(&names, &count);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (count)
    {
        dwError = LsaPstorepCreatePluginList(&pluginList, names, count);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        LW_RTL_LOG_DEBUG("No LSA pstore plugins are configured.");
    }

cleanup:
    if (dwError)
    {
        LsaPstorepDestroyPluginList(pluginList);
        pluginList = NULL;
    }

    LSA_PSTORE_FREE_STRING_ARRAY_A(&names, &count);

    *PluginList = pluginList;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
LsaPstorepInitializeLibraryInternal(
    VOID
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;

    if (pState->Init.Done)
    {
        LwInterlockedIncrement(&pState->RefCount);

        dwError = pState->Init.Error;
        GOTO_CLEANUP_EE(EE);
    }

    dwError = LsaPstorepLoadPlugins(&pState->Plugins);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepBackendInitialize();
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    pState->NeedClose = TRUE;

cleanup:
    if (dwError)
    {
        LsaPstorepCleanupLibraryInternal();
    }

    pState->Init.Error = dwError;
    pState->Init.Done = TRUE;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
VOID
LsaPstorepCleanupLibraryInternal(
    VOID
    )
{
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    pthread_once_t onceControl = ONCE_INIT;

    if (pState->NeedClose)
    {
        LsaPstorepBackendCleanup();
        pState->NeedClose = FALSE;
    }

    LsaPstorepDestroyPluginList(pState->Plugins),
    pState->Plugins = NULL;

    pState->Init.OnceControl = onceControl;
    pState->Init.Error = 0;
    pState->Init.Done = FALSE;
}

static
VOID
LsaPstoreInitializeLibraryOnce(
    VOID
    )
{
    LsaPstorepInitializeLibraryInternal();
}

DWORD
LsaPstorepEnsureInitialized(
    VOID
    )
{
    DWORD dwError = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;

    if (pState->RefCount)
    {
        pthread_once(&pState->Init.OnceControl, LsaPstoreInitializeLibraryOnce);
        dwError = pState->Init.Error;
    }
    else
    {
        dwError = ERROR_DLL_INIT_FAILED;
    }

    LSA_PSTORE_LOG_LEAVE_ERROR(dwError);
    return dwError;
}

DWORD
LsaPstorepCallPluginSetPasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    DWORD i = 0;
    DWORD count = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfoA = NULL;

    count = pState->Plugins ? pState->Plugins->Count : 0;

    for (i = 0; i < count; i++)
    {
        PLSA_PSTORE_PLUGIN_INFO pluginInfo = &pState->Plugins->PluginInfo[i];

        if (pluginInfo->Dispatch->SetPasswordInfoW)
        {
            dwError = pluginInfo->Dispatch->SetPasswordInfoW(
                            pluginInfo->Context,
                            pPasswordInfo);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        }
        else if (pluginInfo->Dispatch->SetPasswordInfoA)
        {
            dwError = LsaPstorepConvertWideToAnsiPasswordInfo(
                            pPasswordInfo,
                            &pPasswordInfoA);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

            dwError = pluginInfo->Dispatch->SetPasswordInfoA(
                            pluginInfo->Context,
                            pPasswordInfoA);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        }
    }

cleanup:
    LSA_PSTORE_FREE_PASSWORD_INFO_A(&pPasswordInfoA);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepCallPluginDeletePasswordInfo(
    IN OPTIONAL PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    DWORD i = 0;
    DWORD count = 0;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfoA = NULL;

    count = pState->Plugins ? pState->Plugins->Count : 0;

    for (i = 0; i < count; i++)
    {
        PLSA_PSTORE_PLUGIN_INFO pluginInfo = &pState->Plugins->PluginInfo[i];

        if (pluginInfo->Dispatch->DeletePasswordInfoW)
        {
            dwError = pluginInfo->Dispatch->DeletePasswordInfoW(
                            pluginInfo->Context,
                            pAccountInfo);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        }
        else if (pluginInfo->Dispatch->SetPasswordInfoA)
        {
            if (pAccountInfo)
            {
                dwError = LsaPstorepConvertWideToAnsiAccountInfo(
                                pAccountInfo,
                                &pAccountInfoA);
                GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
            }

            dwError = pluginInfo->Dispatch->DeletePasswordInfoA(
                            pluginInfo->Context,
                            pAccountInfoA);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        }
    }

cleanup:
    if (pAccountInfoA)
    {
        LSA_PSTOREP_FREE_ACCOUNT_INFO_A(&pAccountInfoA);
    }

    LSA_PSTORE_LOG_LEAVE_ERROR(dwError);
    return dwError;
}
