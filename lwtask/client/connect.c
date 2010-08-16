/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"

DWORD
LwTaskOpenServer(
    PLW_TASK_CLIENT_CONNECTION* ppConnection
    )
{
    DWORD dwError = 0;
    PLW_TASK_CLIENT_CONNECTION pConnection = NULL;

    BAIL_ON_INVALID_POINTER(ppConnection);

    dwError = LwAllocateMemory(sizeof(LW_TASK_CLIENT_CONNECTION), (PVOID*)&pConnection);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pConnection->pProtocol));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskIpcAddProtocolSpec(pConnection->pProtocol);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_new(NULL, pConnection->pProtocol, &pConnection->pAssoc));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                    pConnection->pAssoc,
                                    LWMSG_CONNECTION_MODE_LOCAL,
                                    CACHEDIR "/" LW_TASK_SERVER_FILENAME));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_connect(pConnection->pAssoc, NULL));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session(pConnection->pAssoc, &pConnection->pSession));
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppConnection = pConnection;

cleanup:

    return dwError;

error:

    if (pConnection)
    {
        if (pConnection->pAssoc)
        {
            lwmsg_assoc_delete(pConnection->pAssoc);
        }

        if (pConnection->pProtocol)
        {
            lwmsg_protocol_delete(pConnection->pProtocol);
        }

        LwFreeMemory(pConnection);
    }

    if (ppConnection)
    {
        *ppConnection = NULL;
    }

    goto cleanup;
}

DWORD
LwTaskContextAcquireCall(
    PLW_TASK_CLIENT_CONNECTION pConnection, /* IN     */
    LWMsgCall**                ppCall       /*    OUT */
    )
{
    return MAP_LWMSG_ERROR(lwmsg_assoc_acquire_call(
                                    pConnection->pAssoc,
                                    ppCall));
}

DWORD
LwTaskCloseServer(
    PLW_TASK_CLIENT_CONNECTION pConnection
    )
{
    if (pConnection->pAssoc)
    {
        lwmsg_assoc_close(pConnection->pAssoc);
        lwmsg_assoc_delete(pConnection->pAssoc);
    }

    if (pConnection->pProtocol)
    {
        lwmsg_protocol_delete(pConnection->pProtocol);
    }

    LwFreeMemory(pConnection);

    return 0;
}

