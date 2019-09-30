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
 * Abstract: Samr interface (rpc server library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <pthread.h>
#include <wchar.h>

#include <dce/rpc.h>
#include <dce/smb.h>
#include <dce/lrpc.h>
#include <dce/dcethread.h>
#include <wc16str.h>
#include <wc16printf.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/rc4.h>
#include <openssl/des.h>
#include <lw/base.h>
#include <lwsid.h>
#include <lwtime.h>
#include <lwio/lwio.h>
#include <lwnet.h>
#include <lwkrb5.h>
#include <lw/rpc/lsa.h>
#include <lw/rpc/samr.h>

#include <lsa/lsa.h>
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include <lsautils.h>
#include <lsarpcsrv.h>
#include <lsasrvutils.h>
#include <lsautils.h>
#include <rpcctl-register.h>
#include <directory.h>
#include <lsaadprovider.h>
#include <lsasrvapi.h>

#include "samr_cfg.h"
#include "samr_srv.h"
#include "samrdefs.h"
#include "samr_contexthandle.h"
#include "samr_accesstoken.h"
#include "samr_security.h"
#include "samr_crypto.h"
#include "samr_memory.h"
#include "samr.h"
#include "samr_h.h"

#include "externs.h"


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
