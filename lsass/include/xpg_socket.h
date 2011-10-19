/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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

#ifndef __LSA_XPG_SOCKET_H__
#define __LSA_XPG_SOCKET_H__

extern int _xnet_accept(int, struct sockaddr *, socklen_t *);
extern int _xnet_bind(int, const struct sockaddr *, socklen_t);
extern int _xnet_connect(int, const struct sockaddr *, socklen_t);
extern int _xnet_getpeername(int, struct sockaddr *, socklen_t *);
extern int _xnet_getsockname(int, struct sockaddr *, socklen_t *);
extern int _xnet_getsockopt(int, int, int, void *, socklen_t *);
extern ssize_t _xnet_recv(int, void *, size_t, int);
extern ssize_t _xnet_recvfrom(int, void *, size_t, int,
                                 struct sockaddr *, socklen_t *);
extern ssize_t _xnet_recvmsg(int, struct msghdr *, int);
extern ssize_t _xnet_send(int, const void *, size_t, int);
extern ssize_t _xnet_sendmsg(int, const struct msghdr *, int);
extern ssize_t _xnet_sendto(int, const void *, size_t, int,
                               const struct sockaddr *, socklen_t);
extern int _xnet_setsockopt(int, int, int, const void *, socklen_t);
extern int _xnet_socket(int, int, int);
extern int _xnet_socketpair(int, int, int, int[2]);

#define accept       _xnet_accept
#define bind         _xnet_bind
#define connect      _xnet_connect
#define getpeername  _xnet_getpeername
#define getsockname  _xnet_getsockname
#define getsockopt   _xnet_getsockopt
#define recv         _xnet_recv
#define recvfrom     _xnet_recvfrom
#define recvmsg      _xnet_recvmsg
#define send         _xnet_send
#define sendto       _xnet_sendto
#define sendmsg      _xnet_sendmsg
#define setsockopt   _xnet_setsockopt
#define socket       _xnet_socket
#define socketpair   _xnet_socketpair

#endif
