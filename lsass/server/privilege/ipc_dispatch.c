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
 *        ipc_dispatch.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Privileges
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
LsaSrvIpcCreateError(
    DWORD errCode,
    PCSTR pszErrorMessage,
    PLSA_IPC_ERROR* ppError
    );

static
HANDLE
LsaSrvIpcGetSessionData(
    LWMsgCall* pCall
    );


LWMsgStatus
LsaSrvIpcPrivsEnumPrivilegesSids(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD err = ERROR_SUCCESS;
    PLSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_REQ pRequest = pIn->data;
    PLSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_RESP pResponse = NULL;
    PLUID_AND_ATTRIBUTES pPrivileges = NULL;
    DWORD numPrivileges = 0;
    DWORD systemAccessRights = 0;
    PLSA_IPC_ERROR pError = NULL;

    err = LsaSrvPrivsEnumAccountRightsSids(
                  LsaSrvIpcGetSessionData(pCall),
                  pRequest->ppszSids,
                  pRequest->NumSids,
                  &pPrivileges,
                  &numPrivileges,
                  &systemAccessRights);
    if (err == ERROR_SUCCESS)
    {
        err = LwAllocateMemory(sizeof(*pResponse),
                               OUT_PPVOID(&pResponse));
        BAIL_ON_LSA_ERROR(err);

        pResponse->pPrivileges        = pPrivileges;
        pResponse->NumPrivileges      = numPrivileges;
        pResponse->SystemAccessRights = systemAccessRights;

        pOut->tag  = LSA_PRIVS_R_ENUM_PRIVILEGES_SIDS;
        pOut->data = pResponse;
    }
    else
    {
        err = LsaSrvIpcCreateError(err, NULL, &pError);
        BAIL_ON_LSA_ERROR(err);

        pOut->tag  = LSA2_R_ERROR;
        pOut->data = pError;
    }

error:
    return MAP_LW_ERROR_IPC(err);
}


LWMsgStatus
LsaSrvIpcPrivsAddAccountRights(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_PRIVS_IPC_ADD_ACCOUNT_RIGHTS_REQ pRequest = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    PSID pSid = NULL;

    ntStatus = RtlAllocateSidFromCString(
                  &pSid,
                  pRequest->pszSid);
    if (ntStatus)
    {
        err = LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_LSA_ERROR(err);
    }

    err = LsaSrvPrivsAddAccountRights(
                  LsaSrvIpcGetSessionData(pCall),
                  NULL,
                  pSid,
                  pRequest->ppwszAccountRights,
                  pRequest->NumAccountRights);
    if (err == ERROR_SUCCESS)
    {
        pOut->tag  = LSA_PRIVS_R_ADD_ACCOUNT_RIGHTS;
        pOut->data = NULL;
    }
    else
    {
        err = LsaSrvIpcCreateError(err, NULL, &pError);
        BAIL_ON_LSA_ERROR(err);

        pOut->tag  = LSA2_R_ERROR;
        pOut->data = pError;
    }

error:
    RTL_FREE(&pSid);

    return MAP_LW_ERROR_IPC(err);
}


LWMsgStatus
LsaSrvIpcPrivsRemoveAccountRights(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_PRIVS_IPC_REMOVE_ACCOUNT_RIGHTS_REQ pRequest = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    PSID pSid = NULL;

    ntStatus = RtlAllocateSidFromCString(
                  &pSid,
                  pRequest->pszSid);
    if (ntStatus)
    {
        err = LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_LSA_ERROR(err);
    }

    err = LsaSrvPrivsRemoveAccountRights(
                  LsaSrvIpcGetSessionData(pCall),
                  NULL,
                  pSid,
                  pRequest->RemoveAll,
                  pRequest->ppwszAccountRights,
                  pRequest->NumAccountRights);
    if (err == ERROR_SUCCESS)
    {
        pOut->tag  = LSA_PRIVS_R_REMOVE_ACCOUNT_RIGHTS;
        pOut->data = NULL;
    }
    else
    {
        err = LsaSrvIpcCreateError(err, NULL, &pError);
        BAIL_ON_LSA_ERROR(err);

        pOut->tag  = LSA2_R_ERROR;
        pOut->data = pError;
    }

error:
    RTL_FREE(&pSid);

    return MAP_LW_ERROR_IPC(err);
}


LWMsgStatus
LsaSrvIpcPrivsEnumAccountRights(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_REQ pRequest = pIn->data;
    PLSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_RESP pResponse = NULL;
    PWSTR *ppwszAccountRights = NULL;
    DWORD NumAccountRights = 0;
    PLSA_IPC_ERROR pError = NULL;
    PSID pSid = NULL;

    ntStatus = RtlAllocateSidFromCString(
                  &pSid,
                  pRequest->pszSid);
    if (ntStatus)
    {
        err = LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_LSA_ERROR(err);
    }

    err = LsaSrvPrivsEnumAccountRights(
                  LsaSrvIpcGetSessionData(pCall),
                  NULL,
                  pSid,
                  &ppwszAccountRights,
                  &NumAccountRights);
    if (err == ERROR_SUCCESS)
    {
        err = LwAllocateMemory(sizeof(*pResponse),
                               OUT_PPVOID(&pResponse));
        BAIL_ON_LSA_ERROR(err);

        pResponse->ppwszAccountRights = ppwszAccountRights;
        pResponse->NumAccountRights   = NumAccountRights;

        pOut->tag  = LSA_PRIVS_R_ENUM_ACCOUNT_RIGHTS;
        pOut->data = pResponse;
    }
    else
    {
        err = LsaSrvIpcCreateError(err, NULL, &pError);
        BAIL_ON_LSA_ERROR(err);

        pOut->tag  = LSA2_R_ERROR;
        pOut->data = pError;
    }

error:
    RTL_FREE(&pSid);

    return MAP_LW_ERROR_IPC(err);
}


static
HANDLE
LsaSrvIpcGetSessionData(
    LWMsgCall* pCall
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    return lwmsg_session_get_data(pSession);
}

