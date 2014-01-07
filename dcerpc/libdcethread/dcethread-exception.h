#ifndef __DCETHREAD_EXCEPTION_H__
#define __DCETHREAD_EXCEPTION_H__

#include <dce/dcethread.h>

void dcethread__init_exceptions(void);
void dcethread__frame_push(dcethread_frame* frame);
void dcethread__frame_pop(dcethread_frame* frame);
void dcethread__exc_init(dcethread_exc* exc, const char* name);
void dcethread__exc_setstatus(dcethread_exc* exc, int value);
int dcethread__exc_getstatus(dcethread_exc* exc);
const char* dcethread__exc_getname(dcethread_exc* exc);
int dcethread__exc_matches(dcethread_exc* exc, dcethread_exc* pattern);
__DCETHREAD_NORETURN__ void dcethread__exc_raise(dcethread_exc* exc, const char* file, unsigned int line);
void dcethread__exc_handle_interrupt(dcethread* thread, void* data);
dcethread_exc* dcethread__exc_from_errno(int err);
void dcethread__exc_set_uncaught_handler(void (*handler) (dcethread_exc*, const char*, unsigned int, void*), void* data);

#endif
