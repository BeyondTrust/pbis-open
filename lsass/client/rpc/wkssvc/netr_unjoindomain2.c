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
 *        netr_unjoindomain2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetrUnjoinDomain2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
NetrUnjoinDomain2(
    IN  WKSS_BINDING               hBinding,
    IN  PWSTR                      pwszServerName,
    IN  PWSTR                      pwszAccountName,
    IN  PENC_JOIN_PASSWORD_BUFFER  pPassword,
    IN  DWORD                      dwUnjoinFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccountName, ntStatus);
    BAIL_ON_INVALID_PTR(pPassword, ntStatus);

    DCERPC_CALL(dwError, cli_NetrUnjoinDomain2(
                              (handle_t)hBinding,
                              pwszServerName,
                              pwszAccountName,
                              pPassword,
                              dwUnjoinFlags));
    BAIL_ON_WIN_ERROR(dwError);

error:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;
}
