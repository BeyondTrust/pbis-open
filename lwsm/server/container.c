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
 *        container.c
 *
 * Abstract:
 *
 *        Logic for managing service containers
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

#define CONTAINER_LOCK() pthread_mutex_lock(&gContainerLock)
#define CONTAINER_UNLOCK() pthread_mutex_unlock(&gContainerLock)

#define LWSMD_PATH (SBINDIR "/lwsmd")

#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(WCHAR),                            \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ZERO_TERMINATED,                     \
    LWMSG_ATTR_ENCODING("utf-16")

#define LWMSG_PWSTR                    \
    LWMSG_POINTER_BEGIN,               \
    LWMSG_UINT16(WCHAR),               \
    LWMSG_POINTER_END,                 \
    LWMSG_ATTR_ZERO_TERMINATED,        \
    LWMSG_ATTR_ENCODING("utf-16")

typedef enum _CONTAINER_TAG
{
    CONTAINER_RES_VOID,
    CONTAINER_RES_STATUS,
    CONTAINER_RES_HANDLE,
    CONTAINER_REQ_START,
    CONTAINER_REQ_STOP,
    CONTAINER_REQ_REFRESH,
    CONTAINER_REQ_SET_LOG_TARGET,
    CONTAINER_REQ_SET_LOG_LEVEL,
    CONTAINER_REQ_GET_LOG_STATE,
    CONTAINER_RES_GET_LOG_STATE,
    CONTAINER_REQ_GET_FACILITIES,
    CONTAINER_RES_GET_FACILITIES,
    CONTAINER_REQ_REGISTER
} CONTAINER_TAG, *PCONTAINER_TAG;

typedef struct _CONTAINER_HANDLE
{
    PLW_SVCM_INSTANCE pInstance;
    LWMsgCall* pCall;
    LW_SERVICE_STATE State;
} CONTAINER_HANDLE, *PCONTAINER_HANDLE;

typedef struct _CONTAINER_START_REQ
{
    PWSTR pName;
    PWSTR pPath;
    DWORD ArgCount;
    PWSTR* ppArgs;
    DWORD FdCount;
    int* pFds;
    DWORD FdLimit;
    LW_SM_LOGGER_TYPE LogType;
    LW_SM_LOG_LEVEL LogLevel;
    PWSTR pLogTarget;
    DWORD CoreSize;
} CONTAINER_START_REQ, *PCONTAINER_START_REQ;

typedef struct _CONTAINER_STATUS_RES
{
    DWORD Error;
} CONTAINER_STATUS_RES, *PCONTAINER_STATUS_RES;

typedef struct _CONTAINER_SET_LOG_INFO_REQ
{
    PSTR pFacility;
    LW_SM_LOGGER_TYPE type;
    PSTR pszTarget;
} CONTAINER_SET_LOG_INFO_REQ, *PCONTAINER_SET_LOG_INFO_REQ;

typedef struct _CONTAINER_IPC_SET_LOG_LEVEL_REQ
{
    PSTR pFacility;
    LW_SM_LOG_LEVEL Level;
} CONTAINER_SET_LOG_LEVEL_REQ, *PCONTAINER_SET_LOG_LEVEL_REQ;

typedef struct _CONTAINER_IPC_GET_LOG_STATE_REQ
{
    PSTR pFacility;
} CONTAINER_GET_LOG_STATE_REQ, *PCONTAINER_GET_LOG_STATE_REQ;

typedef struct _CONTAINER_IPC_GET_LOG_STATE_RES
{
    LW_SM_LOGGER_TYPE type;
    PSTR pszTarget;
    LW_SM_LOG_LEVEL Level;
} CONTAINER_GET_LOG_STATE_RES, *PCONTAINER_GET_LOG_STATE_RES;

typedef struct _CONTAINER_KEY
{
    uid_t Uid;
    gid_t Gid;
    PWSTR pGroup;
} CONTAINER_KEY, *PCONTAINER_KEY;

typedef struct _CONTAINER
{
    DWORD volatile Refs;
    int Sockets[2];
    PLW_TASK pTask;
    LWMsgPeer* pPeer;
    LWMsgSession* pSession;
    PLW_WORK_ITEM pDeleteItem;
    pid_t Pid;
    DWORD Error;
    CONTAINER_KEY Key;
    LW_HASHTABLE_NODE Node;
    SM_LINK Instances;
} CONTAINER, *PCONTAINER;

typedef struct _CONTAINER_INSTANCE
{
    PLW_SERVICE_OBJECT pObject;
    PCONTAINER pContainer;
    LWMsgHandle* pHandle;
    LONG volatile State;
    PWSTR pName;
    PWSTR pPath;
    PWSTR* ppArgs;
    PWSTR pGroup;
    DWORD FdLimit;
    LW_SM_LOGGER_TYPE LogType;
    LW_SM_LOG_LEVEL LogLevel;
    PWSTR pLogTarget;
    LWMsgParams In;
    LWMsgParams Out;
    CONTAINER_START_REQ Start;
    SM_LINK Link;
    DWORD CoreSize;
} CONTAINER_INSTANCE, *PCONTAINER_INSTANCE;

