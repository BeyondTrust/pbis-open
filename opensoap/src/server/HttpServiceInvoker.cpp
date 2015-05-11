/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: HttpServiceInvoker.cpp,v $
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
#include "HttpServiceInvoker.h"

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

/*
#define HTTP_INTERNAL_SERVER_ERROR         500
#define HTTP_NOT_IMPLEMENTED               501
#define HTTP_BAD_GATEWAY                   502
#define HTTP_SERVICE_UNAVAILABLE           503
#define HTTP_GATEWAY_TIME_OUT              504
#define HTTP_VERSION_NOT_SUPPORTED         505
#define HTTP_VARIANT_ALSO_VARIES           506
#define HTTP_INSUFFICIENT_STORAGE          507
#define HTTP_NOT_EXTENDED                  510
*/

using namespace std;
using namespace OpenSOAP;

//#define DEBUG
static string CLASS_SIG("HttpServiceInvoker");
extern TraceLog *tlog;

void 
HttpServiceInvoker::OpenSOAPAPITransport(SoapMessage* request,
                                         SoapMessage* response)
{
    const string FAULT_CLIENT("Client");
    const string FAULT_SERVER("Server");

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

    //------------------------------------------------------------
    DataRetainer dr(srvConf.getSoapMessageSpoolPath());
    dr.SetId(request->getMessageId());
    string contentTypeVal;
    dr.GetHttpHeaderElement("content-type", contentTypeVal);
    string soapActionVal;
    dr.GetHttpHeaderElement("SOAPAction", soapActionVal);
    dr.Compose();


    DataRetainer responseDr(srvConf.getSoapMessageSpoolPath());
    responseDr.Create();

    //pre set id
    string responseId;
    responseDr.GetId(responseId);
    response->setMessageId(responseId);

    AppLogger::Write(ERR_DEBUG, 
                     "content-type=[%s], SOAPAction=[%s]",
                     contentTypeVal.c_str(),
                     soapActionVal.c_str());

    AppLogger::Write(ERR_DEBUG, 
                     "Request-HttpBodyFile=[%s], Response-HttpBodyFile=[%s]",
                     dr.GetHttpBodyFileName().c_str(),
                     responseDr.GetHttpBodyFileName().c_str());
    //------------------------------------------------------------

    /* Pointer to Transport instance */
    OpenSOAPTransportPtr soap_t = NULL;
    /* Pointer to Stream instance */
    OpenSOAPCStdioPtr soap_s = NULL;
    /* File pointers */
    FILE *in_fp = NULL;
    FILE *out_fp = NULL;

    int ret = OPENSOAP_NO_ERROR;
    int tp_status = 0;
    char *content_type = NULL;

    /* Content-type for request HTTP */
    const char *req_charset = NULL;

    /* variables to calculate input file size */
    size_t in_fsize = 0;
    struct stat sb;

    /* *REQUIRED* Open input file as File pointer */
    if (!(in_fp  = fopen(dr.GetHttpBodyFileName().c_str(), "rb"))) {
        AppLogger::Write(ERR_ERROR, "%s: %s file=[%s]",
                         __func__,//METHOD_LABEL,
                         "fopen failed.",
                         dr.GetHttpBodyFileName().c_str());

        Exception e(-1, OPENSOAP_FILEOPEN_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG
        throw e;
    }

    /* *REQUIRED* Get the size of input file */
    if (!fstat(fileno(in_fp), &sb) && sb.st_size > 0) {
        in_fsize = sb.st_size;
#ifdef DEBUG
        cerr << "(" << in_fsize << ")" << endl;
#endif
    } else {
        in_fsize = 0;
#ifdef DEBUG
        cerr << "(Unknown?:" << in_fsize << ")" << endl;
#endif
    }

    /* *REQUIRED* Open output file as File pointer */
    if (!(out_fp  = fopen(responseDr.GetHttpBodyFileName().c_str(), "wb"))) {
        AppLogger::Write(ERR_ERROR, "%s: %s file=[%s]",
                         __func__,//METHOD_LABEL,
                         "fopen failed.",
                         responseDr.GetHttpBodyFileName().c_str());

        Exception e(-1, OPENSOAP_FILEOPEN_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

        throw e;
    }

    /* *REQUIRED* Create OpenSOAP Stream from input and output file */
    ret = OpenSOAPCStdioCreateWithFILEPtr(in_fp, out_fp, &soap_s);
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << " OpenSOAPCStdioCreateWithFILEPtr : " << ret << endl;
    }
#endif

    //close file ptr
    fclose(in_fp);
    fclose(out_fp);

    /* ?REQUIRED? Set OpenSOAP Stream Binary-mode */
    ret = OpenSOAPCStdioSetBinaryMode(soap_s);
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << " OpenSOAPCStdioSetBinaryMode : " << ret << endl;
    }
#endif

    /* *REQUIRED* Create Transort instance */
    ret = OpenSOAPTransportCreate(&soap_t);
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << "OpenSOAPTransportCreate : " << ret << endl;
    }
