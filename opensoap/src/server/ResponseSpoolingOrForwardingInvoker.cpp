/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ResponseSpoolingOrForwardingInvoker.cpp,v $
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
#include "ResponseSpoolingOrForwardingInvoker.h"
#include "HttpServiceInvoker.h"
#include "MsgInfo.h"

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
//#include "FileIDFunc.h"
#include "DataRetainer.h"
#include "SrvConfAttrHandler.h"
#include "SpoolAdopter.h"
#include "TTLAdopter.h"
#include "StringUtil.h"
#include "XmlModifier.h"
#include "SoapDef.h"

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
ResponseSpoolingOrForwardingInvoker::invoke(SoapMessage& request, SoapMessage& response)
{
    static char METHOD_LABEL[] = "ResponseSpoolingOrForwardingInvoker::invoke";

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();

    //check TTL
    TTLAdopter ttlAdptr;
    string backwardPath;
    bool result = ttlAdptr.ref(msgInfo->messageId, backwardPath);
    if (!result) {
        //not entry
        SoapException se(-1, OPENSOAPSERVER_RUNTIME_ERROR,
                         ERR_ERROR, __FILE__, __LINE__,
                         &request);
        se.SetErrText("message_id entry in ttl_table not found");
        //fault
        se.setHttpStatusCode(500);
        se.setFaultCode("Server");//FAULT_SERVER);
        se.setFaultString(se.GetErrText());
        se.setFaultActor(se.getMyUri());
        se.setDetail(se.GetErrText());
        throw se;
    }

    //check backward_path entry
    if (backwardPath.empty()) {
        //push local spool
        //
        AppLogger::Write(APLOG_DEBUG9, "%s %s",
                         METHOD_LABEL, 
                         "SpoolAdopter::push now");

        string sendMessage;
        sendMessage = msgInfo->messageId;
        sendMessage += ",";
        sendMessage += request.getMessageId();

        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s SpoolAdopter::push sendMessage=[%s]",
                         METHOD_LABEL,
                         sendMessage.c_str());

        SpoolAdopter spoolAdptr;
        string result = spoolAdptr.push(sendMessage);
        
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s SpoolAdopter::push result=[%s]",
                         METHOD_LABEL,
                         result.c_str());
        if (OpenSOAP::Result::SUCCESS != result) {
            AppLogger::Write(APLOG_ERROR, "%s %s",
                             METHOD_LABEL,
                             "spool push failed");
        }

        //return result
        string resStr = 
            createResponseSoapMsgAsResult(result);

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

        DataRetainer dr(srvConf.getSoapMessageSpoolPath());
        dr.SetId(request.getMessageId());
        //set life status
        dr.SetLifeStatus(DataRetainer::KEEP);

    }
    else {
        //has backward_path
        //forwarding to backward_path url
        try {
            //invoker 
            HttpServiceInvoker httpInvoker(srvConf);
            httpInvoker.setEndPointUrl(backwardPath);
            httpInvoker.setTimeoutPeriod(srvConf.getLimitTTLSecond());
            
            //invoke
            httpInvoker.invoke(request, response);
            
            //trace
            //tlog_local.SetComment("RESPONSE_FORWARDING INVOKED");
            //tlog_local.TraceUpdate();
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
    }

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG
}


// End of ResponseSpoolingOrForwardingInvoker.cpp

