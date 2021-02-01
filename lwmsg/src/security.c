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
    else if (token->tclass != other->tclass)
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
