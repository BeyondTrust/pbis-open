#ifndef __GPASYSHDR_H__
#define __GPASYSHDR_H__

#include <ctbase.h>
//#include <uuid/uuid.h>
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
#include <dlfcn.h>

#endif /* __GPASYSHDR_H__ */
