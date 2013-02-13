/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        workqueue.h
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __IOTEST_WORK_QUEUE_H__
#define __IOTEST_WORK_QUEUE_H__

struct _IOTEST_WORK_ITEM;
typedef struct _IOTEST_WORK_ITEM *PIOTEST_WORK_ITEM;

typedef VOID (*PIOTEST_WORK_CALLBACK)(
    IN PIOTEST_WORK_ITEM pWorkItem,
    IN PVOID pContext
    );

struct _IOTEST_WORK_QUEUE;
typedef struct _IOTEST_WORK_QUEUE *PIOTEST_WORK_QUEUE;

NTSTATUS
ItCreateWorkItem(
    OUT PIOTEST_WORK_ITEM* ppWorkItem
    );

VOID
ItDestroyWorkItem(
    IN OUT PIOTEST_WORK_ITEM* ppWorkItem
    );

NTSTATUS
ItCreateWorkQueue(
    OUT PIOTEST_WORK_QUEUE* ppWorkQueue
    );

VOID
ItDestroyWorkQueue(
    IN OUT PIOTEST_WORK_QUEUE* ppWorkQueue
    );

NTSTATUS
ItAddWorkQueue(
    IN PIOTEST_WORK_QUEUE pWorkQueue,
    IN PIOTEST_WORK_ITEM pWorkItem,
    IN PVOID pContext,
    IN ULONG WaitSeconds,
    IN PIOTEST_WORK_CALLBACK Callback
    );

BOOLEAN
ItRemoveWorkQueue(
    IN PIOTEST_WORK_QUEUE pWorkQueue,
    IN PIOTEST_WORK_ITEM pWorkItem
    );

#endif /* __IOTEST_WORK_QUEUE_H__ */
