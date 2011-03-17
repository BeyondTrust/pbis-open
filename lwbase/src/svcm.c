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
 * HAVE QUESTIONS, OR WISHTO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        svcm.c
 *
 * Abstract:
 *
 *        Service module API -- implementation
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#define LW_RTL_LOG_COMPONENT "svcm"

#include "config.h"
#include "svcm-internal.h"

#include <lw/base.h>
#include <lw/rtlgoto.h>
#include <lw/rtllog.h>
#include <dlfcn.h>
#include <assert.h>

#define GCOS(s) GOTO_CLEANUP_ON_STATUS(s)
#define STRINGIFY(token) (#token)

/****************************/

static
NTSTATUS
ValidateModuleTable(
    PLW_SVCM_MODULE pTable,
    PCSTR pPath
    );

/****************************/

static struct
{
    PLW_THREAD_POOL volatile pPool;
} gSvcmState;

static
NTSTATUS
InitPool(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_THREAD_POOL pNewPool = NULL;

    if (!gSvcmState.pPool)
    {
        status = LwRtlCreateThreadPool(&pNewPool, NULL);
        GCOS(status);

        if (InterlockedCompareExchangePointer(
            OUT_PPVOID(&gSvcmState.pPool),
            pNewPool,
            NULL) != NULL)
        {
            LwRtlFreeThreadPool(&pNewPool);
        }
    }

cleanup:

    return status;
}

static
NTSTATUS
LwRtlSvcmInitializeInstance(
    PLW_SVCM_INSTANCE pInstance,
    PCWSTR pServiceName,
    PCSTR pModuleName,
    LW_SVCM_MODULE_ENTRY_FUNCTION Entry
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    pInstance->pTable = Entry();

    status = ValidateModuleTable(pInstance->pTable, pModuleName);
    GCOS(status);

    status = pInstance->pTable->Init(pServiceName, pInstance);
    if (status)
    {
        LW_RTL_LOG_ERROR(
            "Could not initialize service module '%s': %s (0x%lx)",
            pModuleName,
            LwNtStatusToName(status),
            (unsigned long) status);
    }
    GCOS(status);

cleanup:

    return status;
}

LW_NTSTATUS
LwRtlSvcmLoadEmbedded(
    LW_IN LW_PCWSTR pServiceName,
    LW_IN LW_SVCM_MODULE_ENTRY_FUNCTION Entry,
    LW_OUT PLW_SVCM_INSTANCE* ppInstance
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_SVCM_INSTANCE pInstance = NULL;

    status = InitPool();
    GCOS(status);

    status = LW_RTL_ALLOCATE_AUTO(&pInstance);
    GCOS(status);

    LW_RTL_LOG_DEBUG("Loading embedded service: %s", pServiceName);

    status = LwRtlSvcmInitializeInstance(pInstance, pServiceName, "<embedded>", Entry);
    GCOS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        LwRtlSvcmUnload(pInstance);
        pInstance = NULL;
    }

    *ppInstance = pInstance;

    return status;
}

