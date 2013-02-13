/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
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

#include "config.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <iconv.h>
#include <pthread.h>

#include <lwio/lwio.h>

#include <dce/rpc.h>
#include <dce/smb.h>
#include <dce/dcethread.h>
#include <dce/schannel.h>
#include <wc16str.h>
#include <wc16printf.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/rc4.h>
#include <openssl/hmac.h>
#include <openssl/des.h>
#include <openssl/rand.h>
#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <lwmem.h>
#include <lwstr.h>
#include <lwbuffer.h>

#include <lsautils.h>
#include <lw/rpc/samr.h>
#include <lw/rpc/netlogon.h>
#include <lw/rpcmacros.h>
#include <lw/swab.h>

#include "netrdefs.h"
#include "netr_credentials.h"
#include "netr_memory.h"
#include "netr_stubmemory.h"
#include "netlogon_h.h"

#include "externs.h"
