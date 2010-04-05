/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        connection.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Protocols API - SMBV2
 *
 *        Connection
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
NfsConnection2FindSession_SMB_V2(
    PNFS_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_NFS_CONNECTION     pConnection,
    ULONG64                  ullUid,
    PLWIO_NFS_SESSION_2*     ppSession
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_NFS_SESSION_2 pSession = NULL;

    if (ullUid)
    {
        if (pSmb2Context->pSession)
        {
            if (pSmb2Context->pSession->ullUid != ullUid)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                pSession = NfsSession2Acquire(pSmb2Context->pSession);
            }
        }
        else
        {
            ntStatus = NfsConnection2FindSession(
                            pConnection,
                            ullUid,
                            &pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            pSmb2Context->pSession = NfsSession2Acquire(pSession);
        }
    }
    else if (pSmb2Context->pSession)
    {
        pSession = NfsSession2Acquire(pSmb2Context->pSession);
    }
    else
    {
        ntStatus = STATUS_NO_SUCH_LOGON_SESSION;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    *ppSession = NULL;

    goto cleanup;
}