static LWMsgTypeSpec gLogLevelSpec[] =
{
    LWMSG_ENUM_BEGIN(LW_SM_LOG_LEVEL, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_DEFAULT),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_ALWAYS),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_ERROR),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_WARNING),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_INFO),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_VERBOSE),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_DEBUG),
    LWMSG_ENUM_VALUE(LW_SM_LOG_LEVEL_TRACE),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLogTypeSpec[] =
{
    LWMSG_ENUM_BEGIN(LW_SM_LOGGER_TYPE, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(LW_SM_LOGGER_DEFAULT),
    LWMSG_ENUM_VALUE(LW_SM_LOGGER_NONE),
    LWMSG_ENUM_VALUE(LW_SM_LOGGER_SYSLOG),
    LWMSG_ENUM_VALUE(LW_SM_LOGGER_FILE),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gStringListSpec[] =
{
    LWMSG_POINTER_BEGIN,
    LWMSG_PWSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_ZERO_TERMINATED,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gContainerStatusSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONTAINER_STATUS_RES),
    LWMSG_MEMBER_UINT32(CONTAINER_STATUS_RES, Error),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gContainerHandleInSpec[] =
{
    LWMSG_HANDLE(CONTAINER_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gContainerHandleOutSpec[] =
{
    LWMSG_HANDLE(CONTAINER_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gContainerStartSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONTAINER_START_REQ),
    LWMSG_MEMBER_PWSTR(CONTAINER_START_REQ, pName),
    LWMSG_MEMBER_PWSTR(CONTAINER_START_REQ, pPath),
    LWMSG_MEMBER_UINT32(CONTAINER_START_REQ, ArgCount),
    LWMSG_MEMBER_POINTER_BEGIN(CONTAINER_START_REQ, ppArgs),
    LWMSG_PWSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(CONTAINER_START_REQ, ArgCount),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_UINT32(CONTAINER_START_REQ, FdCount),
    LWMSG_MEMBER_POINTER_BEGIN(CONTAINER_START_REQ, pFds),
    LWMSG_FD,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(CONTAINER_START_REQ, FdCount),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_UINT32(CONTAINER_START_REQ, FdLimit),
    LWMSG_MEMBER_TYPESPEC(CONTAINER_START_REQ, LogType, gLogTypeSpec),
    LWMSG_MEMBER_PWSTR(CONTAINER_START_REQ, pLogTarget),
    LWMSG_MEMBER_TYPESPEC(CONTAINER_START_REQ, LogLevel, gLogLevelSpec),
    LWMSG_MEMBER_UINT32(CONTAINER_START_REQ, CoreSize),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSetLogInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONTAINER_SET_LOG_INFO_REQ),
    LWMSG_MEMBER_PSTR(CONTAINER_SET_LOG_INFO_REQ, pFacility),
    LWMSG_MEMBER_TYPESPEC(CONTAINER_SET_LOG_INFO_REQ, type, gLogTypeSpec),
    LWMSG_MEMBER_PSTR(CONTAINER_SET_LOG_INFO_REQ, pszTarget),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSetLogLevelSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONTAINER_SET_LOG_LEVEL_REQ),
    LWMSG_MEMBER_PSTR(CONTAINER_SET_LOG_LEVEL_REQ, pFacility),
    LWMSG_MEMBER_TYPESPEC(CONTAINER_SET_LOG_LEVEL_REQ, Level, gLogLevelSpec),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gGetLogStateReqSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONTAINER_GET_LOG_STATE_REQ),
    LWMSG_MEMBER_PSTR(CONTAINER_GET_LOG_STATE_REQ, pFacility),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gGetLogStateResSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONTAINER_GET_LOG_STATE_RES),
    LWMSG_MEMBER_TYPESPEC(CONTAINER_GET_LOG_STATE_RES, type, gLogTypeSpec),
    LWMSG_MEMBER_PSTR(CONTAINER_GET_LOG_STATE_RES, pszTarget),
    LWMSG_MEMBER_TYPESPEC(CONTAINER_GET_LOG_STATE_RES, Level, gLogLevelSpec),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegisterReqSpec[] =
{
    LWMSG_PWSTR,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gContainerProtocol[] =
{
    LWMSG_MESSAGE(CONTAINER_RES_VOID, NULL),
    LWMSG_MESSAGE(CONTAINER_RES_STATUS, gContainerStatusSpec),
    LWMSG_MESSAGE(CONTAINER_RES_HANDLE, gContainerHandleOutSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_START, gContainerStartSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_STOP, gContainerHandleInSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_REFRESH, gContainerHandleInSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_SET_LOG_TARGET, gSetLogInfoSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_SET_LOG_LEVEL, gSetLogLevelSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_GET_LOG_STATE, gGetLogStateReqSpec),
    LWMSG_MESSAGE(CONTAINER_RES_GET_LOG_STATE, gGetLogStateResSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_GET_FACILITIES, NULL),
    LWMSG_MESSAGE(CONTAINER_RES_GET_FACILITIES, gStringListSpec),
    LWMSG_MESSAGE(CONTAINER_REQ_REGISTER, gRegisterReqSpec),
    LWMSG_PROTOCOL_END,
};

#define DECLARE(name) \
    extern PLW_SVCM_MODULE LW_RTL_SVCM_ENTRY_POINT_NAME(name)(VOID)

#define EMBED(name) \
{ #name, LW_RTL_SVCM_ENTRY_POINT_NAME(name) }


#ifdef EMBED_LWREG
DECLARE(lwreg);
#endif

#ifdef EMBED_NETLOGON
DECLARE(netlogon);
#endif

#ifdef EMBED_LWIO
DECLARE(lwio);
#endif

#ifdef EMBED_LSASS
DECLARE(lsass);
#endif

struct {
    PCSTR pName;
    LW_SVCM_MODULE_ENTRY_FUNCTION Entry;
} gEmbedded[] =
{
#ifdef EMBED_LWREG
    EMBED(lwreg),
#endif
#ifdef EMBED_NETLOGON
    EMBED(netlogon),
#endif
#ifdef EMBED_LWIO
    EMBED(lwio),
#endif
#ifdef EMBED_LSASS
    EMBED(lsass),
#endif
    {NULL, NULL}
};

static pthread_mutex_t gContainerLock = PTHREAD_MUTEX_INITIALIZER;
static PLW_HASHTABLE gpContainers = NULL;
static BOOLEAN volatile gContainerShutdown = FALSE;
static PLW_TASK_GROUP gpContainerGroup = NULL;

static
DWORD
SetLimits(
    DWORD FdLimit,
    DWORD CoreSize
    )
{
    DWORD dwError = 0;
    struct rlimit limit = {0};

    if (FdLimit)
    {
        (void) getrlimit(RLIMIT_NOFILE, &limit);

        if (FdLimit > limit.rlim_cur)
        {
            limit.rlim_cur = FdLimit;
        }

        if (FdLimit > limit.rlim_max)
        {
            limit.rlim_max = FdLimit;
        }

        (void) setrlimit(RLIMIT_NOFILE, &limit);

        (void) getrlimit(RLIMIT_NOFILE, &limit);
    }

    if (CoreSize)
    {
        (void) getrlimit(RLIMIT_CORE, &limit);

        if (CoreSize > limit.rlim_cur)
        {
            limit.rlim_cur = CoreSize;
        }

        if (CoreSize > limit.rlim_max)
        {
            limit.rlim_max = CoreSize;
        }

        (void) setrlimit(RLIMIT_CORE, &limit);

        (void) getrlimit(RLIMIT_CORE, &limit);
    }

    return dwError;
}

static
LW_PCVOID
ContainerGetKey(
    PLW_HASHTABLE_NODE pNode,
    PVOID pUserData
    )
{
    PCONTAINER pContainer = LW_STRUCT_FROM_FIELD(pNode, CONTAINER, Node);

    return &pContainer->Key;
}

static
ULONG
ContainerDigest(
    PCVOID _pKey,
    PVOID pUserData
    )
{
    PCONTAINER_KEY pKey = (PCONTAINER_KEY) _pKey;
    ULONG digest = 0;

    digest += LwRtlHashDigestPwstr(pKey->pGroup, NULL);
    digest *= 31;
    digest += pKey->Gid;
    digest *= 32;
    digest += pKey->Uid;

    return digest;
}

static
BOOLEAN
ContainerEqual(
    PCVOID _pKey1,
    PCVOID _pKey2,
    PVOID pUserData
    )
{
    PCONTAINER_KEY pKey1 = (PCONTAINER_KEY) _pKey1;
    PCONTAINER_KEY pKey2 = (PCONTAINER_KEY) _pKey2;

    return (
        LwRtlWC16StringIsEqual(pKey1->pGroup, pKey2->pGroup, TRUE) &&
        pKey1->Uid == pKey2->Uid &&
        pKey1->Gid == pKey2->Gid);
}

static
VOID
DeleteContainer(
    PLW_WORK_ITEM pItem,
    PVOID pData
    )
{
    PCONTAINER pContainer = pData;

    if (pContainer)
    {
        LW_SAFE_FREE_MEMORY(pContainer->Key.pGroup);

        if (pContainer->Sockets[0] >= 0)
        {
            close(pContainer->Sockets[0]);
        }

        if (pContainer->Sockets[1] >= 0)
        {
            close(pContainer->Sockets[1]);
        }

        if (pContainer->pSession && pContainer->pPeer)
        {
            lwmsg_peer_disconnect(pContainer->pPeer);
        }

        if (pContainer->pPeer)
        {
            lwmsg_peer_delete(pContainer->pPeer);
        }

        LwRtlReleaseTask(&pContainer->pTask);

        LwFreeMemory(pContainer);
    }

    LwRtlFreeWorkItem(&pItem);
}

static
VOID
InvalidateContainer(
    PCONTAINER pContainer
    )
{
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PCONTAINER_INSTANCE pInstance = NULL;

    LwRtlHashTableRemove(gpContainers, &pContainer->Node);

    for (pLink = LwSmLinkBegin(&pContainer->Instances);
        LwSmLinkValid(&pContainer->Instances, pLink);
        pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pInstance = LW_STRUCT_FROM_FIELD(pLink, CONTAINER_INSTANCE, Link);

        LwSmLinkRemove(&pInstance->Link);
        pInstance->State = LW_SERVICE_STATE_DEAD;
        LwSmNotifyServiceObjectStateChange(pInstance->pObject, LW_SERVICE_STATE_DEAD);
    }
}

static
DWORD
SpawnContainer(
    PCONTAINER pContainer
    )
{
    DWORD dwError = ERROR_SUCCESS;
    pid_t pid = -1;
    sigset_t set;
    PSTR pName = NULL;

    dwError = LwWc16sToMbs(pContainer->Key.pGroup, &pName);
    BAIL_ON_ERROR(dwError);

    sigemptyset(&set);

    pid = fork();

    if (pid == -1)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }
    else if (pid == 0)
    {
        /* Move socket fd to 4 if necessary */
        if (pContainer->Sockets[1] != 4)
        {
            if (dup2(pContainer->Sockets[1], 4) < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_ERROR(dwError);
            }

            close(pContainer->Sockets[1]);
        }

        /* Put ourselves in our own process group to dissociate from
           the parent's controlling terminal, if any */
        if (setpgid(getpid(), getpid()) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        /* Reset the signal mask */
        dwError = LwMapErrnoToLwError(pthread_sigmask(SIG_SETMASK, &set, NULL));
        BAIL_ON_ERROR(dwError);

        /* Set user and group ids */
        if (pContainer->Key.Gid && setgid(pContainer->Key.Gid) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        if (pContainer->Key.Uid && setuid(pContainer->Key.Uid) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        /* Exec container */
        if (execl(LWSMD_PATH, CONTAINER_PROCESS_NAME, pName, NULL) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
    }
    else
    {
        pContainer->Pid = pid;
        close(pContainer->Sockets[1]);
        pContainer->Sockets[1] = -1;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pName);

    return dwError;

error:

    if (pid == 0)
    {
        abort();
    }

    goto cleanup;
}

static
VOID
ReleaseContainer(
    PCONTAINER pContainer,
    PCONTAINER_INSTANCE pInstance
    )
{
    if (pInstance)
    {
        LwSmLinkRemove(&pInstance->Link);

        if (pContainer->Instances.pNext == &pContainer->Instances)
        {
            /* If this is a container we created on demand,
             * remove it from the hash table now.
             */
            if (pContainer->pPeer)
            {
                LwRtlHashTableRemove(gpContainers, &pContainer->Node);
            }

            if (pContainer->pTask)
            {
                LwRtlCancelTask(pContainer->pTask);
            }
        }
    }

    if (--pContainer->Refs == 0)
    {
        if (pContainer->pDeleteItem)
        {
            LwRtlScheduleWorkItem(pContainer->pDeleteItem, 0);
        }
        else
        {
            DeleteContainer(NULL, pContainer);
        }
    }
}

static
VOID
ContainerTask(
    PLW_TASK pTask,
    PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    PLW_TASK_EVENT_MASK pWaitMask,
    PLONG64 pTime
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCONTAINER pContainer = (PCONTAINER) pContext;
    siginfo_t siginfo;
    int status = 0;

    CONTAINER_LOCK();

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        dwError = SpawnContainer(pContext);
        BAIL_ON_ERROR(dwError);

        dwError = LwNtStatusToWin32Error(LwRtlSetTaskUnixSignal(pTask, SIGCHLD, TRUE));
        BAIL_ON_ERROR(dwError);
    }

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        if (kill(pContainer->Pid, SIGTERM) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }

        /* We are shutting down, so don't bother waiting */
        if (gContainerShutdown)
        {
            dwError = ERROR_CANCELLED;
            BAIL_ON_ERROR(dwError);
        }
    }

    if (WakeMask & LW_TASK_EVENT_UNIX_SIGNAL)
    {
        while (LwRtlNextTaskUnixSignal(pTask, &siginfo))
        {
            if (siginfo.si_signo == SIGCHLD &&
                waitpid(pContainer->Pid, &status, WNOHANG) == pContainer->Pid)
            {
                dwError = ERROR_CANCELLED;
                BAIL_ON_ERROR(dwError);
            }
        }
    }

    *pWaitMask = LW_TASK_EVENT_UNIX_SIGNAL;

error:

    if (dwError)
    {
        pContainer->Error = dwError;
        InvalidateContainer(pContainer);
    }

    CONTAINER_UNLOCK();

    if (dwError)
    {
        ReleaseContainer(pContainer, NULL);
        LwRtlSetTaskUnixSignal(pTask, SIGCHLD, FALSE);
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }
}

static
DWORD
CreateContainer(
    PCONTAINER_KEY pKey,
    PCONTAINER* ppContainer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCONTAINER pContainer = NULL;
    static const WCHAR wszDirect[] = {'d', 'i', 'r', 'e', 'c', 't', 0};

    dwError = LwAllocateMemory(sizeof(*pContainer), OUT_PPVOID(&pContainer));
    BAIL_ON_ERROR(dwError);

    LwSmLinkInit(&pContainer->Instances);
    pContainer->Pid = -1;
    pContainer->Refs = 1;
    pContainer->Sockets[0] = -1;
    pContainer->Sockets[1] = -1;
    pContainer->Key.Gid = pKey->Gid;
    pContainer->Key.Uid = pKey->Uid;

    /*
     * Because lwmsg_peer_delete() can block on a task, we create a work item
     * to free the container when the reference count hits 0.  This makes calling
     * ReleaseContainer() from a task safe.
     */
    dwError = LwRtlCreateWorkItem(gpPool, &pContainer->pDeleteItem, DeleteContainer, pContainer);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateWc16String(&pContainer->Key.pGroup, pKey->pGroup);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_new(NULL, gState.pContainerProtocol, &pContainer->pPeer));
    BAIL_ON_ERROR(dwError);

    if (LwRtlWC16StringIsEqual(pContainer->Key.pGroup, wszDirect, TRUE))
    {
        dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_connect_endpoint(
            pContainer->pPeer,
            LWMSG_ENDPOINT_DIRECT,
            "Container"));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_connect(pContainer->pPeer, &pContainer->pSession));
        BAIL_ON_ERROR(dwError);

        pContainer->Pid = getpid();
        pContainer->Refs = 0;
    }
    else
    {
        dwError = LwErrnoToWin32Error(socketpair(AF_UNIX, SOCK_STREAM, 0, pContainer->Sockets));
        BAIL_ON_ERROR(dwError);

        dwError = MAP_LWMSG_STATUS(lwmsg_peer_connect_fd(
            pContainer->pPeer,
            LWMSG_ENDPOINT_PAIR,
            pContainer->Sockets[0],
            &pContainer->pSession));
        BAIL_ON_ERROR(dwError);
        pContainer->Sockets[0] = -1;

        dwError = LwNtStatusToWin32Error(LwRtlCreateTask(
            gpPool,
            &pContainer->pTask,
            gpContainerGroup,
            ContainerTask,
            pContainer));
        BAIL_ON_ERROR(dwError);

        LwRtlWakeTask(pContainer->pTask);
    }

cleanup:

    *ppContainer = pContainer;

    return dwError;

error:

    ReleaseContainer(pContainer, NULL);
    pContainer = NULL;

    goto cleanup;
}

static
DWORD
AcquireContainer(
    PCONTAINER_KEY pKey,
    PCONTAINER_INSTANCE pInstance,
    PCONTAINER* ppContainer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCONTAINER pContainer = NULL;
    PLW_HASHTABLE_NODE pNode = NULL;

    if (!gpContainers)
    {
        dwError = LwNtStatusToWin32Error(
            LwRtlCreateHashTable(
                &gpContainers,
                ContainerGetKey,
                ContainerDigest,
                ContainerEqual,
                NULL,
                11));
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(
        LwRtlHashTableFindKey(gpContainers, &pNode, pKey));

    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = CreateContainer(pKey, &pContainer);
        BAIL_ON_ERROR(dwError);

        LwRtlHashTableInsert(gpContainers, &pContainer->Node, NULL);
    }
    else
    {
        pContainer = LW_STRUCT_FROM_FIELD(pNode, CONTAINER, Node);
    }

    pContainer->Refs++;

    LwSmLinkInsertBefore(&pContainer->Instances, &pInstance->Link);

    *ppContainer = pContainer;

cleanup:

    return dwError;

error:

    if (pContainer)
    {
        ReleaseContainer(pContainer, NULL);
    }

    goto cleanup;
}

static
VOID
ContainerDeleteInstance(
    PCONTAINER_INSTANCE pInstance
    )
{
    if (pInstance)
    {
        if (pInstance->pContainer)
        {
            ReleaseContainer(pInstance->pContainer, pInstance);
        }

        LW_SAFE_FREE_MEMORY(pInstance->pName);
        LW_SAFE_FREE_MEMORY(pInstance->pPath);
        LW_SAFE_FREE_MEMORY(pInstance->pGroup);
        LW_SAFE_FREE_MEMORY(pInstance->pLogTarget);
        LwSmFreeStringList(pInstance->ppArgs);

        LwFreeMemory(pInstance);
    }
}

static
VOID
ContainerStartComplete(
    LWMsgCall* pCall,
    LWMsgStatus status,
    PVOID pData
    )
{
    DWORD dwError = MAP_LWMSG_STATUS(status);
    PCONTAINER_INSTANCE pInstance = (PCONTAINER_INSTANCE) pData;

    CONTAINER_LOCK();

    BAIL_ON_ERROR(dwError);

    switch (pInstance->Out.tag)
    {
    default:
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_STATUS:
        dwError = ((PCONTAINER_STATUS_RES) pInstance->Out.data)->Error;
        if (!dwError)
        {
            dwError = ERROR_INTERNAL_ERROR;
        }
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_HANDLE:
        pInstance->pHandle = (LWMsgHandle*) pInstance->Out.data;
        pInstance->Out.data = NULL;
        break;
    }

error:

    pInstance->State = dwError ? LW_SERVICE_STATE_DEAD : LW_SERVICE_STATE_RUNNING;

    CONTAINER_UNLOCK();

    LwSmNotifyServiceObjectStateChange(pInstance->pObject, pInstance->State);

    lwmsg_call_destroy_params(pCall, &pInstance->Out);
    lwmsg_call_release(pCall);
}

static
DWORD
ContainerStart(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);
    ULONG ArgCount = 0;
    LWMsgStatus status = LWMSG_STATUS_ERROR;
    LWMsgCall* pCall = NULL;
    CONTAINER_KEY key = {0};

    CONTAINER_LOCK();

    if (pInstance->State == LW_SERVICE_STATE_DEAD)
    {
        pInstance->State = LW_SERVICE_STATE_STOPPED;
    }

    if (pInstance->State != LW_SERVICE_STATE_STOPPED)
    {
        goto cleanup;
    }

    pInstance->State = LW_SERVICE_STATE_STARTING;

    key.Gid = getgid();
    key.Uid = getuid();
    key.pGroup = pInstance->pGroup;

    if (pInstance->pHandle)
    {
        if (pInstance->pContainer->pSession)
        {
            lwmsg_session_release_handle(pInstance->pContainer->pSession, pInstance->pHandle);
        }
        pInstance->pHandle = NULL;
    }

    if (pInstance->pContainer)
    {
        ReleaseContainer(pInstance->pContainer, pInstance);
        pInstance->pContainer = NULL;
    }

    dwError = AcquireContainer(&key, pInstance, &pInstance->pContainer);
    BAIL_ON_ERROR(dwError);

    for (ArgCount = 0; pInstance->ppArgs[ArgCount]; ArgCount++);

    pInstance->Start.pName = pInstance->pName;
    pInstance->Start.pPath = pInstance->pPath;
    pInstance->Start.ArgCount = ArgCount;
    pInstance->Start.ppArgs = pInstance->ppArgs;
    pInstance->Start.FdCount = 0;
    pInstance->Start.FdLimit = pInstance->FdLimit;
    pInstance->Start.LogType = pInstance->LogType;
    pInstance->Start.LogLevel = pInstance->LogLevel;
    pInstance->Start.pLogTarget = pInstance->pLogTarget;
    pInstance->Start.CoreSize = pInstance->CoreSize;
    pInstance->In.tag = CONTAINER_REQ_START;
    pInstance->In.data = &pInstance->Start;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pInstance->pContainer->pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    status = lwmsg_call_dispatch(pCall, &pInstance->In, &pInstance->Out, ContainerStartComplete, pInstance);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
    case LWMSG_STATUS_PENDING:
        break;
    default:
        dwError = MAP_LWMSG_STATUS(status);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    CONTAINER_UNLOCK();

    if (status != LWMSG_STATUS_PENDING && pCall)
    {
        ContainerStartComplete(pCall, status, pInstance);
    }

    return dwError;

error:

    goto cleanup;
}

