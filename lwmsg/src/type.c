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
 *        type.c
 *
 * Abstract:
 *
 *        Type specification API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "type-private.h"
#include "util-private.h"

LWMsgTypeFlags
lwmsg_type_get_flags(
    const LWMsgType* attrs
    )
{
    return attrs->flags;
}

size_t
lwmsg_type_get_custom_flags(
    const LWMsgType* attrs
    )
{
    return attrs->custom;
}

LWMsgStatus
lwmsg_type_get_integer_range(
    const LWMsgType* attrs,
    size_t* low,
    size_t* high
    )
{
    if (attrs->flags & LWMSG_TYPE_FLAG_RANGE)
    {
        *low = attrs->range_low;
        *high = attrs->range_high;
        return LWMSG_STATUS_SUCCESS;
    }
    else
    {
        return LWMSG_STATUS_INVALID_PARAMETER;
    }
}
