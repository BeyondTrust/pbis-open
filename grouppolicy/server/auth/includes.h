/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        includes.h
 *
 * Abstract:
 *
 *        Likewise Group Policy
 *
 *        LSASS Authentication Service Interface
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "config.h"
#include "gposystem.h"
#include "grouppolicy.h"
#include "gpodefines.h"

#ifndef KRB5_PRIVATE
#define KRB5_PRIVATE 1
#ifndef KRB5_DEPRECATED
#define KRB5_DEPRECATED 1
#include <krb5.h>
#endif
#endif

#include "lsa/lsa.h"
#include "lwnet.h"

#include "cterr.h"
#include "gpaprocutils.h"

#include "lwerror.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwfile.h"

#include "gpoutils.h"
#include "gpauthsvc.h"

#include "authsvc.h"
#include "externs.h"
