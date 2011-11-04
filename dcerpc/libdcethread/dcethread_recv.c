/* Clean */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

ssize_t
dcethread_recv(int s, void *buf, size_t len, int flags)
{
    DCETHREAD_SYSCALL(ssize_t, recv(s, buf, len, flags));
}

#endif /* API */
