#include <config.h>
#include <unistd.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

int
dcethread_pause()
{
    DCETHREAD_SYSCALL(int, pause());
}

#endif /* API */

#ifdef TEST

#include "dcethread-test.h"

static void*
basic_thread(void* data)
{
    volatile int interrupt_caught = 0;

    DCETHREAD_TRY
    {
	MU_ASSERT(!interrupt_caught);
	while (1)
	{
	    dcethread_pause();
	}
    }
    DCETHREAD_CATCH(dcethread_interrupt_e)
    {
	MU_ASSERT(!interrupt_caught);
	interrupt_caught = 1;
    }
    DCETHREAD_ENDTRY;

    MU_ASSERT(interrupt_caught);

    return NULL;
}

MU_TEST(dcethread_pause, interrupt_pre)
{
    dcethread* thread;

    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, basic_thread, NULL) );
    MU_TRY_DCETHREAD( dcethread_interrupt(thread) );
    MU_TRY_DCETHREAD( dcethread_join(thread, NULL) );
}

MU_TEST(dcethread_pause, interrupt_post)
{
    dcethread* thread;
    struct timespec ts;

    ts.tv_nsec = 100000000;
    ts.tv_sec = 0;
    
    MU_TRY_DCETHREAD( dcethread_create(&thread, NULL, basic_thread, NULL) );
    MU_TRY_DCETHREAD( dcethread_delay(&ts) );
    MU_TRY_DCETHREAD( dcethread_interrupt(thread) );
    MU_TRY_DCETHREAD( dcethread_join(thread, NULL) );
}

#endif /* TEST */
