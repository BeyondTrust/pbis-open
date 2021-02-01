/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */



/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwtaskutils.h
 *
 * Abstract:
 *
 *        BeyondTrust Task System (LWTASK)
 *
 *        Utilities
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

/*
 * Logging
 */

#ifndef IsNullOrEmptyString
#define IsNullOrEmptyString(str) (!(str) || !(*(str)))
#endif

typedef enum
{
    LW_TASK_LOG_LEVEL_ALWAYS = 0,
    LW_TASK_LOG_LEVEL_ERROR,
    LW_TASK_LOG_LEVEL_WARNING,
    LW_TASK_LOG_LEVEL_INFO,
    LW_TASK_LOG_LEVEL_VERBOSE,
    LW_TASK_LOG_LEVEL_DEBUG

} LW_TASK_LOG_LEVEL;

typedef enum
{
    LW_TASK_LOG_TARGET_DISABLED = 0,
    LW_TASK_LOG_TARGET_CONSOLE,
    LW_TASK_LOG_TARGET_FILE,
    LW_TASK_LOG_TARGET_SYSLOG

} LW_TASK_LOG_TARGET;

typedef VOID (*PFN_LW_TASK_LOG_MESSAGE)(
                    HANDLE            hLog,
                    LW_TASK_LOG_LEVEL logLevel,
                    PCSTR             pszFormat,
                    va_list           msgList
                    );

typedef struct __LW_TASK_LOG_INFO
{
    LW_TASK_LOG_LEVEL  maxAllowedLogLevel;
    LW_TASK_LOG_TARGET logTarget;
    PSTR               pszPath;

} LW_TASK_LOG_INFO, *PLW_TASK_LOG_INFO;

extern HANDLE                  ghLwTaskLog;
extern LW_TASK_LOG_LEVEL       gLwTaskMaxLogLevel;
extern PFN_LW_TASK_LOG_MESSAGE gpfnLwTaskLogger;

#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gLwTaskLogLock;

#define LW_TASK_LOCK_LOGGER   pthread_mutex_lock(&gLwTaskLogLock)
#define LW_TASK_UNLOCK_LOGGER pthread_mutex_unlock(&gLwTaskLogLock)

#define _LW_TASK_LOG_PREFIX_THREAD(Format) \
        "0x%lx:" Format, ((unsigned long)pthread_self())

#else

#define LW_TASK_LOCK_LOGGER
#define LW_TASK_UNLOCK_LOGGER

#define _LW_TASK_LOG_PREFIX_THREAD(Format) (Format)

#endif

#define _LW_TASK_LOG_PREFIX_LOCATION(Format, Function, File, Line) \
    _LW_TASK_LOG_PREFIX_THREAD("[%s() %s:%d] " Format), \
    (Function), \
    (File), \
    (Line)

