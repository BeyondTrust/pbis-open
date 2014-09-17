/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ThreadDef.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <iostream>
#include <stdexcept>

#include "ThreadDef.h"
#include "ServerCommon.h"
#include "AppLogger.h"

//#define DEBUG

using namespace std;

extern
void OPENSOAP_API
OpenSOAP::Thread::lockMutex(ThrMutexHandle& mt)
{
    static char METHOD_LABEL[] = "lockMutex: ";

#if defined(WIN32)
    DWORD ret = WaitForSingleObject(mt, INFINITE);
    if (WAIT_FAILED == ret) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"mutex lock failed.");
        throw runtime_error("mutex lock failed");
    }
#else //WIN32
    int status = pthread_mutex_lock(&mt);
    if (0 != status) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                        ,"mutex lock failed.","rc",status);
        throw runtime_error("pthread_mutex_lock failed");
    }
#endif //define(WIN32)/else

}

extern
void OPENSOAP_API
OpenSOAP::Thread::unlockMutex(ThrMutexHandle& mt)
{
    static char METHOD_LABEL[] = "unlockMutex: ";

#if defined(WIN32)
    BOOL bRet = ReleaseMutex(mt);
    if (!bRet) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"mutex unlock failed.");
        throw runtime_error("unlock mutex failed");
    }
#else //WIN32
    int status = pthread_mutex_unlock(&mt);
    if (0 != status) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                        ,"mutex unlock failed.","rc",status);
        throw runtime_error("pthread_mutex_unlock failed");
    }
#endif //define(WIN32)/else

}
