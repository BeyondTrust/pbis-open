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
#ifdef UNICODE
      #undef UNICODE
#endif

   #include <stdio.h>
   #include <stdlib.h>
   #include <fcntl.h>
   #include <time.h>
   #include <string.h>
   #include <errno.h>    

#ifdef _WIN32

   #include <windows.h>
   #include <rpc.h>
   #define SECURITY_WIN32
   #include <security.h>
   #include <ntsecapi.h>

#else

  #include <stdarg.h>
  #include <netdb.h>
  #include <ctype.h>
  #include <sys/types.h>
  #include <pthread.h>
  #include <syslog.h>
  #include <signal.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #include <dirent.h>
  #include <lw/winerror.h>
  #include <compat/dcerpc.h>

#endif


