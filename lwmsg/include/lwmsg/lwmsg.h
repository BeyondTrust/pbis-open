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
