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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (SMBSS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"
#include "ioinit.h"
#include "ioipc.h"

static
NTSTATUS
LwioSrvSetDefaults(
    PLWIO_CONFIG pConfig
    );

static
DWORD
SMBSrvInitialize(
    VOID
    );

static
DWORD
SMBInitCacheFolders(
    VOID
    );

static
DWORD
SMBSrvExecute(
    VOID
    );

#ifdef ENABLE_STATIC_DRIVERS

extern NTSTATUS IO_DRIVER_ENTRY(rdr)(IO_DRIVER_HANDLE, ULONG);
extern NTSTATUS IO_DRIVER_ENTRY(srv)(IO_DRIVER_HANDLE, ULONG);
extern NTSTATUS IO_DRIVER_ENTRY(npfs)(IO_DRIVER_HANDLE, ULONG);
extern NTSTATUS IO_DRIVER_ENTRY(pvfs)(IO_DRIVER_HANDLE, ULONG);

static IO_STATIC_DRIVER gStaticDrivers[] =
{
#ifdef ENABLE_RDR
    IO_STATIC_DRIVER_ENTRY(rdr),
#endif
#ifdef ENABLE_SRV
    IO_STATIC_DRIVER_ENTRY(srv),
#endif
#ifdef ENABLE_PVFS
    IO_STATIC_DRIVER_ENTRY(pvfs),
#endif
#ifdef ENABLE_NPFS
    IO_STATIC_DRIVER_ENTRY(npfs),
#endif
#ifdef ENABLE_DFS
    IO_STATIC_DRIVER_ENTRY(dfs),
#endif

    IO_STATIC_DRIVER_END
};

#endif

static LWMsgContext* pContext = NULL;
static LWMsgProtocol* pProtocol = NULL;
static LWMsgPeer* pServer = NULL;

NTSTATUS
LwIoSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
LwIoSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}

