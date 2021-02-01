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
 *        lwmsg.h
 *
 * Abstract:
 *
 *        Primary public header file
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_H__
#define __LWMSG_H__

#define __LWMSG_INCLUDE_ENABLE__
#ifdef LW_DISABLE_DEPRECATED
#ifndef LWMSG_DISABLE_DEPRECATED
#define LWMSG_DISABLE_DEPRECATED
#endif
#endif

#include <lwmsg/data.h>
#include <lwmsg/buffer.h>
#include <lwmsg/context.h>
#include <lwmsg/protocol.h>
#include <lwmsg/status.h>
#include <lwmsg/type.h>
#include <lwmsg/assoc.h>
#include <lwmsg/connection.h>
#include <lwmsg/time.h>
#include <lwmsg/security.h>
#include <lwmsg/common.h>
#include <lwmsg/archive.h>

#ifndef LWMSG_NO_THREADS
#include <lwmsg/peer.h>
#endif

#undef __LWMSG_INCLUDE_ENABLE__

#endif
