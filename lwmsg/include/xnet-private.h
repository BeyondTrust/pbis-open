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
 *        xnet-private.h
 *
 * Abstract:
 *
 *        X/Open networking API wrappers (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_XNET_PRIVATE_H__
#define __LWMSG_XNET_PRIVATE_H__

#ifdef __hpux__

#include <sys/socket.h>

int _xnet_accept(int, struct sockaddr *, socklen_t *);
int _xnet_bind(int, const struct sockaddr *, socklen_t);
int _xnet_connect(int, const struct sockaddr *, socklen_t);
int _xnet_getpeername(int, struct sockaddr *, socklen_t *);
int _xnet_getsockname(int, struct sockaddr *, socklen_t *);
int _xnet_getsockopt(int, int, int, void *, socklen_t *);
ssize_t _xnet_recv(int, void *, size_t, int);
ssize_t _xnet_recvfrom(int, void *, size_t, int,
                                 struct sockaddr *, socklen_t *);
ssize_t _xnet_recvmsg(int, struct msghdr *, int);
ssize_t _xnet_send(int, const void *, size_t, int);
ssize_t _xnet_sendmsg(int, const struct msghdr *, int);
ssize_t _xnet_sendto(int, const void *, size_t, int,
                               const struct sockaddr *, socklen_t);
int _xnet_setsockopt(int, int, int, const void *, socklen_t);
int _xnet_socket(int, int, int);
int _xnet_socketpair(int, int, int, int[2]);

#define accept(a,b,c) _xnet_accept(a,b,c)
#define bind         _xnet_bind
#define connect(a,b,c) _xnet_connect(a,b,c)
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

#endif
