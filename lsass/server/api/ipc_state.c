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
 *        ipc_state.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authorization Subsystem (LSASS)
 *
 *        Server Connection State API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

static
DWORD
LsaSrvIpcCheckPermissions(
    LWMsgSecurityToken* token,
    uid_t* puid,
    gid_t* pgid,
    pid_t* ppid
    )
{
    DWORD dwError = 0;
    uid_t euid = (uid_t)-1;
    gid_t egid = (gid_t)-1;
    pid_t cpid = (pid_t)-1;

    if (strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        LSA_LOG_WARNING("Unsupported authentication type");
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(token, &euid, &egid));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_pid(token, &cpid));
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_LOG_VERBOSE("Permission granted for (uid = %i, gid = %i, pid = %i) to open LsaIpcServer",
                    (int) euid,
                    (int) egid,
                    (int) cpid);
    
error:
    *puid = euid;
    *pgid = egid;
    *ppid = cpid;

    return dwError;
}

void
LsaSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    )
{
    LsaSrvCloseServer(pSessionData);
}

LWMsgStatus
LsaSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    )
{
    DWORD dwError = 0;
    HANDLE Handle = (HANDLE)NULL;
    uid_t UID;
    gid_t GID;
    pid_t PID;

    dwError = LsaSrvIpcCheckPermissions(pToken, &UID, &GID, &PID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvOpenServer(UID, GID, PID, &Handle);
    BAIL_ON_LSA_ERROR(dwError);

    *ppSessionData = Handle;

cleanup:

    return MAP_LW_ERROR_IPC(dwError);

error:

    goto cleanup;
}
