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
 *        ipc_log.c
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        Inter-process communication (Server) API for Log Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

LWMsgStatus
RSysSrvIpcSetLogInfo(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PRSYS_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;
    LWMsgSession* session = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session(assoc, &session));
    BAIL_ON_RSYS_ERROR(dwError);

    Handle = lwmsg_session_get_data(session);

    dwError = RSysSrvSetLogInfo((HANDLE)Handle,
                                (PRSYS_LOG_INFO)pRequest->object);

    if (!dwError)
    {
        pResponse->tag = RSYS_R_SET_LOGINFO_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = RSysSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_RSYS_ERROR(dwError);

        pResponse->tag = RSYS_R_SET_LOGINFO_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_RSYS_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RSysSrvIpcGetLogInfo(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PRSYS_LOG_INFO pLogInfo = NULL;
    PRSYS_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;
    LWMsgSession* session = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session(assoc, &session));
    BAIL_ON_RSYS_ERROR(dwError);

    Handle = lwmsg_session_get_data(session);

    dwError = RSysSrvGetLogInfo((HANDLE)Handle,
                               &pLogInfo);

    if (!dwError)
    {
        pResponse->tag = RSYS_R_GET_LOGINFO_SUCCESS;
        pResponse->object = pLogInfo;
        pLogInfo = NULL;
    }
    else
    {
        dwError = RSysSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_RSYS_ERROR(dwError);

        pResponse->tag = RSYS_R_GET_LOGINFO_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    if (pLogInfo)
    {
        RSysFreeLogInfo(pLogInfo);
    }
    return MAP_RSYS_ERROR_IPC(dwError);

error:
    goto cleanup;
}
