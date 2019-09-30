/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samr_connect5.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrConnect5 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NTSTATUS
SamrSrvConnect5(
    /* [in] */ handle_t             hBinding,
    /* [in] */ PCWSTR               pwszSystemName,
    /* [in] */ DWORD                dwAccessMask,
    /* [in] */ DWORD                dwLevelIn,
    /* [in] */ PSAMR_CONNECT_INFO   pInfoIn,
    /* [out] */ PDWORD              pdwLevelOut,
    /* [out] */ PSAMR_CONNECT_INFO  pInfoOut,
    /* [out] */ CONNECT_HANDLE     *hConn
    )
{
    const DWORD dwConnectVersion = 5;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PCONNECT_CONTEXT pConnCtx = NULL;
    SAMR_CONNECT_INFO Info;
    DWORD dwLevel = 0;

    ntStatus = SamrSrvConnectInternal(hBinding,
                                      pwszSystemName,
                                      dwAccessMask,
                                      dwConnectVersion,
                                      dwLevelIn,
                                      pInfoIn,
                                      &dwLevel,
                                      &Info,
                                      &pConnCtx);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *pdwLevelOut = dwLevel;
    *pInfoOut    = Info;
    *hConn       = (CONNECT_HANDLE)pConnCtx;

cleanup:
    return ntStatus;

error:
    if (pConnCtx)
    {
        SamrSrvConnectContextFree(pConnCtx);
    }

    *pdwLevelOut = 1;
    memset(pInfoOut, 0, sizeof(*pInfoOut));

    *hConn = NULL;
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
