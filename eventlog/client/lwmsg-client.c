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
 * Copyright (C) Likewise Software
 * All rights reserved.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 * Eventlog Client API
 *
 */
#include "includes.h"

static LW_EVT_CLIENT_CONNECTION_CONTEXT gContext = {0};

static volatile LONG glLibraryRefCount = 0;
#ifndef BROKEN_ONCE_INIT
#if defined(__LWI_SOLARIS__) || defined (__LWI_AIX__)
#define BROKEN_ONCE_INIT 1
#else
#define BROKEN_ONCE_INIT 0
#endif
#endif

#if BROKEN_ONCE_INIT
static pthread_once_t gOnceControl = {PTHREAD_ONCE_INIT};
#else
static pthread_once_t gOnceControl = PTHREAD_ONCE_INIT;
#endif
static DWORD gdwOnceError = 0;

VOID
LwmEvtOpenServerOnce(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &gContext.pProtocol));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(gContext.pProtocol, LwEvtIPCGetProtocolSpec()));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_new(NULL, gContext.pProtocol, &gContext.pClient));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
                                  gContext.pClient, 
                                  LWMSG_ENDPOINT_DIRECT,
                                  "eventlog"));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_add_connect_endpoint(
                                  gContext.pClient,
                                  LWMSG_ENDPOINT_LOCAL,
                                  CACHEDIR "/" EVT_SERVER_FILENAME));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_connect(gContext.pClient, &gContext.pSession));
    BAIL_ON_EVT_ERROR(dwError);

cleanup:

    gdwOnceError = dwError;

    return;

error:

    if (gContext.pClient)
    {
        lwmsg_peer_delete(gContext.pClient);
        gContext.pClient = NULL;
    }
    
    if (gContext.pProtocol)
    {
        lwmsg_protocol_delete(gContext.pProtocol);
        gContext.pProtocol = NULL;
    }
    
    goto cleanup;
}

DWORD
LwmEvtOpenServer(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT* ppConn
    )
{
    DWORD dwError = 0;

    if (!ppConn)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    pthread_once(&gOnceControl, LwmEvtOpenServerOnce);
    
    dwError = gdwOnceError;
    BAIL_ON_EVT_ERROR(dwError);

    *ppConn = &gContext;

cleanup:

    return dwError;

error:

    if (ppConn) 
    {
        *ppConn = NULL;
    }
    
    goto cleanup;
}

DWORD
LwmEvtCloseServer(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn
    )
{
    return 0;
}

static
__attribute__((constructor))
VOID
LwmEvtOpenServerConstructor(
    VOID
    )
{
    LwInterlockedIncrement(&glLibraryRefCount);
}


static
__attribute__((destructor))
VOID
LwmEvtCloseServerOnce(
    VOID
    )
{
    if (!LwInterlockedDecrement(&glLibraryRefCount))
    {
        if (gContext.pClient)
        {
            lwmsg_peer_delete(gContext.pClient);
        }

        if (gContext.pProtocol)
        {
            lwmsg_protocol_delete(gContext.pProtocol);
        }

        memset(&gContext, 0, sizeof(gContext));
    }
}

DWORD
LwmEvtAcquireCall(
    HANDLE hConnection,
    LWMsgCall** ppCall
    )
{
    DWORD dwError = 0;
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pContext = hConnection;

    dwError = MAP_LWMSG_ERROR(lwmsg_peer_acquire_call(pContext->pClient, ppCall));
    BAIL_ON_EVT_ERROR(dwError);
        
error:

    return dwError;
}

DWORD
LwmEvtGetRecordCount(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pNumMatched
    )
{
    DWORD dwError = 0;
    PEVT_IPC_GENERIC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LwmEvtAcquireCall(pConn, &pCall);
    BAIL_ON_EVT_ERROR(dwError);

    in.tag = EVT_Q_GET_RECORD_COUNT;
    in.data = (PVOID)pSqlFilter;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_EVT_ERROR(dwError);
    
    switch (out.tag)
    {
    case EVT_R_GET_RECORD_COUNT:
        *pNumMatched = *(PDWORD)out.data;
        break;
    case EVT_R_GENERIC_ERROR:
        pError = (PEVT_IPC_GENERIC_ERROR) out.data;
        dwError = pError->Error;
        BAIL_ON_EVT_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }
    return dwError;

error:
    if (pNumMatched)
    {
        *pNumMatched = 0;
    }
    goto cleanup;
}

DWORD
LwmEvtReadRecords(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN DWORD MaxResults,
    IN PCWSTR pSqlFilter,
    OUT PDWORD pCount,
    OUT PLW_EVENTLOG_RECORD* ppRecords
    )
{
    DWORD dwError = 0;
    PEVT_IPC_GENERIC_ERROR pError = NULL;
    EVT_IPC_READ_RECORDS_REQ req = { 0 };
    PEVT_IPC_RECORD_ARRAY pRes = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LwmEvtAcquireCall(pConn, &pCall);
    BAIL_ON_EVT_ERROR(dwError);

    req.MaxResults = MaxResults;
    req.pFilter = pSqlFilter;

    in.tag = EVT_Q_READ_RECORDS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_EVT_ERROR(dwError);
    
    switch (out.tag)
    {
    case EVT_R_READ_RECORDS:
        pRes = (PEVT_IPC_RECORD_ARRAY)out.data;
        *pCount = pRes->Count;
        *ppRecords = pRes->pRecords;
        pRes->Count = 0;
        pRes->pRecords = NULL;
        break;
    case EVT_R_GENERIC_ERROR:
        pError = (PEVT_IPC_GENERIC_ERROR) out.data;
        dwError = pError->Error;
        BAIL_ON_EVT_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }
    return dwError;

error:
    *pCount = 0;
    *ppRecords = NULL;
    goto cleanup;
}

DWORD
LwmEvtWriteRecords(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN DWORD Count,
    IN PLW_EVENTLOG_RECORD pRecords 
    )
{
    DWORD dwError = 0;
    PEVT_IPC_GENERIC_ERROR pError = NULL;
    EVT_IPC_RECORD_ARRAY req = { 0 };

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LwmEvtAcquireCall(pConn, &pCall);
    BAIL_ON_EVT_ERROR(dwError);

    req.Count = Count;
    req.pRecords = pRecords;

    in.tag = EVT_Q_WRITE_RECORDS;
    in.data = &req;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_EVT_ERROR(dwError);
    
    switch (out.tag)
    {
    case EVT_R_GENERIC_SUCCESS:
        break;
    case EVT_R_GENERIC_ERROR:
        pError = (PEVT_IPC_GENERIC_ERROR) out.data;
        dwError = pError->Error;
        BAIL_ON_EVT_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
LwmEvtDeleteRecords(
    PLW_EVT_CLIENT_CONNECTION_CONTEXT pConn,
    IN PCWSTR pSqlFilter
    )
{
    DWORD dwError = 0;
    PEVT_IPC_GENERIC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LwmEvtAcquireCall(pConn, &pCall);
    BAIL_ON_EVT_ERROR(dwError);

    in.tag = EVT_Q_DELETE_RECORDS;
    in.data = (PVOID)pSqlFilter;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_EVT_ERROR(dwError);
    
    switch (out.tag)
    {
    case EVT_R_GENERIC_SUCCESS:
        break;
    case EVT_R_GENERIC_ERROR:
        pError = (PEVT_IPC_GENERIC_ERROR) out.data;
        dwError = pError->Error;
        BAIL_ON_EVT_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }
    return dwError;

error:
    goto cleanup;
}
