/* Clean */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

int
dcethread_select(int nfds, fd_set *readfds, fd_set *writefds,
                 fd_set *exceptfds, struct timeval *timeout)
{
    DCETHREAD_SYSCALL(int, select(nfds, readfds, writefds, exceptfds, timeout));
}

#endif /* API */
