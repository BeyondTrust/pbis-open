/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
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

#endif
