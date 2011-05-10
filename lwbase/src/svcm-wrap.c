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
 *        svcm-wrap.c
 *
 * Abstract:
 *
 *        Service module API -- Simple wrapper program that loads a service
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include <lw/base.h>
#include <lw/rtlgoto.h>
#include <lw/svcm.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define GCOS(s) GOTO_CLEANUP_ON_STATUS(s)

static
VOID
MainTask(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    );

static struct
{
    PWSTR pServiceName;
    PWSTR pServicePath;
    ULONG ArgCount;
    PWSTR* ppArgs;
    PLW_THREAD_POOL pPool;
    PLW_TASK pMainTask;
    PLW_SVCM_INSTANCE pService;
} gState =
{
    .pServiceName = NULL,
    .pServicePath = NULL,
    .ArgCount = 0,
    .ppArgs = NULL,
    .pPool = NULL,
    .pMainTask = NULL,
    .pService = NULL
};

static void Help()
{
    printf("Usage: lw-svcm-wrap <service name> <service module path>\n");
}

int main(
    int ArgCount,
    char** ppArgs
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i = 0;

    if (ArgCount < 3)
    {
        Help();
        exit(1);

    }

    if (!strcmp(ppArgs[1], "--help") || !strcmp(ppArgs[1], "-h"))
    {
        Help();
        exit(0);
    }

    status = LwRtlWC16StringAllocateFromCString(
        &gState.pServiceName,
        ppArgs[1]);
    GCOS(status);

    status = LwRtlWC16StringAllocateFromCString(
        &gState.pServicePath,
        ppArgs[2]);
    GCOS(status);

    gState.ArgCount = ArgCount - 2;

    status = LW_RTL_ALLOCATE_ARRAY_AUTO(&gState.ppArgs, gState.ArgCount);
    GCOS(status);

    for (i = 0; i < gState.ArgCount; i++)
    {
        status = LwRtlWC16StringAllocateFromCString(
            &gState.ppArgs[i],
            ppArgs[i+2]);
        GCOS(status);
    }

    status = LwRtlSvcmLoadModule(
        gState.pServiceName,
        gState.pServicePath,
        &gState.pService);
    GCOS(status);

    LwRtlBlockSignals();

    status = LwRtlCreateThreadPool(&gState.pPool, NULL);
    GCOS(status);

    status = LwRtlCreateTask(gState.pPool, &gState.pMainTask, NULL, MainTask, NULL);
    GCOS(status);

    LwRtlWakeTask(gState.pMainTask);

    status = LwRtlMain();
    GCOS(status);

cleanup:

    if (gState.pService)
    {
        LwRtlSvcmUnload(gState.pService);
        gState.pService = NULL;
    }

    if (gState.pMainTask)
    {
        LwRtlCancelTask(gState.pMainTask);
        LwRtlWaitTask(gState.pMainTask);
        LwRtlReleaseTask(&gState.pMainTask);
    }

    LwRtlFreeThreadPool(&gState.pPool);

    return LwNtStatusToErrno(status);
}

static
VOID
NotifyStart(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pUnused
    )
{
    return;
}

static
VOID
NotifyStop(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pUnused
    )
{
    LwRtlExitMain(Status);
}

static
VOID
MainTask(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    siginfo_t info;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        status = LwRtlSetTaskUnixSignal(pTask, SIGTERM, TRUE);
        GCOS(status);

        status = LwRtlSetTaskUnixSignal(pTask, SIGINT, TRUE);
        GCOS(status);

        status = LwRtlSetTaskUnixSignal(pTask, SIGHUP, TRUE);
        GCOS(status);

        status = LwRtlSvcmStart(
            gState.pService,
            gState.ArgCount,
            gState.ppArgs,
            0,
            NULL,
            NotifyStart,
            NULL);
        GCOS(status);

        *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;
    }
    else if (WakeMask & LW_TASK_EVENT_UNIX_SIGNAL)
    {
        while (LwRtlNextTaskUnixSignal(pTask, &info))
        {
            switch(info.si_signo)
            {
            case SIGTERM:
            case SIGINT:
                status = LwRtlSvcmStop(gState.pService, NotifyStop, NULL);
                GCOS(status);
                *pWaitMask = LW_TASK_EVENT_COMPLETE;
                goto cleanup;
            case SIGHUP:
                status = LwRtlSvcmRefresh(gState.pService, NULL, NULL);
                GCOS(status);
                break;
            default:
                break;
            }
        }

        *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;
    }

cleanup:

    if (status)
    {
        LwRtlExitMain(status);
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }

    return;
}
