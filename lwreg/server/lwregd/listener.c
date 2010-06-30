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
 *        Registry
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "registryd.h"

#define MAX_DISPATCH 8
#define MAX_CLIENTS 512

static LWMsgContext* gpContext = NULL;
static LWMsgProtocol* gpProtocol = NULL;
static LWMsgServer* gpServer = NULL;

static
LWMsgBool
RegSrvLogIpc (
    LWMsgLogLevel level,
    const char* pszMessage,
    const char* pszFunction,
    const char* pszFilename,
    unsigned int line,
    void* pData
    )
{
    RegLogLevel regLevel;
    LWMsgBool result;

    switch (level)
    {
    case LWMSG_LOGLEVEL_ALWAYS:
        regLevel = REG_LOG_LEVEL_ALWAYS;
        break;
    case LWMSG_LOGLEVEL_ERROR:
        regLevel = REG_LOG_LEVEL_ERROR;
        break;
    case LWMSG_LOGLEVEL_WARNING:
        regLevel = REG_LOG_LEVEL_WARNING;
        break;
    case LWMSG_LOGLEVEL_INFO:
        regLevel = REG_LOG_LEVEL_INFO;
        break;
    case LWMSG_LOGLEVEL_VERBOSE:
        regLevel = REG_LOG_LEVEL_VERBOSE;
        break;
    case LWMSG_LOGLEVEL_DEBUG:
        regLevel = REG_LOG_LEVEL_DEBUG;
        break;
    case LWMSG_LOGLEVEL_TRACE:
    default:
        regLevel = REG_LOG_LEVEL_TRACE;
        break;
    }

    REG_LOCK_LOGGER;
    if (pszMessage)
    {
        if (gRegMaxLogLevel >= regLevel)
        {
            RegLogMessage(gpfnRegLogger, ghRegLog, regLevel, "[IPC] %s", pszMessage);
            result = LWMSG_TRUE;
        }
        else
        {
            result = LWMSG_FALSE;
        }
    }
    else
    {
        result = (gRegMaxLogLevel >= regLevel);
    }
    REG_UNLOCK_LOGGER;

    return result;
}


DWORD
RegSrvStartListenThread(
    void
    )
{
    PSTR pszCachePath = NULL;
    PSTR pszCommPath = NULL;
    BOOLEAN bDirExists = FALSE;
    DWORD dwError = 0;
    static LWMsgTime idleTimeout = {30, 0};

    dwError = RegSrvGetCachePath(&pszCachePath);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCheckDirectoryExists(pszCachePath, &bDirExists);
    BAIL_ON_REG_ERROR(dwError);

    if (!bDirExists)
    {
        // Directory should be RWX for root and accessible to all
        // (so they can see the socket.
        mode_t mode = S_IRWXU | S_IRGRP| S_IXGRP | S_IROTH | S_IXOTH;
        dwError = RegCreateDirectory(pszCachePath, mode);
        BAIL_ON_REG_ERROR(dwError);
    }


    dwError = LwRtlCStringAllocatePrintf(&pszCommPath, "%s/%s",
                                      pszCachePath, REG_SERVER_FILENAME);
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &gpContext));
    BAIL_ON_REG_ERROR(dwError);

    lwmsg_context_set_log_function(gpContext, RegSrvLogIpc, NULL);

    /* Set up IPC protocol object */
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(gpContext, &gpProtocol));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
                              gpProtocol,
                              RegIPCGetProtocolSpec()));
    BAIL_ON_REG_ERROR(dwError);

    /* Set up IPC server object */
    dwError = MAP_LWMSG_ERROR(lwmsg_server_new(gpContext, gpProtocol, &gpServer));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_add_dispatch_spec(
                              gpServer,
                              RegSrvGetDispatchSpec()));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_endpoint(
                              gpServer,
                              LWMSG_CONNECTION_MODE_LOCAL,
                              pszCommPath,
                              0666));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_dispatch(
                                  gpServer,
                                  MAX_DISPATCH));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_clients(
                                  gpServer,
                                  MAX_CLIENTS));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_backlog(
                                  gpServer,
                                  REG_MAX(5, MAX_CLIENTS / 4)));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_timeout(
                                  gpServer,
                                  LWMSG_TIMEOUT_IDLE,
                                  &idleTimeout));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_session_functions(
                                  gpServer,
                                  RegSrvIpcConstructSession,
                                  RegSrvIpcDestructSession,
                                  NULL));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_start(gpServer));

error:

    LWREG_SAFE_FREE_STRING(pszCachePath);
    LWREG_SAFE_FREE_STRING(pszCommPath);

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
RegSrvStopListenThread(
    void
    )
{
    DWORD dwError = 0;

    if (gpServer)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_server_stop(gpServer));
        BAIL_ON_REG_ERROR(dwError);
    }

error:

    if (gpServer)
    {
        lwmsg_server_delete(gpServer);
        gpServer = NULL;
    }

    return dwError;
}
