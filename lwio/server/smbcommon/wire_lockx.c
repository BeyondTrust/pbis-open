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

NTSTATUS
WireUnmarshallLockingAndXRequest(
    PBYTE                             pParams,
    ULONG                             ulBytesAvailable,
    ULONG                             ulOffset,
    PSMB_LOCKING_ANDX_REQUEST_HEADER* ppRequestHeader,
    PLOCKING_ANDX_RANGE*              ppUnlockRange,
    PLOCKING_ANDX_RANGE_LARGE_FILE*   ppUnlockRangeLarge,
    PLOCKING_ANDX_RANGE*              ppLockRange,
    PLOCKING_ANDX_RANGE_LARGE_FILE*   ppLockRangeLarge
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader = NULL;
    PLOCKING_ANDX_RANGE              pUnlockRange = NULL;
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge = NULL;
    PLOCKING_ANDX_RANGE              pLockRange = NULL;
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge = NULL;
    PBYTE pDataCursor = pParams;

    if (ulBytesAvailable < sizeof(SMB_LOCKING_ANDX_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB_LOCKING_ANDX_REQUEST_HEADER)pDataCursor;
    pDataCursor += sizeof(SMB_LOCKING_ANDX_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB_LOCKING_ANDX_REQUEST_HEADER);
    ulOffset += sizeof(SMB_LOCKING_ANDX_REQUEST_HEADER);

    if (pRequestHeader->usNumUnlocks)
    {
        if (pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            if (ulBytesAvailable < (sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumUnlocks))
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pUnlockRangeLarge = (PLOCKING_ANDX_RANGE_LARGE_FILE)pDataCursor;
            pDataCursor += sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumUnlocks;
            ulBytesAvailable -= sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumUnlocks;
            ulOffset += sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumUnlocks;
        }
        else
        {
            if (ulBytesAvailable < (sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumUnlocks))
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pUnlockRange = (PLOCKING_ANDX_RANGE)pDataCursor;
            pDataCursor += sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumUnlocks;
            ulBytesAvailable -= sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumUnlocks;
            ulOffset += sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumUnlocks;
        }

    }

    if (pRequestHeader->usNumLocks)
    {
        if (pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            if (ulBytesAvailable < (sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumLocks))
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pLockRangeLarge = (PLOCKING_ANDX_RANGE_LARGE_FILE)pDataCursor;
            pDataCursor += sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumLocks;
            ulBytesAvailable -= sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumLocks;
            ulOffset += sizeof(LOCKING_ANDX_RANGE_LARGE_FILE) * pRequestHeader->usNumLocks;
        }
        else
        {
            if (ulBytesAvailable < (sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumLocks))
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pLockRange = (PLOCKING_ANDX_RANGE)pDataCursor;
            pDataCursor += sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumLocks;
            ulBytesAvailable -= sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumLocks;
            ulOffset += sizeof(LOCKING_ANDX_RANGE) * pRequestHeader->usNumLocks;
        }
    }

    *ppRequestHeader = pRequestHeader;
    *ppUnlockRange = pUnlockRange;
    *ppUnlockRangeLarge = pUnlockRangeLarge;
    *ppLockRange = pLockRange;
    *ppLockRangeLarge = pLockRangeLarge;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppUnlockRange = NULL;
    *ppUnlockRangeLarge = NULL;
    *ppLockRange = NULL;
    *ppLockRangeLarge = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallLockingAndXResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PSMB_LOCKING_ANDX_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_LOCKING_ANDX_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT   usPackageBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB_LOCKING_ANDX_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB_LOCKING_ANDX_RESPONSE_HEADER)pParams;
    usPackageBytesUsed += sizeof(SMB_LOCKING_ANDX_RESPONSE_HEADER);

    pResponseHeader->usByteCount = usPackageBytesUsed;

    *ppResponseHeader = pResponseHeader;
    *pusPackageBytesUsed = usPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppResponseHeader = NULL;
    *pusPackageBytesUsed = 0;

    goto cleanup;
}
