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
 *        test-task.c
 *
 * Abstract:
 *
 *        Task manager unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include <lwmsg/lwmsg.h>
#include <moonunit/moonunit.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "util-private.h"
#include "test-private.h"
#include "task-private.h"

LWMsgTaskManager* manager = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t event = PTHREAD_COND_INITIALIZER;

MU_FIXTURE_SETUP(task)
{
    MU_TRY(lwmsg_task_acquire_manager(&manager));
}

MU_FIXTURE_TEARDOWN(task)
{
    lwmsg_task_release_manager(manager);
}

static
void
basic_work_item(
    void* data
    )
{
    LWMsgBool volatile *value = data;

    pthread_mutex_lock(&lock);
    *value = LWMSG_TRUE;
    pthread_cond_signal(&event);
    pthread_mutex_unlock(&lock);
}

MU_TEST(task, basic_work_item)
{
    LWMsgBool volatile value = LWMSG_FALSE;
    union
    {
        LWMsgBool volatile *bvalue;
        void* vvalue;
    } pun;

    pun.bvalue = &value;

    MU_TRY(lwmsg_task_dispatch_work_item(manager, basic_work_item, pun.vvalue));

    pthread_mutex_lock(&lock);
    while (!value)
    {
        pthread_cond_wait(&event, &lock);
    }
    pthread_mutex_unlock(&lock);
}

static const int target = 5;

static
void
timer_task(
    LWMsgTask* task,
    void* data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* next_trigger,
    LWMsgTaskTime* next_time
    )
{
    int volatile * value = data;

    if (trigger & LWMSG_TASK_TRIGGER_INIT)
    {
        MU_VERBOSE("Waking up for the first time");
    }
    else
    {
        MU_VERBOSE("Waking up on timer");
    }

    if (*value < target)
    {
        (*value)++;
        *next_trigger = LWMSG_TASK_TRIGGER_TIME;
        *next_time = 10000000ll;
    }
    else
    {
        *next_trigger = 0;
    }
}

MU_TEST(task, timer_task)
{
    int value = 0;
    LWMsgTask* task = NULL;

    MU_TRY(lwmsg_task_new(manager, NULL, timer_task, (void*) &value, &task));

    lwmsg_task_wake(task);
    lwmsg_task_wait(task);
    lwmsg_task_release(task);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, value, target);
}


