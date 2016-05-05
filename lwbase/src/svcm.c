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
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

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

/* used to implement a shutdown timer */
struct _TIMER {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;

    /* indicates if the timer was cancelled */
    unsigned int cancelled : 1;
};

struct _SHUTDOWN_TIMER_REQUEST {
    PSTR serviceName;
    struct _TIMER * shutdownTimer;
    /* the pthread cond wait time in seconds */
    unsigned short delaySeconds;
};

static struct _TIMER gShutdownTimer = { 
      PTHREAD_MUTEX_INITIALIZER,
      PTHREAD_COND_INITIALIZER,
      LW_FALSE
};


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
    LW_DWORD uShutdownTimeout,
    LW_SVCM_MODULE_ENTRY_FUNCTION Entry
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    pInstance->ShutdownTimeout = uShutdownTimeout;
    pInstance->pTable = Entry();

    status = LwRtlCStringAllocateFromWC16String(&pInstance->pServiceName, pServiceName);
    GCOS(status);

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
    LW_IN LW_PCWSTR pwServiceName,
    LW_IN LW_DWORD uShutdownTimeout,
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

    LW_RTL_LOG_DEBUG("Loading embedded service: %s", pwServiceName);

    status = LwRtlSvcmInitializeInstance(pInstance, pwServiceName, "<embedded>", uShutdownTimeout, Entry);
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
    LW_IN LW_DWORD uShutdownTimeout,
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

    status = LwRtlSvcmInitializeInstance(pInstance, pServiceName, pModulePathA, uShutdownTimeout, Entry);
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

        RTL_FREE(&pInstance->pServiceName);
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


/**
 * @brief Shutdown timer thread performs a cond timed wait
 * of the supplied 'timer' and if it expires, this kills
 * this process.
 */
static void *ShutdownTimerThread(void *arg) {
    struct _SHUTDOWN_TIMER_REQUEST * request = (struct _SHUTDOWN_TIMER_REQUEST *)arg;
    struct _TIMER * timer = request->shutdownTimer;
    struct timespec timeout = { .tv_sec = time(NULL) + request->delaySeconds};
    PSTR serviceName = NULL;

    int status = 0;

    status = LwRtlCStringDuplicate(&serviceName, request->serviceName);
    GCOS(status);

    LW_RTL_FREE(&request->serviceName);
    LW_RTL_FREE(&request);

    status = pthread_mutex_lock(&(timer->mutex));
    if (status != 0) {
      LW_RTL_LOG_WARNING("Could not lock the '%s' shutdown timer: error %s (%d). Will NOT use shutdown timer.", 
              serviceName, ErrnoToName(status), status);
      GCOS(status);
    }

    while (!timer->cancelled) {
        status = pthread_cond_timedwait(&(timer->cond), &(timer->mutex), &timeout);

        if (status == ETIMEDOUT) {
           break;
        } else if (status != 0) {
            /* break the loop for errors which indicate programming errors */ 
            if (status == EINVAL || status == EPERM) {
              LW_RTL_LOG_ERROR("Error waiting on for '%s' shutdown timeout: error %s (%d).", 
                      serviceName, ErrnoToName(status), status);
              break;
            }
        }
    }

    if (!timer->cancelled) {
      pthread_mutex_unlock(&(timer->mutex));
      LW_RTL_LOG_WARNING("Shutdown timer for service '%s' expired, sending SIGKILL", serviceName); 
      kill(getpid(), SIGKILL);
    } else {
      pthread_mutex_unlock(&(timer->mutex));
    }

cleanup:
    LW_RTL_FREE(&serviceName);
    return NULL;
}


/**
 * @brief Create the shutdown timer thread. 
 * @return STATUS_SUCCESS or LW_STATUS_INSUFFICIENT_RESOURCES 
 */
static 
NTSTATUS 
BeginShutdownTimer(PSTR pServiceName, struct _TIMER * timer, LW_DWORD uShutdownTimeout)
{
    NTSTATUS status = STATUS_SUCCESS;
    pthread_t timerThread;
    int thread_status  = 0;

    /* this MUST be freed by the shutdown thread */
    struct _SHUTDOWN_TIMER_REQUEST * request = NULL;

    status = LW_RTL_ALLOCATE_AUTO(&request);
    if (status != STATUS_SUCCESS) {
      LW_RTL_LOG_WARNING("Could not create the %s shutdown timer request. Will NOT use shutdown timer.", pServiceName);
      GCOS(status);
    }

    request->shutdownTimer = &gShutdownTimer;
    request->delaySeconds = uShutdownTimeout;
    status = LwRtlCStringDuplicate(&request->serviceName, pServiceName);
    GCOS(status);

    thread_status = pthread_create(&timerThread, NULL, ShutdownTimerThread, request);
    if (thread_status != 0) {
      LW_RTL_LOG_WARNING("Could not %s create the shutdown timer thread: error %s (%d). Will NOT use shutdown timer.", 
              request->serviceName, ErrnoToName(thread_status), thread_status);
      status = LW_STATUS_INSUFFICIENT_RESOURCES;
    }

cleanup:
    if (status) {
        /* an error occurred, so free the request as
         * the shutdown thread won't */
        LW_RTL_FREE(&request->serviceName);
        LW_RTL_FREE(&request);
    }

    return status;
}


static 
VOID 
CancelShutdownTimer(struct _TIMER * timer) {
    if (timer) {
      pthread_mutex_lock(&(timer->mutex));
      timer->cancelled = LW_TRUE;
      pthread_cond_signal(&(timer->cond));
      pthread_mutex_unlock(&(timer->mutex));
    }
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

    LW_RTL_LOG_INFO("Executing stop for service '%s'", pState->pInstance->pServiceName); 
    LW_RTL_LOG_INFO("Starting %d second shutdown timer for service '%s'", 
            pState->pInstance->ShutdownTimeout,
            pState->pInstance->pServiceName); 
    BeginShutdownTimer(pState->pInstance->pServiceName, &gShutdownTimer, pState->pInstance->ShutdownTimeout);
    status = pState->pInstance->pTable->Stop(pState->pInstance);
    CancelShutdownTimer(&gShutdownTimer);
    LW_RTL_LOG_INFO("Stop of service '%s' completed.", pState->pInstance->pServiceName); 

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

VOID
LwRtlSvcmFreePool(
    VOID
    )
{
    LwRtlFreeThreadPool((PLW_THREAD_POOL*) &gSvcmState.pPool);
}

static
__attribute__((destructor))
VOID
SvcmDestructor(
    VOID
    )
{
    LwRtlSvcmFreePool();
}
