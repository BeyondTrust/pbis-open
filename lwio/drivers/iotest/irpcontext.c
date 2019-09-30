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
