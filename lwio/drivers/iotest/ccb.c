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
 *        ccb.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

NTSTATUS
ItpCreateCcb(
    OUT PIT_CCB* ppCcb,
    IN PUNICODE_STRING pPath
    )
{
    NTSTATUS status;
    int EE = 0;
    PIT_CCB pCcb = NULL;

    status = IO_ALLOCATE(&pCcb, IT_CCB, sizeof(*pCcb));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlUnicodeStringDuplicate(&pCcb->Path, pPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        ItpDestroyCcb(&pCcb);
    }

    *ppCcb = pCcb;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

VOID
ItpDestroyCcb(
    IN OUT PIT_CCB* ppCcb
    )
{
    PIT_CCB pCcb = *ppCcb;

    if (pCcb)
    {
        RtlUnicodeStringFree(&pCcb->Path);
        IoMemoryFree(pCcb);
        *ppCcb = NULL;
    }
}

NTSTATUS
ItpGetCcb(
    OUT PIT_CCB* ppCcb,
    IN PIRP pIrp
    )
{
    PIT_CCB pCcb = (PIT_CCB) IoFileGetContext(pIrp->FileHandle);
    assert(pCcb);
    *ppCcb = pCcb;
    return pCcb ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
}
