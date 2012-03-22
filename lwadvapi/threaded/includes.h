/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include "config.h"
#include <stdarg.h>
#include <krb5.h>
#include <gssapi.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sasl/sasl.h>

#include <lw/rtlmemory.h>
#include <lw/security-types.h>
#include <lw/security-api.h>
#include <lw/attrs.h>
#include <lw/atomic.h>
#include <wc16str.h>

#include "lwdef.h"
#include "lwmem.h"
#include "lwfile.h"
#include "lwstr.h"
#include "lwldap.h"
#include "lwerror.h"
#include "krb5error-table.h"
#include "lwkrb5.h"
#include "bail.h"
#include "externs.h"

#include "lwldap_p.h"
#include "lwkrb5_p.h"
#include "lwtime.h"
#include "lwsecurityidentifier.h"

#if !defined(HAVE_STRTOLL) && defined(HAVE___STRTOLL)
#define strtoll __strtoll
#endif

#if !defined(HAVE_STRTOULL) && defined(HAVE___STRTOULL)
#define strtoull __strtoull
#endif

#endif /* __INCLUDES_H__ */
