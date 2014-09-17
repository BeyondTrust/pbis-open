/* Clean */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

ssize_t
dcethread_recvfrom(int s, void *buf, size_t len, int flags,
                   struct sockaddr *from, socklen_t *fromlen)
{
    DCETHREAD_SYSCALL(ssize_t, recvfrom(s, buf, len, flags, from, fromlen));
}

#endif /* API */
