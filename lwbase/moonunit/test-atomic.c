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

#include <moonunit/moonunit.h>
#include <lw/base.h>
#include <pthread.h>

#define INCREMENT_TIMES 100
#define INCREMENT_THREADS 100
#define EXCHANGE_THREADS 10

typedef struct _EXCHANGE_INFO
{
    LONG volatile * plValue;
    LONG lOldValue;
    LONG lNewValue;
} EXCHANGE_INFO, *PEXCHANGE_INFO;

static pthread_mutex_t barrierMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t barrierCond = PTHREAD_COND_INITIALIZER;
static BOOLEAN bBarrierOpen = FALSE;

static
VOID
BarrierWait(
    void
    )
{
    pthread_mutex_lock(&barrierMutex);
    while (!bBarrierOpen)
    {
        pthread_cond_wait(&barrierCond, &barrierMutex);
    }
    pthread_mutex_unlock(&barrierMutex);
}

static
VOID
BarrierOpen(
    void
    )
{
    pthread_mutex_lock(&barrierMutex);
    bBarrierOpen = TRUE;
    pthread_cond_broadcast(&barrierCond);
    pthread_mutex_unlock(&barrierMutex);
}

static
PVOID
AtomicIncrementThread(
    PVOID pData
    )
{
    LONG volatile * plValue = (LONG*) pData;
    int i = 0;

    BarrierWait();

    for (i = 0; i < INCREMENT_TIMES; i++)
    {
        InterlockedIncrement(plValue);
    }

    return NULL;
}

static
PVOID
AtomicDecrementThread(
    PVOID pData
    )
{
    LONG volatile * plValue = (LONG*) pData;
    int i = 0;

    BarrierWait();

    for (i = 0; i < INCREMENT_TIMES; i++)
    {
        InterlockedDecrement(plValue);
    }

    return NULL;
}

static
PVOID
AtomicExchangeThread(
    PVOID pData
    )
{
    PEXCHANGE_INFO pInfo = (PEXCHANGE_INFO) pData;
    LONG lOldValue = 0;

    BarrierWait();

    do
    {
        lOldValue = InterlockedCompareExchange(
            pInfo->plValue,
            pInfo->lNewValue,
            pInfo->lOldValue);
    } while (lOldValue != pInfo->lOldValue);

    return NULL;
}

MU_TEST(Atomic, InterlockedIncrement)
{
    pthread_t threads[INCREMENT_THREADS];
    int i;
    LONG volatile lValue = 0;

    for (i = 0; i < INCREMENT_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, AtomicIncrementThread, (void*) &lValue);
    }

    BarrierOpen();

    for (i = 0; i < INCREMENT_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, InterlockedRead(&lValue), INCREMENT_THREADS * INCREMENT_TIMES);
}

MU_TEST(Atomic, InterlockedDecrement)
{
    pthread_t threads[INCREMENT_THREADS];
    int i;
    LONG volatile lValue = 0;

    for (i = 0; i < INCREMENT_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, AtomicDecrementThread, (void*) &lValue);
    }

    BarrierOpen();

    for (i = 0; i < INCREMENT_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, InterlockedRead(&lValue), - (INCREMENT_THREADS * INCREMENT_TIMES));
}

MU_TEST(Atomic, InterlockedCompareExchange)
{
    pthread_t threads[EXCHANGE_THREADS];
    EXCHANGE_INFO info[EXCHANGE_THREADS];
    int i;
    LONG volatile lValue = 0;
    
    for (i = 0; i < EXCHANGE_THREADS; i++)
    {
        info[i].plValue = &lValue;
        info[i].lOldValue = i;
        info[i].lNewValue = i+1;
        pthread_create(&threads[i], NULL, AtomicExchangeThread, &info[i]);
    }

    BarrierOpen();

    for (i = 0; i < EXCHANGE_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, InterlockedRead(&lValue), EXCHANGE_THREADS);
}
