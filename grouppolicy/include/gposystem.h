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

#ifdef HAVE_SYS_VARARGS_H
   #include <sys/varargs.h>
#endif

   #include <fcntl.h>
   #include <time.h>
   #include <string.h>

#ifdef HAVE_STRINGS_H
   #include <strings.h>
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
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <arpa/nameser.h>
  #include <resolv.h>
  #include <syslog.h>
  #include <signal.h>
  #include <unistd.h>
  #include <sys/un.h>
  #include <sys/stat.h>
  #include <pwd.h>
  #include <grp.h>
  #include <dlfcn.h>
  #include <libgen.h>

#if defined(__LWI_DARWIN_X64__)
// Handle Max OS system 10.6 here
#include <utmpx.h>
#else
// Handle all other OS types with below checks, including legacy Mac OS systems < 10.6
#if defined(HAVE_UTMPX_H)
#include <utmpx.h>
#endif

#if defined(HAVE_UTMP_H)
#include <utmp.h>
#endif
#endif // end else case for LWI_DARWIN_x64

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if defined(__hpux__) && defined(_XOPEN_SOURCE_EXTENDED)
#    include "xpg_socket.h"
#endif

#if HAVE_WC16STR_H
#include <wc16str.h>
#endif

#if HAVE_PATHS_H
#include <paths.h>
#endif

#endif

