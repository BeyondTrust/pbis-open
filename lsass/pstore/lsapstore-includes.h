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
 *     lsapstore-includes.h
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Main include file
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "config.h"

#include <lsa/lsapstore-api.h>
#include <lsa/lsapstore-plugin.h>

#include "lsapstore-backend-types.h"

#include <lw/winerror.h>
#include <lw/rtlgoto.h>
#include <lw/rtlmemory.h>
#include <lw/atomic.h>
#include <lw/rtlstring.h>
#include <reg/lwreg.h>
#include <lw/rtllog.h>
#include <lwerror.h>

#include <pthread.h>
#include <assert.h>

#if PTHREAD_ONCE_INIT_BROKEN
#define ONCE_INIT {PTHREAD_ONCE_INIT}
#else
#define ONCE_INIT PTHREAD_ONCE_INIT
#endif

#define LSA_PSTOREP_LOCK(pIsLocked) \
    do { \
        assert(!*(pIsLocked)); \
        LsaPstorepLock(); \
        *(pIsLocked) = TRUE; \
    } while (0)

#define LSA_PSTOREP_UNLOCK(pIsLocked) \
    do { \
        if (*(pIsLocked)) \
        { \
            LsaPstorepUnlock(); \
            *(pIsLocked) = FALSE; \
        } \
    } while (0)

//
// Memory Helpers
//

#define LSA_PSTORE_ALLOCATE(ppMemory, Size) \
    LwNtStatusToWin32Error(LW_RTL_ALLOCATE(OUT_PPVOID(ppMemory), VOID, Size))

#define LSA_PSTORE_ALLOCATE_AUTO(ppMemory) \
    LwNtStatusToWin32Error(LW_RTL_ALLOCATE_AUTO(ppMemory))

#define LSA_PSTORE_FREE_SECURE_CSTRING(pMemory) \
    LW_RTL_MAKE_CUSTOM_FREE_SECURE_STRING(LsaPstoreFreeMemory, pMemory, CHAR)

#define LSA_PSTORE_FREE_SECURE_WC16STRING(pMemory) \
    LW_RTL_MAKE_CUSTOM_FREE_SECURE_STRING(LsaPstoreFreeMemory, pMemory, WCHAR)

//
// Logging
//

#define LSA_PSTORE_LOG_LEAVE_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            LW_RTL_LOG_DEBUG("-> %u (%s)", dwError, LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError))); \
        } \
    } while (0)

#define LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE) \
    do { \
        if ((dwError) || (EE)) \
        { \
            LW_RTL_LOG_DEBUG("-> %u (%s) (EE = %d)", dwError, LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError)), EE); \
        } \
    } while (0)

#define LSA_PSTORE_LOG_LEAVE_ERROR_EE_FMT(dwError, EE, Format, ...) \
    do { \
        if ((dwError) || (EE)) \
        { \
            LW_RTL_LOG_DEBUG(Format " -> %u (%s) (EE = %d)", ## __VA_ARGS__, dwError, LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError)), EE); \
        } \
    } while (0)

// lsapstore-utils.c

#define LSA_PSTOREP_FREE_ACCOUNT_INFO_A(ppAccountInfo) \
    LW_RTL_MAKE_CUSTOM_FREE(LsaPstorepFreeAccountInfoA, ppAccountInfo)

VOID
LsaPstorepFreePasswordInfoContentsW(
    IN OUT PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );

VOID
LsaPstorepFreeAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    );

DWORD
LsaPstorepConvertAnsiToWidePasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    );

DWORD
LsaPstorepConvertWideToAnsiPasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    );

DWORD
LsaPstorepConvertWideToAnsiAccountInfo(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    );

DWORD
LsaPstorepCheckPasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo
    );

PSTR
LsaPstorepCStringDowncase(
    IN OUT PSTR String
    );

PSTR
LsaPstorepCStringUpcase(
    IN OUT PSTR String
    );

PWSTR
LsaPstorepWC16StringDowncase(
    IN OUT PWSTR String
    );

PWSTR
LsaPstorepWC16StringUpcase(
    IN OUT PWSTR String
    );

BOOLEAN
LsaPstorepWC16StringIsDowncase(
    IN PCWSTR String
    );

BOOLEAN
LsaPstorepWC16StringIsUpcase(
    IN PCWSTR String
    );

DWORD
LsaPstorepRegGetDword(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    OUT PDWORD ValueData
    );

DWORD
LsaPstorepRegGetQword(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    OUT PULONG64 ValueData
    );

DWORD
LsaPstorepRegGetStringA(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    OUT PSTR* ValueData
    );

DWORD
LsaPstorepRegGetMultiStringA(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    OUT PSTR** StringArray,
    OUT PDWORD Count
    );

DWORD
LsaPstorepRegSetDword(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    IN DWORD ValueData
    );

DWORD
LsaPstorepRegSetStringA(
    IN HANDLE RegistryConnection,
    IN HKEY KeyHandle,
    IN PCSTR ValueName,
    IN PCSTR ValueData
    );

DWORD
LsaPstorepOpenPlugin(
    IN PCSTR Path,
    IN PCSTR InitFunctionName,
    OUT PVOID* Handle,
    OUT PVOID* InitFunction
    );

VOID
LsaPstorepClosePlugin(
    IN PVOID Handle
    );

// lsapstore-init.c

DWORD
LsaPstorepEnsureInitialized(
    OUT PLSA_PSTORE_BACKEND_STATE* Backend
    );

VOID
LsaPstorepLock(
    VOID
    );

VOID
LsaPstorepUnlock(
    VOID
    );

// lsapstore-plugin.c

DWORD
LsaPstorepCallPluginSetPasswordInfo(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );

DWORD
LsaPstorepCallPluginDeletePasswordInfo(
    IN OPTIONAL PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    );

