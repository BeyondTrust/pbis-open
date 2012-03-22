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
 *     Code to call plugins
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

typedef struct _LSA_PSTORE_CALL_PLUGIN_ARGS {
    union {
        PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo;
        PLSA_MACHINE_ACCOUNT_INFO_W AccountInfo;
    };
} LSA_PSTORE_CALL_PLUGIN_ARGS, *PLSA_PSTORE_CALL_PLUGIN_ARGS;

typedef DWORD (*LSA_PSTORE_CALL_PLUGIN_CALLBACK)(
    IN PCSTR PluginName,
    IN PLSA_PSTORE_PLUGIN_DISPATCH Dispatch,
    IN PLSA_PSTORE_PLUGIN_CONTEXT Context,
    IN PLSA_PSTORE_CALL_PLUGIN_ARGS Arguments,
    OUT PCSTR* Method
    );


//
// Functions
//

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
DWORD
LsaPstorepCallPlugin(
    IN PCSTR Operation,
    IN LSA_PSTORE_CALL_PLUGIN_CALLBACK Callback,
    IN PLSA_PSTORE_CALL_PLUGIN_ARGS Arguments
    )
{
    DWORD dwError = 0;
    int EE = 0;
    LSA_PSTORE_PLUGIN_INFO pluginInfo = { 0 };
    PSTR* pluginNames = 0;
    DWORD pluginCount = 0;
    DWORD i = 0;
    PCSTR method = NULL;

    dwError = LsaPstorepGetPluginNames(&pluginNames, &pluginCount);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    for (i = 0; i < pluginCount; i++)
    {
        LsaPstorepCleanupPlugin(&pluginInfo);

        dwError = LsaPstorepInitializePlugin(&pluginInfo, pluginNames[i]);
        if (dwError)
        {
            LW_RTL_LOG_ERROR("Failed to load plugin %s with error = %u (%s)",
                    pluginNames[i], dwError,
                    LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError)));
            dwError = 0;
            continue;
        }

        dwError = Callback(
                        pluginInfo.Name,
                        pluginInfo.Dispatch,
                        pluginInfo.Context,
                        Arguments,
                        &method);
        if (dwError)
        {
            if (method)
            {
                LW_RTL_LOG_ERROR(
                        "Failed %s operation on plugin %s "
                        "while calling %s method with error = %u (%s)",
                        LW_RTL_LOG_SAFE_STRING(Operation), pluginNames[i],
                        method, dwError,
                        LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError)));
            }
            else
            {
                LW_RTL_LOG_ERROR(
                        "Failed %s operation on plugin %s "
                        "with error = %u (%s)",
                        LW_RTL_LOG_SAFE_STRING(Operation), pluginNames[i],
                        dwError,
                        LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError)));
            }
            dwError = 0;
            continue;
        }
    }

cleanup:
    LsaPstorepCleanupPlugin(&pluginInfo);

    LSA_PSTORE_FREE_STRING_ARRAY_A(&pluginNames, &pluginCount);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
LsaPstorepCallPluginSetPasswordInfoCallback(
    IN PCSTR PluginName,
    IN PLSA_PSTORE_PLUGIN_DISPATCH Dispatch,
    IN PLSA_PSTORE_PLUGIN_CONTEXT Context,
    IN PLSA_PSTORE_CALL_PLUGIN_ARGS Arguments,
    OUT PCSTR* Method
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PCSTR method = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = Arguments->PasswordInfo;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfoA = NULL;

    if (Dispatch->SetPasswordInfoW)
    {
        method = "SetPasswordInfoW";
        dwError = Dispatch->SetPasswordInfoW(Context, pPasswordInfo);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else if (Dispatch->SetPasswordInfoA)
    {
        dwError = LsaPstorepConvertWideToAnsiPasswordInfo(
                        pPasswordInfo,
                        &pPasswordInfoA);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        method = "SetPasswordInfoA";
        dwError = Dispatch->SetPasswordInfoA(Context, pPasswordInfoA);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:    
    LSA_PSTORE_FREE_PASSWORD_INFO_A(&pPasswordInfoA);

    *Method = method;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
LsaPstorepCallPluginDeletePasswordInfoCallback(
    IN PCSTR PluginName,
    IN PLSA_PSTORE_PLUGIN_DISPATCH Dispatch,
    IN PLSA_PSTORE_PLUGIN_CONTEXT Context,
    IN PLSA_PSTORE_CALL_PLUGIN_ARGS Arguments,
    OUT PCSTR* Method
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PCSTR method = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo = Arguments->AccountInfo;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfoA = NULL;

    if (Dispatch->DeletePasswordInfoW)
    {
        method = "DeletePasswordInfoW";
        dwError = Dispatch->DeletePasswordInfoW(Context, pAccountInfo);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else if (Dispatch->DeletePasswordInfoA)
    {
        if (pAccountInfo)
        {
            dwError = LsaPstorepConvertWideToAnsiAccountInfo(
                            pAccountInfo,
                            &pAccountInfoA);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        }

        method = "DeletePasswordInfoA";
        dwError = Dispatch->DeletePasswordInfoA(Context, pAccountInfoA);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:    
    LSA_PSTOREP_FREE_ACCOUNT_INFO_A(&pAccountInfoA);

    *Method = method;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorepCallPluginSetPasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    LSA_PSTORE_CALL_PLUGIN_ARGS args = { { 0 } };

    args.PasswordInfo = pPasswordInfo;

    dwError = LsaPstorepCallPlugin(
                    "set password",
                    LsaPstorepCallPluginSetPasswordInfoCallback,
                    &args);

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
    LSA_PSTORE_CALL_PLUGIN_ARGS args = { { 0 } };

    args.AccountInfo = pAccountInfo;

    dwError = LsaPstorepCallPlugin(
                    "delete password",
                    LsaPstorepCallPluginDeletePasswordInfoCallback,
                    &args);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}
