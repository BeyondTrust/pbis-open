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
