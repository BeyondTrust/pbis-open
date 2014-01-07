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
 *        wkss_stubmemory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        WksSvc DCE/RPC stub memory cleanup functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


VOID
WkssCleanStubNetrWkstaInfo(
    PNETR_WKSTA_INFO pInfo,
    DWORD            dwLevel
    )
{
    unsigned32 rpcStatus = 0;

    switch (dwLevel)
    {
    case 102:
    case 101:
        if (pInfo->pInfo102)
        {
            rpc_sm_client_free(pInfo->pInfo102->wksta102_lan_root, &rpcStatus);
        }
        /* missing break is intentional (see NETR_WKSTA_INFO in wksssvc.h) */

    case 100:
        if (pInfo->pInfo100)
        {
            rpc_sm_client_free(pInfo->pInfo100->wksta100_name, &rpcStatus);
            rpc_sm_client_free(pInfo->pInfo100->wksta100_domain, &rpcStatus);
        }
        break;
    }
}


VOID
WkssCleanStubNetrWkstaUserInfo(
    PNETR_WKSTA_USER_INFO pInfo
    )
{
    unsigned32 rpcStatus = 0;
    DWORD i = 0;

    switch (pInfo->dwLevel)
    {
    case 0:
        for (i = 0; i < pInfo->Ctr.pInfo0->dwCount; i++)
        {
            if (pInfo->Ctr.pInfo0->pInfo[i].wkui0_username)
            {
                rpc_sm_client_free(pInfo->Ctr.pInfo0->pInfo[i].wkui0_username,
                                   &rpcStatus);
            }
        }
        break;

    case 1:
        for (i = 0; i < pInfo->Ctr.pInfo1->dwCount; i++)
        {
            if (pInfo->Ctr.pInfo1->pInfo[i].wkui1_username)
            {
                rpc_sm_client_free(pInfo->Ctr.pInfo1->pInfo[i].wkui1_username,
                                   &rpcStatus);
            }

            if (pInfo->Ctr.pInfo1->pInfo[i].wkui1_logon_domain)
            {
                rpc_sm_client_free(pInfo->Ctr.pInfo1->pInfo[i].wkui1_logon_domain,
                                   &rpcStatus);
            }

            if (pInfo->Ctr.pInfo1->pInfo[i].wkui1_oth_domains)
            {
                rpc_sm_client_free(pInfo->Ctr.pInfo1->pInfo[i].wkui1_oth_domains,
                                   &rpcStatus);
            }

            if (pInfo->Ctr.pInfo1->pInfo[i].wkui1_logon_server)
            {
                rpc_sm_client_free(pInfo->Ctr.pInfo1->pInfo[i].wkui1_logon_server,
                                   &rpcStatus);
            }
        }
        break;
    }
}
