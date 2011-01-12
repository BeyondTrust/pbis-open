/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolingInvoker.cpp,v $
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
#include "HttpServiceInvoker.h"
#include "SpoolingInvoker.h"
#include "MsgInfo.h"

#include "ServerCommon.h"
#include "SSMLInfoFunc.h"
#include "FileIDFunc.h"
#include "DataRetainer.h"
#include "SrvConfAttrHandler.h"
#include "SpoolAdopter.h"
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
SpoolingInvoker::invoke(SoapMessage& request, SoapMessage& response)
{
//#ifdef DEBUG
    static char METHOD_LABEL[] = "SpoolingInvoker::invoke";
//#endif //DEBUG

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

    //invoke service
    Invoker::invoke(request, response);
    //response not return to caller
    //response spooling 
    //return spooling result message to caller
    
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "service invoke response",
                     response.toString().c_str());

    //add <response_msg> into response
    bool result = attachResponseMsgIntoHeader(response);
    if (!result) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "attach response_msg tag into header failed.");
#if 0
        SoapException se(-1, OPENSOAPSERVER_RUNTIME_ERROR,
                         ERR_ERROR, __FILE__, __LINE__,
                         &request);
        se.SetErrText("async message queueing failed.");
        //fault
        se.setHttpStatusCode(500);
        se.setFaultCode("Server");//FAULT_SERVER);
        se.setFaultString(se.GetErrText());
        se.setFaultActor(se.getMyUri());
        se.setDetail(se.GetErrText());
        throw se;
#endif
    }

    //update message
    //request.deserialize(soapMsg);
    DataRetainer dr(srvConf.getSoapMessageSpoolPath());
    dr.SetId(response.getMessageId());
    dr.SetSoapEnvelope(response.toString());

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s [%s]",
                     METHOD_LABEL, 
                     "add response_msg element",
                     response.toString().c_str());

    //
    string resultStr;
    SoapMessage localResponse;
    if (msgInfo->backwardPath.empty()) {
        //push to Spool
        //
        AppLogger::Write(APLOG_DEBUG9, "%s %s",
                         METHOD_LABEL, 
                         "SpoolAdopter::push now");

        string sendMessage;
        sendMessage = msgInfo->messageId;
        sendMessage += ",";
        sendMessage += response.getMessageId();

        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s SpoolAdopter::push sendMessage=[%s]",
                         METHOD_LABEL,
                         sendMessage.c_str());

        SpoolAdopter spoolAdptr;
        resultStr = spoolAdptr.push(sendMessage);
        
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s SpoolAdopter::push result=[%s]",
                         METHOD_LABEL,
                         resultStr.c_str());
#if 0
        if (OpenSOAP::Result::SUCCESS != result) {
            //fault return
            //exception
            SoapException se(-1, OPENSOAPSERVER_RUNTIME_ERROR,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("async message queueing failed.");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode("Server");//FAULT_SERVER);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }
#endif

        //create local response
        string responseStr = 
            createResponseSoapMsgAsResult(resultStr);

        DataRetainer responseDr(srvConf.getSoapMessageSpoolPath());
        responseDr.Create();
        string responseFile = responseDr.GetHttpBodyFileName();
        ofstream ofst(responseFile.c_str());
        ofst << responseStr << endl;
        ofst.close();
        
        //temp.
        responseDr.AddHttpHeaderElement("content-type", "text/xml;");
        //
        //responseDr.Decompose();

        string responseId;
        responseDr.GetId(responseId);
        responseDr.GetSoapEnvelope(responseStr);

        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                         METHOD_LABEL,
                         "spooling response",
                         responseStr.c_str());

        response.deserialize(responseStr);
        response.setMessageId(responseId);

        //
        //responseDr.Compose();


    } //has backward_path in request
    else {
        //forwarding to backward_path url

        try {
            //invoker 
            HttpServiceInvoker httpInvoker(srvConf);
            httpInvoker.setEndPointUrl(msgInfo->backwardPath);
            httpInvoker.setTimeoutPeriod(srvConf.getLimitTTLSecond());
            
            //invoke
            httpInvoker.invoke(response, localResponse);

            //check localResponse Result(success/failer)
            //debug
            AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                             METHOD_LABEL,
                             "localResponse",
                             localResponse.toString().c_str());
            //check done.
            int ret = deleteFileID(response.getMessageId(),
                                   srvConf.getSoapMessageSpoolPath());
            if (0 != ret) {
                AppLogger::Write(APLOG_ERROR, "%s %s ret=(%d)",
                                 METHOD_LABEL,
                                 "deleteFileID failed",
                                 ret);
            }

            //swap response
            response.deserialize(localResponse.toString());
            response.setMessageId(localResponse.getMessageId());

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
        //resultStr = OpenSOAP::Result::SUCCESS;
    }


    //response.deserialize(resStr);
    //response.setMessageId(resultStr);

    //trace
    tlog_local.SetComment("RESPONSE_SPOOLING_SERVICE_INVOKER INVOKED");
    tlog_local.TraceUpdate();

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG
}

