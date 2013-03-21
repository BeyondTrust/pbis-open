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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 *
 * Eventlog Server API
 *
 */
#include "includes.h"

static
DWORD
LwmEvtSrvCreateError(
    DWORD ErrorCode,
    PCWSTR pErrorMessage,
    PEVT_IPC_GENERIC_ERROR* ppError
    )
{
    DWORD dwError = 0;
    PEVT_IPC_GENERIC_ERROR pError = NULL;

    dwError = LwAllocateMemory(sizeof(*pError), (PVOID*) &pError);
    BAIL_ON_EVT_ERROR(dwError);

    if (pErrorMessage)
    {
        dwError = LwAllocateWc16String(
                        (PWSTR*)&pError->pErrorMessage,
                        pErrorMessage);
        BAIL_ON_EVT_ERROR(dwError);
    }

    pError->Error = ErrorCode;

    *ppError = pError;

error:
    return dwError;
}


void
LwmEvtSrvDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    )
{
    PLWMSG_LW_EVENTLOG_CONNECTION pConn =
        (PLWMSG_LW_EVENTLOG_CONNECTION)pSessionData;

    if (pConn)
    {
        if (pConn->pUserToken)
        {
            RtlReleaseAccessToken(&pConn->pUserToken);
        }
        LW_SAFE_FREE_MEMORY(pConn);
    }
}

LWMsgStatus
LwmEvtSrvConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    )
{
    DWORD dwError = 0;
    PLWMSG_LW_EVENTLOG_CONNECTION pConn = NULL;
    uid_t uid = 0;
    gid_t gid = 0;

    if (strcmp(lwmsg_security_token_get_type(pToken), "local"))
    {
        EVT_LOG_WARNING("Unsupported authentication type");
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(*pConn),
                    (PVOID*)&pConn);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(
                pToken,
                &uid,
                &gid));
    BAIL_ON_EVT_ERROR(dwError);

    pConn->Uid = uid;
    pConn->Gid = gid;

    *ppSessionData = pConn;

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    LW_SAFE_FREE_MEMORY(pConn);
    *ppSessionData = NULL;
    goto cleanup;
}

