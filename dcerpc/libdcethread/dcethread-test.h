#ifndef __DCETHREAD_TEST_H__
#define __DCETHREAD_TEST_H__

#include <moonunit/interface.h>
#include <errno.h>
#include <string.h>

#define MU_TRY_DCETHREAD(expr)						\
    do									\
    {									\
        if ((expr))							\
	    MU_FAILURE("Expression %s failed: %s",			\
		       #expr, strerror(errno));				\
    } while (0);							\

#endif

void dcethread__test_init();