bool 
SpoolingInvoker::attachResponseMsgIntoHeader(SoapMessage& response)
{
    static char METHOD_LABEL[] = "SpoolingInvoker::attachResponseMsgIntoHeader:";
    XmlModifier xmlMod(response.getDoc());
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string val;
    
    int result = 0;
    string baseQuery;

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();

    baseQuery = "/" + OpenSOAP::SoapTag::ENVELOPE;
    baseQuery += "/" + OpenSOAP::SoapTag::HEADER;

    // add header 
    //fmt = "/Envelope/Header=?";    
    fmt = baseQuery + "=?";
    val = "";
    //modified for Duplicate error : <Header>TAG
    //result = xmlMod.attach(fmt, val, NULL);
    result = xmlMod.attachNoDuplicate(fmt, val, NULL);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s] %s %s=(%d)", 
                         METHOD_LABEL,
                         "modify message",
                         fmt.c_str(),
                         "attach failed.",
                         "code", result);
        return false;
    }

    // add header block
    //fmt = "/Envelope/Header/?";
    fmt = baseQuery + "/?";
    //val = "opensoap-header-block";
    val = OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
    //nsDef.href = "http://header.opensoap.jp/1.0/";
    //nsDef.prefix = "opensoap-header";
    nsDef.href = OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER;
    nsDef.prefix = OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER_PREFIX;

    result = xmlMod.attach(fmt, val, &nsDef);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s] %s %s=(%d)", 
                         METHOD_LABEL,
                         "modify message",
                         fmt.c_str(),
                         "attach failed.",
                         "code", result);
        return false;
    }

    //message_id
    baseQuery += "/";
    baseQuery += OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
    baseQuery += "/";

    fmt = baseQuery + OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID;
    fmt += "=?";
    val = msgInfo->messageId;
    
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s attach response_msg fmt=[%s] val=[%s]",
                     METHOD_LABEL,
                     fmt.c_str(),
                     val.c_str());
    //
    result = xmlMod.attach(fmt, val, NULL);
    if (0 != result) {
        AppLogger::Write(APLOG_ERROR, "%s %s %s=(%d)",
                         METHOD_LABEL,
                         "message_id element attach failed",
                         "result", result);
        return false;
    }

    //response_msg
    fmt = baseQuery + OpenSOAP::ExtSoapHeaderTag::RESPONSE_MSG;
    fmt += "=?";
    val = OpenSOAP::BoolString::STR_TRUE;
    
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s attach response_msg fmt=[%s] val=[%s]",
                     METHOD_LABEL,
                     fmt.c_str(),
                     val.c_str());
    //
    result = xmlMod.attach(fmt, val, NULL);
    if (0 != result) {
        AppLogger::Write(APLOG_ERROR, "%s %s %s=(%d)",
                         METHOD_LABEL,
                         "response_msg element attach failed",
                         "result", result);
        return false;
    }

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s %s=(%d) %s=[%s]",
                     METHOD_LABEL,
                     "attach response_msg",
                     "result", result,
                     "msg", response.toString().c_str());

/*
    tlog_local.SetComment("add <response_msg>");
    tlog_local.TraceUpdate();
*/

    return true;
}

// End of SpoolingInvoker.cpp

