#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

void
dcethread_exc_setstatus(dcethread_exc* exc, int value)
{
    dcethread__exc_setstatus(exc, value);
}
