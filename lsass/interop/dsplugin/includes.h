
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
#include <lw/types.h>
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
#include <reg/lwreg.h>
#include "lwio/lwio.h"
#include "lwio/ntfileapi.h"
#include <lw/winerror.h>
#include <lsa/lsa.h>
#include <lwldap.h>
#include <lwmem.h>
#include <lwfile.h>
#include <lwstr.h>
#include <lsaauth.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "macadutil/defs.h"
#include "macadutil/structs.h"
#include "macadutil/api.h"
#include "macadutil/adinfo.h"
#include "macadutil/aduserinfo.h"
#include "macadutil/adukrb5.h"
#include "macadutil/aducopy.h"
#include "macadutil/cfgparser.h"
#include "macadutil/credcontext.h"
#include "macadutil/directory.h"
#include "macadutil/filexfer.h"
#include "macadutil/gss.h"
#include "macadutil/gpcache.h"
#include "macadutil/macerror.h"
#include "macadutil/mcxutil.h"
#include "macadutil/notify.h"
#include "macadutil/netinfo.h"
#include "macadutil/policyutils.h"
#include "macadutil/xfer.h"

#include "libgpldap/gpodefines.h"
#include "libgpldap/gpadefines.h"
#include "libgpldap/gpadnshlp.h"
#include "libgpldap/gpapolicyutils.h"

#ifdef __cplusplus
}
#endif

#include "LWIStruct.h"
#include "PlugInShell.h"
#include "Utilities.h"

