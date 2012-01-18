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
 *        netr_stubmemory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Netlogon rpc DCE/RPC stub memory cleanup functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


VOID
NetrCleanStubDomainTrustList(
    NetrDomainTrustList *pTrustList
    )
{
    unsigned32 rpcStatus = 0;
    UINT32 i = 0;

    for (i = 0; i < pTrustList->count; i++) {
        NetrDomainTrust *pTrust = &pTrustList->array[i];

        rpc_sm_client_free(pTrust->netbios_name, &rpcStatus);
        rpc_sm_client_free(pTrust->dns_name, &rpcStatus);
        if (pTrust->sid) {
            rpc_sm_client_free(pTrust->sid, &rpcStatus);
        }
    }

    rpc_sm_client_free(pTrustList->array, &rpcStatus);
}


static
VOID
NetrCleanSamBaseInfo(
    NetrSamBaseInfo *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    rpc_sm_client_free(pInfo->account_name.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->full_name.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->logon_script.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->profile_path.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->home_directory.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->home_drive.Buffer, &rpcStatus);

    pInfo->groups.dwCount = 0;
    rpc_sm_client_free(pInfo->groups.pRids, &rpcStatus);

    rpc_sm_client_free(pInfo->logon_server.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->domain.Buffer, &rpcStatus);

    if (pInfo->domain_sid) {
        rpc_sm_client_free(pInfo->domain_sid, &rpcStatus);
    }
}


static
VOID
NetrCleanSamInfo2(
    NetrSamInfo2 *pInfo
    )
{
    NetrCleanSamBaseInfo(&pInfo->base);
}


static
VOID
NetrFreeSamInfo2(
    NetrSamInfo2 *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    if (pInfo == NULL) return;

    NetrCleanSamInfo2(pInfo);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


static
VOID
NetrCleanSidAttr(
    NetrSidAttr *pSidAttr,
    UINT32 Count
    )
{
    unsigned32 rpcStatus = 0;
    UINT32 i = 0;

    for (i = 0; pSidAttr && i < Count; i++) {
        if (pSidAttr[i].sid) {
            rpc_sm_client_free(pSidAttr[i].sid, &rpcStatus);
        }
    }
}


static
void
NetrFreeSidAttr(
    NetrSidAttr *pSidAttr,
    UINT32 Count)
{
    unsigned32 rpcStatus = 0;

    NetrCleanSidAttr(pSidAttr, Count);
    rpc_sm_client_free(pSidAttr, &rpcStatus);
}


static
VOID
NetrCleanSamInfo3(
    NetrSamInfo3 *pInfo
    )
{
    NetrCleanSamBaseInfo(&pInfo->base);

    if (pInfo->sids) {
        NetrFreeSidAttr(pInfo->sids,
                        pInfo->sidcount);
    }
}


static
VOID
NetrFreeSamInfo3(
    NetrSamInfo3 *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    if (pInfo == NULL) return;

    NetrCleanSamInfo3(pInfo);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


static
VOID
NetrCleanSamInfo6(
    NetrSamInfo6 *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    NetrCleanSamBaseInfo(&pInfo->base);

    if (pInfo->sids) {
        NetrFreeSidAttr(pInfo->sids,
                        pInfo->sidcount);
    }

    rpc_sm_client_free(pInfo->forest.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->principal.Buffer, &rpcStatus);
}


static
VOID
NetrFreeSamInfo6(
    NetrSamInfo6 *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    if (pInfo == NULL) return;

    NetrCleanSamInfo6(pInfo);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


static
VOID
NetrCleanPacInfo(
    NetrPacInfo *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    rpc_sm_client_free(pInfo->pac, &rpcStatus);
    rpc_sm_client_free(pInfo->auth, &rpcStatus);

    rpc_sm_client_free(pInfo->logon_domain.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->logon_server.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->principal_name.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown1.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown2.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown3.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown4.Buffer, &rpcStatus);
}


static
VOID
NetrFreePacInfo(
    NetrPacInfo *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    if (pInfo == NULL) return;

    NetrCleanPacInfo(pInfo);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


VOID
NetrCleanStubValidationInfo(
    NetrValidationInfo *pInfo,
    UINT16 Level
    )
{
    switch (Level) {
    case 2:
        NetrFreeSamInfo2(pInfo->sam2);
        break;
    case 3:
        NetrFreeSamInfo3(pInfo->sam3);
        break;
    case 4:
        NetrFreePacInfo(pInfo->pac4);
        break;
    case 5:
        NetrFreePacInfo(pInfo->pac5);
        break;
    case 6:
        NetrFreeSamInfo6(pInfo->sam6);
        break;
    default:
        break;
    }
}


static
VOID
NetrCleanDomainTrustInfo(
    NetrDomainTrustInfo *pInfo
    )
{
    unsigned32 rpcStatus = 0;
    UINT32 i = 0;

    if (pInfo == NULL) return;

    rpc_sm_client_free(pInfo->domain_name.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->full_domain_name.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->forest.Buffer, &rpcStatus);
    rpc_sm_client_free(pInfo->sid, &rpcStatus);

    for (i = 0;
         i < sizeof(pInfo->unknown1)/sizeof(pInfo->unknown1[0]);
         i++)
    {
        rpc_sm_client_free(pInfo->unknown1[i].Buffer, &rpcStatus);
    }
}


static
VOID
NetrFreeDomainInfo1(
    NetrDomainInfo1 *pInfo
    )
{
    unsigned32 rpcStatus = 0;
    UINT32 i = 0;

    if (pInfo == NULL) return;

    NetrCleanDomainTrustInfo(&pInfo->domain_info);

    for (i = 0; i < pInfo->num_trusts; i++) {
        NetrCleanDomainTrustInfo(&pInfo->trusts[i]);
    }

    rpc_sm_client_free(pInfo->trusts, &rpcStatus);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


VOID
NetrCleanStubDomainInfo(
    NetrDomainInfo *pInfo,
    UINT16 Level
    )
{
    if (pInfo == NULL) return;

    switch (Level) {
    case 1:
        NetrFreeDomainInfo1(pInfo->info1);
        break;
    case 2:
        NetrFreeDomainInfo1(pInfo->info2);
        break;
    }
}


VOID
NetrCleanStubDcNameInfo(
    DsrDcNameInfo *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    rpc_sm_client_free(pInfo->dc_name, &rpcStatus);
    rpc_sm_client_free(pInfo->dc_address, &rpcStatus);
    rpc_sm_client_free(pInfo->domain_name, &rpcStatus);
    rpc_sm_client_free(pInfo->forest_name, &rpcStatus);
    rpc_sm_client_free(pInfo->dc_site_name, &rpcStatus);
    rpc_sm_client_free(pInfo->cli_site_name, &rpcStatus);
}


VOID
NetrFreeStubDcNameInfo(
    DsrDcNameInfo *pInfo
    )
{
    unsigned32 rpcStatus = 0;

    if (pInfo == NULL) return;

    NetrCleanStubDcNameInfo(pInfo);
    rpc_sm_client_free(pInfo, &rpcStatus);
}
