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
 *        Likewise Security and Authentication Subsystem (LSASS) Listener
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */
#include "lsassd.h"

#define MAX_DISPATCH 8
#ifdef __LWI_SOLARIS__
#define MAX_CLIENTS 200
#else
#define MAX_CLIENTS 512
#endif

static LWMsgContext* gpContext = NULL;
static LWMsgProtocol* gpProtocol = NULL;
static LWMsgServer* gpServer = NULL;

static LWMsgContext* gpNtlmContext = NULL;
static LWMsgProtocol* gpNtlmProtocol = NULL;
static LWMsgServer* gpNtlmServer = NULL;

static
LWMsgBool
LsaSrvLogIpc (
    LWMsgLogLevel level,
    const char* pszMessage,
    const char* pszFilename,
    unsigned int line,
    void* pData
    )
{
    LsaLogLevel lsaLevel;
    LWMsgBool result;

    switch (level)
    {
    case LWMSG_LOGLEVEL_ERROR:
        lsaLevel = LSA_LOG_LEVEL_ERROR;
        break;
    case LWMSG_LOGLEVEL_WARNING:
        lsaLevel = LSA_LOG_LEVEL_WARNING;
        break;
    case LWMSG_LOGLEVEL_INFO:
        lsaLevel = LSA_LOG_LEVEL_INFO;
        break;
    case LWMSG_LOGLEVEL_VERBOSE:
        lsaLevel = LSA_LOG_LEVEL_VERBOSE;
        break;
    case LWMSG_LOGLEVEL_DEBUG:
        lsaLevel = LSA_LOG_LEVEL_DEBUG;
        break;
    case LWMSG_LOGLEVEL_TRACE:
    default:
        lsaLevel = LSA_LOG_LEVEL_TRACE;
        break;
    }

    LSA_LOCK_LOGGER;
    if (pszMessage)
    {
        if (gLsaMaxLogLevel >= lsaLevel)
        {
            LsaLogMessage(gpfnLogger, ghLog, lsaLevel, "[IPC] %s", pszMessage);
            result = LWMSG_TRUE;
        }
        else
        {
            result = LWMSG_FALSE;
        }
    }
    else
    {
        result = (gLsaMaxLogLevel >= lsaLevel);
    }
    LSA_UNLOCK_LOGGER;

    return result;
}

static
void
LsaSrvHandleIpcException(
    LWMsgServer* pServer,
    LWMsgStatus status,
    void* pData
    )
{
    switch (status)
    {
    case LWMSG_STATUS_MEMORY:
    case LWMSG_STATUS_RESOURCE_LIMIT:
        /* Attempt to recover from resource shortages */
        break;
    default:
        /* Abort on any other exception */
        LSA_LOG_ERROR("Terminating on fatal IPC exception");
        kill(getpid(), SIGTERM);
        break;
    }
}

DWORD
LsaSrvStartListenThread(
    void
    )
{
    PSTR pszCachePath = NULL;
    PSTR pszCommPath = NULL;
    BOOLEAN bDirExists = FALSE;
    DWORD dwError = 0;
    static LWMsgTime idleTimeout = {10, 0};

    dwError = LsaSrvGetCachePath(&pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckDirectoryExists(pszCachePath, &bDirExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bDirExists)
    {
        // Directory should be RWX for root and accessible to all
        // (so they can see the socket.
        mode_t mode = S_IRWXU | S_IRGRP| S_IXGRP | S_IROTH | S_IXOTH;
        dwError = LsaCreateDirectory(pszCachePath, mode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(&pszCommPath, "%s/%s",
                                      pszCachePath, LSA_SERVER_FILENAME);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &gpContext));
    BAIL_ON_LSA_ERROR(dwError);

    lwmsg_context_set_log_function(gpContext, LsaSrvLogIpc, NULL);

    /* Set up IPC protocol object */
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(gpContext, &gpProtocol));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
                              gpProtocol,
                              LsaIPCGetProtocolSpec()));
    BAIL_ON_LSA_ERROR(dwError);

    /* Set up IPC server object */
    dwError = MAP_LWMSG_ERROR(lwmsg_server_new(gpContext, gpProtocol, &gpServer));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_add_dispatch_spec(
                              gpServer,
                              LsaSrvGetDispatchSpec()));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_endpoint(
                              gpServer,
                              LWMSG_CONNECTION_MODE_LOCAL,
                              pszCommPath,
                              0666));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_dispatch(
                                  gpServer,
                                  MAX_DISPATCH));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_clients(
                                  gpServer,
                                  MAX_CLIENTS));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_backlog(
                                  gpServer,
                                  LSA_MAX(5, MAX_CLIENTS / 4)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_timeout(
                                  gpServer,
                                  LWMSG_TIMEOUT_IDLE,
                                  &idleTimeout));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_session_functions(
                                  gpServer,
                                  LsaSrvIpcConstructSession,
                                  LsaSrvIpcDestructSession,
                                  NULL));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_exception_function(
                                  gpServer,
                                  LsaSrvHandleIpcException,
                                  NULL));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_start(gpServer));

