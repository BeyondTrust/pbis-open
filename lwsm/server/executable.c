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
 *        executable.c
 *
 * Abstract:
 *
 *        Logic for managing external executable service objects
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

#ifdef __APPLE__
#include <crt_externs.h>
#define ENVIRON (*_NSGetEnviron())
#else
extern char **environ;
#define ENVIRON environ
#endif

typedef struct _SM_PROCESS_TABLE
{
    pthread_mutex_t lock;
    pthread_mutex_t* pLock;
} SM_PROCESS_TABLE;

typedef struct _SM_EXECUTABLE
{
    PLW_TASK pTask;
    LW_SERVICE_STATE state;
    pid_t pid;
    int notifyFd;
    LW_SERVICE_TYPE type;
    PWSTR pwszPath;
    PWSTR* ppwszArgs;
    PWSTR* ppwszEnv;
    DWORD dwFdLimit;
    DWORD dwCoreSize;
    PLW_SERVICE_OBJECT pObject;
} SM_EXECUTABLE, *PSM_EXECUTABLE;

static SM_PROCESS_TABLE gProcTable =
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .pLock = &gProcTable.lock
};

static
VOID
LwSmExecutableFree(
    PSM_EXECUTABLE pExec
    );

static
DWORD
LwSmExecProgram(
    PSM_EXECUTABLE pExec,
    int* pNotifyPipe
    );

static
VOID
LwSmExecutableTask(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    );

static
DWORD
LwSmForkProcess(
    PSM_EXECUTABLE pExec
    )
{
    DWORD dwError = 0;
    pid_t pid = -1;
    struct timespec ts = {1, 0};
    int notifyPipe[2] = {-1, -1};

    if (pExec->type == LW_SERVICE_TYPE_EXECUTABLE)
    {
        if (pipe(notifyPipe) != 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
    }

    pid = fork();

    if (pid == -1)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }
    else if (pid == 0)
    {
        dwError = LwSmExecProgram(
            pExec,
            pExec->type == LW_SERVICE_TYPE_EXECUTABLE ?
            notifyPipe :
            NULL
            );
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        pExec->pid = pid;
        
        if (pExec->type == LW_SERVICE_TYPE_EXECUTABLE)
        {
            /* Close write side of pipe since we do not need it */
            close(notifyPipe[1]);
            notifyPipe[1] = -1;

            /* Save read side of pipe as notification fd */
            pExec->notifyFd = notifyPipe[0];
            notifyPipe[0] = -1;
        }
        else
        {
            pExec->notifyFd = -1;

            /* Wait for process to start up */
            if (nanosleep(&ts, NULL) != 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_ERROR(dwError);
            }
        }
    }

cleanup:

    if (notifyPipe[0] >= 0)
    {
        close(notifyPipe[0]);
    }

    if (notifyPipe[1] >= 0)
    {
        close(notifyPipe[1]);
    }

    return dwError;

error:

    pExec->state = LW_SERVICE_STATE_DEAD;

    goto cleanup;
}

static
DWORD
LwSmExecutableStart(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = LwSmGetServiceObjectData(pObject);
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gProcTable.lock);

    if (pExec->state != LW_SERVICE_STATE_STOPPED &&
        pExec->state != LW_SERVICE_STATE_DEAD)
    {
        dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
        BAIL_ON_ERROR(dwError);
    }

    pExec->pid = -1;
    pExec->state = LW_SERVICE_STATE_STARTING;
        
    LwSmNotifyServiceObjectStateChange(pObject, pExec->state);

    dwError = LwSmForkProcess(pExec);
    BAIL_ON_ERROR(dwError);

    /* Create a task to track the process */
    dwError = LwNtStatusToWin32Error(
        LwRtlCreateTask(
            gpPool,
            &pExec->pTask,
            NULL,
            LwSmExecutableTask,
            pExec));
    BAIL_ON_ERROR(dwError);

    /* Wake up the task */
    LwRtlWakeTask(pExec->pTask);
        
cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmSetLimits(
    PSM_EXECUTABLE pExec
    )
{
    DWORD dwError = 0;
    struct rlimit limit = {0};

    if (pExec->dwFdLimit)
    {
        limit.rlim_cur = pExec->dwFdLimit;
        limit.rlim_max = pExec->dwFdLimit;

        /* Ignore errors */
        (void) setrlimit(RLIMIT_NOFILE, &limit);
    }
    if (pExec->dwCoreSize)
    {
        limit.rlim_cur = pExec->dwCoreSize;
        limit.rlim_max = pExec->dwCoreSize;
        (void) setrlimit(RLIMIT_CORE, &limit);
    }

    return dwError;
}

