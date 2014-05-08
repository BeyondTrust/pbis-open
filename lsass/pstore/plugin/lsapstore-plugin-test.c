/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *  
 * Module Name:
 *
 *     lsapstore-plugin-test.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Test Plugin
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lsa/lsapstore-plugin.h>
#include <lw/rtllog.h>
#include <lw/rtlgoto.h>
#include <lw/errno.h>
#include <lwerror.h>
#include <lwmem.h>
#include <lwstr.h>
#include <unistd.h>
#include <assert.h>

#define _PLUGIN_LOG_RAW(Name, Format, ...) \
    LW_RTL_LOG_DEBUG("PLUGIN(%s) - " Format, Name, ## __VA_ARGS__)
    
#define PLUGIN_LOG(Context, Format, ...) \
    _PLUGIN_LOG_RAW((Context)->Name, Format, ## __VA_ARGS__)

#define PLUGIN_LOG_LEAVE_ERROR_EE(dwError, EE) \
    do { \
        if ((dwError) || (EE)) \
        { \
            LW_RTL_LOG_DEBUG("-> %u (%s) (EE = %d)", dwError, LW_RTL_LOG_SAFE_STRING(LwWin32ExtErrorToName(dwError)), EE); \
        } \
    } while (0)

// The dummy plugin will log the hostname on each call.
typedef struct _LSA_PSTORE_PLUGIN_CONTEXT {
    PSTR Name;
} LSA_PSTORE_PLUGIN_CONTEXT;

static
VOID
PluginCleanup(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext
    )
{
    assert(pContext);

    PLUGIN_LOG(pContext, "CLEANUP");

    LW_SAFE_FREE_STRING(pContext->Name);

    LwFreeMemory(pContext);
}
            
static
DWORD
PluginSetPasswordInfoW(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainName = NULL;

    assert(pContext);

    if (pPasswordInfo->Account.DnsDomainName)
    {
        dwError = LwWc16sToMbs(pPasswordInfo->Account.DnsDomainName, &dnsDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    PLUGIN_LOG(pContext, "SET_W: DnsDomainName = %s",
            LW_RTL_LOG_SAFE_STRING(dnsDomainName));

cleanup:
    LW_SAFE_FREE_MEMORY(dnsDomainName);

    PLUGIN_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
PluginSetPasswordInfoA(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PCSTR dnsDomainName = pPasswordInfo->Account.DnsDomainName;

    assert(pContext);

    PLUGIN_LOG(pContext, "SET_A: DnsDomainName = %s",
            LW_RTL_LOG_SAFE_STRING(dnsDomainName));

    PLUGIN_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
PluginDeletePasswordInfoW(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN OPTIONAL PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR dnsDomainName = NULL;

    assert(pContext);

    if (pAccountInfo && pAccountInfo->DnsDomainName)
    {
        dwError = LwWc16sToMbs(pAccountInfo->DnsDomainName, &dnsDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    PLUGIN_LOG(pContext, "DELETE_W: DnsDomainName = %s",
            LW_RTL_LOG_SAFE_STRING(dnsDomainName));

cleanup:
    LW_SAFE_FREE_MEMORY(dnsDomainName);

    PLUGIN_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
PluginDeletePasswordInfoA(
    IN PLSA_PSTORE_PLUGIN_CONTEXT pContext,
    IN OPTIONAL PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PCSTR dnsDomainName = pAccountInfo ? pAccountInfo->DnsDomainName : NULL;

    assert(pContext);

    PLUGIN_LOG(pContext, "DELETE_A: DnsDomainName = %s",
            LW_RTL_LOG_SAFE_STRING(dnsDomainName));

    PLUGIN_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LsaPstorePluginInitializeContext(
    IN ULONG Version,
    IN PCSTR pName,
    OUT PLSA_PSTORE_PLUGIN_DISPATCH* ppDispatch,
    OUT PLSA_PSTORE_PLUGIN_CONTEXT* ppContext
    )
{
    DWORD dwError = 0;
    int EE = 0;
    // compiler type check for this function
    LSA_PSTORE_PLUGIN_INITIALIZE_FUNCTION unused = LsaPstorePluginInitializeContext;
    static LSA_PSTORE_PLUGIN_DISPATCH globalDispatch = {
        .Cleanup = PluginCleanup,
        .SetPasswordInfoW = PluginSetPasswordInfoW,
        .SetPasswordInfoA = PluginSetPasswordInfoA,
        .DeletePasswordInfoW = PluginDeletePasswordInfoW,
        .DeletePasswordInfoA = PluginDeletePasswordInfoA
    };
    PLSA_PSTORE_PLUGIN_DISPATCH dispatch = &globalDispatch;
    PLSA_PSTORE_PLUGIN_CONTEXT context = NULL;

    assert(unused);

    if (Version != LSA_PSTORE_PLUGIN_VERSION)
    {
        dwError = ERROR_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    dwError = LwAllocateMemory(sizeof(*context), OUT_PPVOID(&context));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwAllocateString(pName, &context->Name);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    PLUGIN_LOG(context, "INITIALIZE");

cleanup:
    if (dwError)
    {
        if (context)
        {
            PluginCleanup(context);
        }
        dispatch = NULL;
        context = NULL;
    }

    *ppDispatch = dispatch;
    *ppContext = context;

    return dwError;
}
