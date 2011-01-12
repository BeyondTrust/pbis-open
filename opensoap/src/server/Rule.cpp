/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Rule.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <iostream>
#include <sstream>
#include "Rule.h"
#include "RequestQueueingInvoker.h"
#include "ResponseQueryInvoker.h"
#include "SpoolingInvoker.h"
#include "ForwardingInvoker.h"
#include "ResponseSpoolingOrForwardingInvoker.h"

#include "SrvConf.h"
#include "SrvConfAttrHandler.h"
#include "MsgAttrHandler.h"
#include "SSMLInfoFunc.h"
#include "SOAPMessageFunc.h"

#include "XmlModifier.h"
#include "StringUtil.h"
#include "DataRetainer.h"

//for log, exception
#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

#include "SoapDef.h"

using namespace std;
using namespace OpenSOAP;
using namespace OpenSOAPv1_2_00;

//#define DEBUG

extern TraceLog		*tlog;

typedef enum eServiceType {
    DEFAULT_SERVICES,
    ForwardingService,
    AsyncQueueingService, //done
    ServiceInvokeAndResponseSpoolingService,
    AsyncSpoolingService,
    AsyncQueryService,
} ServiceType;


void
Rule::parse(SoapMessage& request, string& serviceMethod, string& serviceNs,
            int& type)
{
    static char METHOD_LABEL[] = "Rule::parse";
    
    const string FAULT_CLIENT("Client");
    const string INVALID_REQUEST("Invalid Request"); //for fault
    
    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
#endif //DEBUG

    //for TraceLog
    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
    string reqSoapMessageId(request.getMessageId());
    msgInfo->SetRequestID(reqSoapMessageId);

#ifdef DEBUG
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    //soap message handler
    MsgAttrHandler msgAttrHndl(request.toString());
    string query;
    vector<string> values;
    vector<string>::iterator itr;

    //query string
    string baseQuery = "/" + OpenSOAP::SoapTag::ENVELOPE;

    //-------------
    //check common
    //-------------
    query = baseQuery + "/??";
    msgAttrHndl.queryXml(query, values);
    if (values.size() <= 0) {
        AppLogger::Write(APLOG_ERROR,"%s %s %s=[%s]",
                         METHOD_LABEL,
                         "<Envelope> does not have child element.",
                         "in msgId", reqSoapMessageId.c_str());
        //return OPENSOAP_PARAMETER_BADVALUE;
        //throw exception
        return;
    }
    else {

        bool hasBody = false;
        bool hasHeader = false;
        bool hasOther = false;
        for(itr = values.begin(); itr != values.end(); itr++) {
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG9,"%s %s<%s>",
                             METHOD_LABEL
                             ,"RequestMessage: <Envelope> has ",
                             (*itr).c_str());
#endif //DEBUG
            if (*itr == OpenSOAP::SoapTag::BODY) {
                hasBody = true;
                //debug
                AppLogger::Write(APLOG_DEBUG9,"%s %s",
                                 METHOD_LABEL,
                                 "check hasBody done");

            }
            else if (*itr == OpenSOAP::SoapTag::HEADER) {
                hasHeader = true;
                //debug
                AppLogger::Write(APLOG_DEBUG9,"%s %s",
                                 METHOD_LABEL,
                                 "check hasHeader done");
            }
            else {
                hasOther = true;
                AppLogger::Write(APLOG_WARN,"%s %s=[%s] %s=[%s]",
                                 METHOD_LABEL
                                 ,"<Envelope> has unexpected child."
                                 ,(*itr).c_str()
                                 ,"in msg_id",reqSoapMessageId.c_str());
            }
        }

        if (!hasBody) {
            //
            AppLogger::Write(APLOG_WARN,"%s %s %s=[%s]",
                             METHOD_LABEL
                             ,"<Envelope> has unexpected child."
                             ,"in msg_id",reqSoapMessageId.c_str());
        }            
        if (hasHeader) {
            //
            msgInfo->hasExtHeader = true;
        }            
        if (hasOther) {
            //
        }            
    }

    //-------------
    //Header parts
    //-------------
    baseQuery += "/";
    baseQuery += OpenSOAP::SoapTag::HEADER;
    baseQuery += "/";

    //check opensoap-header-block
    query = baseQuery + "??";
    
    values.clear();
    msgAttrHndl.queryXml(query, values);
    for (itr = values.begin(); itr != values.end(); itr++) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s]",
                         METHOD_LABEL
                         ,"check opensoap-header-block:"
                         ,"query",query.c_str()
                         ,"value",(*itr).c_str());
