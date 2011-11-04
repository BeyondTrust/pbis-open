/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntlmsrvapi.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication API header (NTLM Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef __NTLMSRVAPI_H__
#define __NTLMSRVAPI_H__

#include <config.h>

#include <pthread.h>

#include <ntlm/sspintlm.h>
#include <ntlmipc.h>

#include <openssl/des.h>
#include <openssl/md5.h>
#include <openssl/md4.h>
#include <openssl/rc4.h>
#include <openssl/hmac.h>

#include <time.h>
#include <netdb.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <lsa/lsa.h>
#include <lsasrvcred.h>
#include <lsasrvapi2.h>
#include <lsalist.h>
#include <lwsecurityidentifier.h>
#include <lsautils.h>
#include <lsaadprovider.h>
#include <lwdef.h>
#include <lwkrb5.h>
#include <gssapi/gssapi_ext.h>
#include <lwmem.h>
#include <lwstr.h>
#include <wc16str.h>
#include <uuid/uuid.h>
#include <lwio/lwio.h>
#include <lw/rpc/samr.h>
#include <lw/rpc/netlogon.h>
#include <lw/rpc/krb5pac.h>
#include <lw/swab.h>
#include <lsasrvutils.h>

#include "defines.h"
#include "structs.h"
#include "externs.h"
#include "prototypes.h"

#endif // __NTLMSRVAPI_H__
