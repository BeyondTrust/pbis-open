/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsassd.h
 *
 * Abstract:
 *
 *        BeyondTrust SMB Subsystem
 *
 *        Service (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "config.h"
#include "lwiosys.h"

#include "lwio/lwio.h"

#include <reg/lwntreg.h>
#include <lwmsg/lwmsg.h>

#include <lwerror.h>
#include <lwdscache.h>
#include <lw/base.h>
#include <lwnet.h>

#include "lwiodef.h"
#include "lwioutils.h"
#include "lwioipc.h"

#include "defs.h"
#include "structs.h"
#include "cfg.h"
#include "servermain.h"
#include "ipc.h"
#include "ioinit.h"

#include "externs.h"

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#if HAVE_SYS_USER_H
#include <sys/user.h>
#endif

#include <sys/time.h>
#include <sys/resource.h>