#endif //DEBUG

        if (OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK == *itr) {
            msgInfo->hasExtHeaderBlock = true;
            break;
        }
    }

    //
    baseQuery += OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
    baseQuery += "/";

    //check async_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::ASYNC;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        msgInfo->setAsync(values[0]);
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s] %s=[%s]",
                     METHOD_LABEL
                     ,"check async:"
                     ,"query",query.c_str()
                     ,"value",(values.size()>0?values[0].c_str():"<none>")
                     ,"compare",(OpenSOAP::BoolString::STR_TRUE).c_str());
#endif //DEBUG

    //check in
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::IN;
    query += "=?";

    bool hasIn = false;
    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        msgInfo->setIn(values[0]);
        hasIn = true;
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s]",
                     METHOD_LABEL,
                     "check in:",
                     "query", query.c_str(),
                     "value", (values.size()>0?values[0].c_str():"<none>"));

#endif //DEBUG

    //check forwardPathArray_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::FORWARDER;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::FORWARD_PATH;
    query += "=??";
    
    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        //msgInfo->SetForwardPathArray(values);
        msgInfo->forwardPathArray = values;
        msgInfo->hasForwardPath = true;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=(%d)",
                     METHOD_LABEL,
                     "check forward_path:",
                     "query", query.c_str(),
                     "values size", values.size());
    {
        vector<string>::iterator itr;
        for(itr = msgInfo->forwardPathArray.begin(); 
            itr != msgInfo->forwardPathArray.end();
            itr++) {
            AppLogger::Write(APLOG_DEBUG9, "forward_path=[%s]",
                             itr->c_str());
        }
    }
#endif //DEBUG

    //check messageId_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        //msgInfo->SetMessageId(values[0]);
        msgInfo->messageId = values[0];
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s]",
                     METHOD_LABEL,
                     "check message_id:",
                     "query", query.c_str(),
                     "value", (values.size()>0?values[0].c_str():"<none>"));
#endif //DEBUG

    //check hopcount_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::FORWARDER;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::HOPCOUNT;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        int hopCount = 0;
        StringUtil::fromString(values[0], hopCount);
        if (hopCount >= 0) {
            msgInfo->hasHopcount = true;
            msgInfo->hopcount = hopCount;
        }
        else {
            //ignore hopcount
            AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                             ,"hopcount is minus value. ignore it.");
        }
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s]",
                     METHOD_LABEL,
                     "check forwarder hopcount:",
                     "query", query.c_str(),
                     "value", (values.size()>0?values[0].c_str():"<none>"));
#endif //DEBUG
    
    //check receivedPathArray_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::URL;
    query += "=??";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        msgInfo->receivedPathUrlArray = values;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=(%d)",
                     METHOD_LABEL,
                     "check received_path url:",
                     "query", query.c_str(),
                     "values size", values.size());
#endif //DEBUG

    
    //check receivedTimeArray_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::TIME;
    query += "=??";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        msgInfo->receivedPathTimeArray = values;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=(%d)",
                     METHOD_LABEL,
                     "check received_path time:",
                     "query", query.c_str(),
                     "values size", values.size());
#endif //DEBUG

    //check relation of receivedPathArray_ and receivedTimeArray_
    if (msgInfo->receivedPathUrlArray.size() != 
        msgInfo->receivedPathTimeArray.size()) {
        //exception
        SoapException se(-1, OPENSOAP_MEM_OUTOFRANGE,
                         ERR_ERROR, __FILE__, __LINE__,
                         &request);
        se.SetErrText("number of <received_path> children elements <url> and <time> is not same.");
        //fault
        se.setHttpStatusCode(500);
        se.setFaultCode(FAULT_CLIENT);
        se.setFaultString(se.GetErrText());
        se.setFaultActor(se.getMyUri());
        se.setDetail(se.GetErrText());
        throw se;
    }

#ifdef DEBUG
    {
        vector<string>::iterator itr;
        int i = 0;
        for(itr = msgInfo->receivedPathUrlArray.begin(), i=0;
            itr != msgInfo->receivedPathUrlArray.end();
            itr++, i++) {
            AppLogger::Write(APLOG_DEBUG9, "received_path url=[%s] time=[%s]",
                             itr->c_str(),
                             msgInfo->receivedPathTimeArray[i].c_str());
        }
    }
#endif //DEBUG


    //check forwarding loop
    if (isIncludedMyUrlInReceivedPath()) {
        AppLogger::Write(APLOG_DEBUG9,"%s %s",
                         METHOD_LABEL,
                         "isIncludedMyUrlInReceivedPath true.");

        //fault
        //exception
        SoapException se(-1, OPENSOAPSERVER_LOGIC_ERROR,
                         ERR_ERROR, __FILE__, __LINE__,
                         &request);
        se.SetErrText("Routing Loop Detected");
        //fault
        se.setHttpStatusCode(500);
        se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
        se.setFaultString(se.GetErrText());
        se.setFaultActor(se.getMyUri());
        se.setDetail(se.GetErrText());
        throw se;
    }

    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s %s",
                     METHOD_LABEL,
                     "isIncludedMyUrlInReceivedPath false.");

    //check ttlSecond
    //check ttlHoptimes
    //check ttlAsyncSecond
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::TTL;
    query += "=??";

    values.clear();
    //extract ttl array
    msgAttrHndl.queryXml(query, values);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s=(%d)",
                     METHOD_LABEL,
                     "ttl array size", values.size());
