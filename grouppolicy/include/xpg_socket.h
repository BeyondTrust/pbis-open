#ifndef __GPA_XPG_SOCKET_H__
#define __GPA_XPG_SOCKET_H__

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
