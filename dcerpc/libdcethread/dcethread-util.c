#include <errno.h>

#include "dcethread-util.h"

int
dcethread__set_errno(int err)
{
    errno = err;

    return err ? -1 : 0;
}
