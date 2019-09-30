/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        account.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        LSA Account API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
LsaIpcAcquireCall(
    HANDLE hServer,
    LWMsgCall** ppCall
    );


DWORD
LsaPrivsEnumAccountRightsSids(
    IN HANDLE hLsaConnection,
    IN PSTR *ppszSids,
    IN DWORD NumSids,
    OUT PLUID_AND_ATTRIBUTES *ppPrivileges,
    OUT PDWORD pNumPrivileges,
    OUT PDWORD pSystemAccessRights
    )
{
    DWORD dwError = ERROR_SUCCESS;
    LSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_REQ request = {0};
    PLSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_RESP pResponse = NULL;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsaConnection, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    request.ppszSids = ppszSids;
    request.NumSids  = NumSids;

    in.tag  = LSA_PRIVS_Q_ENUM_PRIVILEGES_SIDS;
    in.data = &request;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_PRIVS_R_ENUM_PRIVILEGES_SIDS:
            pResponse = (PLSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_RESP)out.data;
            *ppPrivileges        = pResponse->pPrivileges;
            *pNumPrivileges      = pResponse->NumPrivileges;
            *pSystemAccessRights = pResponse->SystemAccessRights;
            out.data = NULL;
            break;

        case LSA2_R_ERROR:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

error:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;
}


DWORD
LsaPrivsAddAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_PRIVS_IPC_ADD_ACCOUNT_RIGHTS_REQ request = {0};
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsaConnection, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    request.NumAccountRights = NumAccountRights;

    ntStatus = RtlAllocateCStringFromSid(&request.pszSid,
                                         pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    request.ppwszAccountRights = ppwszAccountRights;

    in.tag  = LSA_PRIVS_Q_ADD_ACCOUNT_RIGHTS;
    in.data = &request;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_PRIVS_R_ADD_ACCOUNT_RIGHTS:
            break;

        case LSA2_R_ERROR:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

error:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    RTL_FREE(&request.pszSid);

    return dwError;
}


DWORD
LsaPrivsRemoveAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    IN BOOLEAN RemoveAll,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_PRIVS_IPC_REMOVE_ACCOUNT_RIGHTS_REQ request = {0};
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsaConnection, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    request.RemoveAll        = RemoveAll;
    request.NumAccountRights = NumAccountRights;

    ntStatus = RtlAllocateCStringFromSid(&request.pszSid,
                                         pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    request.ppwszAccountRights = ppwszAccountRights;

    in.tag  = LSA_PRIVS_Q_REMOVE_ACCOUNT_RIGHTS;
    in.data = &request;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_PRIVS_R_REMOVE_ACCOUNT_RIGHTS:
            break;

        case LSA2_R_ERROR:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

error:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    RTL_FREE(&request.pszSid);

    return dwError;
}


DWORD
LsaPrivsEnumAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    OUT PWSTR **pppwszAccountRights,
    OUT PDWORD pNumAccountRights
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_REQ request = {0};
    PLSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_RESP pResponse = NULL;
    PLSA_IPC_ERROR pError = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LsaIpcAcquireCall(hLsaConnection, &pCall);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateCStringFromSid(&request.pszSid,
                                         pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    in.tag  = LSA_PRIVS_Q_ENUM_ACCOUNT_RIGHTS;
    in.data = &request;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (out.tag)
    {
        case LSA_PRIVS_R_ENUM_ACCOUNT_RIGHTS:
            pResponse = (PLSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_RESP)out.data;
            out.data  = NULL;
            break;

        case LSA2_R_ERROR:
            pError = (PLSA_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppwszAccountRights = pResponse->ppwszAccountRights;
    *pNumAccountRights   = pResponse->NumAccountRights;

error:
    if (dwError)
    {
        *pppwszAccountRights = NULL;
        *pNumAccountRights   = 0;
    }

    RTL_FREE(&request.pszSid);

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;
}


static
DWORD
LsaIpcAcquireCall(
    HANDLE hServer,
    LWMsgCall** ppCall
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = hServer;

    dwError = MAP_LWMSG_ERROR(lwmsg_session_acquire_call(pContext->pSession, ppCall));
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}
