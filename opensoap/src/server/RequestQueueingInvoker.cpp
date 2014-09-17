/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: RequestQueueingInvoker.cpp,v $
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
#include "RequestQueueingInvoker.h"
#include "MsgInfo.h"
#include "SoapDef.h"

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
//#include "FileIDFunc.h"
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
RequestQueueingInvoker::invoke(SoapMessage& request, SoapMessage& response)
{
    static char METHOD_LABEL[] = "RequestQueueingInvoker::invoke";

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    //
    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();

    AppLogger::Write(APLOG_DEBUG9, "%s %s",
                     METHOD_LABEL, 
                     "QueueAdopter::push now");

    //create and add new uniq. id
    try {    
        QueueAdopter::QueueType qType;
        if (msgInfo->operationExist) {
            qType = QueueAdopter::LOCAL;
        }
        else {
            qType = QueueAdopter::FORWARD;
        }
        QueueAdopter queueAdptr(qType);
        string result = queueAdptr.push(request.getMessageId());
        
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s QueueAdopter::push result=[%s]",
                         METHOD_LABEL,
                         result.c_str());

        if (OpenSOAP::Result::SUCCESS != result) {
            //fault return
            //exception
            SoapException se(-1, OPENSOAPSERVER_RUNTIME_ERROR,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("async message queueing failed.");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }

        //create send message for ttl
        string sendMessage;
        sendMessage = msgInfo->messageId;
        sendMessage += ",";
        sendMessage += request.getMessageId();
        sendMessage += ",";
        sendMessage += StringUtil::toString(getInvokeTTL());
        sendMessage += ",";
        sendMessage += msgInfo->backwardPath;

        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s TTLAdopter::push sendMessage=[%s]",
                         METHOD_LABEL,
                         sendMessage.c_str());

        TTLAdopter ttlAdptr;
        result = ttlAdptr.push(sendMessage);
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s TTLAdopter::push result=[%s]",
                         METHOD_LABEL,
                         result.c_str());

        if (OpenSOAP::Result::SUCCESS != result) {
            //remove queue and fault return
            result = queueAdptr.expire(request.getMessageId());
            if (OpenSOAP::Result::SUCCESS != result) {
                AppLogger::Write(APLOG_ERROR, "%s %s %s=[%s]",
                                 METHOD_LABEL,
                                 "queue expire failed."
                                 "fileId",
                                 request.getMessageId().c_str());
            }
            //exception
            SoapException se(-1, OPENSOAPSERVER_RUNTIME_ERROR,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("ttl infomation for async entry failed.");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }

        //return message_id 
#if 1
        string resStr = 
            createResponseSoapMsgAsMessageId(msgInfo->messageId);
#else
        string resStr = 
            createResponseSoapMsgAsMessageId(msgInfo->messageId);
#endif

        DataRetainer responseDr(srvConf.getSoapMessageSpoolPath());
        responseDr.Create();
        string responseFile = responseDr.GetHttpBodyFileName();
        ofstream ofst(responseFile.c_str());
        ofst << resStr << endl;
        ofst.close();
        
        //temp.
        responseDr.AddHttpHeaderElement("content-type", "text/xml;");
        //
        responseDr.Decompose();

        string responseId;
        responseDr.GetSoapEnvelope(resStr);
        responseDr.GetId(responseId);

        response.deserialize(resStr);
        response.setMessageId(responseId);

        //
        responseDr.Compose();
        
        //trace
        tlog_local.SetComment("SYNCPROC_SERVICE_INVOKER INVOKED");
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
RequestQueueingInvoker::getInvokeTTL(/*const SoapMessage& request*/)
{
    static char METHOD_LABEL[] = "RequestQueueingInvoker::getInvokeTTL:";
    
    //check ttl
    
    // 1st. priority in SSML
    SSMLInfo ssmlInfo;
    ssmlInfo.setOperationName(methodName);
    ssmlInfo.setNamespace(nsUri);
    bool operationExist = getSSMLInfo(ssmlInfo);
    long invokeTTL = ssmlInfo.getAsyncTTL();

    //debug
    AppLogger::Write(APLOG_DEBUG9, 
                     "%s check invokeTTL methodName=[%s] nsUri=[%s] operationExist=(%d)",
                     METHOD_LABEL,
                     methodName.c_str(),
                     nsUri.c_str(),
                     operationExist);

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s invokeTTL in ssml=(%d)",
                     METHOD_LABEL,
                     invokeTTL);

    // when value is zero, 2nd. priority in server.conf
    if (invokeTTL <= 0) {
        invokeTTL = srvConf.getLimitTTLAsync();
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s invokeTTL in srvConf=(%d)",
                         METHOD_LABEL,
                         invokeTTL);
    }
    // request-header value smaller than that value
    // update by request-header value
    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
    int headerTTL = msgInfo->ttlAsyncSecond;
    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s invokeTTL in SoapHeader=(%d)",
                     METHOD_LABEL,
                     headerTTL);

    if (headerTTL > 0 && headerTTL < invokeTTL) {
        invokeTTL = headerTTL;
    }

    return invokeTTL;
}

// End of ServiceInvoker.cpp

