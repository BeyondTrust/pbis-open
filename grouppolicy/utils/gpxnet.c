/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        gpxnet.c
 *
 * Abstract:
 *
 *        HP-UX X/Open socket API wrappers        
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */
#include "includes.h"

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

static void* volatile _xnet_handle = NULL;

static const char* _xnet_paths[] =
{
  "/usr/lib/libxnet.sl",
  "/usr/lib/hpux32/libxnet.so",
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
GPAXnetInit(
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
    GPAXnetInit();
    if (_p_xnet_accept != NULL)
    {
        return _p_xnet_accept(s, addr, len);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int
_xnet_bind (int s, const struct sockaddr *addr, socklen_t len)
{
    GPAXnetInit();
    if (_p_xnet_bind != NULL)
    {
        return _p_xnet_bind(s, addr, len);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int
_xnet_connect (int s, const struct sockaddr *addr, socklen_t len)
{
    GPAXnetInit();
    if (_p_xnet_connect != NULL)
    {
        return _p_xnet_connect(s, addr, len);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int
_xnet_getpeername (int s, struct sockaddr *addr, socklen_t *len)
{
    GPAXnetInit();
    if (_p_xnet_getpeername != NULL)
    {
        return _p_xnet_getpeername(s, addr, len);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int
_xnet_getsockname (int s, struct sockaddr *addr, socklen_t *len)
{
    GPAXnetInit();
    if (_p_xnet_getsockname != NULL)
    {
        return _p_xnet_getsockname(s, addr, len);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int _xnet_getsockopt (int s, int level, int name, void *val, socklen_t *len)
{
    GPAXnetInit();
    if (_p_xnet_getsockopt != NULL)
    {
        return _p_xnet_getsockopt(s, level, name, val, len);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

ssize_t
_xnet_recv (int s, void *buf, size_t len, int flags)
{
    GPAXnetInit();
    if (_p_xnet_recv != NULL)
    {
        return _p_xnet_recv(s, buf, len, flags);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

ssize_t
_xnet_recvfrom (int s, void *buf, size_t len, int flags,
                struct sockaddr *addr, socklen_t *slen)
{
    GPAXnetInit();
    if (_p_xnet_recvfrom != NULL)
    {
        return _p_xnet_recvfrom(s, buf, len, flags, addr, slen);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

ssize_t
_xnet_recvmsg (int s, struct msghdr *msg, int flags)
{
    GPAXnetInit();
    if (_p_xnet_recvmsg != NULL)
    {
        return _p_xnet_recvmsg(s, msg, flags);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

ssize_t
_xnet_send (int s, const void *buf, size_t len, int flags)
{
    GPAXnetInit();
    if (_p_xnet_send != NULL)
    {
        return _p_xnet_send(s, buf, len, flags);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

ssize_t
_xnet_sendmsg (int s, const struct msghdr *msg, int flags)
{
    GPAXnetInit();
    if (_p_xnet_sendmsg != NULL)
    {
        return _p_xnet_sendmsg(s, msg, flags);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

ssize_t
_xnet_sendto (int s, const void * buf, size_t len, int flags,
              const struct sockaddr *addr, socklen_t slen)
{
    GPAXnetInit();
    if (_p_xnet_sendto != NULL)
    {
        return _p_xnet_sendto(s, buf, len, flags, addr, slen);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int
_xnet_setsockopt (int s, int level, int opt, const void *val, socklen_t len)
{
    GPAXnetInit();
    if (_p_xnet_setsockopt != NULL)
    {
        return _p_xnet_setsockopt(s, level, opt, val, len);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int
_xnet_socket (int d, int t, int p)
{
    GPAXnetInit();
    if (_p_xnet_socket != NULL)
    {
        return _p_xnet_socket(d, t, p); 
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

int
_xnet_socketpair (int d, int t, int p, int pair[2])
{
    GPAXnetInit();
    if (_p_xnet_socketpair != NULL)
    {
        return _p_xnet_socketpair(d, t, p, pair);
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

#endif
