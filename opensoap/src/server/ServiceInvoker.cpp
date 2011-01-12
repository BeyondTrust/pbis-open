/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ServiceInvoker.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>

#include <OpenSOAP/CStdio.h>
#include <string>
#include <sys/stat.h>

//for debug
#include <sstream>
#include <typeinfo>

//for stdio
#include <sys/wait.h>
//#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include "SOAPMessageFunc.h"
#include "StringUtil.h"
#include "ServiceInvoker.h"

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
#include "FileIDFunc.h"
#include "DataRetainer.h"
#include "SrvConfAttrHandler.h"
#include "SoapMessageSerializer.h"

//for log, exception
#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

using namespace std;
using namespace OpenSOAP;


//#define DEBUG
static string CLASS_SIG("ServiceInvoker");
extern TraceLog *tlog;

ServiceInvoker::ServiceInvoker(SrvConf& aSrvConf) 
    : srvConf(aSrvConf)
{
    pthread_mutex_init(&timerMutex, NULL);
    pthread_cond_init(&timerCond, NULL);
}

ServiceInvoker::~ServiceInvoker()
{
    pthread_mutex_destroy(&timerMutex);
    pthread_cond_destroy(&timerCond);
}

ThrFuncReturnType 
ServiceInvoker::timerControlledInvoke(ThrFuncArgType arg)
{
    ServiceInvoker* that = (ServiceInvoker*)arg;
    if (!that) {
        //cast failed
        AppLogger::Write(ERR_ERROR, 
                         "%s::%s: %s",
                         CLASS_SIG.c_str(),
                         __func__,
                         "thread function argument is invalid. (null)");
        ReturnThread(NULL);
    }

    TraceLog tlog_local(*tlog);
    
    try {
        that->invokeImpl(that->requestRef, that->responseRef);
    }
    catch (SoapException& se) {
        se.AddFuncCallStack();
        AppLogger::Write(se);
        SoapMessageSerializer sms;
        sms.serialize(*(that->responseRef), se);
        //throw se;
    }
    catch (Exception& e) {
        e.AddFuncCallStack();
        AppLogger::Write(e);
        SoapMessageSerializer sms;
        map<string,string> hdrElem;
        string key("status");
        string val("500");
        hdrElem.insert(make_pair(key, val));
        sms.serialize(*(that->responseRef), 
                      that->requestRef->createSoapFault("Server",
                                                        e.GetErrText(),
                                                        SoapException::getMyUri(),
                                                        e.GetErrText()),
                      hdrElem);
        //throw e;
    }
    catch (...) {
        AppLogger::Write(ERR_ERROR, "%s::%s: %s",
                         CLASS_SIG.c_str(),
                         __func__,
                         "unknow error occurred.");
        SoapMessageSerializer sms;
        map<string,string> hdrElem;
        string key("status");
        string val("500");
        hdrElem.insert(make_pair(key, val));
        sms.serialize(*(that->responseRef), 
                      that->requestRef->createSoapFault("Server",
                                                        "Internal Error",
                                                        SoapException::getMyUri(),
                                                        "unknown error occurred"));

    }
    //exclusive lock
    Thread::lockMutex(that->timerMutex);
    //send signal for waitobject
    pthread_cond_signal(&(that->timerCond));
    //exclusive unlock
    Thread::unlockMutex(that->timerMutex);
    
    ReturnThread(NULL);
}

void 
ServiceInvoker::invokeImpl(SoapMessage* request, SoapMessage* response)
{
    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate(mainThreadId);
    //tlog_local.TraceWrite();
#endif //DEBUG

    //no proc.

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

    return;
}

void 
ServiceInvoker::setTimeoutPeriod(const int t)
{
    timeoutPeriod = t;
    AppLogger::Write(ERR_DEBUG, "%s::%s: timeoutPeriod=(%d)",
                     CLASS_SIG.c_str(),
                     __func__,
                     timeoutPeriod);
}

