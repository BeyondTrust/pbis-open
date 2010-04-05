/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        includes.h
 *
 * Abstract:
 *
 *       Mac Workgroup Manager
 *
 *       AD Utility API (Private Header)
 *
 *       Common sub header
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "config.h"
#include "macadsys.h"

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

#include "lwio/lwio.h"
#include "lwio/ntfileapi.h"
#include "lwnet.h"
#include "lsa/lsa.h"
#include "grouppolicy.h"

#include "macadutil.h"
#include "lwutils.h"

#include "defs.h"
#include "structs.h"
#include "api.h"
#include "mcxutil.h"
#include "directory.h"
#include "policyutils.h"
#include "gss.h"
#include "adukrb5.h"
#include "adinfo.h"
#include "gpcache.h"
#include "xfer.h"
#include "filexfer.h"
#include "credcontext.h"
#include "aducopy.h"

#include "externs.h"

