/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
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
    int EE = 0;

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
