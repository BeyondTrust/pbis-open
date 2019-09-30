/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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

#ifndef _LIB_ADTOOL_INCLUDES_H_
#define _LIB_ADTOOL_INCLUDES_H_

#include "config.h"

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <uuid/uuid.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <krb5.h>

/* popt includes */
#include <popt.h>

/* lwbase includes */
#include <lw/types.h>
#include <lw/attrs.h>

/* lwadvpi includes */
#include <lwstr.h>
#include <lwfile.h>
#include <lwmem.h>
#include <lwerror.h>
#include <lwldap.h>
#include <lwtime.h>
#include <lwsecurityidentifier.h>
#include <lwkrb5.h>

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

#include <btkrb5.h>

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
#include "gen_keytab.h"

#endif /* _ADTOOL_INCLUDES_H_ */
