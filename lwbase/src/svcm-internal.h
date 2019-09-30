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
 * Module Name:
 *
 *        svcm-internal.h
 *
 * Abstract:
 *
 *        Service module API -- internal
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LW_SVCM_INTERNAL_H__
#define __LW_SVCM_INTERNAL_H__

#include <lw/svcm.h>
#include <lw/threadpool.h>
#include <pthread.h>
#include <assert.h>

#define LOCK_INSTANCE(i) (assert(pthread_mutex_lock(&i->Lock) == 0))
#define UNLOCK_INSTANCE(i) (assert(pthread_mutex_unlock(&i->Lock) == 0))
#define WAIT_INSTANCE(i) { while((i)->Busy) assert(pthread_cond_wait(&(i)->Event, &(i)->Lock) == 0); }
#define SIGNAL_INSTANCE(i) (assert(pthread_cond_signal(&(i)->Event) == 0))

typedef struct _SVCM_START_STATE
{
    PLW_SVCM_INSTANCE pInstance;
    ULONG ArgCount;
    PWSTR* ppArgs;
    ULONG FdCount;
    int* pFds;
    LW_SVCM_NOTIFY_FUNCTION Notify;
    PVOID pNotifyContext;
} SVCM_START_STATE, *PSVCM_START_STATE;

typedef struct _SVCM_COMMAND_STATE
{
    PLW_SVCM_INSTANCE pInstance;
    LW_SVCM_NOTIFY_FUNCTION Notify;
    PVOID pNotifyContext;
} SVCM_COMMAND_STATE, *PSVCM_COMMAND_STATE;

struct _LW_SVCM_INSTANCE
{
    PVOID pDlHandle;
    PSTR pServiceName;
    PLW_SVCM_MODULE pTable;
    PLW_WORK_ITEM pStopItem;
    PSVCM_COMMAND_STATE pStopState;

    /* the timeout in seconds before the shutdown 
     * timer kills the process */
    LW_DWORD ShutdownTimeout;

    PVOID pServiceData;
};

#endif
