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
 *        lsmbsys.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) System Headers
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
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

#ifdef HAVE_STDBOOL_H
   #include <stdbool.h>
#endif

#ifdef HAVE_NETINET_IN6_H
/* HP-UX hack for broken system header*/
#if defined(__hpux) && defined(__hppa) && defined(_XOPEN_SOURCE_EXTENDED)
#undef _XOPEN_SOURCE_EXTENDED
#ifdef _NETINET_IN6_H
#error included netinet/in6.h too late
#endif
#include <netinet/in6.h>
#define _XOPEN_SOURCE_EXTENDED 1
#else
#include <netinet/in6.h>
#endif
#endif
#include <netinet/in.h>

#include <stdarg.h>
/* pthread.h must be included before errno so that a thread-safe errno is
   used if available.
 */
#include <pthread.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <wctype.h>
#include <sys/types.h>
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
#include <arpa/inet.h>
#include <locale.h>

#include <semaphore.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#if HAVE_WC16STR_H
#include <wc16str.h>
#endif

#if HAVE_WC16PRINTF_H
#include <wc16printf.h>
#endif

#ifdef HAVE_DLFCN_H
   #include <dlfcn.h>
#endif

#include <assert.h>

