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