static
VOID
ContainerStopComplete(
    LWMsgCall* pCall,
    LWMsgStatus status,
    PVOID pData
    )
{
    DWORD dwError = MAP_LWMSG_STATUS(status);
    PCONTAINER_INSTANCE pInstance = (PCONTAINER_INSTANCE) pData;

    CONTAINER_LOCK();

    if (pInstance->pContainer->pSession && pInstance->pHandle)
    {
        lwmsg_session_release_handle(pInstance->pContainer->pSession, pInstance->pHandle);
    }
    pInstance->pHandle = NULL;

    ReleaseContainer(pInstance->pContainer, pInstance);
    pInstance->pContainer = NULL;

    pInstance->State = dwError ? LW_SERVICE_STATE_DEAD : LW_SERVICE_STATE_STOPPED;

    CONTAINER_UNLOCK();

    LwSmNotifyServiceObjectStateChange(pInstance->pObject, pInstance->State);

    lwmsg_call_destroy_params(pCall, &pInstance->Out);
    lwmsg_call_release(pCall);
}

static
DWORD
ContainerStop(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);
    LWMsgStatus status = LWMSG_STATUS_ERROR;
    LWMsgCall* pCall = NULL;

    CONTAINER_LOCK();

    if (pInstance->State == LW_SERVICE_STATE_DEAD)
    {
        pInstance->State = LW_SERVICE_STATE_STOPPED;
    }

    if (pInstance->State != LW_SERVICE_STATE_RUNNING)
    {
        goto cleanup;
    }

    pInstance->State = LW_SERVICE_STATE_STOPPING;

    pInstance->In.tag = CONTAINER_REQ_STOP;
    pInstance->In.data = pInstance->pHandle;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pInstance->pContainer->pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    status = lwmsg_call_dispatch(pCall, &pInstance->In, &pInstance->Out, ContainerStopComplete, pInstance);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
    case LWMSG_STATUS_PENDING:
        break;
    default:
        dwError = MAP_LWMSG_STATUS(status);
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    CONTAINER_UNLOCK();

    if (status != LWMSG_STATUS_PENDING && pCall)
    {
        ContainerStopComplete(pCall, status, pInstance);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ContainerGetStatus(
    PLW_SERVICE_OBJECT pObject,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);

    CONTAINER_LOCK();

    pStatus->state = pInstance->State;
    if (pInstance->pContainer)
    {
        pStatus->pid = pInstance->pContainer->Pid;
        pStatus->home = pStatus->pid == getpid() ? LW_SERVICE_HOME_SERVICE_MANAGER : LW_SERVICE_HOME_CONTAINER;

    }
    else
    {
        pStatus->pid = -1;
        pStatus->home = LW_SERVICE_HOME_CONTAINER;
    }

    CONTAINER_UNLOCK();

    return dwError;
}

