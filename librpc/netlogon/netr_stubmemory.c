/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


VOID
NetrCleanStubDomainTrustList(
    NetrDomainTrustList *pTrustList
    )
{
    RPCSTATUS rpcStatus = 0;
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
    RPCSTATUS rpcStatus = 0;

    rpc_sm_client_free(pInfo->account_name.string, &rpcStatus);
    rpc_sm_client_free(pInfo->full_name.string, &rpcStatus);
    rpc_sm_client_free(pInfo->logon_script.string, &rpcStatus);
    rpc_sm_client_free(pInfo->profile_path.string, &rpcStatus);
    rpc_sm_client_free(pInfo->home_directory.string, &rpcStatus);
    rpc_sm_client_free(pInfo->home_drive.string, &rpcStatus);

    pInfo->groups.count = 0;
    rpc_sm_client_free(pInfo->groups.rids, &rpcStatus);

    rpc_sm_client_free(pInfo->logon_server.string, &rpcStatus);
    rpc_sm_client_free(pInfo->domain.string, &rpcStatus);

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
    RPCSTATUS rpcStatus = 0;

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
    RPCSTATUS rpcStatus = 0;
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
    RPCSTATUS rpcStatus = 0;

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
    RPCSTATUS rpcStatus = 0;

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
    RPCSTATUS rpcStatus = 0;

    NetrCleanSamBaseInfo(&pInfo->base);

    if (pInfo->sids) {
        NetrFreeSidAttr(pInfo->sids,
                        pInfo->sidcount);
    }

    rpc_sm_client_free(pInfo->forest.string, &rpcStatus);
    rpc_sm_client_free(pInfo->principal.string, &rpcStatus);
}


static
VOID
NetrFreeSamInfo6(
    NetrSamInfo6 *pInfo
    )
{
    RPCSTATUS rpcStatus = 0;

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
    RPCSTATUS rpcStatus = 0;

    rpc_sm_client_free(pInfo->pac, &rpcStatus);
    rpc_sm_client_free(pInfo->auth, &rpcStatus);

    rpc_sm_client_free(pInfo->logon_domain.string, &rpcStatus);
    rpc_sm_client_free(pInfo->logon_server.string, &rpcStatus);
    rpc_sm_client_free(pInfo->principal_name.string, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown1.string, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown2.string, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown3.string, &rpcStatus);
    rpc_sm_client_free(pInfo->unknown4.string, &rpcStatus);
}


static
VOID
NetrFreePacInfo(
    NetrPacInfo *pInfo
    )
{
    RPCSTATUS rpcStatus = 0;

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
    RPCSTATUS rpcStatus = 0;
    UINT32 i = 0;

    if (pInfo == NULL) return;

    rpc_sm_client_free(pInfo->domain_name.string, &rpcStatus);
    rpc_sm_client_free(pInfo->full_domain_name.string, &rpcStatus);
    rpc_sm_client_free(pInfo->forest.string, &rpcStatus);
    rpc_sm_client_free(pInfo->sid, &rpcStatus);

    for (i = 0;
         i < sizeof(pInfo->unknown1)/sizeof(pInfo->unknown1[0]);
         i++)
    {
        rpc_sm_client_free(pInfo->unknown1[i].string, &rpcStatus);
    }
}


static
VOID
NetrFreeDomainInfo1(
    NetrDomainInfo1 *pInfo
    )
{
    RPCSTATUS rpcStatus = 0;
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
    RPCSTATUS rpcStatus = 0;

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
    RPCSTATUS rpcStatus = 0;

    if (pInfo == NULL) return;

    NetrCleanStubDcNameInfo(pInfo);
    rpc_sm_client_free(pInfo, &rpcStatus);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
