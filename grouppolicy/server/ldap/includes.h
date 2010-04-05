

#include "config.h"
#include "gposystem.h"
#include "grouppolicy.h"
#include "gpodefines.h"

#include <uuid/uuid.h>

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

#include "cterr.h"

#include "lwerror.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwfile.h"

#include "gpoutils.h"
#include "gpauthsvc.h"
#include "gpldap.h"

#include "gpldapdef.h"
#include "gpadirectory.h"
#include "gpagss.h"
#include "gpaquery.h"