#endif //DEBUG
    if (values.size() > 0) {
        long* ttlVal = new long[values.size()];
        vector<string> typeValues;
        query = baseQuery + OpenSOAP::ExtSoapHeaderTag::TTL;
        query += ",";
        query += OpenSOAP::ExtSoapHeaderAttributes::TYPE;
        query += "=??";
        
        msgAttrHndl.queryXml(query, typeValues);
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s %s=(%d)",
                         METHOD_LABEL,
                         "ttl-type array size", typeValues.size());
#endif //DEBUG

        //check miss match size
        if (values.size() != typeValues.size()) {
            AppLogger::Write(APLOG_ERROR,"%s %s",
                             METHOD_LABEL,
                             "extract ttl and type failed.");
            delete[] ttlVal;
            
            //exception
            SoapException se(-1, OPENSOAP_MEM_OUTOFRANGE,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("number of <ttl> elements and type attributes is not same.");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode(FAULT_CLIENT);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }

        //compare type and get each value
        for(int i=0; i<values.size();i++) {
            StringUtil::fromString(values[i], ttlVal[i]);
            if (typeValues[i] == 
                OpenSOAP::ExtSoapHeaderAttributes::SECOND) {
                
                if (ttlVal[i] >0) {
                    msgInfo->hasTTLSecond = true;
                    msgInfo->ttlSecond = ttlVal[i];
#ifdef DEBUG
                    AppLogger::Write(APLOG_DEBUG9,"%s %s=(%ld)",
                                     METHOD_LABEL,
                                     "set ttlSecond_", ttlVal[i]);
#endif //DEBUG
                }
                else {
                    //ignore
                    AppLogger::Write(APLOG_WARN,"%s %s",
                                     METHOD_LABEL,
                                     "ttl second value is minus. ignore it.");
                }
            }
            else if (typeValues[i] == 
                     OpenSOAP::ExtSoapHeaderAttributes::HOPTIMES) {

                if (ttlVal[i] >0) {
                    msgInfo->hasTTLHoptimes = true;
                    msgInfo->ttlHoptimes = ttlVal[i];
#ifdef DEBUG
                    AppLogger::Write(APLOG_DEBUG9,"%s %s=(%ld)",
                                     METHOD_LABEL,
                                     "set ttlHoptimes_", ttlVal[i]);
#endif //DEBUG
                }
                else {
                    //ignore
                    AppLogger::Write(APLOG_WARN,"%s %s",
                                     METHOD_LABEL,
                                     "ttl hoptimes value is minus. ignore it.");
                }
            }
            else if (typeValues[i] == 
                     OpenSOAP::ExtSoapHeaderAttributes::ASYNCSECOND) {
                
                if (ttlVal[i] >0) {
                    msgInfo->hasTTLAsyncSecond = true;
                    msgInfo->ttlAsyncSecond = ttlVal[i];
#ifdef DEBUG
                    AppLogger::Write(APLOG_DEBUG9,"%s %s=(%ld)",
                                     METHOD_LABEL,
                                     "set ttlAsyncSecond_", ttlVal[i]);
#endif //DEBUG
                }
                else {
                    //ignore
                    AppLogger::Write(APLOG_WARN,"%s %s",
                                     METHOD_LABEL,
                                     "ttl second value is minus. ignore it.");
                }
            }
        }
        delete[] ttlVal;
        AppLogger::Write(APLOG_DEBUG9, "delete ttlVal done");
    }

    //check backwardPath
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::BACKWARD_PATH;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        msgInfo->backwardPath = values[0];
    }
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s]",
                     METHOD_LABEL,
                     "check backward_path:",
                     "query", query.c_str(),
                     "value", (values.size()>0?values[0].c_str():"<none>"));
#endif //DEBUG

    //check responseMsg
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::RESPONSE_MSG;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0 && OpenSOAP::BoolString::STR_TRUE == values[0]) {
        msgInfo->isResponseMsg = true;

        //check must item
        if (msgInfo->messageId.empty()) {
            //exe
            //exception
            SoapException se(-1, OPENSOAP_PARAMETER_BADVALUE,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("<response_msg> contained but <message_id> not contained");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode(FAULT_CLIENT);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s]",
                     METHOD_LABEL,
                     "check response_msg:",
                     "query", query.c_str(),
                     "value", (values.size()>0?values[0].c_str():"<none>"));
#endif //DEBUG

    //check undelete
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::UNDELETE;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        msgInfo->setUndelete(values[0]);
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=[%s] %s=[%s]",
                     METHOD_LABEL,
                     "check undelete:",
                     "query", query.c_str(),
                     "value", (values.size()>0?values[0].c_str():"<none>"));
