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
 *        threadpool.h
 *
 * Abstract:
 *
 *        Thread pool API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWBASE_THREADPOOL_H__
#define __LWBASE_THREADPOOL_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <signal.h>

LW_BEGIN_EXTERN_C

/**
 * @file threadpool.h
 * @brief Thread pool API
 */

/**
 * @defgroup threadpool Thread pool
 * @brief Event-triggered task subsystem
 *
 * The thread pool API exposes a simple interface for creating
 * asynchronous tasks that wake up (run) whenever certain trigger
 * conditions are met, such as a file descriptor becoming readable
 * or a timeout expiring.  The thread pool interface abstracts
 * away the underlying event polling mechanism and thread
 * architecture.
 */

/*@{*/

/**
 * @brief Thread pool
 *
 * An opaque structure representing a particular thread pool.
 * All tasks and task groups are created within the context of a thread
 * pool which is responsible for their implementation and lifecycle.
 */
typedef struct _LW_THREAD_POOL LW_THREAD_POOL, *PLW_THREAD_POOL;

/**
 * @brief Task group
 *
 * An opaque structure which represents a group of related tasks.
 * Task groups allow multiple tasks to be triggered, cancelled, or
 * waited upon simultaneously
 */
typedef struct _LW_TASK_GROUP LW_TASK_GROUP, *PLW_TASK_GROUP;

/**
 * @brief Task
 *
 * An opaque structure representing a single task.  Each task has
 * an associated #LW_TASK_FUNCTION which is run whenever the
 * conditions for task wakeup are met.
 */
typedef struct _LW_TASK LW_TASK, *PLW_TASK;

/**
 * @brief Thread pool attributes
 *
 * An opaque data structure which can be used to specify a set of
 * custom attributes when creating a thread pool
 */
typedef struct _LW_THREAD_POOL_ATTRIBUTES LW_THREAD_POOL_ATTRIBUTES, *PLW_THREAD_POOL_ATTRIBUTES;

/**
 * @brief Event mask
 *
 * A bitmask representing a set of events.
 */
typedef enum _LW_TASK_EVENT_MASK
{
    /**
     * A special value which indicates that
     * the task is complete when returned by an
     * #LW_TASK_FUNCTION through the pWaitMask parameter.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_COMPLETE     = 0x000,
    /**
     * A special bit which indicates that a task has been
     * run for the first time when passed as WakeMask
     * parameter to an #LW_TASK_FUNCTION
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_INIT         = 0x001,
    
    /**
     * Indicates that the task was explicitly woken by
     * an external caller, such as through #LwRtlWakeTask()
     * or #LwRtlCancelTask()
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_EXPLICIT     = 0x002,

    /**
     * Indicates that the task has been cancelled.
     * Once cancelled, this bit will continue to be set
     * on the WakeMask parameter of the #LW_TASK_FUNCTION
     * for all subsequent wakeups until the task completes.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_CANCEL       = 0x004,
    
    /**
     * Indicates that the last set timeout has expired.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_TIME         = 0x008,

    /**
     * Indicates that a file descriptor has become readable.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_FD_READABLE  = 0x010,

    /**
     * Indicates that a file descriptor has become writable.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_FD_WRITABLE  = 0x020,

    /**
     * Indicates that an exception event occurred on a file
     * descriptor.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_FD_EXCEPTION = 0x040,
    
    /**
     * Indicates that the task should be immediately run
     * again after other tasks have had a chance to run.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_YIELD        = 0x080,

    /**
     * Indicates that one or more UNIX signals which the task
     * subscribes to have arrived.
     *
     * @hideinitializer
     */
    LW_TASK_EVENT_UNIX_SIGNAL  = 0x100
} LW_TASK_EVENT_MASK, *PLW_TASK_EVENT_MASK;

/**
 * @brief Scheduling flags
 *
 * Controls how work items are scheduled.
 */
typedef enum LW_SCHEDULE_FLAGS
{
    /**
     * Indicates the the work item should be placed at the front
     * of the work queue.
     *
     * @hideinitializer
     */
    LW_SCHEDULE_HIGH_PRIORITY         = 0x00010000
} LW_SCHEDULE_FLAGS;

