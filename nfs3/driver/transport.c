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

#include "includes.h"


struct __NFS3_TRANSPORT
{
    PNFS3_LISTENER  pListener;
    PLW_THREAD_POOL pPool;
};


NTSTATUS
Nfs3TransportCreate(
    PNFS3_TRANSPORT* ppTransport,
    const PNFS3_TRANSPORT_CALLBACKS pCallbacks
    )
{
    NTSTATUS  ntStatus = STATUS_SUCCESS;
    PNFS3_TRANSPORT pTransport = NULL;
    PLW_THREAD_POOL_ATTRIBUTES pAttrs = NULL;

    ntStatus = Nfs3AllocateMemoryClear(sizeof(*pTransport), (PVOID*)&pTransport);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateThreadPoolAttributes(&pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    /* We don't presently use work threads, so turn them off */
    ntStatus = LwRtlSetThreadPoolAttribute(
                    pAttrs, 
                    LW_THREAD_POOL_OPTION_WORK_THREADS, 
                    0);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateThreadPool(&pTransport->pPool, pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = Nfs3ListenerCreate(&pTransport->pListener, pTransport->pPool,
                                  pCallbacks);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LwRtlFreeThreadPoolAttributes(&pAttrs);

    *ppTransport = pTransport;

    return ntStatus;

error:

    Nfs3TransportFree(&pTransport);

    goto cleanup;
}

VOID
Nfs3TransportFree(
    PNFS3_TRANSPORT* ppTransport
    )
{
    PNFS3_TRANSPORT pTransport = *ppTransport;

    if (pTransport)
    {
        Nfs3ListenerFree(&pTransport->pListener);
        LwRtlFreeThreadPool(&pTransport->pPool);
        Nfs3FreeMemory((PVOID*)&pTransport);
    }

    *ppTransport = pTransport;
}
