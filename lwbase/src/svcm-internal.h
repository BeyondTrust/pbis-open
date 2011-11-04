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
    PLW_SVCM_MODULE pTable;
    PLW_WORK_ITEM pStopItem;
    PSVCM_COMMAND_STATE pStopState;
    PVOID pServiceData;
};

#endif
