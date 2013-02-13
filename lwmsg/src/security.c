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
 *        security.c
 *
 * Abstract:
 *
 *        Security token API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "security-private.h"
#include "util-private.h"

LWMsgStatus
lwmsg_security_token_new(
    LWMsgSecurityTokenClass* tclass,
    LWMsgSecurityToken** out_token
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSecurityToken* token = NULL;

    token = calloc(1, sizeof(*token) + tclass->private_size);

    BAIL_ON_ERROR(status = tclass->construct(token));

    token->tclass = tclass;

    *out_token = token;

done:

    return status;

error:

    if (token)
    {
        if (token->tclass)
        {
            token->tclass->destruct(token);
        }
        
        free(token);
    }

    goto done;
}

void*
lwmsg_security_token_get_private(
    LWMsgSecurityToken* token
    )
{
    return (void*) token->private_data;
}

const char*
lwmsg_security_token_get_type(
    LWMsgSecurityToken* token
    )
{
    return token->tclass->get_type(token);
}

LWMsgBool
lwmsg_security_token_equal(
    LWMsgSecurityToken* token,
    LWMsgSecurityToken* other
    )
{
    /* Two tokens are equal if and only if they have the same
       class and the class equal method returns true.  As
       an optimization, two tokens are considered equal
       automatically if they have the same address */
       
    if (token == other)
    {
        return LWMSG_TRUE;
    }
    else if (token->tclass != token->tclass)
    {
        return LWMSG_FALSE;
    }
    else
    {
        return token->tclass->equal(token, other);
    }
}

LWMsgBool
lwmsg_security_token_can_access(
    LWMsgSecurityToken* token,
    LWMsgSecurityToken* other
    )
{
    return token->tclass->can_access(token, other);
}

LWMsgStatus
lwmsg_security_token_copy(
    LWMsgSecurityToken* token,
    LWMsgSecurityToken** out_token
    )
{
    return token->tclass->copy(token, out_token);
}


LWMsgStatus
lwmsg_security_token_to_string(
    LWMsgSecurityToken* token,
    LWMsgBuffer* buffer
    )
{
    return token->tclass->to_string(token, buffer);
}

void
lwmsg_security_token_delete(
    LWMsgSecurityToken* token
    )
{
    token->tclass->destruct(token);

    free(token);
}
