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
 * Module Name:
 *
 *        lwsm-includes.h
 *
 * Abstract:
 *
 *        Convenience include file (includes everything to build lwsm)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWSM_INCLUDES_H__
#define __LWSM_INCLUDES_H__

#include "config.h"

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <syslog.h>
#include <locale.h>
#include <lw/winerror.h>
#include <lw/svcm.h>
#include <lwerror.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwdscache.h>
#include <reg/lwreg.h>
#include <reg/regutil.h>
#include <lwmsg/lwmsg.h>
#include <lwio/lwio.h>
#include <lwnet.h>
#include <wc16printf.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#ifdef HAVE_STRINGS_H
   #include <strings.h>
#endif

#include "server.h"
#include "lwptimer.h"
#include "lwsmsyslog.h"

#endif
