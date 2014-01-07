/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa_stubmemory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Lsa rpc DCE/RPC stub memory cleanup functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


VOID
LsaCleanStubTranslatedSidArray(
    TranslatedSidArray *pArray
    )
{
    unsigned32 rpcStatus = 0;

    rpc_sm_client_free(pArray->sids, &rpcStatus);
}


VOID
LsaCleanStubTranslatedSidArray2(
    TranslatedSidArray2 *pArray
    )
{
    unsigned32 rpcStatus = 0;

    rpc_sm_client_free(pArray->sids, &rpcStatus);
}


VOID
LsaCleanStubTranslatedSidArray3(
    TranslatedSidArray3 *pArray
    )
{
    unsigned32 rpcStatus = 0;
    int i = 0;

    for (i = 0; i < pArray->count; i++)
    {
        rpc_sm_client_free(pArray->sids[i].sid, &rpcStatus);
    }

    rpc_sm_client_free(pArray->sids, &rpcStatus);
}


VOID
LsaCleanStubTranslatedNameArray(
    TranslatedNameArray *pArray
    )
{
    unsigned32 rpcStatus = 0;
    int i = 0;

    for (i = 0; i < pArray->count; i++)
    {
        TranslatedName *pName = &(pArray->names[i]);

        rpc_sm_client_free(pName->name.Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pArray->names, &rpcStatus);
}


VOID
LsaCleanStubRefDomainList(
    RefDomainList *pRefDomList
    )
{
    unsigned32 rpcStatus = 0;
    int i = 0;

    for (i = 0; i < pRefDomList->count; i++)
    {
        LsaDomainInfo *pDomInfo = &(pRefDomList->domains[i]);

        rpc_sm_client_free(pDomInfo->name.Buffer, &rpcStatus);
        if (pDomInfo->sid)
        {
            rpc_sm_client_free(pDomInfo->sid, &rpcStatus);
        }
    }

    rpc_sm_client_free(pRefDomList->domains, &rpcStatus);
}


VOID
LsaFreeStubRefDomainList(
    RefDomainList *pRefDomList
    )
{
    unsigned32 rpcStatus = 0;

    LsaCleanStubRefDomainList(pRefDomList);
    rpc_sm_client_free(pRefDomList, &rpcStatus);
}


VOID
LsaCleanStubPolicyInformation(
    LsaPolicyInformation *pPolicyInfo,
    UINT32 Level
    )
{
    unsigned32 rpcStatus = 0;

    switch (Level) {
    case LSA_POLICY_INFO_AUDIT_EVENTS:
        rpc_sm_client_free(pPolicyInfo->audit_events.settings, &rpcStatus);
        break;

    case LSA_POLICY_INFO_DOMAIN:
    case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
        rpc_sm_client_free(pPolicyInfo->domain.name.Buffer, &rpcStatus);
        rpc_sm_client_free(pPolicyInfo->domain.sid, &rpcStatus);
        break;

    case LSA_POLICY_INFO_PD:
        rpc_sm_client_free(pPolicyInfo->pd.name.Buffer, &rpcStatus);
        break;

    case LSA_POLICY_INFO_REPLICA:
        rpc_sm_client_free(pPolicyInfo->replica.source.Buffer, &rpcStatus);
        rpc_sm_client_free(pPolicyInfo->replica.account.Buffer, &rpcStatus);
        break;

    case LSA_POLICY_INFO_DNS:
        rpc_sm_client_free(pPolicyInfo->dns.name.Buffer, &rpcStatus);
        rpc_sm_client_free(pPolicyInfo->dns.dns_domain.Buffer, &rpcStatus);
        rpc_sm_client_free(pPolicyInfo->dns.dns_forest.Buffer, &rpcStatus);
        rpc_sm_client_free(pPolicyInfo->dns.sid, &rpcStatus);
        break;

    case LSA_POLICY_INFO_AUDIT_LOG:
    case LSA_POLICY_INFO_ROLE:
    case LSA_POLICY_INFO_QUOTA:
    case LSA_POLICY_INFO_DB:
    case LSA_POLICY_INFO_AUDIT_FULL_SET:
    case LSA_POLICY_INFO_AUDIT_FULL_QUERY:
    default:
        break;
    }
}


VOID
LsaFreeStubPolicyInformation(
    LsaPolicyInformation *pPolicyInfo,
    UINT32 Level
    )
{
    unsigned32 rpcStatus = 0;

    LsaCleanStubPolicyInformation(pPolicyInfo, Level);
    rpc_sm_client_free(pPolicyInfo, &rpcStatus);
}


VOID
LsaFreeStubPrivilegeSet(
    PPRIVILEGE_SET pPrivileges
    )
{
    unsigned32 rpcStatus = 0;

    rpc_sm_client_free(pPrivileges, &rpcStatus);
}


VOID
LsaCleanStubUnicodeString(
    PUNICODE_STRING pString
    )
{
    unsigned32 rpcStatus = 0;

    rpc_sm_client_free(pString->Buffer, &rpcStatus);

    pString->Buffer = NULL;
    pString->Length = 0;
    pString->MaximumLength = 0;
}


VOID
LsaFreeStubUnicodeString(
    PUNICODE_STRING pString
    )
{
    unsigned32 rpcStatus = 0;

    LsaCleanStubUnicodeString(pString);
    rpc_sm_client_free(pString, &rpcStatus);
}


void
LsaCleanStubSecurityDescriptorBuffer(
    PLSA_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    )
{
    unsigned32 rpcStatus = 0;
    rpc_sm_client_free(pSecDescBuffer->pBuffer,
                       &rpcStatus);
}


void
LsaFreeStubSecurityDescriptorBuffer(
    PLSA_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    )
{
    unsigned32 rpcStatus = 0;

    LsaCleanStubSecurityDescriptorBuffer(pSecDescBuffer);
    rpc_sm_client_free(pSecDescBuffer, &rpcStatus);
}


void
LsaCleanStubAccountBuffer(
    PLSA_ACCOUNT_ENUM_BUFFER pBuffer
    )
{
    unsigned32 rpcStatus = 0;
    int i = 0;

    for (i = 0; i < pBuffer->NumAccounts; i++)
    {
        rpc_sm_client_free(pBuffer->pAccount[i].pSid, &rpcStatus);
    }

    rpc_sm_client_free(pBuffer->pAccount, &rpcStatus);
}


void
LsaCleanStubAccountRights(
    PLSA_ACCOUNT_RIGHTS pBuffer
    )
{
    unsigned32 rpcStatus = 0;
    int i = 0;

    for (i = 0; i < pBuffer->NumAccountRights; i++)
    {
        rpc_sm_client_free(pBuffer->pAccountRight[i].Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pBuffer->pAccountRight, &rpcStatus);
}


void
LsaCleanStubPrivilegeBuffer(
    PLSA_PRIVILEGE_ENUM_BUFFER pBuffer
    )
{
    unsigned32 rpcStatus = 0;
    int i = 0;

    for (i = 0; i < pBuffer->NumPrivileges; i++)
    {
        rpc_sm_client_free(pBuffer->pPrivilege[i].Name.Buffer, &rpcStatus);
    }

    rpc_sm_client_free(pBuffer->pPrivilege, &rpcStatus);
}
