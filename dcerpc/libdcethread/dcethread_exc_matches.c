#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

int
dcethread_exc_matches(dcethread_exc* exc, dcethread_exc* pattern)
{
    return dcethread__exc_matches(exc, pattern);
}
