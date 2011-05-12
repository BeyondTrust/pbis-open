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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        task.h
 *
 * Abstract:
 *
 *        Task manager API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */
#ifndef __LWMSG_TASK_H__
#define __LWMSG_TASK_H__

#include <lw/base.h>


/*
 * The task manager code was moved into lwbase and became the threadpool
 * API.  This is now just a wrapper around it.
 */

typedef LW_THREAD_POOL LWMsgTaskManager;
typedef LW_TASK_GROUP LWMsgTaskGroup;
typedef LW_TASK LWMsgTask;
typedef LW_TASK_EVENT_MASK LWMsgTaskTrigger;
typedef LONG64 LWMsgTaskTime;

#define LWMSG_TASK_TRIGGER_INIT         LW_TASK_EVENT_INIT
#define LWMSG_TASK_TRIGGER_EXPLICIT     LW_TASK_EVENT_EXPLICIT
#define LWMSG_TASK_TRIGGER_CANCEL       LW_TASK_EVENT_CANCEL
#define LWMSG_TASK_TRIGGER_TIME         LW_TASK_EVENT_TIME
#define LWMSG_TASK_TRIGGER_FD_READABLE  LW_TASK_EVENT_FD_READABLE
#define LWMSG_TASK_TRIGGER_FD_WRITABLE  LW_TASK_EVENT_FD_WRITABLE
#define LWMSG_TASK_TRIGGER_FD_EXCEPTION LW_TASK_EVENT_FD_EXCEPTION
#define LWMSG_TASK_TRIGGER_YIELD        LW_TASK_EVENT_YIELD
#define LWMSG_TASK_TRIGGER_UNIX_SIGNAL  LW_TASK_EVENT_UNIX_SIGNAL

typedef LW_TASK_FUNCTION LWMsgTaskFunction;
typedef LW_WORK_ITEM_FUNCTION_COMPAT LWMsgWorkItemFunction;

static inline
LWMsgStatus
lwmsg_error_map_ntstatus(
    NTSTATUS status
    )
{
    switch (status)
    {
    case STATUS_SUCCESS:
        return LWMSG_STATUS_SUCCESS;
    case STATUS_PENDING:
        return LWMSG_STATUS_PENDING;
    case STATUS_INSUFFICIENT_RESOURCES:
        return LWMSG_STATUS_MEMORY;
    default:
        return LWMSG_STATUS_ERROR;
    }
}

#define __MAP_NTSTATUS(x) lwmsg_error_map_ntstatus((x))

static inline
LWMsgStatus
lwmsg_task_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup* group,
    LWMsgTaskFunction func,
    void* data,
    LWMsgTask** task
    )
{
    return __MAP_NTSTATUS(
        LwRtlCreateTask(
            manager,
            task,
            group,
            func,
            data));
}

static inline
LWMsgStatus
lwmsg_task_group_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup** group
    )
{
    return __MAP_NTSTATUS(
        LwRtlCreateTaskGroup(
            manager,
            group));
}

static inline
void
lwmsg_task_release(
    LWMsgTask* task
    )
{
    LwRtlReleaseTask(&task);
}

static inline
void
lwmsg_task_group_delete(
    LWMsgTaskGroup* group
    )
{
    LwRtlFreeTaskGroup(&group);
}

static inline
LWMsgStatus
lwmsg_task_set_trigger_fd(
    LWMsgTask* task,
    int fd
    )
{
    return __MAP_NTSTATUS(
        LwRtlSetTaskFd(
            task,
            fd,
            LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE));
}

static inline
LWMsgStatus
lwmsg_task_unset_trigger_fd(
    LWMsgTask* task,
    int fd
    )
{
    return __MAP_NTSTATUS(
        LwRtlSetTaskFd(
            task,
            fd,
            0));
}

static inline
LWMsgBool
lwmsg_task_set_unix_signal(
    LWMsgTask* task,
    int sig,
    LWMsgBool disp
    )
{
    return __MAP_NTSTATUS(
        LwRtlSetTaskUnixSignal(task, sig, disp));
}

static inline
LWMsgBool
lwmsg_task_next_unix_signal(
    LWMsgTask* task,
    siginfo_t* info
    )
{
    return LwRtlNextTaskUnixSignal(task, info);
}

static inline
void
lwmsg_task_wake(
    LWMsgTask* task
    )
{
    LwRtlWakeTask(task);
}

static inline
void
lwmsg_task_cancel(
    LWMsgTask* task
    )
{
    LwRtlCancelTask(task);
}

static inline
void
lwmsg_task_wait(
    LWMsgTask* task
    )
{
    LwRtlWaitTask(task);
}

static inline
void
lwmsg_task_group_wake(
    LWMsgTaskGroup* group
    )
{
    LwRtlWakeTaskGroup(group);
}

static inline
void
lwmsg_task_group_cancel(
    LWMsgTaskGroup* group
    )
{
    LwRtlCancelTaskGroup(group);
}

static inline
void
lwmsg_task_group_wait(
    LWMsgTaskGroup* group
    )
{
    LwRtlWaitTaskGroup(group);
}

static inline
LWMsgStatus
lwmsg_task_dispatch_work_item(
    LWMsgTaskManager* manager,
    LWMsgWorkItemFunction func,
    void* data
    )
{
    return __MAP_NTSTATUS(
        LwRtlQueueWorkItem(
            manager,
            func,
            data,
            0));
}

static inline
LWMsgStatus
lwmsg_task_acquire_manager(
    LWMsgTaskManager** manager
    )
{
    return __MAP_NTSTATUS(
        LwRtlCreateThreadPool(
            (PLW_THREAD_POOL*) (PVOID) manager,
            NULL));
}

static inline
void
lwmsg_task_release_manager(
    LWMsgTaskManager* manager
    )
{
    LwRtlFreeThreadPool(&manager);
}

/*@}*/

#endif
