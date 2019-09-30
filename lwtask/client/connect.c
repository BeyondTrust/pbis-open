/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

