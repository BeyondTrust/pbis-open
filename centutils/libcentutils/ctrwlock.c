/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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