#endif //DEBUG

    //-------------
    //Body parts
    //-------------

    string baseQueryBody;
    baseQueryBody = "/";
    baseQueryBody += OpenSOAP::SoapTag::ENVELOPE;
    baseQueryBody += "/";
    baseQueryBody += OpenSOAP::SoapTag::BODY;
    baseQueryBody += "/";

    //check method
    query = baseQueryBody + "?";

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "Body query",
                     query.c_str());
    
    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {

        msgInfo->methodName = values[0];
        serviceMethod = values[0];

        //check Namespace
        query = baseQueryBody + msgInfo->methodName + "," +
            OpenSOAP::XMLDef::XMLNS + "=?";

        values.clear();
        msgAttrHndl.queryXml(query, values);

        if (values.size() > 0) {
            msgInfo->methodNamespace = values[0];
            serviceNs = values[0];
        }
        else {
            AppLogger::Write(APLOG_WARN,"%s%s %s=[%s] %s=[%s]",METHOD_LABEL
                             ,"method namespace not found."
                             ,"method",msgInfo->methodName.c_str()
                             ,"in msg_id",msgInfo->messageId.c_str());
            //return OPENSOAP_PARAMETER_BADVALUE;
        }

        // check ssml
        SSMLInfo ssmlInfo;
        ssmlInfo.setOperationName(serviceMethod);
        ssmlInfo.setNamespace(serviceNs);
        msgInfo->operationExist = getSSMLInfo(ssmlInfo); 
   }
    else {
        AppLogger::Write(APLOG_WARN,"%s%s %s=[%s]",METHOD_LABEL
                         ,"method not found."
                         ,"in msg_id",msgInfo->messageId.c_str());
        //return OPENSOAP_PARAMETER_BADVALUE;
    }

    //
    //choise type
    //
    //message modify section
    //
    bool messageModified(false);

    if (msgInfo->forwardPathArray.size() > 0) {
#ifdef DEBUG
        tlog_local.SetComment("IN >>Has ForwardPaht Logic");
        tlog_local.TraceUpdate();
#endif //DEBUG
        bool isFinalReceiver
            = checkMyUrlAndPopForwardPathStack(request, messageModified);

        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s %s=(%d) %s=(%d)",
                         METHOD_LABEL,
                         "isFinalReceiver",
                         isFinalReceiver,
                         "messageModified",
                         messageModified);
        
        if (isFinalReceiver) {
            if (msgInfo->operationExist) {
                type = DEFAULT_SERVICES;
                //debug
                AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                                 METHOD_LABEL,
                                 "type",
                                 "DEFAULT_SERVICES");
            }
            else {
                //fault
                //not forwarding
                SoapException se(-1, OPENSOAPSERVER_LOGIC_ERROR,
                                 ERR_ERROR, __FILE__, __LINE__,
                                 &request);
                se.SetErrText("Not Forward as Final Receiver");
                //fault
                se.setHttpStatusCode(500);
                se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
                se.setFaultString(se.GetErrText());
                se.setFaultActor(se.getMyUri());
                se.setDetail(se.GetErrText());
                throw se;
            }
        }
        else {
            type = ForwardingService;
        }

#ifdef DEBUG
        tlog_local.SetComment("OUT <<Has ForwardPaht Logic");
        tlog_local.TraceUpdate();
