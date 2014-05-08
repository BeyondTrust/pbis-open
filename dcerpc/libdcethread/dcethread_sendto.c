/* Clean */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

ssize_t
dcethread_sendto(int s, const void *buf, size_t len, int flags,
                 const struct sockaddr *to, socklen_t tolen)
{
    DCETHREAD_SYSCALL(ssize_t, sendto(s, buf, len, flags, to, tolen));
}

#endif /* API */
