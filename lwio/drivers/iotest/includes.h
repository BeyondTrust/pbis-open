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
 *        includes.h
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 *        Internal Includes
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include "config.h"
#include "lwiosys.h"

#include <lw/rtlstring.h>
#include <lw/rtlgoto.h>

#include <lwio/iodriver.h>

#include "ntlogmacros.h"
#include "lwioutils.h"

#include "workqueue.h"

#define IOTEST_DEVICE_NAME "iotest"
#define IOTEST_DEVICE_PATH "/" IOTEST_DEVICE_NAME

#define IOTEST_INTERNAL_PATH_NAMED_PIPE "/pipe"
#define IOTEST_INTERNAL_PATH_ALLOW "/allow"
#define IOTEST_INTERNAL_PATH_ASYNC "/async"
#define IOTEST_INTERNAL_PATH_TEST_SYNC "/test/sync"
#define IOTEST_INTERNAL_PATH_TEST_ASYNC "/test/async"

#define IOTEST_PATH_ALLOW IOTEST_DEVICE_PATH IOTEST_INTERNAL_PATH_ALLOW
#define IOTEST_PATH_ASYNC IOTEST_DEVICE_PATH IOTEST_INTERNAL_PATH_ASYNC

#include <iotestctl.h>

//
// Driver State
//

typedef struct _IT_DRIVER_STATE {
    PIOTEST_WORK_QUEUE pWorkQueue;
} IT_DRIVER_STATE, *PIT_DRIVER_STATE;

PIT_DRIVER_STATE
ItGetDriverState(
    IN PIRP pIrp
    );

//
// IRP Context
//

struct _IT_IRP_CONTEXT;
typedef struct _IT_IRP_CONTEXT IT_IRP_CONTEXT, *PIT_IRP_CONTEXT;

typedef VOID (*IT_CONTINUE_CALLBACK)(
    IN PIT_IRP_CONTEXT pIrpContext
    );

struct _IT_IRP_CONTEXT {
    PIRP pIrp;
    PIOTEST_WORK_ITEM pWorkItem;
    BOOLEAN IsCancelled;
    IT_CONTINUE_CALLBACK ContinueCallback;
    PVOID ContinueContext;
};

NTSTATUS
ItCreateIrpContext(
    OUT PIT_IRP_CONTEXT* ppIrpContext,
    IN PIRP pIrp
    );

VOID
ItDestroyIrpContext(
    IN OUT PIT_IRP_CONTEXT* ppIrpContext
    );

//
// Async Helper
//

NTSTATUS
ItDispatchAsync(
    IN PIRP pIrp,
    IN ULONG WaitSeconds,
    IN IT_CONTINUE_CALLBACK ContinueCallback,
    IN PVOID ContinueContext
    );

VOID
ItAsyncCompleteSetEvent(
    IN PVOID pCallbackContext
    );

VOID
ItSimpleSuccessContinueCallback(
    IN PIT_IRP_CONTEXT pIrpContext
    );

//
// Create Control Block (CCB)
//

typedef struct _IT_CCB {
    UNICODE_STRING Path;
    BOOLEAN IsNamedPipe;
} IT_CCB, *PIT_CCB;

NTSTATUS
ItpCreateCcb(
    OUT PIT_CCB* ppCcb,
    IN PUNICODE_STRING pPath
    );

VOID
ItpDestroyCcb(
    IN OUT PIT_CCB* ppCcb
    );

NTSTATUS
ItpGetCcb(
    OUT PIT_CCB* ppCcb,
    IN PIRP pIrp
    );

//
// Dispatch Routines
//

NTSTATUS
ItDispatchCreate(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchClose(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchRead(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchWrite(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchDeviceIoControl(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchFsControl(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchFlushBuffers(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchQueryInformation(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchSetInformation(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchCreateNamedPipe(
    IN PIRP pIrp
    );

//
// Tests
//

NTSTATUS
ItTestStartup(
    IN PCSTR pszPath
    );

NTSTATUS
ItTestSyncCreate(
    VOID
    );

NTSTATUS
ItTestAsyncCreate(
    IN BOOLEAN UseAsyncCall,
    IN BOOLEAN DoCancel
    );

NTSTATUS
ItTestRundown(
    VOID
    );

NTSTATUS
ItTestSleep(
    IN PIRP pIrp,
    IN ULONG Seconds
    );

#endif /* __INCLUDES_H__ */