#endif

    /* *REQUIRED* Set URL for Transport */
    ret = OpenSOAPTransportSetURL(soap_t, endPointUrl.c_str());
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << "OpenSOAPTransportSetURL : " << ret << endl;
    }
#endif

    /* (optional) Set SOAPAction: header in (HTTP) Transport */
    ret = OpenSOAPTransportSetSOAPAction(soap_t, soapActionVal.c_str());
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << "OpenSOAPTransportSetSOAPAction : " << ret << endl;
    }
#endif

    if (!contentTypeVal.empty()) {
        /* ?REQUIRED? Set Content-type: header in (HTTP) Transport */
        ret = OpenSOAPTransportSetContentType(soap_t, contentTypeVal.c_str());
#ifdef DEBUG
        if (OPENSOAP_FAILED(ret)) {
            cerr << "OpenSOAPTransportSetContentType : " << ret << endl;
        }
#endif
    } else if (req_charset) {
        /* (optional) Set Charset in Content-type: */
        ret = OpenSOAPTransportSetCharset(soap_t, req_charset);
#ifdef DEBUG
        if (OPENSOAP_FAILED(ret)) {
            cerr << "OpenSOAPTransportSetCharset : " << ret << endl;
        }
#endif
    }
//-------------------------------------------------------
//for performance check
//    tlog_local.SetComment("BEFORE:INVOKE TRANSPORT url="+endPointUrl);
//    tlog_local.TraceUpdate(mainThreadId);
    struct timeval beforetime = {0, 0};
    gettimeofday(&beforetime, NULL);
//-------------------------------------------------------

    /* *REQUIRED* Invoke Transport using Stream */
    ret = OpenSOAPTransportInvokeStream(soap_t, (OpenSOAPStreamPtr)soap_s,
                                        in_fsize, &tp_status);

//-------------------------------------------------------
//for performance check
//    tlog_local.SetComment("AFTER:INVOKE TRANSPORT url="+endPointUrl);
//    tlog_local.TraceUpdate(mainThreadId);

    struct timeval aftertime = {0, 0};
    gettimeofday(&aftertime, NULL);

    long timeDiffer = (aftertime.tv_sec - beforetime.tv_sec) * 1000000L
        + (aftertime.tv_usec - beforetime.tv_usec);

    char tmpbuf[128];
    snprintf(tmpbuf, sizeof(tmpbuf)-1, "TIME=%.3f msec.", timeDiffer/1000.0);
    string tmpcomment("INVOKE TRANSPORT ");
    tmpcomment += tmpbuf;
    tmpcomment += "url=";
    tmpcomment += endPointUrl;