#endif //DEBUG

    }
    else if (msgInfo->isResponseMsg) {
        type = AsyncSpoolingService;
    }
    else if (msgInfo->async) {
        
        if (msgInfo->in) {

            messageModified = true;

            XmlModifier xmlMod(request.getDoc());
            //<in>
            query = baseQuery + OpenSOAP::ExtSoapHeaderTag::IN;
            query += "=?";
                
            //debug
            AppLogger::Write(APLOG_DEBUG9,"%s modified <in> %s=[%s]",
                             METHOD_LABEL,
                             "query", query.c_str());
            //delete <in>
            xmlMod.del(query);
#ifdef DEBUG
            //trace
            tlog_local.SetComment("del <in>");
#endif //DEBUG

            //trace
            msgInfo->setIn(StringUtil::toString(1));
            tlog_local.SetComment("MODIFIED <in>");
            tlog_local.TraceUpdate();

            if (!msgInfo->operationExist) {
                type = ForwardingService;
            }
            else {
                type = ServiceInvokeAndResponseSpoolingService;
            }
        }
        else {
            //done
            type = AsyncQueueingService;
            messageModified = true;
            
            XmlModifier xmlMod(request.getDoc());

            if (msgInfo->messageId.empty()) {

                //<message_id>
                query = baseQuery + OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID;
                query += "=?";

                //debug
                AppLogger::Write(APLOG_DEBUG9,"%s modified <message_id> %s=[%s]",
                                 METHOD_LABEL,
                                 "query", query.c_str());
                //
                xmlMod.attachNoDuplicate(query, 
                                         reqSoapMessageId,
                                         NULL);
                //trace
                msgInfo->messageId = reqSoapMessageId;
                tlog_local.SetComment("add <message_id>");
                tlog_local.TraceUpdate();
            }

            //<in>
            query = baseQuery + OpenSOAP::ExtSoapHeaderTag::IN;
            query += "=?";
                
            //debug
            AppLogger::Write(APLOG_DEBUG9,"%s modified <in> %s=[%s]",
                             METHOD_LABEL,
                             "query", query.c_str());
            if (hasIn) {
                //In already exists
                xmlMod.update(query, StringUtil::toString(1));
#ifdef DEBUG
                //trace
                tlog_local.SetComment("update <in>");
#endif //DEBUG
            }
            else {
                //In not exists
                xmlMod.attachNoDuplicate(query, 
                                         StringUtil::toString(1), 
                                         NULL);
#ifdef DEBUG
                //trace
                tlog_local.SetComment("add <in>");
#endif //DEBUG
            }
            //trace
            msgInfo->setIn(StringUtil::toString(1));
            tlog_local.SetComment("MODIFIED <in>");
            tlog_local.TraceUpdate();
        }
    }
    else if (!msgInfo->messageId.empty()) {
        type = AsyncQueryService;
    }
    else {
        if (msgInfo->operationExist) {
            type = DEFAULT_SERVICES;
        }
        else {
            type = ForwardingService;
        }
    }

    //common proc.
    if (ForwardingService == type) {
        messageModified = true;
        //check and modified forwarder/hopcount
        bool result = checkAndModifiedForwarderHopcount(request);
        if (!result && !msgInfo->hasForwardPath) {
            SoapException se(-1, OPENSOAP_XML_ERROR,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("Not Forwarding by hopcount limit");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }
        //add received_path
        //temp debug ZZZZZZZZZ
        //cerr <<"+++ befor=[" << request.toString() << "] +++"<<endl;

        result = attachReceivedPath(request);
        if (!result) {
            SoapException se(-1, OPENSOAP_XML_ERROR,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("received_path attach failed");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }

        
        //backward_path
        if (msgInfo->async) {
            result = attachOrUpdateBackwardPath(request);
            if (!result) {
                SoapException se(-1, OPENSOAP_XML_ERROR,
                                 ERR_ERROR, __FILE__, __LINE__,
                                 &request);
                se.SetErrText("backwad_path modified failed");
                //fault
                se.setHttpStatusCode(500);
                se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
                se.setFaultString(se.GetErrText());
                se.setFaultActor(se.getMyUri());
                se.setDetail(se.GetErrText());
                throw se;
            }
        }

        //temp debug ZZZZZZZZZZ
        //cerr <<"+++ after=[" << request.toString() << "] +++"<<endl;
        
#ifdef DEBUG
        //trace
        tlog_local.SetComment("add <received_path>");
#endif //DEBUG
    }

    
    if (messageModified) {
        //update message
        //request.deserialize(soapMsg);
        DataRetainer dr(srvConf.getSoapMessageSpoolPath());
        dr.SetId(reqSoapMessageId);
        dr.SetSoapEnvelope(request.toString());
        if (AsyncQueueingService == type) {
            //set life status
            dr.SetLifeStatus(DataRetainer::KEEP);
            //debug
            AppLogger::Write(APLOG_DEBUG9, "%s %s",
                             METHOD_LABEL,
                             "DataRetainer SetLifeStatus(KEEP)");
        }
    }

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG

    tlog_local.SetComment("MESSAGE PARSED");
    tlog_local.TraceUpdate();

    return;
}
//============================================

Rule::Rule(SrvConf& aSrvConf)
    : RuleBase(aSrvConf)
{
    
}

Rule::~Rule()
{
    
}

void 
Rule::initSession()
{
    //ProcessInfo::GetThreadInfo()->SetMsgInfo(new MsgInfo());
}

void 
Rule::termSession()
{
    //delete ProcessInfo::GetThreadInfo()->GetMsgInfo();
}

void 
Rule::invoke(SoapMessage& request, SoapMessage& response)
{
    static char METHOD_LABEL[] = "Rule::invoke";

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG
    
    string methodName;
    string methodNs;
    int type = 0;

    Invoker* invoker = NULL;

    try {
        parse(request, methodName, methodNs, type);

        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s] %s=[%s] %s=(%d)",
                         METHOD_LABEL,
                         "methodName",
                         methodName.c_str(),
                         "methodNs",
                         methodNs.c_str(),
                         "type",
                         type);

        if (type == DEFAULT_SERVICES) {
            //opensoap original message routing
            invoker = new Invoker(srvConf);
        }
        else if (type == ForwardingService) {
            //default forwarding
            invoker = new ForwardingInvoker(srvConf);
        }
        else if (type == AsyncQueueingService) {
            //request queueing
            invoker = new RequestQueueingInvoker(srvConf);
        }
        else if (type == ServiceInvokeAndResponseSpoolingService) {
            invoker = new SpoolingInvoker(srvConf);
        }

        else if (type == AsyncSpoolingService) {
            //response spooling
            invoker = new ResponseSpoolingOrForwardingInvoker(srvConf);
        }

        else if (type == AsyncQueryService) {
            //response query
            invoker = new ResponseQueryInvoker(srvConf);
        }

        else {
            //Exception e;
        }
        
        invoker->setServiceInfo(methodName, methodNs);
        invoker->invoke(request, response);
        delete invoker;

        //trace
        tlog_local.SetComment("INVOKER INVOKED");
        tlog_local.TraceUpdate();
    }
    catch (SoapException& se) {
        se.AddFuncCallStack();
        //AppLogger::Write(se);
        if (invoker) {
            delete invoker;
        }
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with SoapException <<");
//        tlog_local.TraceUpdate();
#endif //DEBUG
        throw se;
    }
    catch (Exception& e) {
        e.AddFuncCallStack();
        //AppLogger::Write(e);
        if (invoker) {
            delete invoker;
        }
        SoapException se(e);
        se.setSoapHeaderRef(&request);
        se.setHttpStatusCode(500);
        se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
        se.setFaultString(e.GetErrText());
        se.setFaultActor(se.getMyUri());
        se.setDetail(e.GetErrText());
        
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw se;
    }
    catch (...) {
        AppLogger::Write(ERR_ERROR, "unknown error");
        if (invoker) {
            delete invoker;
        }

        SoapException se(-1, OPENSOAPSERVER_UNKNOWN_ERROR,
                         ERR_ERROR, __FILE__, __LINE__,
                         &request);
        se.setHttpStatusCode(500);
        se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
        se.setFaultString("Internal Error");
        se.setFaultActor(se.getMyUri());
        se.setDetail("unknown error occured");

#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with unknown exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw se;
    }

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG

}

