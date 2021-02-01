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
 *        driver.c
 *
 * Abstract:
 *
 *        BeyondTrust Posix File System Driver (RDR)
 *
 *        Device I/O Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

static
NTSTATUS
RdrGetSessionKey(
    PRDR_CCB pFile,
    PVOID pBuffer,
    ULONG ulLength,
    PULONG pulLengthUsed
    );

NTSTATUS
RdrFsctl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pOutBuffer = pIrp->Args.IoFsControl.OutputBuffer;
    ULONG OutLength = pIrp->Args.IoFsControl.OutputBufferLength;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);

    switch (pIrp->Args.IoFsControl.ControlCode)
    {
    case IO_FSCTL_SMB_GET_SESSION_KEY:
        status = RdrGetSessionKey(
            pFile,
            pOutBuffer,
            OutLength,
            &pIrp->IoStatusBlock.BytesTransferred);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(status);
    }

error:

    pIrp->IoStatusBlock.Status = status;

    return status;
}

static
NTSTATUS
RdrGetSessionKey(
    PRDR_CCB pFile,
    PVOID pBuffer,
    ULONG ulLength,
    PULONG pulLengthUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_SESSION pSession = pFile->pTree->pSession;

    if (pSession->dwSessionKeyLength > ulLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }
    
    memcpy(pBuffer, pSession->pSessionKey, pSession->dwSessionKeyLength);

    *pulLengthUsed = pSession->dwSessionKeyLength;

error:

    return status;
}
