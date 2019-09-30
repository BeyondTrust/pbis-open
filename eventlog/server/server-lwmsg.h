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
 * Copyright (C) BeyondTrust Corporation 2004-2007
 * Copyright (C) BeyondTrust Software 2007
 * All rights reserved.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 * Eventlog Server API
 *
 */
#ifndef __SERVER_LWMSG_H__
#define __SERVER_LWMSG_H__

typedef struct _LWMSG_LW_EVENTLOG_CONNECTION
{
    ULONG Uid;
    ULONG Gid;

    PACCESS_TOKEN pUserToken;
    BOOLEAN ReadAllowed;
    BOOLEAN WriteAllowed;
    BOOLEAN DeleteAllowed;
} LWMSG_LW_EVENTLOG_CONNECTION, *PLWMSG_LW_EVENTLOG_CONNECTION;

void
LwmEvtSrvDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    );

LWMsgStatus
LwmEvtSrvConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    );

DWORD
LwmEvtSrvGetRecordCount(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

DWORD
LwmEvtSrvReadRecords(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

DWORD
LwmEvtSrvWriteRecords(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

DWORD
LwmEvtSrvDeleteRecords(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    );

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

#endif /* __SERVER_LWMSG_H__ */
