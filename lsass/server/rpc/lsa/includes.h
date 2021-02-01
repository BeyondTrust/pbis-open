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

#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <pthread.h>

#include <dce/rpc.h>
#include <dce/smb.h>
#include <dce/lrpc.h>
#include <dce/dcethread.h>
#include <wc16str.h>
#include <lw/base.h>
#include <lwsid.h>
#include <lwio/lwio.h>
#include <lw/rpc/samr.h>
#include <lw/rpc/lsa.h>
#include <lw/rpc/netlogon.h>
#include <lwio/lwio.h>
#include <lwnet.h>
#include <lwkrb5.h>
#include <lwhash.h>

#include <lsa/lsa.h>
#include <lsaadprovider.h>
#include <lsarpcsrv.h>
#include <lsasrvutils.h>
#include <lsasrvapi2.h>
#include <lsasrvprivilege.h>
#include <rpcctl-register.h>
#include <directory.h>
#include <samr_srv.h>

#include "lsa_cfg.h"
#include "lsa_srv.h"
#include "lsadefs.h"
#include "structs.h"
#include "lsa_contexthandle.h"
#include "lsa_accesstoken.h"
#include "lsa_security.h"
#include "lsa_memory.h"
#include "lsa_accounts.h"
#include "lsa_domaincache.h"
#include "lsa.h"
#include "lsa_h.h"

#include "externs.h"


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
