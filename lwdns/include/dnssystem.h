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
 *        dnssystem.h
 *
 * Abstract:
 *
 *        BeyondTrust Dynamic DNS Updates (LWDNS)
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
