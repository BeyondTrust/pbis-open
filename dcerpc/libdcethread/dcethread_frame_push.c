#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

void
dcethread_frame_push(dcethread_frame* frame)
{
    dcethread__frame_push(frame);
}