bool 
Rule::checkMyUrlAndPopForwardPathStack(SoapMessage& request, bool& modified)
{
    static char METHOD_LABEL[] = "Rule::checkMyUrlAndPopForwardPathStack:";

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
    size_t fwdPathCnt = msgInfo->forwardPathArray.size();
    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s=(%d)",
                     METHOD_LABEL,
                     "fwdPathCnt",
                     fwdPathCnt);
    
    if (0 == fwdPathCnt) {
        return true;
    }
    bool isFinalReceiver(false);
    vector<string>::iterator topFwdPathItr;
    topFwdPathItr = msgInfo->forwardPathArray.begin();
    string topFwdPath(*topFwdPathItr);
    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "topFwdPath",
                     topFwdPath.c_str());
    
    bool isSameAsMyUri = checkIsSameAsMyUrl(topFwdPath);

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s=(%d)",
                     METHOD_LABEL,
                     "isSameAsMyUri",
                     isSameAsMyUri);

    if (isSameAsMyUri) {
        modified = true;
        msgInfo->forwardPathArray.erase(topFwdPathItr);
        if (1 == fwdPathCnt) {
            isFinalReceiver = true;
        }
        //delete first forward_path
        XmlModifier xmlMod(request.getDoc());
        string baseFmt = "/" + 
            OpenSOAP::SoapTag::ENVELOPE + "/" +
            OpenSOAP::SoapTag::HEADER + "/" + 
            OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK + "/" +
            OpenSOAP::ExtSoapHeaderTag::FORWARDER;

        string delFmt = 
            baseFmt + "/" + OpenSOAP::ExtSoapHeaderTag::FORWARD_PATH + "=?";
        
        int ret = xmlMod.del(delFmt);
        if (ret != 0) {
            SoapException se(-1, OPENSOAP_XML_ERROR,
                             ERR_ERROR, __FILE__, __LINE__,
                             &request);
            se.SetErrText("forward_path element remove failed.");
            //fault
            se.setHttpStatusCode(500);
            se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
            se.setFaultString(se.GetErrText());
            se.setFaultActor(se.getMyUri());
            se.setDetail(se.GetErrText());
            throw se;
        }
        //
        if (1 == fwdPathCnt && !msgInfo->hasHopcount) {
            AppLogger::Write(APLOG_DEBUG5, "%s %s",
                             METHOD_LABEL,
                             "delete forwarder header element");

            delFmt = baseFmt + "=?";
            ret = xmlMod.del(delFmt);
            if (ret != 0) {
                SoapException se(-1, OPENSOAP_XML_ERROR,
                                 ERR_ERROR, __FILE__, __LINE__,
                                 &request);
                se.SetErrText("forwarder element remove failed.");
                //fault
                se.setHttpStatusCode(500);
                se.setFaultCode(OpenSOAP::FaultElement::FAULT_SERVER);
                se.setFaultString(se.GetErrText());
                se.setFaultActor(se.getMyUri());
                se.setDetail(se.GetErrText());
                throw se;
            }
        }
    }
    return isFinalReceiver;
}

