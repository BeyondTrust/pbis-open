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
 *        prodcons.c
 *
 * Abstract:
 *
 *        BeyondTrust Task Services
 *
 *        Utilities
 *
 *        Producer Consumer Queue
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

DWORD
LwTaskProdConsInit(
    ULONG                                 ulNumMaxItems,
    PFN_LW_TASK_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem,
    PLW_TASK_PROD_CONS_QUEUE*             ppQueue
    )
{
    DWORD dwError = 0;
    PLW_TASK_PROD_CONS_QUEUE pQueue = NULL;

    if (!ulNumMaxItems)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(LW_TASK_PROD_CONS_QUEUE),
                    (PVOID*)&pQueue);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskProdConsInitContents(
                    pQueue,
                    ulNumMaxItems,
                    pfnFreeItem);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppQueue = pQueue;

cleanup:

    return dwError;

error:

    *ppQueue = NULL;

    if (pQueue)
    {
        LwTaskProdConsFree(pQueue);
    }

    goto cleanup;
}

DWORD
LwTaskProdConsInitContents(
    PLW_TASK_PROD_CONS_QUEUE              pQueue,
    ULONG                                 ulNumMaxItems,
    PFN_LW_TASK_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem
    )
{
    DWORD dwError = 0;

    memset(pQueue, 0, sizeof(*pQueue));

    pthread_mutex_init(&pQueue->mutex, NULL);
    pQueue->pMutex = &pQueue->mutex;

    pQueue->ulNumMaxItems = ulNumMaxItems;
    pQueue->pfnFreeItem = pfnFreeItem;

    pthread_cond_init(&pQueue->event, NULL);
    pQueue->pEvent = &pQueue->event;

    return dwError;
}

DWORD
LwTaskProdConsEnqueue(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    )
{
    DWORD    dwError = 0;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bSignalEvent = FALSE;

    LW_TASK_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    dwError = LwTaskEnqueue(&pQueue->queue, pItem);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!pQueue->ulNumItems)
    {
        bSignalEvent = TRUE;
    }

    pQueue->ulNumItems++;

    if (bSignalEvent)
    {
        pthread_cond_broadcast(&pQueue->event);
    }

cleanup:

    LW_TASK_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskProdConsEnqueueFront(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    )
{
    DWORD    dwError = 0;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bSignalEvent = FALSE;

    LW_TASK_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    dwError = LwTaskEnqueueFront(&pQueue->queue, pItem);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!pQueue->ulNumItems)
    {
        bSignalEvent = TRUE;
    }

    pQueue->ulNumItems++;

    if (bSignalEvent)
    {
        pthread_cond_broadcast(&pQueue->event);
    }

cleanup:

    LW_TASK_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwTaskProdConsDequeue(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    PVOID*               ppItem
    )
{
    DWORD    dwError = 0;
    BOOLEAN  bInLock = FALSE;
    PVOID    pItem = NULL;
    BOOLEAN  bSignalEvent = FALSE;

    LW_TASK_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (!pQueue->ulNumItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    pItem = LwTaskDequeue(&pQueue->queue);

    if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        bSignalEvent = TRUE;
    }

    pQueue->ulNumItems--;

    if (bSignalEvent)
    {
        pthread_cond_broadcast(&pQueue->event);
    }

    LW_TASK_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    *ppItem = pItem;

    return dwError;
}

DWORD
LwTaskProdConsTimedDequeue(
    PLW_TASK_PROD_CONS_QUEUE pQueue,
    struct timespec*         pTimespec,
    PVOID*                   ppItem
    )
{
    DWORD    dwError = 0;
    BOOLEAN  bInLock = FALSE;
    PVOID    pItem = NULL;
    BOOLEAN  bSignalEvent = FALSE;

    LW_TASK_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (!pQueue->ulNumItems)
    {
        BOOLEAN bRetryWait = FALSE;

        do
        {
            bRetryWait = FALSE;

            int unixErrorCode = pthread_cond_timedwait(
                                    &pQueue->event,
                                    &pQueue->mutex,
                                    pTimespec);
            if (unixErrorCode == ETIMEDOUT)
            {
                if (time(NULL) < pTimespec->tv_sec)
                {
                    bRetryWait = TRUE;
                    continue;
                }
            }

            switch (unixErrorCode)
            {
				case ETIMEDOUT:

					dwError = ERROR_TIMEOUT;

					break;

				default:

					dwError = LwErrnoToWin32Error(unixErrorCode);
            }

            /* Don't use BAIL_ON_XXX() here to reduce unnecessary noise */
            if (dwError != ERROR_SUCCESS)
            {
                goto error;
            }
        } while (bRetryWait);
    }

    pItem = LwTaskDequeue(&pQueue->queue);

    if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        bSignalEvent = TRUE;
    }

    pQueue->ulNumItems--;

    LW_TASK_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    if (bSignalEvent)
    {
        pthread_cond_broadcast(&pQueue->event);
    }

    *ppItem = pItem;

cleanup:

    LW_TASK_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return dwError;

error:

    *ppItem = NULL;

    goto cleanup;
}

VOID
LwTaskProdConsFree(
    PLW_TASK_PROD_CONS_QUEUE pQueue
    )
{
    LwTaskProdConsFreeContents(pQueue);

    LwFreeMemory(pQueue);
}

VOID
LwTaskProdConsFreeContents(
    PLW_TASK_PROD_CONS_QUEUE pQueue
    )
{
    if (pQueue->pMutex)
    {
        pthread_mutex_lock(pQueue->pMutex);
    }

    if (pQueue->pEvent)
    {
        pthread_cond_destroy(&pQueue->event);
        pQueue->pEvent = NULL;
    }

    if (pQueue->pfnFreeItem)
    {
        PVOID pItem = NULL;

        while ((pItem = LwTaskDequeue(&pQueue->queue)) != NULL)
        {
            pQueue->pfnFreeItem(pItem);
        }
    }

    if (pQueue->pMutex)
    {
        pthread_mutex_unlock(&pQueue->mutex);
        pthread_mutex_destroy(pQueue->pMutex);
        pQueue->pMutex = NULL;
    }
}
