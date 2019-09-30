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
 *        net_userinfo.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        NetAPI user info buffer handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _NETUSERINFO_H_
#define _NETUSERINFO_H_


union user_info_id {
    DWORD  dwId;
    PSID   pSid;
};


DWORD
NetAllocateUserInfo(
    PVOID                 pInfoBuffer,
    PDWORD                pdwSpaceLeft, 
    DWORD                 dwLevel,
    PVOID                 pSource,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation
    );


DWORD
NetAllocateSamrUserInfo(
    PVOID                 pInfoBuffer,
    PDWORD                pdwSamrLevel,
    PDWORD                pdwSpaceLeft, 
    DWORD                 dwLevel,
    PVOID                 pSource,
    PNET_CONN             pConn,
    PDWORD                pdwSize,
    NET_VALIDATION_LEVEL  eValidation,
    PDWORD                pdwParmErr
    );


DWORD
NetEncryptPasswordBufferEx(
    PBYTE      pPasswordBuffer,
    DWORD      dwPasswordBufferSize,
    PWSTR      pwszPassword,
    DWORD      dwPasswordLen,
    PNET_CONN  pConn
    );


#endif /* _NETUSERINFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
