/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        state.c
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 * 
 *        Server State Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

void
LWNetSrvCloseServer(
    HANDLE hServer
    )
{
    PLWNET_SRV_API_STATE pServerState = (PLWNET_SRV_API_STATE)hServer;

    if (pServerState->hEventLog != (HANDLE)NULL)
    {
       LWNetSrvCloseEventLog(pServerState->hEventLog);
    }
    
    LWNetFreeMemory(pServerState);
}

DWORD
LWNetSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    )
{
    DWORD dwError = 0;
    PLWNET_SRV_API_STATE pServerState = NULL;
    
    dwError = LWNetAllocateMemory(
                    sizeof(LWNET_SRV_API_STATE),
                    (PVOID*)&pServerState);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pServerState->peerUID = peerUID;
    pServerState->peerGID = peerGID;
    
    *phServer = (HANDLE)pServerState;
    
cleanup:

    return dwError;
    
error:

    *phServer = (HANDLE)NULL;
    
    if (pServerState) {
        LWNetSrvCloseServer((HANDLE)pServerState);
    }
    
    goto cleanup;
}

