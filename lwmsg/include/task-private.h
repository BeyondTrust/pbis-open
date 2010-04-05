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

#include <lwmsg/status.h>
#include <lwmsg/time.h>

/**
 * @file task-private.h
 * @brief Task Manager API
 * @internal
 */

/**
 * @internal
 * @defgroup tasks Task management
 * @ingroup public
 * @brief Event-triggered task subsystem
 *
 * The task manager API exposes a simple interface for creating
 * asynchronous tasks that wake up (run) whenever certain trigger
 * conditions are met, such as a file descriptor becoming readable
 * or a timeout expiring.  The task manager interface abstracts
 * away the underlying event polling mechanism and thread pool
 * architecture.
 */

/*@{*/

/**
 * @internal
 * @brief Task manager
 *
 * An opaque structure representing a particular task manager.
 * All tasks and task groups are created within the context of a task
 * manager which is responsible for their implementation and lifecycle.
 */
typedef struct LWMsgTaskManager LWMsgTaskManager;

/**
 * @internal
 * @brief Task group
 *
 * An opaque structure which represents a group of related tasks.
 * Task groups allow multiple tasks to be triggered, cancelled, or
 * waited upon simultaneously
 */
typedef struct LWMsgTaskGroup LWMsgTaskGroup;

/**
 * @internal
 * @brief Task
 *
 * An opaque structure representing a single task.  Each task has
 * an associated #LWMsgTaskFunction which is run whenever the
 * conditions for task wakeup are met.
 */
typedef struct LWMsgTask LWMsgTask;

/**
 * @internal
 * @brief Trigger condition mask
 *
 * A bitmask representing the conditions under which a
 * task will be woken.
 */
typedef enum LWMsgTaskTrigger
{
    /**
     * @internal
     * @brief Task initialized
     *
     * A special bit which indicates that a task has been
     * run for the first time when passed as the trigger
     * parameter to an #LWMsgTaskFunction
     *
     * @hideinitializer
     */
    LWMSG_TASK_TRIGGER_INIT         = 0x01,
    
    /**
     * @internal
     * @brief Task explicitly woken
     *
     * Indicates that the task was explicitly woken by
     * an external caller, such as through #lwmsg_task_wake()
     * or #lwmsg_task_cancel()
     *
     * @hideinitializer
     */
    LWMSG_TASK_TRIGGER_EXPLICIT     = 0x02,

    /**
     * @internal
     * @brief Task cancelled
     *
     * Indicates that the task has been cancelled.
     * Once cancelled, this bit will continue to be set
     * on the trigger parameter of the #LWMsgTaskFunction
     * for all subsequent wakeups until the task completes.
     *
     * @hideinitializer
     */
    LWMSG_TASK_TRIGGER_CANCEL       = 0x04,
    
    /**
     * @internal
     * @brief Timeout expired
     *
     * Indicates that the last set timeout has expired.
     *
     * @hideinitializer
     */
    LWMSG_TASK_TRIGGER_TIME         = 0x08,

    /**
     * @internal
     * @brief File descriptor readable
     *
     * Indicates that a file descriptor has become readable.
     *
     * @hideinitializer
     */
    LWMSG_TASK_TRIGGER_FD_READABLE  = 0x10,

    /**
     * @internal
     * @brief File descriptor writable
     *
     * Indicates that a file descriptor has become writable.
     *
     * @hideinitializer
     */
    LWMSG_TASK_TRIGGER_FD_WRITABLE  = 0x20,

    /**
     * @internal
     * @brief File descriptor exception
     *
     * Indicates that an exception event occurred on a file
     * descriptor.
     *
     * @hideinitializer
     */
    LWMSG_TASK_TRIGGER_FD_EXCEPTION = 0x40
} LWMsgTaskTrigger;

/**
 * @internal
 * @brief Main task function
 *
 * A function which is run whenever the trigger conditions of
 * a task are met.  Before returning, the function should set
 * the new_trigger parameter to the mask of conditions under
 * which it should next wake up, and the time parameter to the
 * next wakeup timeout.  Setting an empty mask indicates that
 * the task is complete and should no longer be scheduled to run.
 * 
 * If a non-negative timeout is set and the task is woken before
 * the timeout expires, on the next call to the function the time
 * parameter will contain how much time was remaining.  Note that
 * the time parameter will be updated in this manner even if
 * #LWMSG_TASK_TRIGGER_TIME is not one of the trigger conditions.
 * Once the time reaches 0, it will not decrease further.
 *
 * @param[in] task_data a user data pointer
 * @param[in] trigger the mask of trigger conditions which were met
 * @param[out] new_trigger the mask of trigger conditions for the next wakeup
 * @param[in,out] time the time left before a timeout is triggered
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
typedef LWMsgStatus (*LWMsgTaskFunction)(
    void* task_data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* new_trigger,
    LWMsgTime* time
    );

/**
 * @internal
 * @brief Work item function
 *
 * A function which is dispatched to a thread pool by
 * #lwmsg_task_dispatch_work_item().  Unlike tasks, work item
 * functions are synchronous and may block.
 *
 * @param[in] work_data user data pointer
 */
typedef void (*LWMsgWorkItemFunction)(
    void* work_data
    );