static
DWORD
ContainerRefresh(
    PLW_SERVICE_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    CONTAINER_LOCK();

    in.tag = CONTAINER_REQ_REFRESH;
    in.data = pInstance->pHandle;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pInstance->pContainer->pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    default:
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_STATUS:
        dwError = ((PCONTAINER_STATUS_RES) out.data)->Error;
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    CONTAINER_UNLOCK();

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ContainerSetLogInfo(
    PLW_SERVICE_OBJECT pObject,
    PCSTR pFacility,
    LW_SM_LOGGER_TYPE type,
    PCSTR pszTarget
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    CONTAINER_SET_LOG_INFO_REQ req = {0};

    CONTAINER_LOCK();

    if (pInstance->State != LW_SERVICE_STATE_RUNNING)
    {
        dwError = ERROR_NOT_READY;
        BAIL_ON_ERROR(dwError);
    }

    req.pFacility = (PSTR) pFacility;
    req.type = type;
    req.pszTarget = (PSTR) pszTarget;

    in.tag = CONTAINER_REQ_SET_LOG_TARGET;
    in.data = &req;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pInstance->pContainer->pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    default:
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_STATUS:
        dwError = ((PCONTAINER_STATUS_RES) pInstance->Out.data)->Error;
        if (!dwError)
        {
            dwError = ERROR_INTERNAL_ERROR;
        }
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_VOID:
        break;
    }

cleanup:

    CONTAINER_UNLOCK();

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ContainerSetLogLevel(
    PLW_SERVICE_OBJECT pObject,
    PCSTR pFacility,
    LW_SM_LOG_LEVEL Level
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    CONTAINER_SET_LOG_LEVEL_REQ req = {0};

    CONTAINER_LOCK();

    if (pInstance->State != LW_SERVICE_STATE_RUNNING)
    {
        dwError = ERROR_NOT_READY;
        BAIL_ON_ERROR(dwError);
    }

    req.pFacility = (PSTR) pFacility;
    req.Level = Level;

    in.tag = CONTAINER_REQ_SET_LOG_LEVEL;
    in.data = &req;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pInstance->pContainer->pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    default:
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_STATUS:
        dwError = ((PCONTAINER_STATUS_RES) pInstance->Out.data)->Error;
        if (!dwError)
        {
            dwError = ERROR_INTERNAL_ERROR;
        }
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_VOID:
        break;
    }

cleanup:

    CONTAINER_UNLOCK();

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ContainerGetLogState(
    PLW_SERVICE_OBJECT pObject,
    PCSTR pFacility,
    PLW_SM_LOGGER_TYPE pType,
    LW_PSTR* ppTarget,
    PLW_SM_LOG_LEVEL pLevel
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    CONTAINER_GET_LOG_STATE_REQ req = {0};
    PCONTAINER_GET_LOG_STATE_RES pRes = NULL;

    CONTAINER_LOCK();

    if (pInstance->State != LW_SERVICE_STATE_RUNNING)
    {
        dwError = ERROR_NOT_READY;
        BAIL_ON_ERROR(dwError);
    }

    req.pFacility = (PSTR) pFacility;

    in.tag = CONTAINER_REQ_GET_LOG_STATE;
    in.data = &req;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pInstance->pContainer->pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    default:
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_STATUS:
        dwError = ((PCONTAINER_STATUS_RES) pInstance->Out.data)->Error;
        if (!dwError)
        {
            dwError = ERROR_INTERNAL_ERROR;
        }
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_GET_LOG_STATE:
        pRes = out.data;
        *pType = pRes->type;
        *pLevel = pRes->Level;
        *ppTarget = pRes->pszTarget;
        pRes->pszTarget = NULL;
        break;
    }

cleanup:

    CONTAINER_UNLOCK();

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ContainerGetFacilityList(
    PLW_SERVICE_OBJECT pObject,
    PWSTR** pppFacilities
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    CONTAINER_LOCK();

    if (pInstance->State != LW_SERVICE_STATE_RUNNING)
    {
        dwError = ERROR_NOT_READY;
        BAIL_ON_ERROR(dwError);
    }

    in.tag = CONTAINER_REQ_GET_FACILITIES;
    in.data = NULL;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pInstance->pContainer->pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    default:
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_STATUS:
        dwError = ((PCONTAINER_STATUS_RES) pInstance->Out.data)->Error;
        if (!dwError)
        {
            dwError = ERROR_INTERNAL_ERROR;
        }
        BAIL_ON_ERROR(dwError);
    case CONTAINER_RES_GET_FACILITIES:
        *pppFacilities = out.data;
        out.data = NULL;
        break;
    }

cleanup:

    CONTAINER_UNLOCK();

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ContainerConstruct(
    PLW_SERVICE_OBJECT pObject,
    PCLW_SERVICE_INFO pInfo,
    PVOID* ppData
    )
{
    DWORD dwError = 0;
    PCONTAINER_INSTANCE pInstance = NULL;

    dwError = LwAllocateMemory(sizeof(*pInstance), OUT_PPVOID(&pInstance));
    BAIL_ON_ERROR(dwError);

    pInstance->pObject = pObject;
    pInstance->State = LW_SERVICE_STATE_STOPPED;
    pInstance->FdLimit = pInfo->dwFdLimit;
    pInstance->CoreSize = pInfo->dwCoreSize;
    pInstance->LogType = pInfo->DefaultLogType;
    pInstance->LogLevel = pInfo->DefaultLogLevel;

    dwError = LwAllocateWc16String(&pInstance->pName, pInfo->pwszName);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateWc16String(&pInstance->pPath, pInfo->pwszPath);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateWc16String(&pInstance->pGroup, pInfo->pwszGroup);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(pInfo->pDefaultLogTarget, &pInstance->pLogTarget);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(pInfo->ppwszArgs, &pInstance->ppArgs);
    BAIL_ON_ERROR(dwError);

    *ppData = pInstance;

error:

    return dwError;
}

static
VOID
ContainerDestruct(
    PLW_SERVICE_OBJECT pObject
    )
{
    PCONTAINER_INSTANCE pInstance = LwSmGetServiceObjectData(pObject);

    ContainerDeleteInstance(pInstance);
}

LW_SERVICE_LOADER_VTBL gContainerVtbl =
{
    .pfnStart = ContainerStart,
    .pfnStop = ContainerStop,
    .pfnGetStatus = ContainerGetStatus,
    .pfnRefresh = ContainerRefresh,
    .pfnConstruct = ContainerConstruct,
    .pfnDestruct = ContainerDestruct,
    .pfnSetLogInfo = ContainerSetLogInfo,
    .pfnSetLogLevel = ContainerSetLogLevel,
    .pfnGetLogState = ContainerGetLogState,
    .pfnGetFacilityList = ContainerGetFacilityList
};

static
VOID
ContainerCleanupNotify(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pContext
    )
{
    LwRtlSvcmUnload(pInstance);
    LwFreeMemory(pContext);
}

static
VOID
ContainerCleanup(
    PVOID pData
    )
{
    PCONTAINER_HANDLE pHandle = pData;

    if (pHandle->State == LW_SERVICE_STATE_RUNNING)
    {
        LwRtlSvcmStop(pHandle->pInstance, ContainerCleanupNotify, pHandle);
    }
    else if (pHandle->pInstance)
    {
        LwRtlSvcmUnload(pHandle->pInstance);
        LwFreeMemory(pHandle);
    }
}

static
VOID
ContainerStartNotify(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pContext
    )
{
    DWORD dwError = LwNtStatusToWin32Error(Status);
    DWORD savedError = 0;
    PCONTAINER_HANDLE pHandle = pContext;
    LWMsgCall* pCall = pHandle->pCall;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    LWMsgParams* pOut = lwmsg_call_get_user_data(pCall);
    LWMsgHandle* pIpcHandle = NULL;
    PCONTAINER_STATUS_RES pStatus = NULL;

    BAIL_ON_ERROR(dwError);

    pHandle->State = LW_SERVICE_STATE_RUNNING;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_register_handle(
        pSession,
        "CONTAINER_HANDLE",
        pHandle,
        ContainerCleanup,
        &pIpcHandle));
    BAIL_ON_ERROR(dwError);

    pOut->tag = CONTAINER_RES_HANDLE;
    pOut->data = pIpcHandle;
    lwmsg_session_retain_handle(pSession, pIpcHandle);
    pIpcHandle = NULL;

error:

    if (dwError)
    {
        savedError = dwError;
        dwError = LwAllocateMemory(sizeof(*pStatus), OUT_PPVOID(&pStatus));
        if (!dwError)
        {
            pOut->tag = CONTAINER_RES_STATUS;
            pOut->data = pStatus;
            pStatus->Error = savedError;
        }

        ContainerCleanup(pHandle);
    }

    lwmsg_call_complete(pCall, dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS);
}