LW_NTSTATUS
LwRtlSvcmLoadModule(
    LW_IN LW_PCWSTR pServiceName,
    LW_IN LW_PCWSTR pModulePath,
    LW_OUT PLW_SVCM_INSTANCE* ppInstance
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_SVCM_INSTANCE pInstance = NULL;
    PSTR pModulePathA = NULL;
    LW_SVCM_MODULE_ENTRY_FUNCTION Entry = NULL;
    PSTR pEntryName = NULL;
    PSTR pBareName = NULL;
    PSTR pFinalSlash = NULL;
    PSTR pFinalDot = NULL;

    status = InitPool();
    GCOS(status);

    status = LwRtlCStringAllocateFromWC16String(&pModulePathA, pModulePath);
    GCOS(status);

    pFinalSlash = strrchr(pModulePathA, '/');
    pFinalDot = strrchr(pModulePathA, '.');

    if (!pFinalSlash)
    {
        pFinalSlash = pModulePathA;
    }
    else
    {
        pFinalSlash++;
    }

    if (!pFinalDot)
    {
        pFinalDot = pModulePathA + strlen(pModulePathA);
    }

    status = LW_RTL_ALLOCATE(&pBareName, CHAR, (pFinalDot - pFinalSlash) + 1);
    GCOS(status);

    memcpy(pBareName, pFinalSlash, pFinalDot - pFinalSlash);

    status = LwRtlCStringAllocatePrintf(&pEntryName, "_LwSvcmEntry_%s", pBareName);
    GCOS(status);

    status = LW_RTL_ALLOCATE_AUTO(&pInstance);
    GCOS(status);

    LW_RTL_LOG_DEBUG("Loading service module: %s", pModulePathA);

    (void) dlerror();
    pInstance->pDlHandle = dlopen(pModulePathA, RTLD_LOCAL | RTLD_NOW);

    if (!pInstance->pDlHandle)
    {
        LW_RTL_LOG_ERROR(
            "Could not load service module '%s': %s",
            pModulePathA,
            dlerror());

        status = LwErrnoToNtStatus(errno);
        GCOS(status);
    }

    (void) dlerror();
    Entry = dlsym(pInstance->pDlHandle, pEntryName);

    if (!Entry)
    {
        LW_RTL_LOG_ERROR(
            "Could not load entry point from service module '%s': %s",
            pModulePathA,
            dlerror());

        status = LwErrnoToNtStatus(errno);
        if (!status)
        {
            status = STATUS_BAD_DLL_ENTRYPOINT;
        }
        GCOS(status);
    }

    status = LwRtlSvcmInitializeInstance(pInstance, pServiceName, pModulePathA, Entry);
    GCOS(status);

cleanup:

    RTL_FREE(&pModulePathA);
    RTL_FREE(&pBareName);
    RTL_FREE(&pEntryName);

    if (!NT_SUCCESS(status))
    {
        LwRtlSvcmUnload(pInstance);
        pInstance = NULL;
    }

    *ppInstance = pInstance;

    return status;
}

VOID
LwRtlSvcmUnload(
    LW_IN LW_OUT PLW_SVCM_INSTANCE pInstance
    )
{
    if (pInstance)
    {
        if (pInstance->pTable && pInstance->pTable->Destroy)
        {
            pInstance->pTable->Destroy(pInstance);
        }

        if (pInstance->pDlHandle)
        {
            dlclose(pInstance->pDlHandle);
        }

        RTL_FREE(&pInstance);
    }
}

VOID
LwRtlSvcmSetData(
    LW_IN LW_OUT PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_PVOID pData
    )
{
    assert(pInstance != NULL);

    pInstance->pServiceData = pData;
}

PVOID
LwRtlSvcmGetData(
    PLW_SVCM_INSTANCE pInstance
    )
{
    assert(pInstance != NULL);

    return pInstance->pServiceData;
}

static
VOID
StartWorkItem(
    PLW_WORK_ITEM pItem,
    PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSVCM_START_STATE pState = pContext;

    status = pState->pInstance->pTable->Start(
        pState->pInstance,
        pState->ArgCount,
        pState->ppArgs,
        pState->FdCount,
        pState->pFds);

    if (pState->Notify)
    {
        pState->Notify(pState->pInstance, status, pState->pNotifyContext);
    }

    RTL_FREE(&pState);
    LwRtlFreeWorkItem(&pItem);
}

static
VOID
StopWorkItem(
    PLW_WORK_ITEM pItem,
    PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSVCM_COMMAND_STATE pState = pContext;

    status = pState->pInstance->pTable->Stop(pState->pInstance);

    if (pState->Notify)
    {
        pState->Notify(pState->pInstance, status, pState->pNotifyContext);
    }

    RTL_FREE(&pState);
    LwRtlFreeWorkItem(&pItem);
}

static
VOID
RefreshWorkItem(
    PLW_WORK_ITEM pItem,
    PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSVCM_COMMAND_STATE pState = pContext;

    if (pState->pInstance->pTable->Refresh)
    {
        status = pState->pInstance->pTable->Refresh(pState->pInstance);
    }

    if (pState->Notify)
    {
        pState->Notify(pState->pInstance, status, pState->pNotifyContext);
    }

    RTL_FREE(&pState);
    LwRtlFreeWorkItem(&pItem);
}


