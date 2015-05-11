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
 *        lsasystem.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) System Headers
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifdef UNICODE
      #undef UNICODE
#endif

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
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

#ifdef HAVE_STDBOOL_H
   #include <stdbool.h>
#endif

#ifndef WIN32
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

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

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

#ifdef HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#if HAVE_WC16STR_H
#include <wc16str.h>
#endif
#if HAVE_WC16PRINTF_H
#include <wc16printf.h>
#endif

#if defined(__hpux__) && defined(_XOPEN_SOURCE_EXTENDED)
#    include "xpg_socket.h"
#endif

#include <assert.h>

#if HAVE_SYS_PSTAT_H
#include <sys/pstat.h>
#endif

#if HAVE_PROCFS_H
#include <procfs.h>
#elif HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif

#if HAVE_KVM_H
#include <kvm.h>
#endif

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#if HAVE_SYS_USER_H
#include <sys/user.h>
#endif

#if HAVE_LIBGEN_H
#include <libgen.h>
#endif

#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#if HAVE_SYS_REGSET_H
#include <sys/regset.h>
#endif

#include <sys/resource.h>

#endif