static
DWORD
LwSmExecProgram(
    PSM_EXECUTABLE pExec,
    int* pNotifyPipe
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    PSTR* ppszArgs = NULL;
    PSTR* ppszEnv = NULL;
    size_t argsLen = 0;
    size_t envLen = 0;
    size_t environLen = 0;
    size_t i = 0;
    sigset_t set;

    sigemptyset(&set);

    /* Put ourselves in our own process group to dissociate from
       the parent's controlling terminal, if any */
    dwError = LwMapErrnoToLwError(setpgid(getpid(), getpid()));
    BAIL_ON_ERROR(dwError);
    
    /* Reset the signal mask */
    dwError = LwMapErrnoToLwError(pthread_sigmask(SIG_SETMASK, &set, NULL));
    BAIL_ON_ERROR(dwError);

    /* Set any applicable limits */
    dwError = LwSmSetLimits(pExec);
    BAIL_ON_ERROR(dwError);

    dwError = LwWc16sToMbs(pExec->pwszPath, &pszPath);
    BAIL_ON_ERROR(dwError);

    argsLen = LwSmStringListLength(pExec->ppwszArgs);

    dwError = LwAllocateMemory((argsLen + 1) * sizeof(*ppszArgs),
	OUT_PPVOID(&ppszArgs));
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < argsLen; i++)
    {
        dwError = LwWc16sToMbs(pExec->ppwszArgs[i], &ppszArgs[i]);
        BAIL_ON_ERROR(dwError);
    }

    if (pNotifyPipe)
    {
        close(pNotifyPipe[0]);
        if (dup2(pNotifyPipe[1], 3) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
        close(pNotifyPipe[1]);
        putenv("LIKEWISE_SM_NOTIFY=3");
    }

    environLen = LwSmStringListLengthA(ENVIRON);
    envLen = LwSmStringListLength(pExec->ppwszEnv);

    dwError = LwAllocateMemory((environLen + envLen + 1) * sizeof(*ppszEnv),
        OUT_PPVOID(&ppszEnv));
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < environLen; i++)
    {
        dwError = LwAllocateString(ENVIRON[i], &ppszEnv[i]);
        BAIL_ON_ERROR(dwError);
    }

    for (i = 0; i < envLen; i++)
    {
        dwError = LwWc16sToMbs(pExec->ppwszEnv[i], &ppszEnv[environLen + i]);
        BAIL_ON_ERROR(dwError);
    }

    if (execve(pszPath, ppszArgs, ppszEnv) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    return dwError;

error:

    /* We are in trouble */
    abort();
}

static
DWORD
LwSmExecutableStop(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = LwSmGetServiceObjectData(pObject);
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gProcTable.lock);

    switch (pExec->state)
    {
    default:
        dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
        BAIL_ON_ERROR(dwError);
        break;
    case LW_SERVICE_STATE_RUNNING:
        pExec->state = LW_SERVICE_STATE_STOPPING;       
        LwSmNotifyServiceObjectStateChange(pObject, pExec->state);
        
        if (kill(pExec->pid, SIGTERM) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
        
        /* The associated task will notice when the
           child process finally exits and update the
           status to LW_SERVICE_STOPPED.  We can release the
           task handle now since we will no longer need
           to use it */
        LwRtlReleaseTask(&pExec->pTask);
        break;
    case LW_SERVICE_STATE_DEAD:
        /* Go directly to stopped state */
        pExec->state = LW_SERVICE_STATE_STOPPED;
        LwSmNotifyServiceObjectStateChange(pObject, pExec->state);
        if (pExec->pTask)
        {
            /* Cancel task in case it's still alive for some reason */
            LwRtlCancelTask(pExec->pTask);
            /* Release task handle */
            LwRtlReleaseTask(&pExec->pTask);
        }
        break;
    }

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmExecutableGetStatus(
    PLW_SERVICE_OBJECT pObject,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = LwSmGetServiceObjectData(pObject);
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gProcTable.lock);

    pStatus->state = pExec->state;
    pStatus->pid = pExec->pid;
    pStatus->home = LW_SERVICE_HOME_STANDALONE;

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;
}

