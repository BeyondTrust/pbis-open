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
 *        Likewise Task Service (LWTASK)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#include "includes.h"

DWORD
LwTaskSrvStartListenThread(
    void
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = CACHEDIR;
    PSTR  pszCommPath = NULL;
    BOOLEAN bDirExists = FALSE;
    static LWMsgTime idleTimeout = {10, 0};

    dwError = LwCheckFileTypeExists(
                    pszCachePath,
                    LWFILE_DIRECTORY,
                    &bDirExists);
    BAIL_ON_LW_TASK_ERROR(dwError);

    if (!bDirExists)
    {
        // Directory should be RWX for root and accessible to all
        // (so they can see the socket.
        mode_t mode = S_IRWXU | S_IRGRP| S_IXGRP | S_IROTH | S_IXOTH;

        dwError = LwCreateDirectory(pszCachePath, mode);
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                    &pszCommPath,
                    "%s/%s",
                    pszCachePath,
                    LW_TASK_SERVER_FILENAME);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &gLwTaskSrvGlobals.pContext));
    BAIL_ON_LW_TASK_ERROR(dwError);

    /* Set up IPC protocol object */
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(
                                    gLwTaskSrvGlobals.pContext,
                                    &gLwTaskSrvGlobals.pProtocol));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskIpcAddProtocolSpec(gLwTaskSrvGlobals.pProtocol);
    BAIL_ON_LW_TASK_ERROR(dwError);

    /* Set up IPC server object */
    dwError = MAP_LWMSG_ERROR(lwmsg_server_new(
                                    gLwTaskSrvGlobals.pContext,
                                    gLwTaskSrvGlobals.pProtocol,
                                    &gLwTaskSrvGlobals.pServer));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwTaskDaemonIpcAddDispatch(gLwTaskSrvGlobals.pServer);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_endpoint(
                                    gLwTaskSrvGlobals.pServer,
                                    LWMSG_CONNECTION_MODE_LOCAL,
                                    pszCommPath,
                                    0666));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_dispatch(
                                    gLwTaskSrvGlobals.pServer,
                                    LW_TASK_MAX_DISPATCH));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_clients(
                                    gLwTaskSrvGlobals.pServer,
                                    LW_TASK_MAX_CLIENTS));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_backlog(
                                    gLwTaskSrvGlobals.pServer,
                                    LW_MAX(5, LW_TASK_MAX_CLIENTS / 4)));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_timeout(
                                    gLwTaskSrvGlobals.pServer,
                                    LWMSG_TIMEOUT_IDLE,
                                    &idleTimeout));
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_start(gLwTaskSrvGlobals.pServer));

cleanup:

    if (pszCommPath)
    {
        LwFreeMemory(pszCommPath);
    }

    return dwError;

error:

    if (gLwTaskSrvGlobals.pServer)
    {
        lwmsg_server_stop(gLwTaskSrvGlobals.pServer);
        lwmsg_server_delete(gLwTaskSrvGlobals.pServer);
        gLwTaskSrvGlobals.pServer = NULL;
    }

    goto cleanup;
}

DWORD
LwTaskSrvStopListenThread(
    void
    )
{
    DWORD dwError = 0;

    if (gLwTaskSrvGlobals.pServer)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_server_stop(gLwTaskSrvGlobals.pServer));
        BAIL_ON_LW_TASK_ERROR(dwError);
    }

error:

    if (gLwTaskSrvGlobals.pServer)
    {
        lwmsg_server_delete(gLwTaskSrvGlobals.pServer);
        gLwTaskSrvGlobals.pServer = NULL;
    }
    if (gLwTaskSrvGlobals.pProtocol)
    {
        lwmsg_protocol_delete(gLwTaskSrvGlobals.pProtocol);
        gLwTaskSrvGlobals.pProtocol = NULL;
    }
    if (gLwTaskSrvGlobals.pContext)
    {
        lwmsg_context_delete(gLwTaskSrvGlobals.pContext);
        gLwTaskSrvGlobals.pContext = NULL;
    }

    return dwError;
}
