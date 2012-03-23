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
 *        xnet.c
 *
 * Abstract:
 *
 *        X/Open networking API wrappers
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "xnet-private.h"

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __hpux__

static int (*_p_xnet_accept) (int, struct sockaddr *, socklen_t *);
static int (*_p_xnet_bind) (int, const struct sockaddr *, socklen_t);
static int (*_p_xnet_connect) (int, const struct sockaddr *, socklen_t);
static int (*_p_xnet_getpeername) (int, struct sockaddr *, socklen_t *);
static int (*_p_xnet_getsockname) (int, struct sockaddr *, socklen_t *);
static int (*_p_xnet_getsockopt) (int, int, int, void *, socklen_t *);
static ssize_t (*_p_xnet_recv) (int, void *, size_t, int);
static ssize_t (*_p_xnet_recvfrom) (int, void *, size_t, int,
                                 struct sockaddr *, socklen_t *);
static ssize_t (*_p_xnet_recvmsg) (int, struct msghdr *, int);
static ssize_t (*_p_xnet_send) (int, const void *, size_t, int);
static ssize_t (*_p_xnet_sendmsg) (int, const struct msghdr *, int);
static ssize_t (*_p_xnet_sendto) (int, const void *, size_t, int,
                               const struct sockaddr *, socklen_t);
static int (*_p_xnet_setsockopt) (int, int, int, const void *, socklen_t);
static int (*_p_xnet_socket) (int, int, int);
static int (*_p_xnet_socketpair) (int, int, int, int[2]);

static void* _xnet_handle = NULL;

static const char* _xnet_paths[] =
{
  // PA RISC
  "/usr/lib/libxnet.sl",
  // PA RISC 64bit
  "/usr/lib/pa20_64/libxnet.sl",

  // Itanium 32bit
  "/usr/lib/hpux32/libxnet.so",
  // Itanium 64bit
  "/usr/lib/hpux64/libxnet.so",
  NULL
};

static void* _xnet_open(void)
{
    void* handle = NULL;
    int i = 0;
 
    for (i = 0; _xnet_paths[i]; i++)
    {
        handle = dlopen(_xnet_paths[i], RTLD_LOCAL);
        if (handle) return handle;
    }
    return NULL;
}

__attribute__((constructor))
static void
__xnet_init__(
    void
    )
{
    if (!_xnet_handle)
    {
        _xnet_handle = _xnet_open();
        _p_xnet_accept = dlsym(_xnet_handle, "accept");
        _p_xnet_bind = dlsym(_xnet_handle, "bind");
        _p_xnet_connect = dlsym(_xnet_handle, "connect");
        _p_xnet_getpeername = dlsym(_xnet_handle, "getpeername");
        _p_xnet_getsockname = dlsym(_xnet_handle, "getsockname");
        _p_xnet_getsockopt = dlsym(_xnet_handle, "getsockopt");
        _p_xnet_recv = dlsym(_xnet_handle, "recv");
        _p_xnet_recvfrom = dlsym(_xnet_handle, "recvfrom");
        _p_xnet_recvmsg = dlsym(_xnet_handle, "recvmsg");
        _p_xnet_send = dlsym(_xnet_handle, "send");
        _p_xnet_sendmsg = dlsym(_xnet_handle, "sendmsg");
        _p_xnet_sendto = dlsym(_xnet_handle, "sendto");
        _p_xnet_setsockopt = dlsym(_xnet_handle, "setsockopt");
        _p_xnet_socket = dlsym(_xnet_handle, "socket");
        _p_xnet_socketpair = dlsym(_xnet_handle, "socketpair");
    }
}

int
_xnet_accept (int s, struct sockaddr *addr, socklen_t *len)
{
    __xnet_init__();
    return _p_xnet_accept(s, addr, len);
}

int
_xnet_bind (int s, const struct sockaddr *addr, socklen_t len)
{
    __xnet_init__();
    return _p_xnet_bind(s, addr, len);
}

int
_xnet_connect (int s, const struct sockaddr *addr, socklen_t len)
{
    __xnet_init__();
    return _p_xnet_connect(s, addr, len);
}

int
_xnet_getpeername (int s, struct sockaddr *addr, socklen_t *len)
{
    __xnet_init__();
    return _p_xnet_getpeername(s, addr, len);
}

int
_xnet_getsockname (int s, struct sockaddr *addr, socklen_t *len)
{
    __xnet_init__();
    return _p_xnet_getsockname(s, addr, len);
}

int _xnet_getsockopt (int s, int level, int name, void *val, socklen_t *len)
{
    __xnet_init__();
    return _p_xnet_getsockopt(s, level, name, val, len);
}

ssize_t
_xnet_recv (int s, void *buf, size_t len, int flags)
{
    __xnet_init__();
    return _p_xnet_recv(s, buf, len, flags);
}

ssize_t
_xnet_recvfrom (int s, void *buf, size_t len, int flags,
                struct sockaddr *addr, socklen_t *slen)
{
    __xnet_init__();
    return _p_xnet_recvfrom(s, buf, len, flags, addr, slen);
}

ssize_t
_xnet_recvmsg (int s, struct msghdr *msg, int flags)
{
    __xnet_init__();
    return _p_xnet_recvmsg(s, msg, flags);
}

ssize_t
_xnet_send (int s, const void *buf, size_t len, int flags)
{
    __xnet_init__();
    return _p_xnet_send(s, buf, len, flags);
}

ssize_t
_xnet_sendmsg (int s, const struct msghdr *msg, int flags)
{
    __xnet_init__();
    return _p_xnet_sendmsg(s, msg, flags);
}

ssize_t
_xnet_sendto (int s, const void * buf, size_t len, int flags,
              const struct sockaddr *addr, socklen_t slen)
{
    __xnet_init__();
    return _p_xnet_sendto(s, buf, len, flags, addr, slen);
}

int
_xnet_setsockopt (int s, int level, int opt, const void *val, socklen_t len)
{
    __xnet_init__();
    return _p_xnet_setsockopt(s, level, opt, val, len);
}

int
_xnet_socket (int d, int t, int p)
{
    __xnet_init__();
    return _p_xnet_socket(d, t, p); 
}

int
_xnet_socketpair (int d, int t, int p, int pair[2])
{
    __xnet_init__();
    return _p_xnet_socketpair(d, t, p, pair);
}

#endif