static
VOID
ContainerStopNotify(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pContext
    )
{
    PCONTAINER_HANDLE pHandle = pContext;
    LWMsgCall* pCall = pHandle->pCall;
    LWMsgParams* pOut = lwmsg_call_get_user_data(pCall);
    DWORD savedError = 0;
    DWORD dwError = LwNtStatusToWin32Error(Status);
    PCONTAINER_STATUS_RES pStatus = NULL;

    BAIL_ON_ERROR(dwError);

    pHandle->State = LW_SERVICE_STATE_STOPPED;

error:

    LwRtlSvcmUnload(pHandle->pInstance);
    pHandle->pInstance = NULL;

    savedError = dwError;
    dwError = LwAllocateMemory(sizeof(*pStatus), OUT_PPVOID(&pStatus));
    if (!dwError)
    {
        pOut->tag = CONTAINER_RES_STATUS;
        pOut->data = pStatus;
        pStatus->Error = savedError;
    }

    lwmsg_call_complete(pCall, dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS);
}


static
VOID
ContainerRefreshNotify(
    PLW_SVCM_INSTANCE pInstance,
    NTSTATUS Status,
    PVOID pContext
    )
{
    DWORD dwError = LwNtStatusToWin32Error(Status);
    DWORD savedError = 0;
    LWMsgCall* pCall = pContext;
    LWMsgParams* pOut = lwmsg_call_get_user_data(pCall);
    PCONTAINER_STATUS_RES pStatus = NULL;

    savedError = dwError;

    dwError = LwAllocateMemory(sizeof(*pStatus), OUT_PPVOID(&pStatus));
    BAIL_ON_ERROR(dwError);

    pOut->tag = CONTAINER_RES_STATUS;
    pOut->data = pStatus;
    pStatus->Error = savedError;

error:

    lwmsg_call_complete(pCall, dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS);
}