NTSTATUS
LwIoSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwioSrvInitializeConfig(&gLwioServerConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&gLwioServerInfo.lock, NULL);
    gLwioServerInfo.pLock = &gLwioServerInfo.lock;

    ntStatus = LwioSrvRefreshConfig(&gLwioServerConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwioSrvSetDefaults(&gLwioServerConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvInitialize();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvExecute();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwIoSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    LWIO_LOG_VERBOSE("LWIO main cleaning up");

    IoCleanup();

    if (pServer)
    {
        LWMsgStatus status2 = lwmsg_peer_stop_listen(pServer);

        if (status2)
        {
            LWIO_LOG_ERROR("Error stopping server. [Error code:%d]", status2);
        }

        lwmsg_peer_delete(pServer);
    }

    if (pProtocol)
    {
        lwmsg_protocol_delete(pProtocol);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    LWIO_LOG_INFO("LWIO Service exiting...");

    if (gLwioServerInfo.pLock)
    {
        pthread_mutex_destroy(&gLwioServerInfo.lock);
        gLwioServerInfo.pLock = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
LwIoSvcmRefresh(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return LwioSrvRefreshConfig(&gLwioServerConfig);
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = LwIoSvcmInit,
    .Destroy = LwIoSvcmDestroy,
    .Start = LwIoSvcmStart,
    .Stop = LwIoSvcmStop,
    .Refresh = LwIoSvcmRefresh
};

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(lwio)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gService;
}

static
NTSTATUS
LwioSrvSetDefaults(
    PLWIO_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWIO_CONFIG defaultConfig;

    strncpy(gpLwioServerInfo->szCachePath, CACHEDIR, PATH_MAX);
    gpLwioServerInfo->szCachePath[PATH_MAX] = '\0';

    strncpy(gpLwioServerInfo->szPrefixPath, PREFIXDIR, PATH_MAX);
    gpLwioServerInfo->szPrefixPath[PATH_MAX] = '\0';

    // Enforce configuration settings
    ntStatus = LwioSrvInitializeConfig(&defaultConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LwioSrvFreeConfigContents(&defaultConfig);

    return ntStatus;

error:

    goto cleanup;    
}

static
DWORD
SMBSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = SMBInitCacheFolders();
    BAIL_ON_LWIO_ERROR(dwError);

#ifdef ENABLE_STATIC_DRIVERS
    dwError = IoInitialize(gStaticDrivers);
#else
    dwError = IoInitialize(NULL);
#endif
    BAIL_ON_LWIO_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
SMBInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = SMBCheckDirectoryExists(
                        CACHEDIR,
                        &bExists);
    BAIL_ON_LWIO_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = SMBCreateDirectory(CACHEDIR, cacheDirMode);
        BAIL_ON_LWIO_ERROR(dwError);
    }

error:

    return dwError;
}

LWMsgBool
LwIoDaemonLogIpc (
    LWMsgLogLevel level,
    const char* pszMessage,
    const char* pszFunction,
    const char* pszFilename,
    unsigned int line,
    void* pData
    )
{
    LWIO_LOG_LEVEL ioLevel = LWIO_LOG_LEVEL_DEBUG;
    LWMsgBool result = LWMSG_FALSE;

    switch (level)
    {
    case LWMSG_LOGLEVEL_ALWAYS:
        ioLevel = LWIO_LOG_LEVEL_ALWAYS;
        break;
    case LWMSG_LOGLEVEL_ERROR:
        ioLevel = LWIO_LOG_LEVEL_ERROR;
        break;
    case LWMSG_LOGLEVEL_WARNING:
        ioLevel = LWIO_LOG_LEVEL_WARNING;
        break;
    case LWMSG_LOGLEVEL_INFO:
        ioLevel = LWIO_LOG_LEVEL_INFO;
        break;
    case LWMSG_LOGLEVEL_VERBOSE:
        ioLevel = LWIO_LOG_LEVEL_VERBOSE;
        break;
    case LWMSG_LOGLEVEL_DEBUG:
        ioLevel = LWIO_LOG_LEVEL_DEBUG;
        break;
    case LWMSG_LOGLEVEL_TRACE:
        ioLevel = LWIO_LOG_LEVEL_TRACE;
        break;
    }
    
    result = LwRtlLogGetLevel() >= ioLevel;

    if (pszMessage && result)
    {
        LW_RTL_LOG_RAW(ioLevel, "lwio-ipc", pszFunction, pszFilename, line, "%s", pszMessage);
    }

    return result;
}

static
DWORD
SMBSrvExecute(
    VOID
    )
{
    DWORD dwError = 0;
    LWMsgTime timeout = { 30, 0 }; /* 30 seconds */
    PCSTR pszSmNotify = NULL;
    int notifyFd = -1;
    char notifyCode = 0;
    int ret = 0;

    dwError = MAP_LWMSG_STATUS(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LWIO_ERROR(dwError);

    lwmsg_context_set_log_function(pContext, LwIoDaemonLogIpc, NULL);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(pContext, &pProtocol));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwIoDaemonIpcAddProtocolSpec(pProtocol);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = IoIpcAddProtocolSpec(pProtocol);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_new(pContext, pProtocol, &pServer));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = LwIoDaemonIpcAddDispatch(pServer);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = IoIpcAddDispatch(pServer);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_listen_endpoint(
                    pServer,
                    LWMSG_ENDPOINT_DIRECT,
                    "lwio",
                    0));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_add_listen_endpoint(
                    pServer,
                    LWMSG_ENDPOINT_LOCAL,
                    LWIO_SERVER_FILENAME,
                    (S_IRWXU | S_IRWXG | S_IRWXO)));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_set_max_listen_clients(
                    pServer,
                    512));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_set_timeout(
                    pServer,
                    LWMSG_TIMEOUT_IDLE,
                    &timeout));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_peer_start_listen(pServer));
    BAIL_ON_LWIO_ERROR(dwError);

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);
        
        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            LWIO_LOG_ERROR("Could not notify service manager: %s (%i)", strerror(errno), errno);
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LWIO_ERROR(dwError);
        }

        close(notifyFd);
    }

cleanup:

    return dwError;

error:

    LWIO_LOG_ERROR("SMB Server stopping due to error [code: %d]", dwError);

    if (pServer)
    {
        LWMsgStatus status2 = lwmsg_peer_stop_listen(pServer);

        if (status2)
        {
            LWIO_LOG_ERROR("Error stopping server. [Error code:%d]", status2);
        }

        lwmsg_peer_delete(pServer);
    }

    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
