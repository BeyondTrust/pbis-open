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
