#ifndef __DCETHREAD_UTIL_H__
#define __DCETHREAD_UTIL_H__

#include <errno.h>

#include "dcethread-exception.h"

int dcethread__set_errno(int err);

#define DCETHREAD_BEGIN_SYSCALL \
    do                                                                  \
    {                                                                   \
        dcethread__unblock_signals();                                    \
        if (dcethread__begin_block (dcethread__self(), NULL, NULL, NULL, NULL)) \
        {                                                               \
            dcethread__dispatchinterrupt(dcethread__self());            \
            return dcethread__set_errno(EINTR);                         \
        }                                                               \
    } while (0)
        
#define DCETHREAD_END_SYSCALL                                           \
    do                                                                  \
    {                                                                   \
        if (dcethread__end_block (dcethread__self(), NULL, NULL))       \
        {                                                               \
            dcethread__dispatchinterrupt(dcethread__self());            \
            return dcethread__set_errno(EINTR);                         \
        }                                                               \
    } while (0)

#define DCETHREAD_SYSCALL(type, expr)      \
    do					   \
    {					   \
        type ret;                          \
        int err;                           \
	DCETHREAD_BEGIN_SYSCALL;	   \
    	ret = (expr);			   \
	err = errno;			   \
	DCETHREAD_END_SYSCALL;		   \
        errno = err;			   \
        return ret;			   \
    } while(0);

#define DCETHREAD_WRAP_THROW(expr)					\
    do									\
    {									\
        int ret = (expr);						\
        if (ret < 0)							\
	    dcethread__exc_raise(dcethread__exc_from_errno(errno), __FILE__, __LINE__); \
	return ret;							\
    } while (0);							\
	      
#endif