static
LWMsgStatus
ContainerSrvStart(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCONTAINER_START_REQ pReq = (PCONTAINER_START_REQ) pIn->data;
    PCONTAINER_HANDLE pHandle = NULL;
    LW_SVCM_MODULE_ENTRY_FUNCTION entry = NULL;
    PSTR pName = NULL;
    int i = 0;
    PSTR pLogTarget = NULL;

    lwmsg_call_set_user_data(pCall, pOut);

    dwError = LwAllocateMemory(sizeof(*pHandle), OUT_PPVOID(&pHandle));
    BAIL_ON_ERROR(dwError);

    pHandle->State = LW_SERVICE_STATE_STOPPED;
    pHandle->pCall = pCall;

    switch (pReq->LogType)
    {
    case LW_SM_LOGGER_DEFAULT:
        break;
    case LW_SM_LOGGER_NONE:
        dwError = LwSmSetLogger(NULL, NULL, NULL);
        BAIL_ON_ERROR(dwError);
        break;
    case LW_SM_LOGGER_FILE:
        dwError = LwWc16sToMbs(pReq->pLogTarget, &pLogTarget);
        BAIL_ON_ERROR(dwError);
        dwError = LwSmSetLoggerToPath(NULL, pLogTarget);
        BAIL_ON_ERROR(dwError);
        break;
    case LW_SM_LOGGER_SYSLOG:
        dwError = LwSmSetLoggerToSyslog(NULL);
        BAIL_ON_ERROR(dwError);
        break;
    }

    switch (pReq->LogLevel)
    {
    case 0:
        break;
    default:
        dwError = LwSmSetMaxLogLevel(NULL, pReq->LogLevel);
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(pReq->pName, &pName);
    BAIL_ON_ERROR(dwError);

    for (i = 0; gEmbedded[i].pName; i++)
    {
        if (!strcmp(gEmbedded[i].pName, pName))
        {
            entry = gEmbedded[i].Entry;
            break;
        }
    }

    if (entry)
    {
        dwError = LwNtStatusToWin32Error(LwRtlSvcmLoadEmbedded(pReq->pName, entry, &pHandle->pInstance));
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        dwError = LwNtStatusToWin32Error(LwRtlSvcmLoadModule(pReq->pName, pReq->pPath, &pHandle->pInstance));
        BAIL_ON_ERROR(dwError);
    }

    dwError = SetLimits(pReq->FdLimit, pReq->CoreSize);
    BAIL_ON_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(LwRtlSvcmStart(
        pHandle->pInstance,
        pReq->ArgCount,
        pReq->ppArgs,
        pReq->FdCount,
        pReq->pFds,
        ContainerStartNotify,
        pHandle));
    BAIL_ON_ERROR(dwError);
    pHandle = NULL;

    lwmsg_call_pend(pCall, NULL, NULL);

error:

    LW_SAFE_FREE_MEMORY(pName);
    LW_SAFE_FREE_MEMORY(pLogTarget);

    if (pHandle)
    {
        if (pHandle->pInstance)
        {
            LwRtlSvcmUnload(pHandle->pInstance);
        }

        LwFreeMemory(pHandle);
    }

    return dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_PENDING;
}

static
LWMsgStatus
ContainerSrvStop(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LWMsgHandle* pIpcHandle = pIn->data;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    PCONTAINER_HANDLE pHandle = NULL;

    lwmsg_call_set_user_data(pCall, pOut);

    dwError = MAP_LWMSG_STATUS(lwmsg_session_get_handle_data(pSession, pIpcHandle, OUT_PPVOID(&pHandle)));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_session_unregister_handle(pSession, pIpcHandle));
    BAIL_ON_ERROR(dwError);

    pHandle->pCall = pCall;

    dwError = LwNtStatusToWin32Error(LwRtlSvcmStop(
           pHandle->pInstance,
           ContainerStopNotify,
           pHandle));
    BAIL_ON_ERROR(dwError);

error:

    return dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_PENDING;
}

