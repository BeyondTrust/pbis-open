#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

void
dcethread_exc_raise(dcethread_exc* exc, const char* file, unsigned int line)
{
    dcethread__exc_raise(exc, file, line);
}
