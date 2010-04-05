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
 *        response.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Client Response Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

NTSTATUS
SMBResponseCreate(
    uint16_t       wMid,
    SMB_RESPONSE **ppResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RESPONSE pResponse = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_RESPONSE),
                    (PVOID*)&pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pResponse->mutex, NULL);

    bDestroyMutex = TRUE;

    pResponse->state = SMB_RESOURCE_STATE_INITIALIZING;

    ntStatus = pthread_cond_init(&pResponse->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pResponse->pTree = NULL;
    pResponse->mid = wMid;
    pResponse->pPacket = NULL;

    *ppResponse = pResponse;

cleanup:

    return ntStatus;

error:

    if (bDestroyCondition)
    {
        pthread_cond_destroy(&pResponse->event);
    }

    if (bDestroyMutex)
    {
        pthread_mutex_destroy(&pResponse->mutex);
    }

    LWIO_SAFE_FREE_MEMORY(pResponse);

    *ppResponse = NULL;

    goto cleanup;
}

/* This function does not remove a socket from it's parent hash; it merely
   frees the memory if the refcount is zero. */
VOID
SMBResponseFree(
    PSMB_RESPONSE pResponse
    )
{
    BOOLEAN bInTreeLock = FALSE;

    pthread_cond_destroy(&pResponse->event);

    pthread_mutex_destroy(&pResponse->mutex);

    if (pResponse->pTree)
    {
        LWIO_LOCK_MUTEX(bInTreeLock, &pResponse->pTree->mutex);
        
        LWIO_LOG_DEBUG("Removing response [mid: %d] from Tree [0x%x]",
                      pResponse->mid, pResponse->pTree);
        
        SMBHashRemoveKey(
            pResponse->pTree->pResponseHash,
            &pResponse->mid);

        LWIO_UNLOCK_MUTEX(bInTreeLock, &pResponse->pTree->mutex);

        SMBTreeRelease(pResponse->pTree);
    }

    /* @todo: use allocator */
    SMBFreeMemory(pResponse);
}

VOID
SMBResponseInvalidate_InLock(
    PSMB_RESPONSE pResponse,
    NTSTATUS ntStatus
    )
{
    pResponse->state = SMB_RESOURCE_STATE_INVALID;
    pResponse->error = ntStatus;
    
    pthread_cond_broadcast(&pResponse->event);
}

VOID
SMBResponseUnlock(
    PSMB_RESPONSE pResponse
    )
{
    BOOLEAN bInLock = TRUE;

    LWIO_UNLOCK_MUTEX(bInLock, &pResponse->mutex);
}