static
DWORD
LwSmExecutableRefresh(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = LwSmGetServiceObjectData(pObject);
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gProcTable.lock);

    switch (pExec->state)
    {
    case LW_SERVICE_STATE_RUNNING:
        if (kill(pExec->pid, SIGHUP) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
        break;
    default:
        break;
    }

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmExecutableConstruct(
    PLW_SERVICE_OBJECT pObject,
    PCLW_SERVICE_INFO pInfo,
    PVOID* ppData
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = NULL;
    BOOLEAN bLocked = FALSE;

    dwError = LwAllocateMemory(sizeof(*pExec), OUT_PPVOID(&pExec));
    BAIL_ON_ERROR(dwError);

    pExec->notifyFd = -1;

    dwError = LwSmCopyString(pInfo->pwszPath, &pExec->pwszPath);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(pInfo->ppwszArgs, &pExec->ppwszArgs);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(pInfo->ppwszEnv, &pExec->ppwszEnv);
    BAIL_ON_ERROR(dwError);

    pExec->pid = -1;
    pExec->type = pInfo->type;
    pExec->state = LW_SERVICE_STATE_STOPPED;
    pExec->pObject = pObject;
    pExec->dwFdLimit = pInfo->dwFdLimit;
    pExec->dwCoreSize = pInfo->dwCoreSize;

    *ppData = pExec;

    LOCK(bLocked, &gProcTable.lock);

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    if (pExec)
    {
        LwSmExecutableFree(pExec);
    }

    goto cleanup;
}

static
VOID
LwSmExecutableFree(
    PSM_EXECUTABLE pExec
    )
{
    if (pExec->notifyFd >= 0)
    {
        close(pExec->notifyFd);
    }

    if (pExec->pTask)
    {
        LwRtlCancelTask(pExec->pTask);
        LwRtlReleaseTask(&pExec->pTask);
    }

    LW_SAFE_FREE_MEMORY(pExec->pwszPath);
    LwSmFreeStringList(pExec->ppwszArgs);
    LwSmFreeStringList(pExec->ppwszEnv);
    LwFreeMemory(pExec);
}


static
VOID
LwSmExecutableDestruct(
    PLW_SERVICE_OBJECT pObject
    )
{
    LwSmExecutableFree(LwSmGetServiceObjectData(pObject));
}

static
VOID
LwSmExecutableTask(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pllTime
    )
{
    BOOLEAN bLocked = FALSE;
    PSM_EXECUTABLE pExec = pContext;
    pid_t pid = -1;
    NTSTATUS status = STATUS_SUCCESS;
    char c = 0;
    int ret = 0;
    siginfo_t info;

    LOCK(bLocked, &gProcTable.lock);

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        status = LwRtlSetTaskUnixSignal(pTask, SIGCHLD, TRUE);
        BAIL_ON_ERROR(status);

        *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;

        if (pExec->notifyFd >= 0)
        {
            status = LwRtlSetTaskFd(pTask, pExec->notifyFd, LW_TASK_EVENT_FD_READABLE);
            BAIL_ON_ERROR(status);

            *pWaitMask |= LW_TASK_EVENT_FD_READABLE;
        }
    }
    else if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }
    else if (WakeMask & LW_TASK_EVENT_FD_READABLE)
    {
        do
        {
            ret = read(pExec->notifyFd, &c, sizeof(c));
        } while (ret < 0 && errno == EINTR);

        if (ret != sizeof(c) || c != 0)
        {
            goto error;
        }

        status = LwRtlSetTaskFd(pTask, pExec->notifyFd, 0);
        BAIL_ON_ERROR(status);

        close(pExec->notifyFd);
        pExec->notifyFd = -1;

        pExec->state = LW_SERVICE_STATE_RUNNING;
        LwSmNotifyServiceObjectStateChange(pExec->pObject, pExec->state);

        *pWaitMask &= ~LW_TASK_EVENT_FD_READABLE;
    }
    else if (WakeMask & LW_TASK_EVENT_UNIX_SIGNAL)
    {
        while (LwRtlNextTaskUnixSignal(pTask, &info))
        {
            if (info.si_signo == SIGCHLD)
            {
                pid = waitpid(pExec->pid, &ret, WNOHANG);
                
                if (pid == pExec->pid)
                {
                    pExec->pid = -1;

                    switch (pExec->state)
                    {
                    case LW_SERVICE_STATE_STOPPING:
                        pExec->state = LW_SERVICE_STATE_STOPPED;
                        LwSmNotifyServiceObjectStateChange(pExec->pObject, pExec->state);
                        *pWaitMask = LW_TASK_EVENT_COMPLETE;
                        goto cleanup;
                    default:
                        /* Uh-oh */
                        goto error;
                    }
                }
            }
        }
    }

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);
    
    return;

error:

    pExec->state = LW_SERVICE_STATE_DEAD;
    LwSmNotifyServiceObjectStateChange(pExec->pObject, pExec->state);
  
    if (pExec->pid >= 0)
    {
        kill(pExec->pid, SIGKILL);
        waitpid(pExec->pid, &ret, 0);
        pExec->pid = -1;
    }

    *pWaitMask = LW_TASK_EVENT_COMPLETE;

    goto cleanup;
}

LW_SERVICE_LOADER_VTBL gExecutableVtbl =
{
    .pfnStart = LwSmExecutableStart,
    .pfnStop = LwSmExecutableStop,
    .pfnGetStatus = LwSmExecutableGetStatus,
    .pfnRefresh = LwSmExecutableRefresh,
    .pfnConstruct = LwSmExecutableConstruct,
    .pfnDestruct = LwSmExecutableDestruct
};