bool
Rule::attachReceivedPath(SoapMessage& request)
{
    static char METHOD_LABEL[] = "Rule::attachReceivedPath:";

    string receivedPathTime;
    struct tm	logtime;
    struct timeval now_time;
    char	time_buf[32];
    char	ms_str[8];
    
    gettimeofday(&now_time, NULL);
    localtime_r((time_t*)&now_time.tv_sec, &logtime);
    strftime(time_buf, 31, "%Y%m%d%H%M%S", &logtime);
    sprintf(ms_str, "%0.3f", 
            ((float)now_time.tv_usec/(float)1000000.0));
    strcat(time_buf, &ms_str[2]);

    string receivedDateTime = time_buf;
  
    XmlModifier xmlMod(request.getDoc());
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string val;
    
    int result = 0;
    string baseQuery;

    baseQuery = "/" + OpenSOAP::SoapTag::ENVELOPE;
    baseQuery += "/" + OpenSOAP::SoapTag::HEADER;

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();

    //debug
    AppLogger::Write(APLOG_DEBUG, "%s %s[%s] %s[%s]",
                     METHOD_LABEL,
                     "message hasExtHeader",
                     (msgInfo->hasExtHeader?"yes":"no"),
                     "hasExtHeaderBlock",
                     (msgInfo->hasExtHeaderBlock?"yes":"no"));
    
    if (!msgInfo->hasExtHeader) {
        // add header 
        //fmt = "/Envelope/Header=?";    
        fmt = baseQuery + "=?";
        val = "";
        //modified for Duplicate error : <Header>TAG
        //result = xmlMod.attach(fmt, val, NULL);
        result = xmlMod.attachNoDuplicate(fmt, val, NULL);
        if (result != 0) {
            AppLogger::Write(APLOG_DEBUG, "%s %s [%s] %s %s=(%d)", 
                             METHOD_LABEL,
                             "modify message",
                             fmt.c_str(),
                             "attach failed.",
                             "code", result);
        }
    }

    if (!msgInfo->hasExtHeaderBlock) {
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
            AppLogger::Write(APLOG_DEBUG, "%s %s [%s] %s %s=(%d)", 
                             METHOD_LABEL,
                             "modify message",
                             fmt.c_str(),
                             "attach failed.",
                             "code", result);
        }
    }

    //received_path
    baseQuery += "/";
    baseQuery += OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
    fmt = baseQuery + "/?";
    //fmt = "/Envelope/Header/opensoap-header-block/?";
    //val = "received_path";
    val = OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH;
    
    //namespace: if NULL , inherit parent NS
    result = xmlMod.attach(fmt, val, NULL);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s] %s %s=(%d)", 
                         METHOD_LABEL,
                         "modify message",
                         fmt.c_str(),
                         "attach failed.",
                         "code", result);
        return false;
    }

    //----------------------------------
    //simple add
    //----------------------------------
    baseQuery += "/";
    baseQuery += OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH;
    baseQuery += "/";
    fmt = baseQuery + OpenSOAP::ExtSoapHeaderTag::URL;
    fmt += "=?";
    //fmt = "/Envelope/Header/opensoap-header-block/received_path/url=?";
    val = srvConf.getBackwardUrls()[0];

    result = xmlMod.attach(fmt, val);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s] %s %s=(%d)", 
                         METHOD_LABEL,
                         "modify message",
                         fmt.c_str(),
                         "attach failed.",
                         "code", result);
        return false;
    }

    //----------------------------------
    //simple add
    //----------------------------------
    //fmt = "/Envelope/Header/opensoap-header-block/received_path/time=?";
    fmt = baseQuery + OpenSOAP::ExtSoapHeaderTag::TIME;
    fmt += "=?";
    val = receivedDateTime;
    
    result = xmlMod.attach(fmt, val);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s] %s %s=(%d)", 
                         METHOD_LABEL,
                         "modify message",
                         fmt.c_str(),
                         "attach failed.",
                         "code", result);
        return false;
    }
    
    return true;
}