#ifndef DOXYGEN
#define LW_WORK_ITEM_FLAGS LW_SCHEDULE_FLAGS
#define LW_WORK_ITEM_HIGH_PRIOTIRY LW_SCHEDULE_HIGH_PRIORITY
#endif

/**
 * @brief Work item structure
 *
 * Opaque structure representing a work item
 */
typedef struct _LW_WORK_ITEM LW_WORK_ITEM, *PLW_WORK_ITEM;


/**
 * @brief Thread pool option
 *
 * An option that can be set on a #LW_THREAD_POOL_ATTRIBUTES
 * structure.  Each option documents the types of the trailing arguments
 * that should be passed to #LwRtlSetThreadPoolAttribute().
 */
typedef enum LW_THREAD_POOL_OPTION
{
    /**
     * (BOOLEAN) Delegate tasks to the global thread pool rather than creating
     * private task threads.  This is the recommended behavior.
     * (Default: TRUE)
     */
    LW_THREAD_POOL_OPTION_DELEGATE_TASKS,
    /**
     * (LONG) Number of task threads to create.  A negative number indicates
     * a multiple of the number of CPUs present on the system.
     * (Default: -1)
     */
    LW_THREAD_POOL_OPTION_TASK_THREADS,
    /**
     * (LONG) Maximum number of work threads to create.  A negative number
     * indicates a multiple of the number of CPUs present on the system.
     * (Default: -4)
     */
    LW_THREAD_POOL_OPTION_WORK_THREADS,
    /**
     * (ULONG) Size of thread stacks for task threads in bytes.  0 indicates
     * the system default.
     * (Default: 0)
     */
    LW_THREAD_POOL_OPTION_TASK_THREAD_STACK_SIZE,
    /**
     * (ULONG) Size of thread stacks for work threads in bytes.  0 indicates
     * the system default.
     * (Default: 0)
     */
    LW_THREAD_POOL_OPTION_WORK_THREAD_STACK_SIZE,
    /**
     * (ULONG) Time in seconds before an idle work thread exits.  0 indicates
     * no timeout.
     * (Default: 30)
     */
    LW_THREAD_POOL_OPTION_WORK_THREAD_TIMEOUT
} LW_THREAD_POOL_OPTION;

/**
 * @brief Main task function
 *
 * A function which is run whenever the trigger conditions of
 * a task are met.  Before returning, the function should set
 * the pWaitMask parameter to the mask of conditions under
 * which it should next wake up, and the pllTime parameter to the
 * next timeout.
 * 
 * If a non-zero time is set and the task is woken before
 * the time expires, on the next call to the function the pllTime
 * parameter will contain how much time was remaining.  Note that
 * the time parameter will be updated in this manner even if
 * #LW_TASK_EVENT_TIME is not one of the trigger conditions.
 * Once the time reaches 0, it will not decrease further.
 *
 * Several trigger flags have special significance:
 *
 * - #LW_TASK_EVENT_INIT<br>
 *   This flag is always set on the first wakeup and never on
 *   subsequent wakeups.
 *
 * - #LW_TASK_EVENT_EXPLICIT<br>
 *   Explicit wakeups always take effect, even if #LW_TASK_EVENT_EXPLICIT
 *   was not set in the wait mask.  It is still useful to set this flag in
 *   the wait mask if the task wishes to wait only for explicit wakeups.
 *
 * - #LW_TASK_EVENT_CANCEL<br>
 *   Once a task is cancelled, this flag will always be set on
 *   every subsequent wakeup until the task completes.
 *
 * - #LW_TASK_EVENT_YIELD<br>
 *   If this flag is set in the wait mask, it indicates that the task is
 *   not yet finished doing work and should be run again immediately
 *   after giving other tasks a chance to run.  When the task is run again,
 *   the wake mask will contain #LW_TASK_EVENT_YIELD in addition to any
 *   other events that occurred in the interim.
 *
 * - #LW_TASK_EVENT_COMPLETE<br>
 *   If the wait mask is set to this value, the task will be considered
 *   complete and will no longer run.  Callers waiting in #LwRtlWaitTask
 *   and related functions will be unblocked.
 *
 * @param[in] pTask the task
 * @param[in] pContex user context pointer
 * @param[in] WakeMask the mask of trigger conditions which woke the task
 * @param[in, out] pWaitMask the mask of trigger conditions to wait for
 * before the next wakeup
 * @param[in,out] pllTime the time remaining until the next timeout (in
 * nanoseconds).
 */
