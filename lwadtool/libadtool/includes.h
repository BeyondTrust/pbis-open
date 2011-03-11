/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        includes.h
 *
 * Abstract:
 *
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#ifndef _ADTOOL_INCLUDES_H_
#define _ADTOOL_INCLUDES_H_

#include "config.h"

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <uuid/uuid.h>

#include <krb5.h>

/* popt includes */
#include <popt.h>

/* lwbase includes */
#include <lw/types.h>
#include <lw/attrs.h>

/* lwadvpi includes */
#include <lwstr.h>
#include <lwmem.h>
#include <lwerror.h>
#include <lwldap.h>
#include <lwtime.h>
#include <lwsecurityidentifier.h>

/* adtool includes */
#include <adtool/types.h>

/* netapi includes */
#include "lw/lmaccess.h"
#include "lw/lmmem.h"
#include "lw/lmcreds.h"

/* lsa includes */
#include <lsa/lsa.h>

/* netlogon includes */
#include <lwnet.h>

/* local includes */
#include "defs.h"
#include "app.h"
#include "error.h"
#include "ldap_ops.h"
#include "cli.h"
#include "utils.h"
#include "ids.h"
#include "auth.h"
#include "net.h"

#endif /* _ADTOOL_INCLUDES_H_ */