static
LWMsgStatus
ContainerSrvRefresh(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    PCONTAINER_HANDLE pHandle = NULL;

    dwError = MAP_LWMSG_STATUS(lwmsg_session_get_handle_data(pSession, (LWMsgHandle*) pIn->data, OUT_PPVOID(&pHandle)));
    BAIL_ON_ERROR(dwError);

    lwmsg_call_set_user_data(pCall, pOut);

    dwError = LwNtStatusToWin32Error(LwRtlSvcmRefresh(pHandle->pInstance, ContainerRefreshNotify, pCall));
    BAIL_ON_ERROR(dwError);

    lwmsg_call_pend(pCall, NULL, NULL);

error:

    return dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_PENDING;
}

static
LWMsgStatus
ContainerSrvSetLogTarget(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCONTAINER_SET_LOG_INFO_REQ pReq = pIn->data;

    switch (pReq->type)
    {
    case LW_SM_LOGGER_NONE:
        dwError = LwSmSetLogger(pReq->pFacility, NULL, NULL);
        break;
    case LW_SM_LOGGER_FILE:
        dwError = LwSmSetLoggerToPath(pReq->pFacility, pReq->pszTarget);
        break;
    case LW_SM_LOGGER_SYSLOG:
        dwError = LwSmSetLoggerToSyslog(pReq->pFacility);
        break;
    case LW_SM_LOGGER_DEFAULT:
        if (!pReq->pFacility)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_ERROR(dwError);
        }
        dwError = LwSmSetLoggerToDefault(pReq->pFacility);
    }

    pOut->tag = CONTAINER_RES_VOID;

error:

    return dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
