/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ServiceInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef ServiceInvoker_H
#define ServiceInvoker_H

#include <string>
#include "SoapMessage.h"
#include "SrvConf.h"
#include "ThreadDef.h"

namespace OpenSOAP {
    class ServiceInvoker {
    public:
        ServiceInvoker(SrvConf& aSrvConf);
        virtual ~ServiceInvoker();
        void invoke(SoapMessage& request, SoapMessage& response);

        //setter
        void setTimeoutPeriod(const int t);
        
    protected:
        //actual invoke function 
        virtual void invokeImpl(SoapMessage* request, SoapMessage* response);
        //for thread function
        static ThrFuncReturnType timerControlledInvoke(ThrFuncArgType arg);

        SrvConf& srvConf;
        SoapMessage* requestRef;
        SoapMessage* responseRef;
        int timeoutPeriod;

        ThrMutexHandle timerMutex;
        ThrCondHandle timerCond;

        //extend: use for keeping main thread info.
        pthread_t mainThreadId;
    };
}

#endif //ServiceInvoker_H