static
DWORD
LwmEvtSrvGetConnection(
    IN LWMsgCall* pCall,
    OUT PLWMSG_LW_EVENTLOG_CONNECTION* ppConn
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = NULL;
    PLWMSG_LW_EVENTLOG_CONNECTION pConn = NULL;
    NTSTATUS status = 0;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;

    if (pCall == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    pSession = lwmsg_call_get_session(pCall);
    if (pSession == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    pConn = (PLWMSG_LW_EVENTLOG_CONNECTION)lwmsg_session_get_data(pSession);
    if (pConn == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (!pConn->pUserToken)
    {
        status = LwMapSecurityCreateContext(&pContext);
        BAIL_ON_EVT_ERROR(status);

        status = LwMapSecurityCreateAccessTokenFromUidGid(
            pContext,
            &pConn->pUserToken,
            pConn->Uid,
            pConn->Gid);
        BAIL_ON_EVT_ERROR(status);

        dwError = EVTCheckAllowed(
                    pConn->pUserToken, 
                    EVENTLOG_READ_RECORD,
                    &pConn->ReadAllowed);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = EVTCheckAllowed(
                    pConn->pUserToken, 
                    EVENTLOG_WRITE_RECORD,
                    &pConn->WriteAllowed);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = EVTCheckAllowed(
                    pConn->pUserToken, 
                    EVENTLOG_DELETE_RECORD,
                    &pConn->DeleteAllowed);
        BAIL_ON_EVT_ERROR(dwError);

        if (!pConn->ReadAllowed &&
            !pConn->WriteAllowed &&
            !pConn->DeleteAllowed)
        {
            dwError = ERROR_ACCESS_DENIED;
            BAIL_ON_EVT_ERROR(dwError);
        }
    }

    *ppConn = pConn;

cleanup:
    if (pContext)
    {
        LwMapSecurityFreeContext(&pContext);
    }
    if (dwError == 0 && status)
    {
        dwError = LwNtStatusToWin32Error(status);
    }
    return dwError;

error:
    *ppConn = NULL;
    goto cleanup;
}

DWORD
LwmEvtSrvGetRecordCount(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PCWSTR pFilter = pIn->data;
    PDWORD pRes = NULL;
    PEVT_IPC_GENERIC_ERROR pError = NULL;
    sqlite3 *pDb = NULL;
    PLWMSG_LW_EVENTLOG_CONNECTION pConn = NULL;

    dwError = LwmEvtSrvGetConnection(
                    pCall,
                    &pConn);
    if (dwError)
    {
    }
    else if (!pConn->ReadAllowed)
    {
        dwError = ERROR_ACCESS_DENIED;
    }
    else
    {
        dwError = LwEvtDbOpen(&pDb);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateMemory(sizeof(*pRes), (PVOID*) &pRes);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwEvtDbGetRecordCount(
                        pDb,
                        pFilter,
                        pRes);
    }
    if (!dwError)
    {
        pOut->tag = EVT_R_GET_RECORD_COUNT;
        pOut->data = pRes;
        pRes = NULL;
    }
    else
    {
        dwError = LwmEvtSrvCreateError(dwError, NULL, &pError);
        BAIL_ON_EVT_ERROR(dwError);

        pOut->tag = EVT_R_GENERIC_ERROR;
        pOut->data = pError;
    }

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }
    LW_SAFE_FREE_MEMORY(pRes);
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

DWORD
LwmEvtSrvReadRecords(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PEVT_IPC_READ_RECORDS_REQ pReq = pIn->data;
    PEVT_IPC_RECORD_ARRAY pRes = NULL;
    PEVT_IPC_GENERIC_ERROR pError = NULL;
    sqlite3 *pDb = NULL;
    PLWMSG_LW_EVENTLOG_CONNECTION pConn = NULL;

    dwError = LwmEvtSrvGetConnection(
                    pCall,
                    &pConn);
    if (dwError)
    {
    }
    else if (!pConn->ReadAllowed)
    {
        dwError = ERROR_ACCESS_DENIED;
    }
    else
    {
        dwError = LwEvtDbOpen(&pDb);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateMemory(sizeof(*pRes), (PVOID*) &pRes);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwEvtDbReadRecords(
                        LwAllocateMemory,
                        LwFreeMemory,
                        pDb,
                        pReq->MaxResults,
                        pReq->pFilter,
                        &pRes->Count,
                        &pRes->pRecords);
    }
    if (!dwError)
    {
        pOut->tag = EVT_R_READ_RECORDS;
        pOut->data = pRes;
        pRes = NULL;
    }
    else
    {
        dwError = LwmEvtSrvCreateError(dwError, NULL, &pError);
        BAIL_ON_EVT_ERROR(dwError);

        pOut->tag = EVT_R_GENERIC_ERROR;
        pOut->data = pError;
    }

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }
    if (pRes)
    {
        LwEvtFreeRecordArray(
            pRes->Count,
            pRes->pRecords);
        LW_SAFE_FREE_MEMORY(pRes);
    }
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

DWORD
LwmEvtSrvWriteRecords(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PEVT_IPC_RECORD_ARRAY pReq = pIn->data;
    PEVT_IPC_GENERIC_ERROR pError = NULL;
    sqlite3 *pDb = NULL;
    PLWMSG_LW_EVENTLOG_CONNECTION pConn = NULL;

    dwError = LwmEvtSrvGetConnection(
                    pCall,
                    &pConn);
    if (dwError)
    {
    }
    else if (!pConn->WriteAllowed)
    {
        dwError = ERROR_ACCESS_DENIED;
    }
    else
    {
        dwError = LwEvtDbOpen(&pDb);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwEvtDbWriteRecords(
                        pDb,
                        pReq->Count,
                        pReq->pRecords);
    }
    if (!dwError)
    {
        // Process SNMP Traps.
        EvtSnmpProcessEvents(pReq->Count, pReq->pRecords); 

        pOut->tag = EVT_R_GENERIC_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LwmEvtSrvCreateError(dwError, NULL, &pError);
        BAIL_ON_EVT_ERROR(dwError);

        pOut->tag = EVT_R_GENERIC_ERROR;
        pOut->data = pError;
    }

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

DWORD
LwmEvtSrvDeleteRecords(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PCWSTR pFilter = pIn->data;
    PEVT_IPC_GENERIC_ERROR pError = NULL;
    sqlite3 *pDb = NULL;
    PLWMSG_LW_EVENTLOG_CONNECTION pConn = NULL;

    dwError = LwmEvtSrvGetConnection(
                    pCall,
                    &pConn);
    if (dwError)
    {
    }
    else if (!pConn->WriteAllowed)
    {
        dwError = ERROR_ACCESS_DENIED;
    }
    else
    {
        dwError = LwEvtDbOpen(&pDb);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwEvtDbDeleteRecords(
                        pDb,
                        pFilter);
    }
    if (!dwError)
    {
        pOut->tag = EVT_R_GENERIC_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LwmEvtSrvCreateError(dwError, NULL, &pError);
        BAIL_ON_EVT_ERROR(dwError);

        pOut->tag = EVT_R_GENERIC_ERROR;
        pOut->data = pError;
    }

cleanup:
    if (pDb != NULL)
    {
        LwEvtDbClose(pDb);
    }
    return MAP_LW_ERROR_IPC(dwError);

error:
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
