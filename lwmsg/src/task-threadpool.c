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
 *        task-select.c
 *
 * Abstract:
 *
 *        Task manager API (select-based implementation)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include <errno.h>

#include "task-threadpool-private.h"

static struct
{
    LWMsgTaskTrigger trigger;
    LW_TASK_EVENT_MASK mask;
} mask_table[] =
{
    {LWMSG_TASK_TRIGGER_INIT, LW_TASK_EVENT_INIT},
    {LWMSG_TASK_TRIGGER_EXPLICIT, LW_TASK_EVENT_EXPLICIT},
    {LWMSG_TASK_TRIGGER_CANCEL, LW_TASK_EVENT_CANCEL},
    {LWMSG_TASK_TRIGGER_TIME, LW_TASK_EVENT_TIME},
    {LWMSG_TASK_TRIGGER_FD_READABLE, LW_TASK_EVENT_FD_READABLE},
    {LWMSG_TASK_TRIGGER_FD_WRITABLE, LW_TASK_EVENT_FD_WRITABLE},
    {LWMSG_TASK_TRIGGER_FD_EXCEPTION, LW_TASK_EVENT_FD_EXCEPTION},
    {LWMSG_TASK_TRIGGER_YIELD, LW_TASK_EVENT_YIELD},
    {0, 0},
};

static
LWMsgTaskTrigger
lwmsg_task_threadpool_event_to_lwmsg_event(
    LW_TASK_EVENT_MASK event
    )
{
    LWMsgTaskTrigger trigger = 0;
    unsigned int i = 0;

    for (i = 0; mask_table[i].trigger; i++)
    {
        if (event & mask_table[i].mask)
        {
            trigger |= mask_table[i].trigger;
        }
    }

    return trigger;
}

static
LW_TASK_EVENT_MASK
lwmsg_task_lwmsg_event_to_threadpool_event(
    LWMsgTaskTrigger trigger
    )
{
    LW_TASK_EVENT_MASK event = 0;
    unsigned int i = 0;

    for (i = 0; mask_table[i].trigger; i++)
    {
        if (trigger & mask_table[i].trigger)
        {
            event |= mask_table[i].mask;
        }
    }

    return event;
}

static
VOID
lwmsg_task_function_proxy(
    LW_IN PLW_TASK pTask,
    LW_IN LW_PVOID pContext,
    LW_IN LW_TASK_EVENT_MASK WakeMask,
    LW_IN LW_OUT LW_TASK_EVENT_MASK* pWaitMask,
    LW_IN LW_OUT LW_LONG64* pllTime
    )
{
    LWMsgTask* task = pContext;
    LWMsgTaskTrigger wait_trigger = 0;
    LWMsgTime new_time = {0, 0};

    if (*pllTime < 0)
    {
        new_time.seconds = -1;
        new_time.microseconds = -1;
    }
    else
    {
        new_time.seconds = *pllTime / 1000000000ll;
        new_time.microseconds = (*pllTime % 1000000000ll) / 1000;
    }

    task->real_function(
        task->task_data,
        lwmsg_task_threadpool_event_to_lwmsg_event(WakeMask),
        &wait_trigger,
        &new_time);

    if (lwmsg_time_is_positive(&new_time))
    {
        *pllTime = new_time.seconds * 1000000000ll + new_time.microseconds * 1000;
    }
    else
    {
        *pllTime = 0;
    }

    *pWaitMask = lwmsg_task_lwmsg_event_to_threadpool_event(wait_trigger);
}

LWMsgStatus
lwmsg_task_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup* group,
    LWMsgTaskFunction func,
    void* data,
    LWMsgTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTask* my_task = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_task));

    my_task->task_data = data;
    my_task->real_function = func;

    BAIL_ON_ERROR(status = MAP_NTSTATUS(
                      LwRtlCreateTask(
                          (PLW_THREAD_POOL) manager,
                          &my_task->real_task,
                          (PLW_TASK_GROUP) group,
                          lwmsg_task_function_proxy,
                          my_task)));

    *task = my_task;

cleanup:

    return status;

error:

    if (my_task)
    {
        lwmsg_task_release(my_task);
    }

    goto cleanup;
}

LWMsgStatus
lwmsg_task_group_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup** group
    )
{
    return MAP_NTSTATUS(
        LwRtlCreateTaskGroup(
            (PLW_THREAD_POOL) manager,
            (PLW_TASK_GROUP*) (PVOID) group));
}

void
lwmsg_task_release(
    LWMsgTask* task
    )
{
    LwRtlReleaseTask(&task->real_task);
    free(task);
}

void
lwmsg_task_group_delete(
    LWMsgTaskGroup* group
    )
{
    LwRtlFreeTaskGroup((PLW_TASK_GROUP*) (PVOID) &group);
}

void
lwmsg_task_set_trigger_fd(
    LWMsgTask* task,
    int fd
    )
{
    LwRtlSetTaskFd(task->real_task, fd,
                   LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE);
}

void
lwmsg_task_wake(
    LWMsgTask* task
    )
{
    LwRtlWakeTask(task->real_task);
}

void
lwmsg_task_cancel(
    LWMsgTask* task
    )
{
    LwRtlCancelTask(task->real_task);
}

void
lwmsg_task_wait(
    LWMsgTask* task
    )
{
    LwRtlWaitTask(task->real_task);
}

void
lwmsg_task_group_wake(
    LWMsgTaskGroup* group
    )
{
    LwRtlWakeTaskGroup((PLW_TASK_GROUP) group);
}

void
lwmsg_task_group_cancel(
    LWMsgTaskGroup* group
    )
{
    LwRtlCancelTaskGroup((PLW_TASK_GROUP) group);
}

void
lwmsg_task_group_wait(
    LWMsgTaskGroup* group
    )
{
    LwRtlWaitTaskGroup((PLW_TASK_GROUP) group);
}

LWMsgStatus
lwmsg_task_dispatch_work_item(
    LWMsgTaskManager* manager,
    LWMsgWorkItemFunction func,
    void* data
    )
{
    return MAP_NTSTATUS(
        LwRtlQueueWorkItem(
            (PLW_THREAD_POOL) manager,
            func,
            data,
            0));
}

LWMsgStatus
lwmsg_task_acquire_manager(
    LWMsgTaskManager** manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = MAP_NTSTATUS(
                      LwRtlCreateThreadPool(
                          (PLW_THREAD_POOL*) (PVOID) manager,
                          NULL)));

error:

    return status;
}

void
lwmsg_task_release_manager(
    LWMsgTaskManager* manager
    )
{
    LwRtlFreeThreadPool((PLW_THREAD_POOL*) (PVOID) &manager);
}
