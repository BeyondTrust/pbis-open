/* Clean */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

ssize_t
dcethread_send(int s, const void *buf, size_t len, int flags)
{
    DCETHREAD_SYSCALL(ssize_t, send(s, buf, len, flags));
}

#endif /* API */
