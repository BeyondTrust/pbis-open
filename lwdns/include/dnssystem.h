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
 *        dnssystem.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */




#ifdef UNICODE
      #undef UNICODE
#endif


#ifdef WIN32

   #include <windows.h>
   #include <rpc.h>
   #define SECURITY_WIN32
   #include <security.h>
   #include <ntsecapi.h>
#endif

   #include <stdio.h>

#ifdef HAVE_STDLIB_H
   #include <stdlib.h>
#endif

#ifdef HAVE_SYS_VARARGS_H
   #include <sys/varargs.h>
#endif

   #include <fcntl.h>

#ifdef HAVE_TIME_H
   #include <time.h>
#endif

#ifdef HAVE_SYS_TIME_H
   #include <sys/time.h>
#endif

   #include <string.h>

#ifdef HAVE_STRINGS_H
   #include <strings.h>
#endif

#ifndef WIN32
  /* pthread.h must be included before errno so that a thread-safe errno is
     used if available.
   */
#ifdef HAVE_PTHREAD_H
  #include <pthread.h>
#endif

  #include <errno.h>
  #include <ctype.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>

#ifdef HAVE_NETDB_H
  #include <netdb.h>
#endif

#ifdef HAVE_ARPA_INET_H
  #include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_IN_H
  #include <netinet/in.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
  #include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
  #include <sys/sockio.h>
#endif

#ifdef HAVE_NET_IF_H

#if defined(__ia64)

//
// Fix to build on HP-UX IA64
// /usr/include/machine/sys/getppdp.h gets included as part of
// including net/if.h; however, getppdp.h has the following definition
// which will not build when using gcc 4.x
// extern union mpinfou spu_info[];
// So, the following define is intended to keep from including this header
//
#define _MACHINE_SYS_GETPPDP_INCLUDED

#endif

#include <net/if.h>

#endif

#ifdef HAVE_NET_IF_DL_H

#include <net/if_dl.h>

#endif

#ifdef HAVE_IFADDRS_H
  #include <ifaddrs.h>
#endif

#ifdef HAVE_ARPA_NAMESER_H
  #include <arpa/nameser.h>
#endif

#ifdef HAVE_RESOLV_H
  #include <resolv.h>
#endif

#if HAVE_WC16STR_H
#include <wc16str.h>
#endif

  #include <uuid/uuid.h>
  #include <krb5.h>
  #include <gssapi/gssapi.h>
  #include <gssapi/gssapi_generic.h>
  #include <gssapi/gssapi_krb5.h>

#endif
