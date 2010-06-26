/*++
	Linux DNS client library implementation
	Copyright (C) 2006 Krishna Ganugapati

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

++*/

#include "config.h"

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

#include "dnsstruct.h"
#include "dnsstrerror.h"
#include "dnserror.h"
#include "dnsutils.h"
#include "dnsdlinkedlist.h"
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
