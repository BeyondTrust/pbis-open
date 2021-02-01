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

#include "config.h"
#include "ctbase.h"

void
CTInitRWLock(
    PCTRWLOCK pLock
    )
{
    pLock->num_active_readers = 0;
    pLock->num_active_writers = 0;
    pLock->num_waiting_readers = 0;
    pLock->num_waiting_writers = 0;

    pthread_mutex_init (&pLock->mutex, NULL);
    pthread_cond_init (&pLock->read_condition, NULL);
    pthread_cond_init (&pLock->write_condition, NULL);
}

DWORD
CTFreeRWLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);

    if (pLock->num_active_readers > 0 || pLock->num_active_writers > 0) {

        pthread_mutex_unlock (&pLock->mutex);

        return LwMapErrnoToLwError(EBUSY);
    }

    if (pLock->num_waiting_readers != 0 || pLock->num_waiting_writers != 0) {

        pthread_mutex_unlock (&pLock->mutex);

        return LwMapErrnoToLwError(EBUSY);
    }

    pthread_mutex_unlock (&pLock->mutex);

    pthread_mutex_destroy (&pLock->mutex);

    pthread_cond_destroy (&pLock->read_condition);

    pthread_cond_destroy (&pLock->write_condition);

    return ERROR_SUCCESS;
}

void
CTAcquireReadLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);

    if (pLock->num_active_writers) {
        pLock->num_waiting_readers++;
        while (pLock->num_active_writers) {
            pthread_cond_wait (&pLock->read_condition, &pLock->mutex);
        }
        pLock->num_waiting_readers--;
    }
    pLock->num_active_readers++;
    pthread_mutex_unlock (&pLock->mutex);
}

void
CTReleaseReadLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);
    pLock->num_active_readers--;
    if (pLock->num_active_readers == 0 && pLock->num_waiting_writers > 0)
    {
        pthread_cond_signal (&pLock->write_condition);
    }
    pthread_mutex_unlock (&pLock->mutex);
}

void
CTAcquireWriteLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);
    if (pLock->num_active_writers || pLock->num_active_readers > 0) {
        pLock->num_waiting_writers++;
        while (pLock->num_active_writers || pLock->num_active_readers > 0) {
            pthread_cond_wait (&pLock->write_condition, &pLock->mutex);
        }
        pLock->num_waiting_writers--;
    }
    pLock->num_active_writers = 1;
    pthread_mutex_unlock (&pLock->mutex);
}

void
CTReleaseWriteLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);
    pLock->num_active_writers = 0;
    if (pLock->num_waiting_readers > 0) {
        pthread_cond_broadcast (&pLock->read_condition);
    } else if (pLock->num_waiting_writers > 0) {
        pthread_cond_signal (&pLock->write_condition);
    }
    pthread_mutex_unlock (&pLock->mutex);
}