/**
 * @internal
 * @brief Create a new task
 *
 * Creates a new task with the specified task manager within
 * the specified group.  The task will be immediately ready to
 * run but will not be woken until an explicit trigger is caused
 * by #lwmsg_task_wake() or similar.  On first wakeup, the
 * #LWMSG_TASK_TRIGGER_INIT bit will be set on the trigger
 * parameter to the #LWMsgTaskFunction.
 *
 * When the task is no longer referenced externally (outside of
 * the #LWMsgTaskFunction itself), it should be released with
 * #lwmsg_task_release().  The task will not be completely freed
 * until it has been released and the #LWMsgTaskFunction has
 * indicated completion by setting an empty wakeup trigger mask.
 *
 * @param[in] manager the task manager
 * @param[in] group the task group which the created task will belong to; may be NULL
 * @param[in] func the function to be run whenever a task wakeup is triggered
 * @param[in] data a user data pointer to pass to func
 * @param[out] task the created task
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_task_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup* group,
    LWMsgTaskFunction func,
    void* data,
    LWMsgTask** task
    );

/**
 * @internal
 * @brief Create a new task group
 *
 * Creates a new task group.  Whenever tasks are created, they may optionally
 * be assigned to a task group.  All tasks in a group can be conveniently
 * woken, cancelled, or waited upon simultaneously.
 *
 * @param[in] manager the task manager
 * @param[out] group the created task group
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_task_group_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup** group
    );

/**
 * @internal
 * @brief Release task
 *
 * Indicates that the given task will no longer be externally referenced
 * (i.e. outside of its own #LWMsgTaskFunction).
 *
 * @param[in,out] task the task
 */
void
lwmsg_task_release(
    LWMsgTask* task
    );

/**
 * @internal
 * @brief Delete task group
 *
 * Deletes the given task group.  The results of calling this function
 * are undefined if incomplete tasks still remain inside the group.
 * To be safe, first call #lwmsg_task_group_cancel() and #lwmsg_task_group_wake()
 * to ensure that all tasks in the group have completed.
 *
 * @param[in] group the task group
 */
void
lwmsg_task_group_delete(
    LWMsgTaskGroup* group
    );

/**
 * @internal
 * @brief Set file descriptor for wakeup triggers
 *
 * Sets the file descriptor which will be used for the
 * LWMSG_TASK_TRIGGER_FD_* trigger conditions.  The result of calling
 * this function from outside of an #LWMsgTaskFunction is undefined.
 *
 * @param task the task
 * @param fd the file descriptor to use for subsequent trigger conditions
 */
void
lwmsg_task_set_trigger_fd(
    LWMsgTask* task,
    int fd
    );

/**
 * @internal
 * @brief Manually wake task
 *
 * Causes the specified task to wake up immediately with the
 * #LWMSG_TASK_TRIGGER_EXPLICIT flag set.
 */
void
lwmsg_task_wake(
    LWMsgTask* task
    );

/**
 * @internal
 * @brief Cancel task
 *
 * Cancels the specified task.  In addition to having the same
 * immediate effects as #lwmsg_task_wake(), the #LWMSG_TASK_TRIGGER_CANCEL
 * flag will be set on all task wakeups until the task completes.
 *
 * @param[in] task the task
 */
void
lwmsg_task_cancel(
    LWMsgTask* task
    );

/**
 * @internal
 * @brief Wait for task completion
 *
 * Waits for the specified task to complete by indicating an empty
 * wakeup trigger mask.
 *
 * @param[in] task the task
 */
void
lwmsg_task_wait(
    LWMsgTask* task
    );

/**
 * @internal
 * @brief Wake task group
 *
 * Equivalent to calling #lwmsg_task_wake() on all tasks in the
 * given task group.
 *
 * @param[in] group the task group
 */
void
lwmsg_task_group_wake(
    LWMsgTaskGroup* group
    );

/**
 * @internal
 * @brief Cancel task group
 *
 * Equivalent to calling #lwmsg_task_cancel() on all tasks in the
 * given task group.
 *
 * @param[in] group the task group
 */
void
lwmsg_task_group_cancel(
    LWMsgTaskGroup* group
    );

/**
 * @internal
 * @brief Wait for task group to complete
 *
 * Equivalent to calling #lwmsg_task_wait() on all tasks in the
 * given task group.
 *
 * @param[in] group the task group
 */
void
lwmsg_task_group_wait(
    LWMsgTaskGroup* group
    );

/**
 * @internal
 * @brief Dispatch synchronous work item
 *
 * Schedules a work item to be run in another thread.  Unlike tasks,
 * work items are not asynchronous and may block arbitrarily.  This
 * functionality is provided as a convenience for tasks which may need
 * to make calls into blocking code.
 *
 * @param[in] manager the task manager
 * @param[in] func the work item function
 * @param[in] data a user data pointer to pass to func
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_task_dispatch_work_item(
    LWMsgTaskManager* manager,
    LWMsgWorkItemFunction func,
    void* data
    );

/**
 * @internal
 * @brief Acquire task manager
 *
 * Acquires a reference a task manager.
 * The caller should retain this reference as long as it wishes
 * to schedule tasks and work items.
 *
 * @param[out] manager the task manager
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_task_acquire_manager(
    LWMsgTaskManager** manager
    );

/**
 * @internal
 * @brief Release reference to task manager
 *
 * Releases a reference to a task manager.
 * Before calling this function, the caller must ensure that
 * all of its tasks have completed and been released, and that
 * all of its task groups have been deleted.
 *
 * @param[in] manager the task manager
 */
void
lwmsg_task_release_manager(
    LWMsgTaskManager* manager
    );

/*@}*/

#endif
