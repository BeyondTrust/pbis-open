/* Clean room */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

int
dcethread_kill(dcethread* thread, int sig)
{
    return dcethread__set_errno(pthread_kill(thread->pthread, sig));
}

int
dcethread_kill_throw(dcethread* thread, int sig)
{
    DCETHREAD_WRAP_THROW(dcethread_kill(thread, sig));
}

#endif