#define _LW_TASK_LOG_WITH_THREAD(Level, Format, ...) \
    _LW_TASK_LOG_MESSAGE(Level, \
                      _LW_TASK_LOG_PREFIX_THREAD(Format), \
                      ## __VA_ARGS__)

#define _LW_TASK_LOG_WITH_LOCATION(Level, Format, Function, File, Line, ...) \
    _LW_TASK_LOG_MESSAGE(Level, \
                  _LW_TASK_LOG_PREFIX_LOCATION(Format, Function, File, Line), \
                  ## __VA_ARGS__)

#define _LW_TASK_LOG_WITH_DEBUG(Level, Format, ...) \
    _LW_TASK_LOG_WITH_LOCATION(Level, Format, \
                            __FUNCTION__, __FILE__, __LINE__, \
                            ## __VA_ARGS__)

#define _LW_TASK_LOG_MESSAGE(Level, Format, ...) \
    LwTaskLogMessage(gpfnLwTaskLogger, ghLwTaskLog, Level, Format, ## __VA_ARGS__)

#define _LW_TASK_LOG_IF(Level, Format, ...)                     \
    do {                                                    \
        LW_TASK_LOCK_LOGGER;                                    \
        if (gpfnLwTaskLogger && (gLwTaskMaxLogLevel >= (Level)))  \
        {                                                   \
            if (gLwTaskMaxLogLevel >= LW_TASK_LOG_LEVEL_DEBUG)     \
            {                                               \
                _LW_TASK_LOG_WITH_DEBUG(Level, Format, ## __VA_ARGS__); \
            }                                               \
            else                                            \
            {                                               \
                _LW_TASK_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
            }                                               \
        }                                                   \
        LW_TASK_UNLOCK_LOGGER;                                  \
    } while (0)

#define LW_TASK_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define LW_TASK_LOG_ALWAYS(szFmt, ...) \
    _LW_TASK_LOG_IF(LW_TASK_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define LW_TASK_LOG_ERROR(szFmt, ...) \
    _LW_TASK_LOG_IF(LW_TASK_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define LW_TASK_LOG_WARNING(szFmt, ...) \
    _LW_TASK_LOG_IF(LW_TASK_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define LW_TASK_LOG_INFO(szFmt, ...) \
    _LW_TASK_LOG_IF(LW_TASK_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define LW_TASK_LOG_VERBOSE(szFmt, ...) \
    _LW_TASK_LOG_IF(LW_TASK_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define LW_TASK_LOG_DEBUG(szFmt, ...) \
    _LW_TASK_LOG_IF(LW_TASK_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)

#define BAIL_ON_LW_TASK_ERROR(dwError)                \
    if ((dwError)) {                                  \
       LW_TASK_LOG_DEBUG("Error [code: %d]", dwError);\
       goto error;                                    \
    }

#define BAIL_ON_NT_STATUS(_status_)             \
    do                                          \
    {                                           \
        if ((_status_) != STATUS_SUCCESS)       \
        {                                       \
            goto error;                         \
        }                                       \
    } while (0)

#define BAIL_ON_INVALID_POINTER(p)            \
        if (NULL == p) {                      \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_TASK_ERROR(dwError);    \
        }

#define BAIL_ON_INVALID_STRING(str)           \
        if (IsNullOrEmptyString(str)) {       \
           dwError = ERROR_INVALID_PARAMETER; \
           BAIL_ON_LW_TASK_ERROR(dwError);    \
        }

#define LW_TASK_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           LW_TASK_LOG_ERROR("Failed to lock mutex: %d. Aborting program", \
               thr_err); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LW_TASK_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           LW_TASK_LOG_ERROR("Failed to unlock mutex: %d. Aborting program", \
               thr_err); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

#define LW_TASK_LOCK_RWMUTEX_SHARED(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_rdlock(mutex); \
       if (thr_err) { \
           LW_TASK_LOG_ERROR("Failed to acquire shared lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LW_TASK_LOCK_RWMUTEX_EXCLUSIVE(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_wrlock(mutex); \
       if (thr_err) { \
           LW_TASK_LOG_ERROR("Failed to acquire exclusive lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LW_TASK_UNLOCK_RWMUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_rwlock_unlock(mutex); \
       if (thr_err) { \
           LW_TASK_LOG_ERROR("Failed to unlock rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

typedef VOID (*PFNLW_TASK_QUEUE_FUNC)(PVOID pData, PVOID pUserData);

typedef DWORD (*PFNLW_TASK_FOREACH_QUEUE_ITEM)(PVOID pItem, PVOID pUserData);

typedef struct __LW_TASK_QUEUE_ITEM
{
    PVOID pItem;

    struct __LW_TASK_QUEUE_ITEM * pNext;

} LW_TASK_QUEUE_ITEM, *PLW_TASK_QUEUE_ITEM;

typedef struct __LW_TASK_QUEUE
{

    PLW_TASK_QUEUE_ITEM pHead;
    PLW_TASK_QUEUE_ITEM pTail;

} LW_TASK_QUEUE, *PLW_TASK_QUEUE;

DWORD
LwTaskInitLogging(
    PCSTR              pszProgramName,
    LW_TASK_LOG_TARGET logTarget,
    LW_TASK_LOG_LEVEL  maxAllowedLogLevel,
    PCSTR              pszPath
    );

PCSTR
LwTaskLogLevelGetLabel(
    LW_TASK_LOG_LEVEL logLevel
    );

DWORD
LwTaskLogGetInfo(
    PLW_TASK_LOG_INFO* ppLogInfo
    );

DWORD
LwTaskLogSetInfo(
    PLW_TASK_LOG_INFO pLogInfo
    );

DWORD
LwTaskShutdownLogging(
    VOID
    );

VOID
LwTaskLogMessage(
    PFN_LW_TASK_LOG_MESSAGE pfnLogger,
    HANDLE                  hLog,
    LW_TASK_LOG_LEVEL       logLevel,
    PCSTR                   pszFormat,
    ...
    );

DWORD
LwTaskValidateLogLevel(
    DWORD dwLogLevel
    );

DWORD
LwTaskDuplicateArgList(
    PLW_TASK_ARG  pTaskArgArray,
    DWORD         dwNumArgs,
    PLW_TASK_ARG* ppTaskArgArray,
    PDWORD        pdwNumArgs
    );

VOID
LwTaskFreeTaskInfoArray(
    PLW_TASK_INFO pTaskInfoArray,
    DWORD         dwNumTaskInfos
    );

VOID
LwTaskFreeArgInfoArray(
    PLW_TASK_ARG_INFO pArgInfoArray,
    DWORD             dwNumArgInfos
    );

VOID
LwTaskFreeArgArray(
    PLW_TASK_ARG pArgArray,
    DWORD        dwNumArgs
    );

DWORD
LwTaskQueueCreate(
    PLW_TASK_QUEUE* ppQueue
    );

DWORD
LwTaskEnqueue(
    PLW_TASK_QUEUE pQueue,
    PVOID          pItem
    );

DWORD
LwTaskEnqueueFront(
    PLW_TASK_QUEUE pQueue,
    PVOID          pItem
    );

PVOID
LwTaskDequeue(
    PLW_TASK_QUEUE pQueue
    );

BOOLEAN
LwTaskQueueIsEmpty(
    PLW_TASK_QUEUE pQueue
    );

DWORD
LwTaskQueueForeach(
    PLW_TASK_QUEUE pQueue,
    PFNLW_TASK_FOREACH_QUEUE_ITEM pfnAction,
    PVOID pUserData
    );
VOID
LwTaskQueueFree(
    PLW_TASK_QUEUE pQueue
    );


