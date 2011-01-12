/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ResponseQueryInvoker.cpp,v $
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
#include "SoapMessageSerializer.h"
#include "ResponseQueryInvoker.h"
#include "MsgInfo.h"

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
#include "DataRetainer.h"
#include "SrvConfAttrHandler.h"
#include "SpoolAdopter.h"
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
ResponseQueryInvoker::invoke(SoapMessage& request, SoapMessage& response)
{
    static char METHOD_LABEL[] = "ResponseQueryInvoker::invoke";

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    AppLogger::Write(APLOG_DEBUG9, "%s %s",
                     METHOD_LABEL, 
                     "SpoolAdopter::pop now");

    string responseFileId;
    //create and add new uniq. id
    try {    
        string sendMessage;
        MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
        sendMessage = msgInfo->messageId;
        sendMessage += ",";
        sendMessage += (msgInfo->undelete ? "1":"0");
        SpoolAdopter spoolAdptr;
        //bool result = spoolAdptr.pop(msgInfo->messageId, responseFileId);
        bool result = spoolAdptr.pop(sendMessage, responseFileId);
        
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s %s %s=(%d) %s=[%s] %s=[%s]",
                         METHOD_LABEL,
                         "SpoolAdopter::pop",
                         "result",
                         result,
                         "sendMessage",
                         sendMessage.c_str(),
                         "responseFileId",
                         responseFileId.c_str());


        if (!result) {
            //fault return
            //exception
            SoapException se(-1, OPENSOAPSERVER_RUNTIME_ERROR,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("async response message query failed.");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode("Server");//FAULT_SERVER);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }

        //create send message for ttl
        SoapMessageSerializer sms;
        sms.deserialize(response, responseFileId);

        DataRetainer responseDr(srvConf.getSoapMessageSpoolPath());
        responseDr.SetId(responseFileId);
        //set life status
        if (msgInfo->undelete) {
            responseDr.SetLifeStatus(DataRetainer::KEEP);
        }
        else {
            responseDr.SetLifeStatus(DataRetainer::DONE);
        }

        AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s] %s=[%s]",
                         METHOD_LABEL,
                         "response fileId",
                         response.getMessageId().c_str(),
                         "pop response",
                         response.toString().c_str());
        
        //trace
        tlog_local.SetComment("SPOOLING_RESPONSE_QUERY_INVOKER INVOKED");
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


// End of ServiceInvoker.cpp