LW_NTSTATUS
LwRtlSvcmStart(
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_ULONG ArgCount,
    LW_IN LW_PWSTR* ppArgs,
    LW_IN LW_ULONG FdCount,
    LW_IN int* pFds,
    LW_IN LW_SVCM_NOTIFY_FUNCTION Notify,
    LW_IN LW_PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSVCM_START_STATE pStartState = NULL;
    PLW_WORK_ITEM pStartItem = NULL;

    if (!pInstance)
    {
        status = STATUS_INVALID_PARAMETER;
        GCOS(status);
    }

    status = LW_RTL_ALLOCATE_AUTO(&pStartState);
    GCOS(status);

    status = LW_RTL_ALLOCATE_AUTO(&pInstance->pStopState);
    GCOS(status);

    status = LwRtlCreateWorkItem(
        gSvcmState.pPool,
        &pStartItem,
        StartWorkItem,
        pStartState);
    GCOS(status);

    status = LwRtlCreateWorkItem(
        gSvcmState.pPool,
        &pInstance->pStopItem,
        StopWorkItem,
        pInstance->pStopState);
    GCOS(status);

    pStartState->pInstance = pInstance;
    pStartState->ArgCount = ArgCount;
    pStartState->ppArgs = ppArgs;
    pStartState->FdCount = FdCount;
    pStartState->pFds = pFds;
    pStartState->Notify = Notify;
    pStartState->pNotifyContext = pContext;

    LwRtlScheduleWorkItem(pStartItem, 0);
    pStartItem = NULL;
    pStartState = NULL;

cleanup:

    LwRtlFreeWorkItem(&pStartItem);
    RTL_FREE(&pStartState);

    if (status != STATUS_SUCCESS)
    {
        RTL_FREE(&pInstance->pStopState);
        LwRtlFreeWorkItem(&pInstance->pStopItem);
    }

    return status;
}

LW_NTSTATUS
LwRtlSvcmStop(
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_SVCM_NOTIFY_FUNCTION Notify,
    LW_IN LW_PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!pInstance)
    {
        status = STATUS_INVALID_PARAMETER;
        GCOS(status);
    }

    assert(pInstance->pStopItem);
    assert(pInstance->pStopState);

    pInstance->pStopState->pInstance = pInstance;
    pInstance->pStopState->Notify = Notify;
    pInstance->pStopState->pNotifyContext = pContext;

    LwRtlScheduleWorkItem(pInstance->pStopItem, 0);
    pInstance->pStopItem = NULL;
    pInstance->pStopState = NULL;

cleanup:

    return status;
}

LW_NTSTATUS
LwRtlSvcmRefresh(
    LW_IN PLW_SVCM_INSTANCE pInstance,
    LW_IN LW_SVCM_NOTIFY_FUNCTION Notify,
    LW_IN LW_PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSVCM_COMMAND_STATE pRefreshState = NULL;
    PLW_WORK_ITEM pRefreshItem = NULL;

    if (!pInstance)
    {
        status = STATUS_INVALID_PARAMETER;
        GCOS(status);
    }

    status = LW_RTL_ALLOCATE_AUTO(&pRefreshState);
    GCOS(status);

    status = LwRtlCreateWorkItem(
        gSvcmState.pPool,
        &pRefreshItem,
        RefreshWorkItem,
        pRefreshState);
    GCOS(status);

    pRefreshState->pInstance = pInstance;
    pRefreshState->Notify = Notify;
    pRefreshState->pNotifyContext = pContext;

    LwRtlScheduleWorkItem(pRefreshItem, 0);
    pRefreshItem = NULL;
    pRefreshState = NULL;

cleanup:

     LwRtlFreeWorkItem(&pRefreshItem);
     RTL_FREE(&pRefreshState);

     return status;
}

static
NTSTATUS
ValidateModuleTable(
    PLW_SVCM_MODULE pTable,
    PCSTR pPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!pTable)
    {
        LW_RTL_LOG_ERROR(
            "Service module '%s' did not provide a function table",
            pPath);
        status = STATUS_BAD_DLL_ENTRYPOINT;
        GCOS(status);
    }

    if (pTable->Size < sizeof(*pTable) ||
        !pTable->Init || !pTable->Destroy ||
        !pTable->Start || !pTable->Stop)
    {
        LW_RTL_LOG_ERROR(
            "Service module '%s' has a bogus function table",
            pPath);
        status = STATUS_BAD_DLL_ENTRYPOINT;
        GCOS(status);
    }

    cleanup:

    return status;
}

static
__attribute__((destructor))
VOID
SvcmDestructor(
    VOID
    )
{
    LwRtlFreeThreadPool((PLW_THREAD_POOL*) &gSvcmState.pPool);
}
