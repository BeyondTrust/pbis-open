#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

int
dcethread_exc_getstatus(dcethread_exc* exc)
{
    return dcethread__exc_getstatus(exc);
}
