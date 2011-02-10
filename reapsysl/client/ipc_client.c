/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipc_client.c
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
RSysOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PRSYS_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    static LWMsgTime connectTimeout = {2, 0};

    BAIL_ON_INVALID_POINTER(phConnection);

    dwError = RTL_ALLOCATE(
                    &pContext,
                    RSYS_CLIENT_CONNECTION_CONTEXT,
                    sizeof(*pContext));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pContext->pProtocol));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(pContext->pProtocol, RSysIPCGetProtocolSpec()));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_new(NULL, pContext->pProtocol, &pContext->pAssoc));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc, 
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  RSYS_CACHE_DIR "/" RSYS_SERVER_FILENAME));
    BAIL_ON_RSYS_ERROR(dwError);

    if (getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_set_timeout(
                                      pContext->pAssoc,
                                      LWMSG_TIMEOUT_ESTABLISH,
                                      &connectTimeout));
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_connect(pContext->pAssoc, NULL));
    BAIL_ON_RSYS_ERROR(dwError);
    
    *phConnection = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    if (pContext)
    {
        if (pContext->pAssoc)
        {
            lwmsg_assoc_delete(pContext->pAssoc);
        }

        if (pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }

        RtlMemoryFree(pContext);
    }

    if (phConnection) 
    {
        *phConnection = (HANDLE)NULL;
    }
    
    goto cleanup;
}

DWORD
RSysCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PRSYS_CLIENT_CONNECTION_CONTEXT pContext =
                     (PRSYS_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (pContext->pAssoc)
    {
        lwmsg_assoc_close(pContext->pAssoc);
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    RtlMemoryFree(pContext);

    return dwError;
}
