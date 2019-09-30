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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        listener.c
 *
 * Abstract:
 *
 *        BeyondTrust Task Service (LWTASK)
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
