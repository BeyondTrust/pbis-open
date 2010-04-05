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
   #include <time.h>
   #include <string.h>

#ifndef WIN32
  #include <errno.h>	
  #include <netdb.h>
  #include <ctype.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <arpa/nameser.h>
//  #include <resolv.h>
  #include <pthread.h>
  #include <syslog.h>
  #include <signal.h>
  #include <unistd.h>
  #include <sys/un.h>
  #include <sys/stat.h>
  #include <pwd.h>
  #include <grp.h>
//#if !defined(__LWI_DARWIN__)
//  #include <utmpx.h>
//#else
//  #include <utmp.h>
//#endif
#endif