bool
Rule::isIncludedMyUrlInReceivedPath() const
{
    static char METHOD_LABEL[] = "Rule::isIncludedMyUrlInReceivedPath:";

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
    string myUrl = srvConf.getBackwardUrls()[0];

    vector<string>::iterator itr;
    for(itr = msgInfo->receivedPathUrlArray.begin();
        itr != msgInfo->receivedPathUrlArray.end();
        itr++) {
        AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s] %s=[%s]",
                         METHOD_LABEL,
                         "compare received_path",
                         itr->c_str(),
                         "to backward_path[0]",
                         myUrl.c_str());

        if (myUrl == *itr) {
            AppLogger::Write(APLOG_DEBUG9, "%s %s",
                             METHOD_LABEL,
                             "check done. include myUrl in received_path");
            return true;
        }
    }
    AppLogger::Write(APLOG_DEBUG9, "%s %s",
                     METHOD_LABEL,
                     "check done. not include myUrl in received_path");

    return false;
}

bool 
Rule::checkIsSameAsMyUrl(const string& forwardPath) const
{
    static char METHOD_LABEL[] = "Rule::checkIsSameAsMyUrl:";

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "forwardPath",
                     forwardPath.c_str());
    
    //compare targetUrl with various myself urls
    vector<string>::iterator itr;
    vector<string> myUrlAry(srvConf.getBackwardUrls());
    for (itr = myUrlAry.begin(); itr != myUrlAry.end();itr++) {
        //debug
        AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s] %s=[%s]",
                         METHOD_LABEL,
                         "compare <backward><url>",
                         (*itr).c_str(),
                         "to <forward_path>",
                         forwardPath.c_str());
        
        if (forwardPath == *itr) {
            return true;
        }
    }
    return false;
}

bool 
Rule::checkAndModifiedForwarderHopcount(SoapMessage& request)
{
    static char METHOD_LABEL[] = "Rule::checkAndModifiedForwarderHopcount:";

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
    if (msgInfo->hasHopcount) {
        if (msgInfo->hopcount <= 0) {
            //not forwarding
            return false;
        }
        //decrement hopcount
        msgInfo->hopcount--;

        //modified message
        XmlModifier xmlMod(request.getDoc());
        //
        string query = "/" + OpenSOAP::SoapTag::ENVELOPE;
        query += "/";
        query += OpenSOAP::SoapTag::HEADER;
        query += "/";
        query += OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
        query += "/";
        query += OpenSOAP::ExtSoapHeaderTag::FORWARDER;
        query += "/";
        query += OpenSOAP::ExtSoapHeaderTag::HOPCOUNT;
        query += "=?";

        //debug
        AppLogger::Write(APLOG_DEBUG9,"%s modified <hopcount> %s=[%s]",
                         METHOD_LABEL,
                         "query", query.c_str());
        //update hopcount
        xmlMod.update(query, StringUtil::toString(msgInfo->hopcount));

#ifdef DEBUG
        //TraceLog instance
        TraceLog tlog_local(*tlog);
        //trace
        tlog_local.SetComment("decrement <hopcount>");
        tlog_local.TraceUpdate();
#endif //DEBUG

        return true;
    }
    else {
        //no limit
        return true;
    }
}

bool
Rule::attachOrUpdateBackwardPath(SoapMessage& request)
{
    static char METHOD_LABEL[] = "Rule::attachOrUpdateBackwardPath:";

    //-----------------------------------------
    // XmlModifier
    //-----------------------------------------
    XmlModifier xmlMod(request.getDoc());
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string val;
    int result = 0;
    //----------------------------------
    //update
    //----------------------------------
    fmt = "/Envelope/Header/opensoap-header-block/backward_path=?";
    val = srvConf.getBackwardUrls()[0];
  
    result = xmlMod.update(fmt, val);
    if (result != 0) {
        //if update target backward_path not exist, create and attach
        AppLogger::Write(APLOG_DEBUG9, "%s %s %s=(%d)",
                         METHOD_LABEL,
                         "update backward_path failed",
                         "result", result);

        //fmt = "/Envelope/Header/opensoap-header-block/backward_path=?";
        //val = backwardURL;
        result = xmlMod.attach(fmt, val, NULL);
        if (result != 0) {
            AppLogger::Write(APLOG_DEBUG9, "%s %s %s=(%d)",
                             METHOD_LABEL,
                             "attach backward_path failed",
                             "result", result);
            return false;
        }
    }
    return true;
}

// End of Rule.cpp
