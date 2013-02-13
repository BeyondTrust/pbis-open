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
 *        Likewise Site Manager Listener
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 * 
 */
#include "includes.h"

static LWMsgContext* gpContext = NULL;
static LWMsgProtocol* gpProtocol = NULL;
static LWMsgPeer* gpServer = NULL;

static
LWMsgBool
LWNetSrvLogIpc (
    LWMsgLogLevel level,
    const char* pszMessage,
    const char* pszFunction,
    const char* pszFilename,
    unsigned int line,
    void* pData
    )
{
    DWORD dwLevel = 0;
    LWMsgBool result;

    switch (level)
    {
    case LWMSG_LOGLEVEL_ALWAYS:
        dwLevel = LWNET_LOG_LEVEL_ALWAYS;
        break;
    case LWMSG_LOGLEVEL_ERROR:
        dwLevel = LWNET_LOG_LEVEL_ERROR;
        break;
    case LWMSG_LOGLEVEL_WARNING:
        dwLevel = LWNET_LOG_LEVEL_WARNING;
        break;
    case LWMSG_LOGLEVEL_INFO:
        dwLevel = LWNET_LOG_LEVEL_INFO;
        break;
    case LWMSG_LOGLEVEL_VERBOSE:
        dwLevel = LWNET_LOG_LEVEL_VERBOSE;
        break;
    case LWMSG_LOGLEVEL_DEBUG:
        dwLevel = LWNET_LOG_LEVEL_DEBUG;
        break;
    case LWMSG_LOGLEVEL_TRACE:
        dwLevel = LWNET_LOG_LEVEL_TRACE;
        break;
    }

    result = LwRtlLogGetLevel() >= dwLevel;
    if (pszMessage && result)
    {
        LW_RTL_LOG_RAW(dwLevel, "netlogon-ipc", pszFunction, pszFilename, line, "%s", pszMessage);
    }

    return result;
}

DWORD
LWNetSrvStartListenThread(
    void
    )
{
    PSTR pszCachePath = NULL;
    PSTR pszCommPath = NULL;
    BOOLEAN bDirExists = FALSE;
    DWORD dwError = 0;

    dwError = LWNetSrvGetCachePath(&pszCachePath);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwCheckFileTypeExists(
                    pszCachePath,
                    LWFILE_DIRECTORY,
                    &bDirExists);
    BAIL_ON_LWNET_ERROR(dwError);

    if (!bDirExists)
    {
        // Directory should be RWX for root and accessible to all
        // (so they can see the socket.
        mode_t mode = S_IRWXU | S_IRGRP| S_IXGRP | S_IROTH | S_IXOTH;
        dwError = LwCreateDirectory(pszCachePath, mode);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(&pszCommPath, "%s/%s",
                                        pszCachePath, LWNET_SERVER_FILENAME);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &gpContext));
    BAIL_ON_LWNET_ERROR(dwError);

    lwmsg_context_set_log_function(gpContext, LWNetSrvLogIpc, NULL);

    /* Set up IPC protocol object */
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(gpContext, &gpProtocol));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
                                  gpProtocol,
                                  LWNetIPCGetProtocolSpec()));
    BAIL_ON_LWNET_ERROR(dwError);

    /* Set up IPC server object */
    dwError = MAP_LWMSG_ERROR(lwmsg_peer_new(gpContext, gpProtocol, &gpServer));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_dispatch_spec(
                                  gpServer,
                                  LWNetSrvGetDispatchSpec()));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_listen_endpoint(
                                  gpServer,
                                  LWMSG_ENDPOINT_DIRECT,
                                  "netlogon",
                                  0));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_listen_endpoint(
                                  gpServer,
                                  LWMSG_ENDPOINT_LOCAL,
                                  pszCommPath,
                                  0666));
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = MAP_LWMSG_ERROR(lwmsg_peer_start_listen(gpServer));

error:

    LWNET_SAFE_FREE_STRING(pszCachePath);
    LWNET_SAFE_FREE_STRING(pszCommPath);

    if (dwError)
    {
        if (gpServer)
        {
            lwmsg_peer_stop_listen(gpServer);
            lwmsg_peer_delete(gpServer);
            gpServer = NULL;
        }
    }

    return dwError;
}

DWORD
LWNetSrvStopListenThread(
    void
    )
{
    DWORD dwError = 0;

    if (gpServer)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_peer_stop_listen(gpServer));
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:

    if (gpServer)
    {
        lwmsg_peer_delete(gpServer);
        gpServer = NULL;
    }

    if (gpProtocol)
    {
        lwmsg_protocol_delete(gpProtocol);
        gpProtocol = NULL;
    }

    return dwError;
}
