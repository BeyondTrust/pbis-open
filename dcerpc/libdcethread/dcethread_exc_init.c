#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

void
dcethread_exc_init(dcethread_exc* exc, const char* name)
{
    dcethread__exc_init(exc, name);
}
