/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        includes.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define IS_ADDR_LINKLOCAL(a)    \
        ((a[0] == 'f') && (a[1] == 'e') && (a[2] == '8') && (a[3] == '0'))   

#if defined(__LWI_NETBSD__)
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif
#endif  // __LWI_NETBSD__

#include "dnssystem.h"
#include "lwdns.h"
#include "dnsdefines.h"
#include <lwdlinked-list.h>
#include <lwkrb5.h>
#include <lw/winerror.h>
#include <lwerror.h>

#include "dnsstruct.h"
#include "dnsstrerror.h"
#include "dnserror.h"
#include "dnsutils.h"
#include "dnsrecord.h"
#include "dnsresponse.h"
#include "dnsrequest.h"
#include "dnsuprequest.h"
#include "dnssock.h"
#include "dnsgss.h"
#include "dnsupdate.h"
#include "dnsupresp.h"
#include "dnsparser.h"
#include "lw/rtlstring.h"

#include "externs.h"

#define LW_UPDATE_DNS_TIMEOUT 30
