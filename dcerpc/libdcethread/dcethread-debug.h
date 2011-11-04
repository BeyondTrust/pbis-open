#ifndef __PTHREAD_WRAP_DEBUG_H__
#define __PTHREAD_WRAP_DEBUG_H__

#define DCETHREAD_DEBUG_ERROR (0)
#define DCETHREAD_DEBUG_WARNING (1)
#define DCETHREAD_DEBUG_INFO (2)
#define DCETHREAD_DEBUG_VERBOSE (3)
#define DCETHREAD_DEBUG_TRACE (4)

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define DCETHREAD_DEBUG(level, ...) (dcethread__debug_printf(__FILE__, __LINE__, level, __VA_ARGS__))
#define DCETHREAD_ERROR(...) DCETHREAD_DEBUG(DCETHREAD_DEBUG_ERROR, __VA_ARGS__)
#define DCETHREAD_WARNING(...) DCETHREAD_DEBUG(DCETHREAD_DEBUG_WARNING, __VA_ARGS__)
#define DCETHREAD_INFO(...) DCETHREAD_DEBUG(DCETHREAD_DEBUG_INFO, __VA_ARGS__)
#define DCETHREAD_VERBOSE(...) DCETHREAD_DEBUG(DCETHREAD_DEBUG_VERBOSE, __VA_ARGS__)
#define DCETHREAD_TRACE(...) DCETHREAD_DEBUG(DCETHREAD_DEBUG_TRACE, __VA_ARGS__)

void dcethread__debug_set_callback(void (*cb) (const char*, unsigned int, int, const char*, void*), void* data);
void dcethread__debug_printf(const char* file, unsigned int line, int level, const char* fmt, ...);
void dcethread__default_log_callback (const char* file, unsigned int line, int level, const char* str, void* data);

#endif
