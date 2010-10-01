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

#include "rdr.h"

NTSTATUS
RdrGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hFile;
    PSMB_SESSION pSession = NULL;
    PBYTE pSessionKey = NULL;
    BOOLEAN bInLock = FALSE;

    /* Because the handle keeps a reference count on the tree, and the tree on
       the session, and the session on the socket, it is safe to access the
       session structure without locking the socket hash mutex to protect
       against reaping. */

    if (hFile == (HANDLE)NULL)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
    }
    if (pdwSessionKeyLength == NULL)
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (ppSessionKey == NULL)
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pSession = pFile->pTree->pSession;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    if (!pSession->pSessionKey)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwIoAllocateMemory(
                    pSession->dwSessionKeyLength,
                    (PVOID*)&pSessionKey);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pSessionKey, pSession->pSessionKey, pSession->dwSessionKeyLength);

    *pdwSessionKeyLength = pSession->dwSessionKeyLength;
    *ppSessionKey = pSessionKey;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *pdwSessionKeyLength = 0;
    *ppSessionKey = NULL;

    goto cleanup;
}
