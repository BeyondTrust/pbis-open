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

typedef struct _LSA_PSTORE_STATE {
    struct {
        pthread_once_t OnceControl;
        DWORD Error;
        BOOLEAN Done;
    } Init;

    LONG RefCount;
    
    struct {
        PSTR Path;
        PVOID Handle;
        PLSA_PSTORE_PLUGIN_DISPATCH Dispatch;
        PLSA_PSTORE_PLUGIN_CONTEXT Context;
    } Plugin;

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
LsaPstorepGetPluginPath(
    OUT PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    int EE = 0;
    HANDLE registryConnection = NULL;
    HKEY keyHandle = NULL;
    PSTR pszPath = NULL;

    dwError = LwRegOpenServer(&registryConnection);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    registryConnection,
                    NULL,
                    LSA_PSTORE_CONFIG_KEY_PATH,
                    0,
                    GENERIC_READ,
                    &keyHandle);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
        GOTO_CLEANUP_EE(EE);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    registryConnection,
                    keyHandle,
                    LSA_PSTORE_CONFIG_VALUE_PLUGIN_PATH,
                    &pszPath);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = 0;
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

    *ppszPath = pszPath;

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
    LSA_PSTORE_PLUGIN_INITIALIZE_FUNCTION initFunction = NULL;

    if (pState->Init.Done)
    {
        LwInterlockedIncrement(&LsaPstoreState.RefCount);

        dwError = pState->Init.Error;
        GOTO_CLEANUP_EE(EE);
    }

    dwError = LsaPstorepGetPluginPath(&pState->Plugin.Path);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (!LwRtlCStringIsNullOrEmpty(pState->Plugin.Path))
    {
        dwError = LsaPstorepOpenPlugin(
                        pState->Plugin.Path,
                        LSA_PSTORE_PLUGIN_INITIALIZE_FUNCTION_NAME,
                        &pState->Plugin.Handle,
                        OUT_PPVOID(&initFunction));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = initFunction(LSA_PSTORE_PLUGIN_VERSION,
                               &pState->Plugin.Dispatch,
                               &pState->Plugin.Context);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        LW_RTL_LOG_VERBOSE("Loaded LSA pstore plugin at %s", pState->Plugin.Path);
    }
    else
    {
        LW_RTL_LOG_VERBOSE("No LSA pstore plugin defined.");
    }

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

    if (pState->Plugin.Dispatch)
    {
        pState->Plugin.Dispatch->Cleanup(pState->Plugin.Context);
        pState->Plugin.Dispatch = NULL;
    }

    pState->Plugin.Context = NULL;

    if (pState->Plugin.Handle)
    {
        LsaPstorepClosePlugin(pState->Plugin.Handle);
        pState->Plugin.Handle = NULL;
    }

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

    if (LsaPstoreState.RefCount)
    {
        pthread_once(&LsaPstoreState.Init.OnceControl, LsaPstoreInitializeLibraryOnce);
        dwError = LsaPstoreState.Init.Error;
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
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfoA = NULL;

    if (LsaPstoreState.Plugin.Dispatch)
    {
        if (LsaPstoreState.Plugin.Dispatch->SetPasswordInfoW)
        {
            dwError = LsaPstoreState.Plugin.Dispatch->SetPasswordInfoW(
                            LsaPstoreState.Plugin.Context,
                            pPasswordInfo);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        }
        else if (LsaPstoreState.Plugin.Dispatch->SetPasswordInfoA)
        {
            dwError = LsaPstorepConvertWideToAnsiPasswordInfo(
                            pPasswordInfo,
                            &pPasswordInfoA);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

            dwError = LsaPstoreState.Plugin.Dispatch->SetPasswordInfoA(
                            LsaPstoreState.Plugin.Context,
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
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfoA = NULL;

    if (LsaPstoreState.Plugin.Dispatch)
    {
        if (LsaPstoreState.Plugin.Dispatch->DeletePasswordInfoW)
        {
            dwError = LsaPstoreState.Plugin.Dispatch->DeletePasswordInfoW(
                            LsaPstoreState.Plugin.Context,
                            pAccountInfo);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        }
        else if (LsaPstoreState.Plugin.Dispatch->SetPasswordInfoA)
        {
            dwError = LsaPstorepConvertWideToAnsiAccountInfo(
                            pAccountInfo,
                            &pAccountInfoA);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

            dwError = LsaPstoreState.Plugin.Dispatch->DeletePasswordInfoA(
                            LsaPstoreState.Plugin.Context,
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
