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
 *        iocontrol.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"
#include "iotestctl.h"

NTSTATUS
ItDispatchDeviceIoControl(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    int EE ATTRIBUTE_UNUSED = 0;

    switch (pIrp->Args.IoFsControl.ControlCode)
    {
        case IOTEST_IOCTL_TEST_SYNC_CREATE:
            if (pIrp->Args.IoFsControl.InputBufferLength ||
                pIrp->Args.IoFsControl.OutputBufferLength)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_EE(EE);
            }
            status = ItTestSyncCreate();
            break;
        case IOTEST_IOCTL_TEST_ASYNC_CREATE:
            if (pIrp->Args.IoFsControl.InputBufferLength ||
                pIrp->Args.IoFsControl.OutputBufferLength)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_EE(EE);
            }
            status = ItTestAsyncCreate(TRUE, FALSE);
            break;
        case IOTEST_IOCTL_TEST_RUNDOWN:
            if (pIrp->Args.IoFsControl.InputBufferLength ||
                pIrp->Args.IoFsControl.OutputBufferLength)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_EE(EE);
            }
            status = ItTestRundown();
            break;
        case IOTEST_IOCTL_TEST_SLEEP:
            if (pIrp->Args.IoFsControl.InputBufferLength ||
                pIrp->Args.IoFsControl.OutputBufferLength)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_EE(EE);
            }
            status = ItTestSleep(pIrp, 10);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (STATUS_PENDING != status)
    {
        pIrp->IoStatusBlock.Status = status;
    }

    return status;
}
