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
 *        util.c
 *
 * Abstract:
 *
 *        Utility functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "util-private.h"
#include "xnet-private.h"
#include "buffer-private.h"

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#ifndef SOLARIS_11
#if HAVE_SYS_VARARGS_H
#include <sys/varargs.h>
#endif
#endif
#include <iconv.h>
#include <errno.h>
#ifdef HAVE_PTHREAD_SIGMASK_IN_LIBC
#include <pthread.h>
#endif
#include <signal.h>
#include <poll.h>

LWMsgStatus
lwmsg_set_close_on_exec(
    int fd
    )
{
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
    {
        return lwmsg_status_map_errno(errno);
    }
    else
    {
        return LWMSG_STATUS_SUCCESS;
    }
}

LWMsgStatus
lwmsg_set_block_sigpipe(
    int fd
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
#ifdef SO_NOSIGPIPE
    int on = 1;
    
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)) < 0)
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }

error:
#endif   
    return status;
}

static
int
lwmsg_begin_ignore_sigpipe(
    LWMsgBool* unblock,
    sigset_t* original
    )
{
    int ret = 0;
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE) && defined(HAVE_PTHREAD_SIGMASK_IN_LIBC)
    sigset_t sigpipemask;
    sigset_t pending;

    sigpending(&pending);

    /* If SIGPIPE was not already pending against the thread,
       we need to modify the thread signal mask to block it
       and then unblock it again once we are done */
    if (!sigismember(&pending, SIGPIPE))
    {
        sigemptyset(&sigpipemask);
        sigaddset(&sigpipemask, SIGPIPE);

        if (pthread_sigmask(SIG_BLOCK, &sigpipemask, original) < 0)
        {
            ret = -1;
            goto error;
        }

        *unblock = LWMSG_TRUE;
    }
    else
    {
        *unblock = LWMSG_FALSE;
    }
    
error:
#endif
    return ret;
}

static
int
lwmsg_end_ignore_sigpipe(
    LWMsgBool* unblock,
    sigset_t* original
    )
{
    int ret = 0;
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE) && defined(HAVE_PTHREAD_SIGMASK_IN_LIBC)
    sigset_t sigpipemask;
    sigset_t pending;
    siginfo_t info;
    struct timespec timeout = {0, 0};

    if (*unblock)
    {
        /* We need to restore the original signal mask for this thread.
           Before doing so, we need to clear any pending SIGPIPE so it
           does not trigger a signal handler as soon as we restore the mask. */
        sigpending(&pending);

        if (sigismember(&pending, SIGPIPE))
        {
            sigemptyset(&sigpipemask);
            sigaddset(&sigpipemask, SIGPIPE);
            
            do
            {
                ret = sigtimedwait(&sigpipemask, &info, &timeout);
            } while (ret < 0 && errno == EINTR);

            ret = 0;
        }
        
        if (pthread_sigmask(SIG_SETMASK, original, NULL))
        {
            ret = -1;
            goto error;
        }
        
        *unblock = LWMSG_FALSE;
    }
        
error:
#endif
    return ret;
}

ssize_t
lwmsg_recvmsg_timeout(
    int sock,
    struct msghdr* msg,
    int flags,
    LWMsgTime* time
    )
{
    int timeout = -1;
    struct pollfd pfd = {0};
    int ret = 0;

#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif

    if (time && lwmsg_time_is_positive(time))
    {
        timeout = time->seconds * 1000 + time->microseconds / 1000;

        pfd.fd = sock;
        pfd.events = POLLIN;

        ret = poll(&pfd, 1, timeout);
        if (ret < 0)
        {
            return ret;
        }
        else if (ret == 0)
        {
            errno = EAGAIN;
            return -1;
        }
    }

    return recvmsg(sock, msg, flags);
}

ssize_t
lwmsg_sendmsg_timeout(
    int sock,
    const struct msghdr* msg,
    int flags,
    LWMsgTime* time
    )
{
    int timeout = -1;
    struct pollfd pfd = {0};
    int ret = 0;
    sigset_t original;
    LWMsgBool unblock = LWMSG_FALSE;
    int err = 0;

#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif

    if (time && lwmsg_time_is_positive(time))
    {
        timeout = time->seconds * 1000 + time->microseconds / 1000;

        pfd.fd = sock;
        pfd.events = POLLOUT;

        ret = poll(&pfd, 1, timeout);
        if (ret < 0)
        {
            return ret;
        }
        else if (ret == 0)
        {
            errno = EAGAIN;
            return -1;
        }
    }

    if (lwmsg_begin_ignore_sigpipe(&unblock, &original) < 0)
    {
        return -1;
    }

    ret = sendmsg(sock, msg, flags);
    err = errno;

    if (lwmsg_end_ignore_sigpipe(&unblock, &original) < 0)
    {
        return -1;
    }

    errno = err;
    return ret;
}

