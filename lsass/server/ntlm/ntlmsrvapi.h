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
 *        ntlmsrvapi.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
