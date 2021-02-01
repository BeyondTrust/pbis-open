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
 *        namedpipe.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"

NTSTATUS
ItDispatchCreateNamedPipe(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    int EE = 0;
    UNICODE_STRING path = pIrp->Args.Create.FileName.Name;
    UNICODE_STRING prefixPath = { 0 };
    UNICODE_STRING allowPrefix = { 0 };
    PIT_CCB pCcb = NULL;
    PIO_ECP_NAMED_PIPE pipeParams = NULL;
    ULONG ecpSize = 0;

    if (!pIrp->Args.Create.EcpList)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = IoRtlEcpListFind(
                    pIrp->Args.Create.EcpList,
                    IO_ECP_TYPE_NAMED_PIPE,
                    (PVOID*)&pipeParams,
                    &ecpSize);
    if (STATUS_NOT_FOUND == status)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (ecpSize != sizeof(*pipeParams))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = RtlUnicodeStringAllocateFromCString(&allowPrefix, IOTEST_INTERNAL_PATH_NAMED_PIPE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // TODO -- Add some IoRtlPath prefix functions...
    if (path.Length <= allowPrefix.Length || !IoRtlPathIsSeparator((path.Buffer[allowPrefix.Length/sizeof(allowPrefix.Buffer[0])])))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    prefixPath.Buffer = path.Buffer;
    prefixPath.Length = allowPrefix.Length;
    prefixPath.MaximumLength = prefixPath.Length;

    // Only succeed for the given prefix.
    if (!RtlUnicodeStringIsEqual(&prefixPath, &allowPrefix, FALSE))
    {
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    // Would have to check whether pipe already exists, etc.

    status = ItpCreateCcb(&pCcb, &path);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCcb->IsNamedPipe = TRUE;

    status = IoFileSetContext(pIrp->FileHandle, pCcb);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCcb = NULL;

cleanup:
    ItpDestroyCcb(&pCcb);
    RtlUnicodeStringFree(&allowPrefix);

    pIrp->IoStatusBlock.Status = status;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