typedef
LW_VOID
(*LW_TASK_FUNCTION)(
    LW_IN PLW_TASK pTask,
    LW_IN LW_PVOID pContext,
    LW_IN LW_TASK_EVENT_MASK WakeMask,
    LW_IN LW_OUT LW_TASK_EVENT_MASK* pWaitMask,
    LW_IN LW_OUT LW_LONG64* pllTime
    );

/**
 * @brief Work item function
 *
 * A function which is called to run a work item
 * created with #LwRtlCreateWorkItem().  A work
 * item function -- unlike a task function -- may
 * safely block.
 *
 * @param[in] pWorkItem the work item
 * @param[in] pContext the user context pointer
 */
typedef
LW_VOID
(*LW_WORK_ITEM_FUNCTION)(
    PLW_WORK_ITEM pWorkItem,
    PVOID pContext
    );

#ifndef DOXYGEN
typedef
LW_VOID
(*LW_WORK_ITEM_FUNCTION_COMPAT)(
    LW_PVOID pContext
    );
#endif

/**
 * @brief Create a new task
 *
 * Creates a new task with the specified thread pool within
 * the specified group.  The task will be immediately ready to
 * run but will not be woken until an explicit trigger is caused
 * by #LwRtlWakeTask() or similar.  On first wakeup, the
 * #LW_TASK_EVENT_INIT bit will be set on WakeMask parameter
 * to the #LW_TASK_FUNCTION.
 *
 * When the task is no longer referenced externally (outside of
 * the #LW_TASK_FUNCTION itself), it should be released with
 * #LwRtlReleaseTask().  The task will not be completely freed
 * until it has been released and the #LW_TASK_FUNCTION has
 * indicated completion by setting a wake mask of #LW_TASK_EVENT_COMPLETE.
 *
 * @param[in] pPool the thread pool
 * @param[out] ppTask the created task
 * @param[in] pGroup an optional task group for the task
 * @param[in] pfnFunc the task wakeup function
 * @param[in] pContext the user context pointer passed to pfnFunc
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 * @retval #LW_STATUS_CANCELLED the specified task group was cancelled
 */
LW_NTSTATUS
LwRtlCreateTask(
    LW_IN PLW_THREAD_POOL pPool,
    LW_OUT PLW_TASK* ppTask,
    LW_IN LW_OPTIONAL PLW_TASK_GROUP pGroup,
    LW_IN LW_TASK_FUNCTION pfnFunc,
    LW_IN LW_PVOID pContext
    );

/**
 * @brief Create a new task group
 *
 * Creates a new task group.  Whenever tasks are created, they may optionally
 * be assigned to a task group.  All tasks in a group can be conveniently
 * woken, cancelled, or waited upon simultaneously.
 *
 * @param[in] pPool the thread pool
 * @param[out] ppGroup the created task
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlCreateTaskGroup(
    LW_IN PLW_THREAD_POOL pPool,
    LW_OUT PLW_TASK_GROUP* ppGroup
    );

/**
 * @brief Release task
 *
 * Indicates that the given task will no longer be externally referenced
 * (i.e. outside of its own #LW_TASK_FUNCTION).  This function may safely
 * be called with *ppTask equal to NULL.  It sets *ppTask to NULL if it
 * was not already.
 *
 * @param[in,out] ppTask a pointer to the task pointer to release
 */
LW_VOID
LwRtlReleaseTask(
    LW_IN LW_OUT PLW_TASK* ppTask
    );

