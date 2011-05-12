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
 *        connection-security.c
 *
 * Abstract:
 *
 *        Connection API
 *        Connection security token logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <config.h>

#include "connection-private.h"
#include "util-private.h"
#include "assoc-private.h"
#include "security-private.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static
LWMsgStatus
lwmsg_local_token_construct(
    LWMsgSecurityToken* token
    )
{
    return LWMSG_STATUS_SUCCESS;
}

static
void
lwmsg_local_token_destruct(
    LWMsgSecurityToken* token
    )
{
    return;
}

static
const char*
lwmsg_local_token_get_type(
    LWMsgSecurityToken* token
    )
{
    return "local";
}

static
LWMsgBool
lwmsg_local_token_equal(
    LWMsgSecurityToken* token,
    LWMsgSecurityToken* other
    )
{
    LocalTokenPrivate* my_priv = lwmsg_security_token_get_private(token);
    LocalTokenPrivate* other_priv = lwmsg_security_token_get_private(token);

    return (my_priv->euid == other_priv->euid &&
            my_priv->egid == other_priv->egid);
}

static
size_t
lwmsg_local_token_hash(
    LWMsgSecurityToken* token
    )
{
    LocalTokenPrivate* priv = lwmsg_security_token_get_private(token);
    static const int shift = ((sizeof(size_t) / 2) * 8);

    return (((size_t) priv->egid) << shift) ^ ((size_t) priv->euid);
}

static
LWMsgStatus
lwmsg_local_token_to_string(
    LWMsgSecurityToken* token,
    LWMsgBuffer* buffer
    )
{
    LocalTokenPrivate* priv = lwmsg_security_token_get_private(token);

    return lwmsg_buffer_print(
        buffer,
        "<local euid:%ld egid:%ld pid:%ld>",
        (long) priv->euid,
        (long) priv->egid,
        (long) priv->pid);
}

static
LWMsgBool
lwmsg_local_token_can_access(
    LWMsgSecurityToken* token,
    LWMsgSecurityToken* other
    )
{
    return lwmsg_security_token_equal(token, other);
}

static
LWMsgStatus
lwmsg_local_token_copy(
    LWMsgSecurityToken* token,
    LWMsgSecurityToken** out_token
    )
{
    LocalTokenPrivate* priv = lwmsg_security_token_get_private(token);

    return lwmsg_local_token_new(priv->euid, priv->egid, priv->pid, out_token);
}

LWMsgStatus
lwmsg_local_token_get_eid(
    LWMsgSecurityToken* token,
    uid_t *out_euid,
    gid_t *out_egid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LocalTokenPrivate* priv = lwmsg_security_token_get_private(token);

    if (strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    if (out_euid)
    {
        *out_euid = priv->euid;
    }

    if (out_egid)
    {
        *out_egid = priv->egid;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_local_token_get_pid(
    LWMsgSecurityToken* token,
    pid_t *out_pid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LocalTokenPrivate* priv = lwmsg_security_token_get_private(token);

    if (strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    if (out_pid)
    {
        *out_pid = priv->pid;
    }

error:

    return status;
}

static LWMsgSecurityTokenClass local_class =
{
    .private_size = sizeof(LocalTokenPrivate),
    .construct = lwmsg_local_token_construct,
    .destruct = lwmsg_local_token_destruct,
    .get_type = lwmsg_local_token_get_type,
    .equal = lwmsg_local_token_equal,
    .can_access = lwmsg_local_token_can_access,
    .copy = lwmsg_local_token_copy,
    .hash = lwmsg_local_token_hash,
    .to_string = lwmsg_local_token_to_string
};

LWMsgStatus
lwmsg_local_token_new(
    uid_t euid,
    gid_t egid,
    pid_t pid,
    LWMsgSecurityToken** out_token
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSecurityToken* token = NULL;
    LocalTokenPrivate* priv = NULL;

    BAIL_ON_ERROR(status = lwmsg_security_token_new(&local_class, &token));

    priv = lwmsg_security_token_get_private(token);

    priv->euid = euid;
    priv->egid = egid;
    priv->pid = pid;

    *out_token = token;

error:

    return status;
}

LWMsgStatus
lwmsg_local_token_from_socket_peer(
    int fd,
    LWMsgSecurityToken** out_token
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uid_t uid;
    gid_t gid;
    pid_t pid = (pid_t)-1;

#if !defined(HAVE_PEERID_METHOD)
    BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
#elif defined(HAVE_GETPEEREID) && HAVE_DECL_GETPEEREID
    if (getpeereid(fd, &uid, &gid))
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }
#elif defined(SO_PEERCRED)
    struct ucred creds;
    socklen_t creds_len = sizeof(struct ucred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &creds, &creds_len))
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }

    uid = creds.uid;
    gid = creds.gid;
    pid = creds.pid;

#endif

    BAIL_ON_ERROR(status = lwmsg_local_token_new(uid, gid, pid, out_token));

error:

    return status;
}

LWMsgStatus
lwmsg_connection_get_endpoint_owner(
    LWMsgAssoc* assoc,
    const char* endpoint,
    uid_t *uid,
    gid_t *gid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    struct stat endpoint_stat;

    if (stat(endpoint, &endpoint_stat))
    {
        BAIL_ON_ERROR(status = RAISE_ERRNO(&assoc->context));
    }

    if (!S_ISSOCK(endpoint_stat.st_mode))
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER,
                          "%s: not a socket", endpoint);
    }

    *uid = endpoint_stat.st_uid;
    *gid = endpoint_stat.st_gid;

error:

    return status;
}
