/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        evtfwd-system.h
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *
 *        System Headers
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
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
   #include <stdlib.h>
   #include <fcntl.h>
   #include <sys/time.h>
   #include <time.h>
   #include <string.h>

#ifdef HAVE_STDBOOL_H
   #include <stdbool.h>
#endif

#ifndef WIN32
  #include <stdarg.h>
  #include <errno.h>	
  #include <netdb.h>
#ifdef HAVE_INTTYPES_H
  #include <inttypes.h>
#endif
  #include <ctype.h>
  #include <wctype.h>
  #include <sys/types.h>
  #include <pthread.h>
  #include <syslog.h>
  #include <signal.h>
  #include <limits.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #include <dirent.h>
  #include <pwd.h>
  #include <grp.h>
  #include <regex.h>
  #include <sys/un.h>
  #include <dlfcn.h>
  #include <arpa/inet.h>
  #include <arpa/nameser.h>
  #include <netinet/in.h>
  #include <resolv.h>

#ifdef HAVE_SOCKET_H
  #include <socket.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
  #include <sys/socket.h>
#endif

#endif

#if HAVE_WC16STR_H
#include <wc16str.h>
#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