void 
ServiceInvoker::invoke(SoapMessage& request, SoapMessage& response)
{
    const string FAULT_SERVER("Server");

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG
    
    //set reference
    requestRef = &request;
    responseRef = &response;

    pthread_t thrId = pthread_t();
    int status = int();
    const int TIMEOUT_STATUS = ETIMEDOUT;
    const int NORMAL_STATUS = 0;

    //exclusive lock
    Thread::lockMutex(timerMutex);
    mainThreadId = pthread_self();

    status = pthread_create(&thrId, NULL, timerControlledInvoke, this);
    if (0 != status) {
        AppLogger::Write(ERR_ERROR,"%s::%s: %s %s(%d) %s=[%s]",
                         CLASS_SIG.c_str(),
                         __func__,
                         "pthread_create failed.",
                         "status",
                         status,
                         "fileId", 
                         request.getMessageId().c_str());
        //exception
        Exception e(-1, OPENSOAPSERVER_THREAD_CREATE_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG

        throw e;
    }
    
    typedef struct timespec TimeSpec;
    TimeSpec ts;
    ts.tv_sec = time(NULL);
    ts.tv_sec += timeoutPeriod;
    ts.tv_nsec = 0;
    
    AppLogger::Write(ERR_DEBUG, "%s::%s: cond_timedwait timeout=(%d)",
                     CLASS_SIG.c_str(),
                     __func__,
                     timeoutPeriod);

    //wait for cond signal
    //  invoke thread send signal before exit, then returned 0
    //  not complete until invokeTTL, then returned ETIMEDOUT
    status = pthread_cond_timedwait(&timerCond, &timerMutex, &ts);
    
    //status : 0(WAIT_OBJECT_0) is invoke thread complete
    //         ETIMEDOUT(WAIT_TIMEOUT) is invoke thread incomplete by timeout
    //         other is invoke thread failed
    if (TIMEOUT_STATUS == status) {
        //
        // invoke thread cancel and create timeout fault message
        //
        AppLogger::Write(ERR_ERROR,"%s::%s: %s fileId=[%s]",
                         CLASS_SIG.c_str(),
                         __func__,
                         "timeout occurred.",
                         request.getMessageId().c_str());
        
        //timeout : cancel thread
        status = pthread_cancel(thrId);
        if (0 != status) {
            string errStr(strerror(errno));
            AppLogger::Write(ERR_ERROR,"%s::%s: %s status=(%d) %s",
                             CLASS_SIG.c_str(),
                             __func__,
                             "pthread_cancel failed.",
                             status,
                             errStr.c_str());
            
            //exception
            SoapException e(-1, OPENSOAPSERVER_THREAD_ERROR,
                            ERR_ERROR, __FILE__, __LINE__,
                            &request);
            e.SetErrText("pthread_cancel failed");
            //fault
            e.setHttpStatusCode(500);
            e.setFaultCode(FAULT_SERVER);
            e.setFaultString(e.GetErrText());
            e.setDetail(e.GetErrText());
            e.setFaultActor(SoapException::getMyUri());
            
#ifdef DEBUG
            //trace
            tlog_local.SetComment("OUT with SoapException <<");
            tlog_local.TraceUpdate();
#endif //DEBUG

            //exclusive unlock
            Thread::unlockMutex(timerMutex);
            throw e;
        }
        
        AppLogger::Write(ERR_DEBUG, "%s::%s: %s",
                         CLASS_SIG.c_str(),
                         __func__,
                         "pthread_cancel done.");
        
        //create timeout fault
        SoapException se(-1, OPENSOAPSERVER_TIMEOUT_ERROR,
                         ERR_ERROR, __FILE__, __LINE__,
                         &request);
        se.SetErrText("Timeout");
        se.setHttpStatusCode(500);
        se.setFaultCode(FAULT_SERVER);
        se.setFaultString(se.GetErrText());
        se.setDetail("");
#if 1
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with SoapException <<");
        tlog_local.TraceUpdate();
#endif //DEBUG

        //exclusive unlock
        Thread::unlockMutex(timerMutex);
        throw se;
#else
        //
        SoapMessageSerializer sms;
        sms.serialize(response, se);

        AppLogger::Write(ERR_DEBUG, "%s::%s: timeoutFault(res_id=[%s]):[%s]",
                         CLASS_SIG.c_str(),
                         __func__,
                         response.getMessageId().c_str(),
                         response.toString().c_str());
#endif //if 1
    }
    else if (NORMAL_STATUS != status) {
        //
        // create internalerror fault message
        //
        string errStr(strerror(errno));
        AppLogger::Write(ERR_ERROR,"%s::%s: %s status=(%d) fileId=[%s] %s",
                         CLASS_SIG.c_str(),
                         __func__,
                         "thread failed.",
                         status,
                         request.getMessageId().c_str(),
                         errStr.c_str());
        
        
        Exception e(-1, OPENSOAPSERVER_THREAD_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
        e.SetErrText("exec thread failed");
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG

        //exclusive unlock
        Thread::unlockMutex(timerMutex);
        throw e;
    }

    //exclusive unlock
    Thread::unlockMutex(timerMutex);

    AppLogger::Write(ERR_DEBUG, "%s::%s: %s",
                     CLASS_SIG.c_str(),
                     __func__,
                     "pthread_cond_timedwait done.");

    //join thread resource
    status = pthread_join(thrId, NULL);
    if (0 != status) {
        string errStr(strerror(errno));
        AppLogger::Write(ERR_ERROR, "%s::%s: %s status=(%d) %s",
                         CLASS_SIG.c_str(),
                         __func__,
                         "pthread_join failed.",
                         status,
                         errStr.c_str());

        Exception e(-1, OPENSOAPSERVER_THREAD_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);

#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG

        throw e;
    }

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG

}

// End of ServiceInvoker.cpp

