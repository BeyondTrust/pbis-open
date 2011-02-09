/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        privileges.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Provider Privileges API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "localclient.h"


DWORD
LsaLocalAddAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgContext *context = NULL;
    LWMsgDataContext* pDataContext = NULL;
    LSA_LOCAL_IPC_ADD_ACCOUNT_RIGHTS_REQ request = {0};
    size_t requestSize = 0;
    PVOID pRequestBlob = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    request.NumAccountRights = NumAccountRights;

    ntStatus = RtlAllocateCStringFromSid(&request.pszSid,
                                         pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    request.ppwszAccountRights = ppwszAccountRights;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaLocalIpcGetAddAccountRightsReqSpec(),
                              &request,
                              &pRequestBlob,
                              &requestSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(hLsaConnection,
                                   LSA_PROVIDER_TAG_LOCAL,
                                   LSA_LOCAL_IO_ADDACCOUNTRIGHTS,
                                   requestSize,
                                   pRequestBlob,
                                   NULL,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (context)
    {
        lwmsg_context_delete(context);
    }

    RTL_FREE(&request.pszSid);

    return dwError;
}


DWORD
LsaLocalRemoveAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    IN BOOLEAN RemoveAll,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgContext *context = NULL;
    LWMsgDataContext* pDataContext = NULL;
    LSA_LOCAL_IPC_REMOVE_ACCOUNT_RIGHTS_REQ request = {0};
    size_t requestSize = 0;
    PVOID pRequestBlob = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    request.RemoveAll        = RemoveAll;
    request.NumAccountRights = NumAccountRights;

    ntStatus = RtlAllocateCStringFromSid(&request.pszSid,
                                         pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    request.ppwszAccountRights = ppwszAccountRights;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaLocalIpcGetRemoveAccountRightsReqSpec(),
                              &request,
                              &pRequestBlob,
                              &requestSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderIoControl(hLsaConnection,
                                   LSA_PROVIDER_TAG_LOCAL,
                                   LSA_LOCAL_IO_REMOVEACCOUNTRIGHTS,
                                   requestSize,
                                   pRequestBlob,
                                   NULL,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

error:
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (context)
    {
        lwmsg_context_delete(context);
    }

    RTL_FREE(&request.pszSid);

    return dwError;
}


DWORD
LsaLocalLookupPrivilegeValue(
    IN HANDLE hLsaConnection,
    IN PCWSTR pwszPrivilegeName,
    OUT PLUID pPrivilegeValue
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t privilegeNameLen = 0;
    DWORD requestSize = 0;
    PVOID pRequestBlob = NULL;
    DWORD responseSize = 0;
    PVOID pResponseBlob = NULL;

    dwError = LwWc16sLen(pwszPrivilegeName,
                         &privilegeNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    requestSize  = sizeof(WCHAR) * (privilegeNameLen + 1);

    dwError = LwAllocateMemory(requestSize,
                               OUT_PPVOID(&pRequestBlob));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pRequestBlob,
           pwszPrivilegeName,
           sizeof(WCHAR) * privilegeNameLen);

    dwError = LsaProviderIoControl(hLsaConnection,
                                   LSA_PROVIDER_TAG_LOCAL,
                                   LSA_LOCAL_IO_LOOKUPPRIVILEGEVALUE,
                                   requestSize,
                                   pRequestBlob,
                                   &responseSize,
                                   &pResponseBlob);
    BAIL_ON_LSA_ERROR(dwError);

    *pPrivilegeValue = *((PLUID)(pResponseBlob));

error:
    if (dwError)
    {
        pPrivilegeValue->LowPart  = 0;
        pPrivilegeValue->HighPart = 0;
    }

    LW_SAFE_FREE_MEMORY(pResponseBlob);

    return dwError;
}


DWORD
LsaLocalLookupPrivilegeName(
    IN HANDLE hLsaConnection,
    IN PLUID pPrivilegeValue,
    OUT PWSTR *ppwszPrivilegeName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD requestSize = 0;
    PVOID pRequestBlob = NULL;
    DWORD responseSize = 0;
    PVOID pResponseBlob = NULL;

    requestSize  = sizeof(*pPrivilegeValue);
    pRequestBlob = pPrivilegeValue;

    dwError = LsaProviderIoControl(hLsaConnection,
                                   LSA_PROVIDER_TAG_LOCAL,
                                   LSA_LOCAL_IO_LOOKUPPRIVILEGENAME,
                                   requestSize,
                                   pRequestBlob,
                                   &responseSize,
                                   &pResponseBlob);
    BAIL_ON_LSA_ERROR(dwError);

    *ppwszPrivilegeName = (PWSTR)pResponseBlob;

error:
    if (dwError)
    {
        *ppwszPrivilegeName = NULL;
    }

    return dwError;
}


DWORD
LsaLocalEnumAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    OUT PWSTR **pppwszAccountRights,
    OUT PDWORD pNumAccountRights
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgContext *context = NULL;
    LWMsgDataContext* pDataContext = NULL;
    PSTR pszAccountSid = NULL;
    PLSA_LOCAL_IPC_ENUM_ACCOUNT_RIGHTS_RESP pResponse = NULL;
    DWORD requestSize = 0;
    PVOID pRequestBlob = NULL;
    DWORD responseSize = 0;
    PVOID pResponseBlob = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateCStringFromSid(&pszAccountSid,
                                         pAccountSid);
    BAIL_ON_NT_STATUS(ntStatus);

    requestSize = sizeof(CHAR) * ((DWORD)(strlen(pszAccountSid) + 1));
    pRequestBlob = pszAccountSid;

    dwError = LsaProviderIoControl(hLsaConnection,
                                   LSA_PROVIDER_TAG_LOCAL,
                                   LSA_LOCAL_IO_ENUMACCOUNTRIGHTS,
                                   requestSize,
                                   pRequestBlob,
                                   &responseSize,
                                   &pResponseBlob);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaLocalIpcGetEnumAccountRightsRespSpec(),
                              pResponseBlob,
                              responseSize,
                              (PVOID*)&pResponse));
    BAIL_ON_LSA_ERROR(dwError);

    *pppwszAccountRights = pResponse->ppwszAccountRights;
    *pNumAccountRights   = pResponse->NumAccountRights;

error:
    if (dwError)
    {
        *pppwszAccountRights = NULL;
        *pNumAccountRights   = 0;
    }

    if (pResponse)
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaLocalIpcGetEnumAccountRightsRespSpec(),
            pResponse);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (context)
    {
        lwmsg_context_delete(context);
    }

    RTL_FREE(&pszAccountSid);
    LW_SAFE_FREE_MEMORY(pResponseBlob);

    return dwError;
}
