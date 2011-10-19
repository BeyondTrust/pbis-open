/* Clean */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

ssize_t
dcethread_sendmsg(int s, const struct msghdr *msg, int flags)
{
    DCETHREAD_SYSCALL(ssize_t, sendmsg(s, msg, flags));
}

#endif /* API */
