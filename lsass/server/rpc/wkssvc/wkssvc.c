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
 *        wkssvc.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Wkssvc rpc server stub functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


WINERROR
srv_NetrWkstaGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ DWORD dwLevel,
    /* [out] */ NETR_WKSTA_INFO *pInfo
    )
{
    WINERROR winError = ERROR_SUCCESS;

    winError = NetrSrvWkstaGetInfo(IDL_handle,
                                   pwszServerName,
                                   dwLevel,
                                   pInfo);
    return winError;
}


WINERROR
srv_wkssvc_Function0x01(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_NetrWkstaUserEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in, out] */ NETR_WKSTA_USER_INFO *pInfo,
    /* [in] */ DWORD dwPrefMaxLen,
    /* [out] */ DWORD *pdwNumEntries,
    /* [in, out] */ DWORD *pdwResume
    )
{
    WINERROR winError = ERROR_SUCCESS;

    winError = NetrSrvWkstaUserEnum(IDL_handle,
                                    pwszServerName,
                                    pInfo,
                                    dwPrefMaxLen,
                                    pdwNumEntries,
                                    pdwResume);
    return winError;
}


WINERROR
srv_wkssvc_Function0x03(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x04(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x05(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x06(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x07(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x08(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x09(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x0a(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x0b(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x0c(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x0d(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x0e(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x0f(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_NetrJoinDomain(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ wchar16_t *pwszDomainName,
    /* [in] */ wchar16_t *pwszAccountOu,
    /* [in] */ wchar16_t *pwszAccountName,
    /* [in] */ wchar16_t *pwszPassword,
    /* [in] */ DWORD dwJoinFlags
    )
{
    WINERROR winError = ERROR_NOT_SUPPORTED;
    return winError;
}


WINERROR
srv_NetrUnjoinDomain(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ wchar16_t *pwszAccountName,
    /* [in] */ wchar16_t *pwszPassword,
    /* [in] */ DWORD dwUnjoinFlags
    )
{
    WINERROR winError = ERROR_NOT_SUPPORTED;
    return winError;
}


WINERROR
srv_wkssvc_Function0x10(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x11(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x12(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x13(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x14(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x15(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_NetrJoinDomain2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ wchar16_t *pwszDomainName,
    /* [in] */ wchar16_t *pwszAccountOu,
    /* [in] */ wchar16_t *pwszAccountName,
    /* [in] */ ENC_JOIN_PASSWORD_BUFFER *pPassword,
    /* [in] */ DWORD dwJoinFlags
    )
{
    WINERROR winError = ERROR_SUCCESS;

    winError = NetrSrvJoinDomain2(IDL_handle,
                                  pwszServerName,
                                  pwszDomainName,
                                  pwszAccountOu,
                                  pwszAccountName,
                                  pPassword,
                                  dwJoinFlags);
    return winError;
}


WINERROR
srv_NetrUnjoinDomain2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ wchar16_t *pwszAccountName,
    /* [in] */ ENC_JOIN_PASSWORD_BUFFER *pPassword,
    /* [in] */ DWORD dwUnjoinFlags
    )
{
    WINERROR winError = ERROR_SUCCESS;

    winError = NetrSrvUnjoinDomain2(IDL_handle,
                                    pwszServerName,
                                    pwszAccountName,
                                    pPassword,
                                    dwUnjoinFlags);
    return winError;
}


WINERROR
srv_wkssvc_Function0x18(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x19(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x1a(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x1b(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x1c(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x1d(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


WINERROR
srv_wkssvc_Function0x1e(
    /* [in] */ handle_t IDL_handle
    )
{
    WINERROR winError = ERROR_SUCCESS;
    return winError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
