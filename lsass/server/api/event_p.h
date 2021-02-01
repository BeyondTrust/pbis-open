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
 *        event_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Eventlog API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EVENT_P_H__
#define __EVENT_P_H__

#ifdef ENABLE_EVENTLOG
typedef struct _EVENT_LOG_RECORD_QUEUE
{
    size_t sCapacity;
    size_t sSize;
    PEVENT_LOG_RECORD pRecords;
} EVENT_LOG_RECORD_QUEUE, *PEVENT_LOG_RECORD_QUEUE;

typedef struct _EVENTLOG_THREAD_STATE
{
    pthread_t writerThread;
    pthread_cond_t wakeUp;
    volatile BOOLEAN bShouldExit;

    pthread_mutex_t queueMutex;
    volatile PEVENT_LOG_RECORD_QUEUE pQueue;
    PSTR pszComputerName;
} EVENTLOG_THREAD_STATE;

#define LSA_MAX_EVENT_ERROR_BACKLOG 100

VOID
LsaSrvWriteLoginSuccessEvent(  
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPamSource,
    DWORD  dwFlags,
    DWORD  dwLoginPhase,
    DWORD  dwErrCode
    );

VOID
LsaSrvWriteLoginFailedEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPamSource,
    DWORD  dwFlags,
    DWORD  dwLoginPhase,
    DWORD  dwErrCode
    );

VOID
LsaSrvWriteLogoutSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    DWORD  dwLoginPhase,
    PCSTR  pszLoginId
    );

VOID
LsaSrvWriteUserPWChangeSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId
    );

VOID
LsaSrvWriteUserPWChangeFailureEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    );

VOID
LsaSrvEmptyQueue(
    PEVENT_LOG_RECORD_QUEUE pWriteQueue
    );

DWORD
LsaSrvSendQueue(
    PHANDLE phEventLog,
    PEVENT_LOG_RECORD_QUEUE pWriteQueue
    );

VOID *
LsaSrvEventWriterRoutine(
    PVOID pvUnused
    );

DWORD
LsaSrvQueueEvent(
    PEVENT_LOG_RECORD pEvent
    );
#else

static inline
VOID
LsaSrvWriteLoginSuccessEvent(  
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPamSource,
    DWORD  dwFlags,
    DWORD  dwLoginPhase,
    DWORD  dwErrCode
    )
{
}

static inline
VOID
LsaSrvWriteLoginFailedEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPamSource,
    DWORD  dwFlags,
    DWORD  dwLoginPhase,
    DWORD  dwErrCode
    )
{
}

static inline
VOID
LsaSrvWriteLogoutSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    DWORD  dwLoginPhase,
    PCSTR  pszLoginId
    )
{
}

static inline
VOID
LsaSrvWriteUserPWChangeSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId
    )
{
}

static inline
VOID
LsaSrvWriteUserPWChangeFailureEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
}

#endif

#endif /* __EVENT_P_H__ */
