/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2010
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
 *        wkssvc.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Wkssvc rpc server functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _WKSSVC_H_
#define _WKSSVC_H_


WINERROR
NetrSrvWkstaGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ DWORD dwLevel,
    /* [out] */ PNETR_WKSTA_INFO pInfo
    );


WINERROR
NetrSrvWkstaUserEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in, out] */ NETR_WKSTA_USER_INFO *pInfo,
    /* [in] */ DWORD dwPrefMaxLen,
    /* [out] */ DWORD *pdwNumEntries,
    /* [in, out] */ DWORD *pdwResume
    );


WINERROR
NetrSrvJoinDomain2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ wchar16_t *pwszDomainName,
    /* [in] */ wchar16_t *pwszAccountOu,
    /* [in] */ wchar16_t *pwszAccountName,
    /* [in] */ ENC_JOIN_PASSWORD_BUFFER *pPassword,
    /* [in] */ DWORD dwJoinFlags
    );


WINERROR
NetrSrvUnjoinDomain2(
    /* [in] */ handle_t                  hBinding,
    /* [in] */ PWSTR                     pwszServerName,
    /* [in] */ PWSTR                     pwszAccountName,
    /* [in] */ PENC_JOIN_PASSWORD_BUFFER pPassword,
    /* [in] */ DWORD                     dwUnjoinFlags
    );


#endif /* _WKSSVC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
