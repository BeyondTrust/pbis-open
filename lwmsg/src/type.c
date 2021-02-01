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
