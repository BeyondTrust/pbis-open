
#include "config.h"

// System headers
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <string.h>
#include <errno.h>	
#include <netdb.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <uuid/uuid.h>
#include <dirent.h>
#include <dlfcn.h>

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_TERMIO_H
#include <sys/termio.h>
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <DirectoryService/DirServicesTypes.h>
#include <DirectoryService/DirServices.h>
#include <DirectoryService/DirServicesUtils.h>
#include <DirectoryService/DirServicesConst.h>

// Likewise headers
//#include <lw/types.h>
#include <lw/attrs.h>

#ifndef KRB5_PRIVATE
#define KRB5_PRIVATE 1
#ifndef KRB5_DEPRECATED
#define KRB5_DEPRECATED 1
#include <krb5.h>
#endif
#endif
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#ifndef LDAP_DEPRECATED
#define LDAP_DEPRECATED 1
#include <ldap.h>
#endif

#include <lwnet.h>
#include "lwio/lwio.h"
#include "lwio/ntfileapi.h"
#include <lw/winerror.h>
#include <lsa/lsa.h>
#include <lwmem.h>
#include <lwfile.h>
#include <lwstr.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include "LWIStruct.h"
#include "PlugInShell.h"
#include "Utilities.h"

