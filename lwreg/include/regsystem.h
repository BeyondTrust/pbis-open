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
 *        regsystem.h
 *
 * Abstract:
 *
 *        Registry System Headers
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
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

#if HAVE_WC16STR_H
#include <wc16str.h>
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

#endif