ContainerSrvSetLogLevel(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCONTAINER_SET_LOG_LEVEL_REQ pReq = pIn->data;

    dwError = LwSmSetMaxLogLevel(pReq->pFacility, pReq->Level);

    pOut->tag = CONTAINER_RES_VOID;

    return dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
ContainerSrvGetLogState(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCONTAINER_GET_LOG_STATE_REQ pReq = pIn->data;
    PCONTAINER_GET_LOG_STATE_RES pRes = NULL;

    dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmGetLoggerState(pReq->pFacility, &pRes->type, &pRes->pszTarget, &pRes->Level);
    BAIL_ON_ERROR(dwError);

    pOut->tag = CONTAINER_RES_GET_LOG_STATE;
    pOut->data = pRes;
    pRes = NULL;

error:

    if (pRes)
    {
        LwFreeMemory(pRes);
    }

    return dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
ContainerSrvGetFacilityList(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PWSTR* ppFacilities = NULL;

    dwError = LwSmGetLogFacilityList(&ppFacilities);
    BAIL_ON_ERROR(dwError);

    pOut->tag = CONTAINER_RES_GET_FACILITIES;
    pOut->data = ppFacilities;

error:

    return dwError ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS;
}

static
VOID
ContainerSrvRegisterCancel(
    LWMsgCall* pCall,
    PVOID pData
    )
{
    PCONTAINER pContainer = pData;

    CONTAINER_LOCK();
    pContainer->pSession = NULL;
    InvalidateContainer(pContainer);
    ReleaseContainer(pContainer, NULL);
    CONTAINER_UNLOCK();

    lwmsg_call_complete(pCall, LWMSG_STATUS_CANCELLED);
}

static
LWMsgStatus
ContainerSrvRegister(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = 0;
    DWORD savedError = 0;
    PCONTAINER_STATUS_RES pRes = NULL;
    CONTAINER_KEY key = {0};
    PCONTAINER pContainer = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    LWMsgSecurityToken* pToken = lwmsg_session_get_peer_security_token(pSession);
    pid_t pid = -1;

    key.pGroup = pIn->data;

    CONTAINER_LOCK();

    if (strcmp(lwmsg_security_token_get_type(pToken), "local"))
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_ERROR(dwError);
    }

    status = lwmsg_local_token_get_eid(pToken, &key.Uid, &key.Gid);
    assert(status == LWMSG_STATUS_SUCCESS);

    status = lwmsg_local_token_get_pid(pToken, &pid);
    assert(status == LWMSG_STATUS_SUCCESS);

    /* Restrict access to root for now */
    if (key.Uid != 0)
    {
        dwError = ERROR_ACCESS_DENIED;
        BAIL_ON_ERROR(dwError);
    }

    if (LwRtlHashTableFindKey(gpContainers, NULL, &key) == ERROR_SUCCESS)
    {
        dwError = ERROR_DUP_NAME;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pContainer), OUT_PPVOID(&pContainer));
    BAIL_ON_ERROR(dwError);

    LwSmLinkInit(&pContainer->Instances);
    pContainer->Pid = pid;
    pContainer->Refs = 1;
    pContainer->Sockets[0] = -1;
    pContainer->Sockets[1] = -1;
    pContainer->Key.Gid = key.Gid;
    pContainer->Key.Uid = key.Uid;
    pContainer->pSession = lwmsg_call_get_session(pCall);

    dwError = LwAllocateWc16String(&pContainer->Key.pGroup, key.pGroup);
    BAIL_ON_ERROR(dwError);

    LwRtlHashTableInsert(gpContainers, &pContainer->Node, NULL);

    lwmsg_call_pend(pCall, ContainerSrvRegisterCancel, pContainer);
    pContainer = NULL;

    pOut->tag = CONTAINER_RES_VOID;
    pOut->data = NULL;
    status = LWMSG_STATUS_PENDING;

error:

    if (dwError)
    {
        savedError = dwError;
        dwError = LwAllocateMemory(sizeof(*pRes), OUT_PPVOID(&pRes));
        if (!dwError)
        {
            pRes->Error = savedError;
            pOut->tag = CONTAINER_RES_STATUS;
            pOut->data = pRes;
            status = LWMSG_STATUS_SUCCESS;
        }
        else
        {
            status = LWMSG_STATUS_MEMORY;
        }

        if (pContainer)
        {
            ReleaseContainer(pContainer, NULL);
        }
    }

    CONTAINER_UNLOCK();

    return status;
}

static
LWMsgDispatchSpec gContainerDispatch[] =
{
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_START, ContainerSrvStart),
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_STOP, ContainerSrvStop),
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_REFRESH, ContainerSrvRefresh),
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_SET_LOG_TARGET, ContainerSrvSetLogTarget),
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_SET_LOG_LEVEL, ContainerSrvSetLogLevel),
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_GET_LOG_STATE, ContainerSrvGetLogState),
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_GET_FACILITIES, ContainerSrvGetFacilityList),
    LWMSG_DISPATCH_END
};

static
LWMsgDispatchSpec gContainerRegisterDispatch[] =
{
    LWMSG_DISPATCH_NONBLOCK(CONTAINER_REQ_REGISTER, ContainerSrvRegister),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
LwSmGetContainerDispatchSpec(
    VOID
    )
{
    return gContainerDispatch;
}

LWMsgDispatchSpec*
LwSmGetContainerRegisterDispatchSpec(
    VOID
    )
{
    return gContainerRegisterDispatch;
}

LWMsgProtocolSpec*
LwSmGetContainerProtocolSpec(
    VOID
    )
{
    return gContainerProtocol;
}

typedef struct _REGISTER_CONTEXT
{
    LWMsgParams in;
    LWMsgParams out;
} REGISTER_CONTEXT, *PREGISTER_CONTEXT;

static
VOID
RegisterComplete(
    LWMsgCall* pCall,
    LWMsgStatus Status,
    PVOID pData
    )
{
    DWORD dwError = MAP_LWMSG_STATUS(Status);
    PREGISTER_CONTEXT pContext = pData;

    BAIL_ON_ERROR(dwError);

    if (pContext->out.tag == CONTAINER_RES_STATUS)
    {
        dwError = ((PCONTAINER_STATUS_RES) pContext->out.data)->Error;
        BAIL_ON_ERROR(dwError);
    }

error:

    if (dwError)
    {
        SM_LOG_ERROR("Service container unregistered: %s", LwWin32ExtErrorToName(dwError));
    }
    else
    {
        SM_LOG_INFO("Service container unregistered");
    }

    lwmsg_call_destroy_params(pCall, &pContext->out);
    lwmsg_call_release(pCall);
    LW_SAFE_FREE_MEMORY(pContext);
    kill(getpid(), SIGTERM);
}

DWORD
LwSmContainerRegister(
    LWMsgPeer* pPeer,
    PWSTR pGroup
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = NULL;
    LWMsgCall* pCall = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PREGISTER_CONTEXT pContext = NULL;
    PSTR pGroupMbs = NULL;

    dwError = LwWc16sToMbs(pGroup, &pGroupMbs);
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    BAIL_ON_ERROR(dwError);

    pContext->in.tag = CONTAINER_REQ_REGISTER;
    pContext->in.data = pGroup;
    pContext->out.tag = LWMSG_TAG_INVALID;
    pContext->out.data = NULL;

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_connect(pPeer, &pSession));
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_session_acquire_call(pSession, &pCall));
    BAIL_ON_ERROR(dwError);

    SM_LOG_INFO("Registering as service container for group: %s", pGroupMbs);

    status = lwmsg_call_dispatch(pCall, &pContext->in, &pContext->out, RegisterComplete, pContext);

    switch (status)
    {
    case LWMSG_STATUS_PENDING:
        pCall = NULL;
        pContext = NULL;
        break;
    case LWMSG_STATUS_SUCCESS:
        dwError = ERROR_INTERNAL_ERROR;
        BAIL_ON_ERROR(dwError);
    default:
        dwError = MAP_LWMSG_STATUS(status);
        BAIL_ON_ERROR(dwError);
    }

error:

    LW_SAFE_FREE_MEMORY(pGroupMbs);
    LW_SAFE_FREE_MEMORY(pContext);

    if (pCall)
    {
        lwmsg_call_release(pCall);
    }

    return dwError;
}

DWORD
LwSmContainerInit(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = LwNtStatusToWin32Error(LwRtlCreateTaskGroup(gpPool, &gpContainerGroup));
    BAIL_ON_ERROR(dwError);

error:

    return dwError;
}

VOID
LwSmContainerShutdown(
    VOID
    )
{
    CONTAINER_LOCK();
    gContainerShutdown = TRUE;
    CONTAINER_UNLOCK();

    if (gpContainerGroup)
    {
        LwRtlCancelTaskGroup(gpContainerGroup);
        LwRtlWaitTaskGroup(gpContainerGroup);
        LwRtlFreeTaskGroup(&gpContainerGroup);
    }

    if (gpContainers)
    {
        /* Table should be empty at this point */
        LwRtlFreeHashTable(&gpContainers);
    }
}