error:

    LW_SAFE_FREE_STRING(pszCachePath);
    LW_SAFE_FREE_STRING(pszCommPath);

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
NtlmSrvStartListenThread(
    void
    )
{
    PSTR pszCachePath = NULL;
    PSTR pszCommPath = NULL;
    BOOLEAN bDirExists = FALSE;
    DWORD dwError = 0;
    static LWMsgTime idleTimeout = {30, 0};

    dwError = LsaSrvGetCachePath(&pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckDirectoryExists(pszCachePath, &bDirExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bDirExists)
    {
        // Directory should be RWX for root and accessible to all
        // (so they can see the socket.
        mode_t mode = S_IRWXU | S_IRGRP| S_IXGRP | S_IROTH | S_IXOTH;
        dwError = LsaCreateDirectory(pszCachePath, mode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
        &pszCommPath,
        "%s/%s",
        pszCachePath,
        NTLM_SERVER_FILENAME);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &gpNtlmContext));
    BAIL_ON_LSA_ERROR(dwError);

    lwmsg_context_set_log_function(gpNtlmContext, LsaSrvLogIpc, NULL);

    /* Set up IPC protocol object */
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(gpNtlmContext, &gpNtlmProtocol));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
        gpNtlmProtocol,
        NtlmIpcGetProtocolSpec()));
    BAIL_ON_LSA_ERROR(dwError);

    /* Set up IPC server object */
    dwError = MAP_LWMSG_ERROR(lwmsg_server_new(
        gpNtlmContext,
        gpNtlmProtocol,
        &gpNtlmServer));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_add_dispatch_spec(
        gpNtlmServer,
        NtlmSrvGetDispatchSpec()));
    BAIL_ON_LSA_ERROR(dwError);


    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_endpoint(
        gpNtlmServer,
        LWMSG_CONNECTION_MODE_LOCAL,
        pszCommPath,
        0666));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_dispatch(
        gpNtlmServer,
        MAX_DISPATCH));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_clients(
        gpNtlmServer,
        MAX_CLIENTS));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_max_backlog(
        gpNtlmServer,
        LSA_MAX(5, MAX_CLIENTS / 4)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_timeout(
        gpNtlmServer,
        LWMSG_TIMEOUT_IDLE,
        &idleTimeout));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_session_functions(
        gpNtlmServer,
        LsaSrvIpcConstructSession,
        LsaSrvIpcDestructSession,
        NULL));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_set_exception_function(
        gpNtlmServer,
        LsaSrvHandleIpcException,
        NULL));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_server_start(gpNtlmServer));

error:

    LW_SAFE_FREE_STRING(pszCachePath);
    LW_SAFE_FREE_STRING(pszCommPath);

    if (dwError)
    {
        if (gpNtlmServer)
        {
            lwmsg_server_stop(gpNtlmServer);
            lwmsg_server_delete(gpNtlmServer);
            gpNtlmServer = NULL;
        }
    }

    return dwError;
}

DWORD
LsaSrvStopListenThread(
    void
    )
{
    DWORD dwError = 0;

    if (gpServer)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_server_stop(gpServer));
        BAIL_ON_LSA_ERROR(dwError);
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
    if (gpContext)
    {
        lwmsg_context_delete(gpContext);
        gpContext = NULL;
    }

    return dwError;
}

DWORD
NtlmSrvStopListenThread(
    void
    )
{
    DWORD dwError = 0;

    if (gpNtlmServer)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_server_stop(gpNtlmServer));
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    if (gpNtlmServer)
    {
        lwmsg_server_delete(gpNtlmServer);
        gpNtlmServer = NULL;
    }
    if (gpNtlmProtocol)
    {
        lwmsg_protocol_delete(gpNtlmProtocol);
        gpNtlmProtocol = NULL;
    }
    if (gpNtlmContext)
    {
        lwmsg_context_delete(gpNtlmContext);
        gpNtlmContext = NULL;
    }

    return dwError;
}
