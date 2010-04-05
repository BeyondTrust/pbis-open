/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        listener.c
 *
 * Abstract:
 *
 *        Reaper for syslog Listener
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 * 
 */
#include "includes.h"

#define MAX_DISPATCH 2
#define MAX_CLIENTS 4

static LWMsgProtocol* gpProtocol = NULL;
static LWMsgServer* gpServer = NULL;

DWORD
RSysSrvStartListenThread(
    void
    )
{
    PSTR pszCachePath = NULL;
    PSTR pszCommPath = NULL;
    BOOLEAN bDirExists = FALSE;
    DWORD dwError = 0;
    static LWMsgTime idleTimeout = {30, 0};

    dwError = RSysSrvGetCachePath(&pszCachePath);
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = RSysCheckDirectoryExists(pszCachePath, &bDirExists);
    BAIL_ON_RSYS_ERROR(dwError);

    if (!bDirExists)
    {
        // Directory should be RWX for root and accessible to all
        // (so they can see the socket.
        mode_t mode = S_IRWXU | S_IRGRP| S_IXGRP | S_IROTH | S_IXOTH;
        dwError = RSysCreateDirectory(pszCachePath, mode);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    dwError = RtlCStringAllocatePrintf(&pszCommPath, "%s/%s",
                                        pszCachePath, RSYS_SERVER_FILENAME);
    BAIL_ON_RSYS_ERROR(dwError);

    /* Set up IPC protocol object */
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &gpProtocol));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
                                  gpProtocol,
                                  RSysIPCGetProtocolSpec()));
    BAIL_ON_RSYS_ERROR(dwError);

    /* Set up IPC server object */
    dwError = MAP_LWMSG_ERROR(lwmsg_server_new(NULL, gpProtocol, &gpServer));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_add_dispatch_spec(
                                  gpServer,
                                  RSysSrvGetDispatchSpec()));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_endpoint(
                                  gpServer,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  pszCommPath,
                                  0666));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_dispatch(
                                  gpServer,
                                  MAX_DISPATCH));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_clients(
                                  gpServer,
                                  MAX_CLIENTS));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_backlog(
                                  gpServer,
                                  LW_MAX(5, MAX_CLIENTS / 4)));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_timeout(
                                  gpServer,
                                  LWMSG_TIMEOUT_IDLE,
                                  &idleTimeout));
    BAIL_ON_RSYS_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_session_functions(
                                  gpServer,
                                  RSysSrvIpcConstructSession,
                                  RSysSrvIpcDestructSession,
                                  NULL));
    BAIL_ON_RSYS_ERROR(dwError);
    
    dwError = MAP_LWMSG_ERROR(lwmsg_server_start(gpServer));

error:

    RtlCStringFree(&pszCachePath);
    RtlCStringFree(&pszCommPath);

    if (dwError)
    {
        if (gpServer)
        {
            lwmsg_server_stop(gpServer);
            lwmsg_server_delete(gpServer);
            gpServer = NULL;
        }
    }

    return dwError;
}

DWORD
RSysSrvStopListenThread(
    void
    )
{
    DWORD dwError = 0;

    if (gpServer)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_server_stop(gpServer));
        BAIL_ON_RSYS_ERROR(dwError);
    }

error:

    if (gpServer)
    {
        lwmsg_server_delete(gpServer);
        gpServer = NULL;
    }

    if (gpProtocol)
    {
        lwmsg_protocol_delete(gpProtocol);
        gpProtocol = NULL;
    }

    return dwError;
}
