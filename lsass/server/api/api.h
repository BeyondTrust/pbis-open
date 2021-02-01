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
 *        api.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        LSA Server API (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"

#include "lsasystem.h"
#include <lsa/lsa.h>
#include <lwmsg/lwmsg.h>

#include <dce/rpc.h>
#include <dce/dcethread.h>
#include <lw/base.h>
#include <reg/reg.h>

#ifdef HAVE_EVENTLOG_H
#include <eventlog.h>
#endif

#include "lsadef.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"
#include "lsaregdef.h"
#include "lwmapsecurity-lsass.h"
#include <lwhash.h>

#include "lsasrvutils.h"
#include "lsaserver.h"
#include <lsa/provider.h>
#include "lsarpcsrv.h"
#include "rpcctl.h"
#include "directory.h"

#include "structs_p.h"
#include "auth_p.h"
#include "auth_provider_p.h"
#include "rpc_server_p.h"
#include "event_p.h"
#include "externs_p.h"
#include "session_p.h"
#include "state_p.h"
#include "metrics_p.h"
#include "status_p.h"
#include "config_p.h"

#include "lsasrvapi.h"
#include "lsasrvapi2.h"
#include "lsasrvprivilege.h"
#include "lsasrvprivilege-ipc.h"

#include "lsaipc-common.h"
#include "lsaipc.h"

#include "ipc_error_p.h"
#include "externs_p.h"

#include <lsarpcsrv.h>

#include "rpcctl-register.h"
#include "externs.h"

VOID
LsaSrvInitializeLock(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvAcquireRead(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvAcquireWrite(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvReleaseRead(
     PLSA_SRV_RWLOCK pLock
     );

VOID
LsaSrvReleaseWrite(
     PLSA_SRV_RWLOCK pLock
     );

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