/**
 * @brief Free task group
 *
 * Deletes the given task group. This function may safely be called
 * with *ppGroup equal to NULL.  It sets *ppGroup to NULL if it was not already.
 *
 * @warning The results of calling this function
 * are undefined if incomplete tasks still remain inside the group.
 * To be safe, first call #LwRtlCancelTaskGroup() and #LwRtlWaitTaskGroup()
 * to ensure that all tasks in the group have completed.
 *
 * @param[in,out] ppGroup a pointer to the group pointer to free
 */
LW_VOID
LwRtlFreeTaskGroup(
    LW_IN LW_OUT PLW_TASK_GROUP* ppGroup
    );

/**
 * @brief Configure file descriptor for wakeup events
 *
 * Configures a file descriptor for subsequent wakeup events.
 * A file descriptor remains configured until it is explictly
 * unconfigured by calling this function with an empty trigger mask.
 * A wakeup on a given file descriptor trigger condition (e.g.
 * LW_TASK_EVENT_FD_READ) will occur only when the condition is
 * set both on the mask passed to this function and in the mask
 * specified when the #LW_TASK_FUNCTION returns.
 *
 * @warning The result of calling this function from outside of an
 * #LW_TASK_FUNCTION is undefined.
 *
 * @param[in] pTask the task
 * @param[in] Fd the file descriptor
 * @param[in] Mask the mask of trigger conditions which for which
 * wakeups will be generated for the given descriptor
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 * @retval #LW_STATUS_INVALID_HANDLE the provided fd was invalid
 */
LW_NTSTATUS
LwRtlSetTaskFd(
    LW_IN PLW_TASK pTask,
    LW_IN int Fd,
    LW_IN LW_TASK_EVENT_MASK Mask
    );

/**
 * @brief Query file descriptor trigger mask
 *
 * Queries for the most recent mask of trigger conditions which
 * are set for the given file descriptor.
 *
 * @warning The result of calling this function from outside of an
 * #LW_TASK_FUNCTION is undefined.
 *
 * @param[in] pTask the task
 * @param[in] Fd the file descriptor
 * @param[out] pMask the mask of trigger conditions which were true
 * for the given fd on this wakeup
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INVALID_HANDLE the provided fd was invalid or
 * was not previously set on the given task
 */
LW_NTSTATUS
LwRtlQueryTaskFd(
    LW_IN PLW_TASK pTask,
    LW_IN int Fd,
    LW_OUT PLW_TASK_EVENT_MASK pMask
    );

/**
 * @brief Configure UNIX signal wakeup events
 *
 * Configures whether the given task will be notified
 * when the specified UNIX signal is delivered to the process.
 *
 * Note that if a task is configured to receive any UNIX signals,
 * it must wait for the #LW_TASK_EVENT_UNIX_SIGNAL event and reap
 * any delivered signals with #LwRtlNextTaskUnixSignal().  Failure
 * to do so can cause the main signal dispatch loop to hang.
 *
 * @warning The result of calling this function from outside of an
 * #LW_TASK_FUNCTION is undefined.
 *
 * @param[in,out] pTask the task
 * @param[in] Sig the signal number
 * @param[in] bSubscribe whether the task should be notified
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INVALID_PARAMETER the provided signal was invalid
 * @retval #LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlSetTaskUnixSignal(
    LW_IN PLW_TASK pTask,
    LW_IN int Sig,
    LW_IN LW_BOOLEAN bSubscribe
    );

/**
 * @brief Get next UNIX signal delivered to task
 *
 * Gets the next UNIX signal which was delivered to the specified task.
 * Because #LW_TASK_EVENT_UNIX_SIGNAL events are edge-triggered, you should
 * continue calling this function as long as it returns TRUE in order to
 * ensure that no signals are missed.
 *
 * @warning The result of calling this function from outside of an
 * #LW_TASK_FUNCTION is undefined.
 *
 * @param[in,out] pTask the task
 * @param[out] pInfo information about the UNIX signal
 * @retval TRUE a signal was pending, and information about it was placed in pInfo
 * @retval FALSE no signal was pending
 */
LW_BOOLEAN
LwRtlNextTaskUnixSignal(
    LW_IN PLW_TASK pTask,
    LW_OUT siginfo_t* pInfo
    );