//-------------------------------------------------------

    if (OPENSOAP_FAILED(ret)) {

        SoapException se(-1, OPENSOAP_TRANSPORT_INVOKE_ERROR,
                         ERR_ERROR, __FILE__, __LINE__,
                         request);

        se.SetErrText("ServiceTransportInvoke failed. url=%s", 
                      endPointUrl.c_str());
        //fault
        se.setFaultString(se.GetErrText());
        se.setHttpStatusCode(500);

        switch (ret) {
/**
 * @def OPENSOAP_TRANSPORT_ERROR		(0x00200000L)
 * @brief Transport Error
 */
        case OPENSOAP_TRANSPORT_ERROR:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Transport Error");
            break;
            
/**
 * @def OPENSOAP_TRANSPORT_INVOKE_ERROR	(0x00210000L)
 * @brief Transport Invoke Error
 */
        case OPENSOAP_TRANSPORT_INVOKE_ERROR:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Transport Invoke Error");
            break;
/**
 * @def OPENSOAP_TRANSPORT_HOST_NOT_FOUND	(0x00210001L)
 * @brief Transport Error - Host Not Found
 *        Maybe DNS error. ---- ADDRINFO ? SOCKET ?
 */
        case OPENSOAP_TRANSPORT_HOST_NOT_FOUND:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Host Not Found");
            break;
/**
 * @def OPENSOAP_TRANSPORT_CONNECTION_REFUSED	(0x00210002L)
 * @brief Transport Error - Connection Refused
 *        No one listening on the remote address
 */
        case OPENSOAP_TRANSPORT_CONNECTION_REFUSED:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Connection Refused");
            break;
/**
 * @def OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT	(0x00210003L)
 * @brief Transport Error - Connection Timeout
 */
        case OPENSOAP_TRANSPORT_CONNECTION_TIMEOUT:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Connection Timeout");
            break;
/**
 * @def OPENSOAP_TRANSPORT_NETWORK_UNREACH	(0x00210004L)
 * @brief Transport Error - Network is unreachable
 */
        case OPENSOAP_TRANSPORT_NETWORK_UNREACH:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Network is unreachable");
            break;
/**
 * @def OPENSOAP_TRANSPORT_HOST_UNREACH	(0x00210005L)
 * @brief Transport Error - Host is unreachable
 */
        case OPENSOAP_TRANSPORT_HOST_UNREACH:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Host is unreachable");
            break;
/**
 * @def OPENSOAP_TRANSPORT_HTTP_ERROR	(0x00220000L)
 * @brief Transport HTTP Error
 */
        case OPENSOAP_TRANSPORT_HTTP_ERROR:
            se.setFaultCode(FAULT_SERVER);
            //se.setFaultActor("");
            se.setDetail("Transport HTTP Error");
            break;

        default:
            se.setFaultCode(FAULT_SERVER);
            se.setFaultActor(se.getMyUri());
            se.setDetail("unknown error");
            break;
        }

        AppLogger::Write(ERR_ERROR, 
                         "%s: %s %s ret=(%x) httpstatus=(%d) EndPointUrl=[%s]",
                         __func__,//METHOD_LABEL,
                         "TransportInvokeStream failed.",
                         se.getDetail().toString().c_str(),
                         ret,
                         tp_status,
                         endPointUrl.c_str());
        throw se;
    }

    //print timestamp
    tlog_local.SetComment(tmpcomment);
    tlog_local.TraceUpdate(mainThreadId);

    //debug
    AppLogger::Write(ERR_DEBUG, "%s: %s done. ret=(%x) HTTP Status=(%d)",
                     __func__,//METHOD_LABEL,
                     "OpenSOAPTransportInvokeStream",
                     ret,
                     tp_status);

    /* Get http status code if needed */
    responseDr.AddHttpHeaderElement("status", StringUtil::toString(tp_status));

    /* Get HTTP Header */
    /* check CONTENT-TYPE */
    ret = OpenSOAPTransportGetHeader(soap_t, "CONTENT-TYPE", &content_type);
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << "OpenSOAPTransportGetHeader : " << ret << endl;
    }
#endif

    //
    responseDr.AddHttpHeaderElement("content-type", content_type);

    /* Close Stream (Disconnect and also close file pointers) */
    ret = OpenSOAPCStdioClose(soap_s);
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << " OpenSOAPCStdioClose : " << ret << endl;
    }
#endif
    /* Free Stream instance */
    ret = OpenSOAPCStdioRelease(soap_s);
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << " OpenSOAPCStdioRelease : " << ret << endl;
    }
#endif

    /* Free Transport instance */
    ret = OpenSOAPTransportRelease(soap_t);
#ifdef DEBUG
    if (OPENSOAP_FAILED(ret)) {
        cerr << " OpenSOAPTransportRelease : " << ret << endl;
    }
#endif

    //---------------------------------------------------------
    //
    //responseDr.AddHttpHeaderElement("content-type", content_type);
    responseDr.Decompose();
    string resStr;
    responseDr.GetSoapEnvelope(resStr);

    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo(mainThreadId)->GetMsgInfo();
    msgInfo->SetResponseID(responseId);

#ifdef DEBUG
    tlog_local.SetComment("get response");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

    //SOAP object update
    response->deserialize(resStr);
    responseDr.Compose();

    //---------------------------------------------------------

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

    return;
}

//================================================
// Http Invoker
//================================================
void
HttpServiceInvoker::invokeImpl(SoapMessage* request, SoapMessage* response)
{
    //static char METHOD_LABEL[] = "HttpServiceInvoker::invokeImpl";
    const string FAULT_SERVER("Server");

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

    int ret = OPENSOAP_NO_ERROR;

    //
    //OpenSOAP-API 
    // in: requestFile, out: responseFile, contentTypeVal
    // okada
    string responseContentTypeVal;
    try {
        OpenSOAPAPITransport(request, response);
    }
    catch (SoapException& se) {
        AppLogger::Write(se);
        se.AddFuncCallStack();

#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with SoapException <<");
        tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

        throw se;
    }
    catch (Exception& e) {
        AppLogger::Write(e);
        e.AddFuncCallStack();

        SoapException se(e);
        se.setSoapHeaderRef(request);
        se.setHttpStatusCode(500);
        se.setFaultCode(FAULT_SERVER);
        se.setFaultString("Service HTTP Transport Error");
        se.setFaultActor(se.getMyUri());
        se.setDetail(e.GetErrText());

#ifdef DEBUG
        //trace
        tlog_local.SetComment("OUT with Exception<<");
        tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

        throw se;
    }

    //for TraceLog
//    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
//    msgInfo->SetResponseID(responseId);

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate(mainThreadId);
#endif //DEBUG

    return;
}

// End of HttpServiceInvoker.cpp

