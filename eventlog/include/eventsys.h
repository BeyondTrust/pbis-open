/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog module - system headers
 *
 */

   #include <stdio.h>
   #include <stdlib.h>
   #include <fcntl.h>
   #include <time.h>
   #include <string.h>

#ifdef _WIN32

   #include <windows.h>
   #include <rpc.h>
   #define SECURITY_WIN32
   #include <security.h>
   #include <ntsecapi.h>

#else

  #include <stdarg.h>
  #include <errno.h>
  #include <netdb.h>
  #include <ctype.h>

#if HAVE_LIMITS_H
  #include <limits.h>
#endif

#if HAVE_SYS_LIMITS_H
  #include <sys/limits.h>
#endif

#if HAVE_SYS_SYSLIMITS_H
  #include <sys/syslimits.h>
#endif

  #include <sys/types.h>
  #include <pthread.h>
  #include <syslog.h>
  #include <signal.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #include <dirent.h>
  #include <pwd.h>
  #include <lwrpcrt/lwrpcrt.h>
  #include <lw/winerror.h>
  #include <dlfcn.h>
  #include <dce/rpcexc.h>
  #include <dce/lrpc.h>

#if HAVE_WC16STR_H
  #include <wc16str.h>
#endif

#endif