/**
 * @brief Manually wake task
 *
 * Causes the specified task to wake up immediately with the
 * #LW_TASK_EVENT_EXPLICIT flag set.
 *
 * @param[in] pTask the task
 */
LW_VOID
LwRtlWakeTask(
    LW_IN PLW_TASK pTask
    );

/**
 * @brief Cancel task
 *
 * Cancels the specified task.  In addition to having the same
 * immediate effects as #LwRtlWakeTask(), the #LW_TASK_EVENT_CANCEL
 * flag will be set on all task wakeups until the task completes.
 *
 * @param[in] pTask the task
 */
LW_VOID
LwRtlCancelTask(
    LW_IN PLW_TASK pTask
    );

/**
 * @brief Wait for task completion
 *
 * Blocks until the specified task completes by setting
 * #LW_TASK_EVENT_COMPLETE as its wait mask.
 *
 * @param[in] pTask the task
 */
LW_VOID
LwRtlWaitTask(
    LW_IN PLW_TASK pTask
    );

/**
 * @brief Wake task group
 *
 * Equivalent to calling #LwRtlWakeTask() on all tasks in the
 * given task group.
 *
 * @param[in] pGroup the task group
 */
LW_VOID
LwRtlWakeTaskGroup(
    LW_IN PLW_TASK_GROUP pGroup
    );

/**
 * @brief Cancel task group
 *
 * All tasks in the task group are cancelled as if by #LwRtlCancelTask().
 * In addition, any subsequent attempts to create a task within the group
 * will fail with #LW_STATUS_CANCELLED.
 *
 * @param[in] pGroup the task group
 */
LW_VOID
LwRtlCancelTaskGroup(
    LW_IN PLW_TASK_GROUP pGroup
    );

/**
 * @brief Wait for task group to complete
 *
 * Equivalent to calling #LwRtlWaitTask() on all tasks in the
 * given task group.
 *
 * @param[in] pGroup the task group
 */
LW_VOID
LwRtlWaitTaskGroup(
    LW_IN PLW_TASK_GROUP pGroup
    );

/**
 * @brief Create work item
 *
 * Creates a work item which can be scheduled to run
 * with #LwRtlScheduleWorkItem().  Work items -- unlike tasks --
 * may block.
 *
 * @param[in] pPool thread pool which will own the work item
 * @param[out] ppWorkItem set to the created work item
 * @param[in] pfnFunc work item function to run
 * @param[in] pContext user context to pass to the callback
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlCreateWorkItem(
    LW_IN PLW_THREAD_POOL pPool,
    LW_OUT PLW_WORK_ITEM* ppWorkItem,
    LW_IN LW_WORK_ITEM_FUNCTION pfnFunc,
    LW_IN PVOID pContext
    );

/**
 * @brief Free work item
 *
 * Frees a work item.  Once a work item has been scheduled,
 * this function may only be called safely from the work item
 * function.
 *
 * @param[in,out] ppWorkItem if *ppWorkItem is not NULL, it is freed and set to NULL
 */
LW_VOID
LwRtlFreeWorkItem(
    LW_IN LW_OUT PLW_WORK_ITEM* ppWorkItem
    );

/**
 * @brief Schedule blocking work item
 *
 * Schedules a work item to be run with the specified scheduling flags.
 *
 * @param[in] pWorkItem the work item
 * @param[in] Flags scheduling control flags
 */
LW_VOID
LwRtlScheduleWorkItem(
    LW_IN PLW_WORK_ITEM pWorkItem,
    LW_IN LW_SCHEDULE_FLAGS Flags
    );

/**
 * @brief Wait for scheduled work items to complete
 *
 * Blocks until all scheduled work items have finished running.
 *
 * @warning It is up to the user to ensure that no further work
 * items will be scheduled.  Otherwise, this function may return
 * even if work items are still scheduled, or block forever.
 *
 * @warning Calling this function from a work item function
 * will cause a deadlock
 */
LW_VOID
LwRtlWaitWorkItems(
    LW_IN PLW_THREAD_POOL pPool
    );

