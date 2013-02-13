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
 *        irpcontext.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"

NTSTATUS
ItCreateIrpContext(
    OUT PIT_IRP_CONTEXT* ppIrpContext,
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIT_IRP_CONTEXT pIrpContext = NULL;

    status = RTL_ALLOCATE(&pIrpContext, IT_IRP_CONTEXT, sizeof(*pIrpContext));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = ItCreateWorkItem(&pIrpContext->pWorkItem);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrpContext->pIrp = pIrp;

cleanup:
    if (status)
    {
        ItDestroyIrpContext(&pIrpContext);
    }

    *ppIrpContext = pIrpContext;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

VOID
ItDestroyIrpContext(
    IN OUT PIT_IRP_CONTEXT* ppIrpContext
    )
{
    PIT_IRP_CONTEXT pIrpContext = *ppIrpContext;

    if (pIrpContext)
    {
        ItDestroyWorkItem(&pIrpContext->pWorkItem);
        RTL_FREE(&pIrpContext);
        *ppIrpContext = NULL;
    }
}
