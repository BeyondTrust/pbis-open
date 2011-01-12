/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ThreadDef.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef THREAD_DEF_H
#define  THREAD_DEF_H

#include <OpenSOAP/Defines.h>

#if defined(WIN32)
#include <windows.h>
#include <process.h>
typedef HANDLE ThrFuncHandle;
typedef HANDLE ThrCondHandle;
typedef HANDLE ThrMutexHandle;
typedef void ThrFuncReturnType;
#define ReturnThread(ret) _endthread()
#else //WIN32
#include <pthread.h>
typedef pthread_t ThrFuncHandle;
typedef pthread_cond_t ThrCondHandle;
typedef pthread_mutex_t ThrMutexHandle;
typedef void* ThrFuncReturnType;
#define ReturnThread(ret) pthread_exit(ret)
#endif

typedef void* ThrFuncArgType;

namespace OpenSOAP {
    namespace Thread {
        extern void OPENSOAP_API
        lockMutex(ThrMutexHandle& mt);
        extern void OPENSOAP_API
        unlockMutex(ThrMutexHandle& mt);
    }
}

#endif // THREAD_DEF_H