#ifndef DOXYGEN
/* Deprecated compat function */
LW_NTSTATUS
LwRtlQueueWorkItem(
    LW_IN PLW_THREAD_POOL pPool,
    LW_IN LW_WORK_ITEM_FUNCTION_COMPAT pfnFunc,
    LW_IN LW_PVOID pContext,
    LW_IN LW_WORK_ITEM_FLAGS Flags
    );
#endif

/**
 * @brief Create thread pool attributes structure
 *
 * Creates a new thread pool attributes structure.
 *
 * @param[out] ppAttrs the created structure
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlCreateThreadPoolAttributes(
    LW_OUT PLW_THREAD_POOL_ATTRIBUTES* ppAttrs
    );

/**
 * @brief Set thread pool attribute
 *
 * Sets the specified option on the given thread pool attributes
 * structure.  See the documentation for #LW_THREAD_POOL_OPTION for
 * available options and the variable argument types for each.
 *
 * @param[in,out] pAttrs the attributes structure
 * @param[in] Option the option to set
 * @param[in] ... parameters for the particular option
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_NOT_SUPPORTED the thread pool implementation does not support that
 * particular option
 */
LW_NTSTATUS
LwRtlSetThreadPoolAttribute(
    LW_IN LW_OUT PLW_THREAD_POOL_ATTRIBUTES pAttrs,
    LW_IN LW_THREAD_POOL_OPTION Option,
    ...
    );

/**
 * @brief Free thread pool attributes
 *
 * Frees a thread pool attributes structure.
 *
 * @param[in,out] ppAttrs a reference to the pointer to free. *ppAttrs
 * may be NULL when called and will be set to NULL before returning.
 */
VOID
LwRtlFreeThreadPoolAttributes(
    LW_IN LW_OUT PLW_THREAD_POOL_ATTRIBUTES* ppAttrs
    );

/**
 * @brief Create new thread pool
 *
 * Creates a new thread pool.
 *
 * @param[out] ppPool the created pool
 * @param[in] pAttrs optional thread pool attributes
 * @retval #LW_STATUS_SUCCESS success
 * @retval #LW_STATUS_INSUFFICIENT_RESOURCES out of memory
 */
LW_NTSTATUS
LwRtlCreateThreadPool(
    LW_OUT PLW_THREAD_POOL* ppPool,
    LW_IN LW_OPTIONAL PLW_THREAD_POOL_ATTRIBUTES pAttrs
    );

/**
 * @brief Free thread pool
 *
 * Frees the given thread pool.  It is safe for *ppPool
 * to be NULL.  This function will set *ppPool to NULL if
 * it was not already.
 *
 * @param[in,out] ppPool a pointer to the thread pool pointer
 */
LW_VOID
LwRtlFreeThreadPool(
    LW_IN LW_OUT PLW_THREAD_POOL* ppPool
    );

/**
 * @brief Main signal loop
 *
 * Runs the threadpool main signal loop.  You must call this function from
 * your program's main thread if you wish to use UNIX signal events with
 * your threadpool tasks.
 *
 * This function will not return until #LwRtlExitMain() is called.
 *
 * @return the status passed to #LwRtlExitMain()
 */
LW_NTSTATUS
LwRtlMain(
    LW_VOID
    );

/**
 * @brief Exit main loop
 *
 * Causes #LwRtlMain() to return with the specified status code
 *
 * @param[in] Status the status that #LwRtlMain() should return
 */
LW_VOID
LwRtlExitMain(
    LW_NTSTATUS Status
    );

/**
 * @brief Block all signals in the current thread
 *
 * Blocks all signals in the current thread.  It is recommended that
 * your application call this function from the main thread before creating
 * any other threads.  This guarantees that all signals will be properly
 * delivered to #LwRtlMain().
 *
 * @retval LW_STATUS_SUCCESS success
 */
LW_NTSTATUS
LwRtlBlockSignals(
    VOID
    );

/*@}*/

LW_END_EXTERN_C

#endif /* __LWBASE_THREADPOOL_H__ */
