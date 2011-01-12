/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ForwardingInvoker.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <string>
#include <sys/stat.h>

#include <fstream>


#include "SOAPMessageFunc.h"
#include "ForwardingInvoker.h"
#include "MsgInfo.h"
#include "HttpServiceInvoker.h" 

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
#include "DataRetainer.h"
#include "SrvConfAttrHandler.h"
#include "QueueAdopter.h"
#include "TTLAdopter.h"
#include "StringUtil.h"

//for log, exception
#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

using namespace OpenSOAPv1_2_00;
using namespace std;
using namespace OpenSOAP;

//#define DEBUG

extern TraceLog *tlog;

//==========================================
//==========================================
void 
ForwardingInvoker::invoke(SoapMessage& request, SoapMessage& response)
{
    static char METHOD_LABEL[] = "ForwardingInvoker::invoke";

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG


    try {
        //invoker 
        HttpServiceInvoker httpInvoker(srvConf);

        MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
        if (msgInfo->forwardPathArray.size() >0) {
            httpInvoker.setEndPointUrl(msgInfo->forwardPathArray[0]);
        }
        else {
            httpInvoker.setEndPointUrl(srvConf.getForwarderUrl());
        }
        httpInvoker.setTimeoutPeriod(getInvokeTTL());
        
        //invoke
        httpInvoker.invoke(request, response);
        
        //trace
        tlog_local.SetComment("FORWARDING_SERVICE_INVOKER INVOKED");
        tlog_local.TraceUpdate();
    }
    catch (SoapException& se) {
        throw se;
    }
    catch (Exception& e) {
        e.AddFuncCallStack();
        //AppLogger::Write(e);
        throw e;
    }
    catch (...) {
        AppLogger::Write(ERR_ERROR, "unknown error");
    }

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG
}

long
ForwardingInvoker::getInvokeTTL() const
{
    static char METHOD_LABEL[] = "ForwardingInvoker::getInvokeTTL:";
    

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
    long invokeTTL = srvConf.getLimitTTLSecond();
    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s invokeTTL in server.conf=(%d)",
                     METHOD_LABEL,
                     invokeTTL);

    if (msgInfo->hasTTLSecond && invokeTTL > msgInfo->ttlSecond) {
        //overwrite
        invokeTTL = msgInfo->ttlSecond;
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s invokeTTL in SoapHeader=(%d)",
                         METHOD_LABEL,
                         invokeTTL);
    }
    return invokeTTL;
}

// End of ServiceInvoker.cpp

