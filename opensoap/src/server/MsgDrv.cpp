/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrv.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#include <process.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include <iostream>
#include <stdexcept>
#include <errno.h>

#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "FwdQueuePushChnlSelector.h"
#include "ReqQueuePushChnlSelector.h"
#include "MsgDrv.h"
#include "SignatureFunc.h"
#include "StringUtil.h"
#include "SpoolAdopter.h"
#include "TTLAdopter.h"

/* common library */
#include "connection.h"
#include "FileIDFunc.h"
#include "SOAPMessageFunc.h"
#include "Timeout.h"
#include "SrvConf.h"
#include "XmlModifier.h"

#include "ThreadDef.h"

/* SSML Information Manager library */
#include "SSMLInfoFunc.h"
#include "SSMLInfo.h"

//#include "RequestMessage.h"
//#include "SrvDrv.h"
//#include "Forwarder.h"

#include "DataRetainer.h"
#include "SoapMessage.h"
#include "SoapMessageSerializer.h"
#include "SoapException.h"

#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

//DEBUG
//#define DEBUG

using namespace OpenSOAP;
using namespace std;

//reference object
static const string CLASS_SIG("MsgDrv");
extern TraceLog		*tlog;

int
MsgDrv::run()
{
    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    //-----------------------------------------------------
    int ret = OPENSOAP_NO_ERROR;

    TraceLog tlog_local(*tlog);

#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    //SOAP object
    SoapMessage request;
    SoapMessage response;
    SoapMessageSerializer sms;

    //read FileID from TransI/F
    string fileIdOfRequestSoapMsg;
    if (0 > transIFchnlDesc_.read(fileIdOfRequestSoapMsg)) {
        AppLogger::Write(ERR_ERROR,"%s::%s: %s"
                         ,CLASS_SIG.c_str()
                         ,__func__
                         ,"read from transI/F failed. <<Critical Error>>"
            );
        Exception e(-1, OPENSOAPSERVER_NETWORK_SOCKET_READ_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
#ifdef DEBUG
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw e;
    }

    // try section 
    // - if caught exception, create fault and send id to transport
    try {
        
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s::%s: %s=[%s]"
                         ,CLASS_SIG.c_str()
                         ,__func__
                         ,"READ From TransIF",fileIdOfRequestSoapMsg.c_str());
#endif
        //set requestId to MsgInfo 
        ProcessInfo::GetThreadInfo()->GetMsgInfo()->
            SetRequestID(fileIdOfRequestSoapMsg);

//#ifdef DEBUG
        tlog_local.SetComment("READ REQUEST_ID FROM TRANSPORT");
        tlog_local.TraceUpdate(); //check RequestID 
//#endif //DEBUG

        //import SOAP Request Envelope into object
        sms.deserialize(request, fileIdOfRequestSoapMsg);

        //for debug
        //print request
        AppLogger::Write(APLOG_DEBUG5, 
                         "%s::%s:(%d): deserialized request=[%s]",
                         CLASS_SIG.c_str(),
                         __func__,
                         __LINE__,
                         request.toString().c_str());
        
        //invoke 
        rule->invoke(request, response);
    }
    catch (SoapException& se) {
        se.AddFuncCallStack();
        AppLogger::Write(se);
        sms.serialize(response, se);
    }
    catch (Exception& e) {
        e.AddFuncCallStack();
        AppLogger::Write(e);

        SoapException se(e);
        se.setFaultCode("Server");
        se.setFaultString("Internal Error");
        se.setFaultActor(se.getMyUri());
        se.setDetail(se.GetErrText());
        se.setHttpStatusCode(500);
        sms.serialize(response, se);
    }
    catch (...) {
        AppLogger::Write(APLOG_ERROR, "%s::%s:(%d): unknown exception caught.",
                         CLASS_SIG.c_str(),
                         __func__,
                         __LINE__);
        
        SoapException se(-1, OPENSOAPSERVER_UNKNOWN_ERROR,
                         ERR_ERROR, __FILE__, __LINE__,
                         &request);
        se.setFaultCode("Server");
        se.setFaultString("Internal Error");
        se.setFaultActor(se.getMyUri());
        se.setDetail("Unknow Error");
        se.setHttpStatusCode(500);
        sms.serialize(response, se);
    }

    //for debug
    //print response
    AppLogger::Write(APLOG_DEBUG5, 
                     "%s::%s:(%d): response messageId=[%s] envelope=[%s]",
                     CLASS_SIG.c_str(),
                     __func__,
                     __LINE__,
                     response.getMessageId().c_str(),
                     response.toString().c_str());

    //write responseId to DSOmodule
    //send fileId to TransI/F
    if (0 > transIFchnlDesc_.write(response.getMessageId())) {
        AppLogger::Write(ERR_ERROR,"%s::%s: %s"
                         ,CLASS_SIG.c_str()
                         ,__func__
                         ,": write to TransI/F failed."
            );
        Exception e(-1, OPENSOAPSERVER_NETWORK_SOCKET_WRITE_ERROR,
                    ERR_ERROR, __FILE__, __LINE__);
#ifdef DEBUG
        tlog_local.SetComment("OUT with Exception <<");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw e;
    }

//#ifdef DEBUG
    tlog_local.SetComment("WRITE RESPONSE_ID TO TRANSPORT");
    tlog_local.TraceUpdate();
//#endif //DEBUG


/*
    }
    catch (Exception e) {
        e.AddFuncCallStack();
//        AppLogger::Write(e);
#ifdef DEBUG
        tlog_local.SetComment("Throw Exception");
        tlog_local.TraceUpdate();
#endif //DEBUG
        throw e;
    }
    catch (...) {
        AppLogger::Write(ERR_ERROR, "unknown error");
    }
*/

    //===============================================
    // end proc.
    //===============================================

#ifdef DEBUG
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG

    return OPENSOAP_NO_ERROR;

}// end of run()


//----- Constructor -----//
MsgDrv::MsgDrv(const ChannelDescriptor& chnlDesc, SrvConf& refSrvConf)
    : transIFchnlDesc_(chnlDesc)
    , srvConf_(refSrvConf)
    , invalidMyselfUrl_(false)
//    , requestMessage_(srvConf_.getSoapMessageSpoolPath())
{
#if defined(WIN32)
    timerMutex_ = CreateMutex(NULL, FALSE, NULL);
    timerCond_ = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    pthread_mutex_init(&timerMutex_, NULL);
    pthread_cond_init(&timerCond_, NULL);
#endif

} // end of MsgDrv (const ChannelDescriptor&)

//----- Destructor -----//
MsgDrv::~MsgDrv ()
{
#if defined(WIN32)
    CloseHandle(timerMutex_);
    CloseHandle(timerCond_);
#else
    pthread_mutex_destroy(&timerMutex_);
    pthread_cond_destroy(&timerCond_);
#endif
} // end of ~MsgDrv ()

//=========================================
#if 0
//=========================================

//extract response from ResponseSpoolManager
void
MsgDrv::popFromResponseSpool()
{
    static char METHOD_LABEL[] = "MsgDrv::popFromResponseSpool: ";
    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - send fileIdOfRequestSoapMsg to ResponseSpoolManager
    // - recv fileIdOfResponseSoapMsg from ResponseSpoolManager
    // - return
    //-----------------------------------------------------

    try {
        SpoolAdopter spoolAdptr;
        spoolAdptr.pop(requestMessage_.getStorageId(), 
                       fileIdOfResponseSoapMsg);
    }
    catch (runtime_error re) {
        //create fault message
        createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Internal I/O Error",
            re.what());
        return;
    }
    catch (exception e) {
        //create fault message
        createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Internal Error",
            "");
        return;
    }

    return;
}

//save to TTL Table
bool
MsgDrv::pushToTTLTable()
{
    static char METHOD_LABEL[] = "MsgDrv::pushToTTLTable: ";

    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - send fileId of request to TTLManager
    // - recv result("success"or"failure") from TTLManager
    // - return 
    //          true : result is success
    //          false : result is failure
    //-----------------------------------------------------
    try {
        TTLAdopter ttlAdptr;
        string result = ttlAdptr.push(requestMessage_.getStorageId());
        return (OpenSOAP::Result::SUCCESS == result) ? true : false;        
    }
    catch (runtime_error re) {
        throw re;
    }
    catch (exception e) {
        throw e;
    }

} // end of pushToTTLTable()

//entry request message into ResponseSpool
int
MsgDrv::entryToResponseSpool(const string& fileIdOfResponse)
{
    static char METHOD_LABEL[] = "MsgDrv::entryToResponseSpool: ";

    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - write fileIdOfRequestSoapMsg to ResponseSpoolManager
    // - read fileIdOfResponseSoapMsg from ResponseSpoolManager
    //
    // - even when error occured, create FaultMessage and 
    //   convert to fileIdOfResponseSoapMsg
    //-----------------------------------------------------

    try {
        SpoolAdopter spoolAdptr;
        string result = spoolAdptr.push(fileIdOfResponse);

        if (result != Result::SUCCESS) {
            createFileIdOfResponseSoapMsgAsFaultMessage(
                "Server",
                "Internal Error",
                "Entry to QueueManager Failed");
            return OPENSOAP_FILE_ERROR;
        }
    }
    catch (runtime_error re) {
        throw re;
    }
    catch (exception e) {
        throw e;
    }

    return OPENSOAP_NO_ERROR;
    
}// end of pushToResponseSpool(string fileIdOfResponseSoapMsg)

//check targetUrl shows myself url.
//use for check that first forward_path is myself
bool
MsgDrv::isThisMyselfUrl(string targetUrl)
{
    static char METHOD_LABEL[] = "MsgDrv::isThisMyselfUrl: ";

    //replace targetUrl: "http://xxx:80/yyy" -> "http://xxx/yyy"
    // - localURLs no use :80
    const string 
        DEFAULT_PORT_STR(":" + StringUtil::toString(OpenSOAP::DEFAULT_PORT));
    
    int idx = targetUrl.find(DEFAULT_PORT_STR+"/"); // search ":80/"
    if (string::npos != idx) {
        targetUrl.replace(idx, DEFAULT_PORT_STR.length(), ""); // remove ":80"
    }
  
    //compare targetUrl with various myself urls
    vector<string>::iterator iter;
    for (iter = localURLs.begin(); iter != localURLs.end(); iter++) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                        ,"localUrl",(*iter).c_str());
#endif //DEBUG
        if ((*iter) == targetUrl) {
            //match...targetUrl shows me.
            return true;
        }
    }
    return false;
}

void
MsgDrv::createFileIdOfResponseSoapMsgAsMessageId(const string& messageId)
{
    int ret = 0;
    string response = createResponseSoapMsgAsMessageId(messageId);
    //update current response message
    ret = updateFileIDContent(fileIdOfResponseSoapMsg,
                              response,
                              srvConf_.getSoapMessageSpoolPath());
}

void
MsgDrv::createFileIdOfResponseSoapMsgAsResult(const string& result)
{
    static char METHOD_LABEL[] = 
        "MsgDrv::createFileIdOfResponseSoapMsgAsResult: ";

    int ret = 0;
    string response = createResponseSoapMsgAsResult(result);

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                        ,"response",response.c_str());
#endif //DEBUG

    //update current response message
    ret = updateFileIDContent(fileIdOfResponseSoapMsg,
                              response,
                              srvConf_.getSoapMessageSpoolPath());

}

//create fault message and swap response message content.
void 
MsgDrv::createFileIdOfResponseSoapMsgAsFaultMessage(
    const std::string faultCode,
    const std::string faultString,
    const std::string detail)
{
    int ret = 0;
    //string faultMessage = 
    responseSoapMsg = 
        createSOAPFaultMessage(faultCode,
                               faultString,
                               srvConf_.getBackwardUrls()[0],
                               detail);
    //update current response message
    ret = updateFileIDContent(fileIdOfResponseSoapMsg,
                              responseSoapMsg,
                              //faultMessage,
                              srvConf_.getSoapMessageSpoolPath());
}

//final proc. before return TransI/F
//use fileIdOfResponseSoapMsg
//not extract soap message from fileIdOfResponseSoapMsg->clear responseSoapMsg
int
MsgDrv::returnToTransIF(const string returnPointLabel)
{
    static char METHOD_LABEL[] = "MsgDrv::returnToTransIF: ";

    //MsgDrv finally proc. return To TransI/F
    int ret = OPENSOAP_NO_ERROR;
    
    //add signature proc.
    if (srvConf_.isAddSignatureTrue()) { //check signature is required
        if (responseSoapMsg.empty()) {
            responseSoapMsg 
                = convertToFileID(fileIdOfResponseSoapMsg,
                                  srvConf_.getSoapMessageSpoolPath());
        }
        if (!addSignatureToString(responseSoapMsg,
                                  srvConf_.getSecKeyPath() 
                                  + SECURITY_DATA_PRIV_KEY)){
            AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
                        ,returnPointLabel.c_str()
                        ,": addSignatureToString failed.");
            
            // recreate response as fault and convert to fileId

            createFileIdOfResponseSoapMsgAsFaultMessage(
                "Server.Security",
                "Security Error",
                "signature addition failed");
        }
        else {
            // convert signature added responsemessage to fileId 
            ret = updateFileIDContent(fileIdOfResponseSoapMsg,
                                      responseSoapMsg,
                                      srvConf_.getSoapMessageSpoolPath());
        }
    }

    //send fileId to TransI/F
    if (0 > transIFchnlDesc_.write(fileIdOfResponseSoapMsg)) {
		AppLogger::Write(ERR_ERROR,"%s%s%s"
						,METHOD_LABEL
						,returnPointLabel.c_str()
						,": write to TransI/F failed."
						);
        return OPENSOAP_IO_WRITE_ERROR;
    }
    return OPENSOAP_NO_ERROR;
}

//-----------------------------------------------------------------------
//check TTLTable entry
//-----------------------------------------------------------------------
//use return: in case async message, true is already queuing poped message
//            in case request of popResponse and reversed response, 
//             false is data expired -> create fault of timeout 
//-----------------------------------------------------------------------
//  - return : true : entry exists has message_id
//             false : message not contained message_id or 
//                     entry not exists has message_id
//-----------------------------------------------------------------------
//use backwardPath: in case reversed response
//               - backwardPath is empty : push to ResponseSpool
//               - backwardPath is not empty : forward to backwardPath as url
//-----------------------------------------------------------------------
//  - backwardPath : when entry exists, set backward_path url or empty
//-----------------------------------------------------------------------
bool
MsgDrv::hasEntryIntoTTLTable(RequestMessage& requet, string& backwardPath)
{
    //ref. message_id
    const string& messageId = requestMessage_.getMessageId();
    //when message_id not exists, TTLTable entry not exists too
    if (messageId.empty()) {
        backwardPath = ""; //.clear();
        return false;
    }
    //check using message_id 
    TTLAdopter ttlAdptr;
    return ttlAdptr.ref(messageId, backwardPath);
}



//--------------------------------------------------------------
// Thread Operations
//--------------------------------------------------------------
//thread control operatio
//common invoke
//  refer member : requestMessage and fileIdOfResponseSoapMsg
int 
MsgDrv::invoke(InvokeFuncPtr invokeFunc)
{
    static char METHOD_LABEL[] = "MsgDrv::invoke: ";

    int ret = OPENSOAP_NO_ERROR;

    //set invoke function ptr.
    invokeFunc_ = invokeFunc;
    
    ThrFuncHandle thrId;

#if defined(WIN32)
    DWORD status = DWORD();
    const DWORD TIMEOUT_STATUS = WAIT_TIMEOUT;
    const DWORD NORMAL_STATUS = WAIT_OBJECT_0;
    thrId = (ThrFuncHandle)_beginthread(processingThreadOnTimer, 0, this);
    if (0 >= thrId) {
		AppLogger::Write(ERR_ERROR,"%s=[%s]"
						,"MsgDrv::invoke: _beginthread failed. fileId"
						,requestMessage_.getStorageId().c_str() 
						);
        //exit routing
        throw runtime_error("_beginthread failed");
    }
#else //WIN32
    int status = int();
    const int TIMEOUT_STATUS = ETIMEDOUT;
    const int NORMAL_STATUS = 0;
    status = pthread_create(&thrId, NULL, processingThreadOnTimer, this);
    if (0 != status) {
		AppLogger::Write(ERR_ERROR,"%s%s(%d)%s=[%s]"
						"MsgDrv::invoke: pthread_create failed. "
						"status"
						,status
						," fileId"
						,requestMessage_.getStorageId().c_str() 
						);
        //exit routing
        throw runtime_error("pthread_create failed");
    }
    
    typedef struct timespec TimeSpec;
    TimeSpec ts;
    ts.tv_sec = time(NULL);
    ts.tv_sec += invokeTTL_; //already set
    ts.tv_nsec = 0;

#endif //defined(WIN32)/else

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG7,"%s%s=(%d)"
                    ,"precheck cond_timedwait:"
                    ,"invokeTTL_",invokeTTL_);
#endif //DEBUG

    //exclusive lock
    Thread::lockMutex(timerMutex_);
    
    //wait for cond signal
    //  invoke thread send signal before exit, then returned 0
    //  not complete until invokeTTL, then returned ETIMEDOUT

#if defined(WIN32)
    status = WaitForSingleObject(timerCond_, invokeTTL_*1000);
#else //WIN32
    status = pthread_cond_timedwait(&timerCond_, &timerMutex_, &ts);
#endif //define(WIN32)/else

    //status : 0(WAIT_OBJECT_0) is invoke thread complete
    //         ETIMEDOUT(WAIT_TIMEOUT) is invoke thread incomplete by timeout
    //         other is invoke thread failed
    if (TIMEOUT_STATUS == status) {
        //
        // invoke thread cancel and create timeout fault message
        //
		AppLogger::Write(ERR_INFO,"%s=[%s]"
						"MsgDrv::invoke: timeout occurred. fileId"
						,requestMessage_.getStorageId().c_str() 
						);

        //timeout : cancel thread
#if defined(WIN32)
        status = TerminateThread(thrId, 0);
        if (0 == status) {
			AppLogger::Write(ERR_ERROR,"%s=(%d)"
						"MsgDrv::invoke: TerminateThread failed. status"
						,status 
						);
            //exclusive unlock
            Thread::unlockMutex(timerMutex_);

            //exit routing
            throw runtime_error("pthread_cancel failed");
        }
#else //defined(WIN32)
        status = pthread_cancel(thrId);
        if (0 != status) {
			AppLogger::Write(ERR_ERROR,"%s=(%d)"
						"MsgDrv::invoke: pthread_cancel failed. status"
						,status 
						);
            //exclusive unlock
            Thread::unlockMutex(timerMutex_);

            //exit routing
            throw runtime_error("pthread_cancel failed");
        }
#ifdef DEBUG
        else {
            AppLogger::Write(APLOG_DEBUG7,"pthread_cancel by timeout done");
        }
#endif //DEBUG

#endif //defined(WIN32)/else

        //create timeout fault
        createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server.Timeout",
            "Request Session Not Completed",
            "invoke process timeout");

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG7,"%s=[%s]"
                        ,"response",responseSoapMsg.c_str());
#endif //DEBUG

        //check how to treat response.
        //  directory return
        //  regist into Spool and result return
        //  regist into FwdQueue and result return
        switch (responseProcessingMethodType_) {
        case REGIST_FWDQUEUE_AND_RESULT_RETURN:
            ret = attachExtHeaderInfoToResponse(responseSoapMsg);
            if (OPENSOAP_FAILED(ret)) {
                throw runtime_error("edit ext-header failed");
            }
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG7,"%s%s=[%s]",METHOD_LABEL
                            ,"after attachExtHeader In Fwd response"
                            ,responseSoapMsg.c_str());
            AppLogger::Write(APLOG_DEBUG7,"%s%s=[%s]",METHOD_LABEL
                            ,"fileId",fileIdOfResponseSoapMsg.c_str());
#endif //DEBUG
            
            ret = updateFileIDContent(fileIdOfResponseSoapMsg,
                                      responseSoapMsg,
                                      srvConf_.getSoapMessageSpoolPath());
            if (0 > ret) {
                throw runtime_error("message storage failed");
            }
            ret = entryQueueAndTTLTable(FWD_QUEUE, fileIdOfResponseSoapMsg);
            if (OPENSOAP_FAILED(ret)) {
                break;
            }
            fileIdOfResponseSoapMsg = ""; //.clear();
            createFileIdOfResponseSoapMsgAsResult(Result::SUCCESS);
            break;
        case REGIST_SPOOL_AND_RESULT_RETURN:
            ret = attachExtHeaderInfoToResponse(responseSoapMsg);
            if (OPENSOAP_FAILED(ret)) {
                throw runtime_error("edit ext-header failed");
            }
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG7,"%s%s=[%s]",METHOD_LABEL
                            ,"after attachExtHeader In Spooling response"
                            ,responseSoapMsg.c_str());
#endif //DEBUG

            ret = updateFileIDContent(fileIdOfResponseSoapMsg,
                                      responseSoapMsg,
                                      srvConf_.getSoapMessageSpoolPath());
            if (0 > ret) {
                throw runtime_error("message storage failed");
            }
            ret = entryToResponseSpool(fileIdOfResponseSoapMsg);
            if (OPENSOAP_FAILED(ret)) {
                break;
            }
            fileIdOfResponseSoapMsg = ""; //.clear();
            createFileIdOfResponseSoapMsgAsResult(Result::SUCCESS);
            break;
        case DIRECT_RETURN:
            break;
        default:
            break;
        }
    }
    else if (NORMAL_STATUS != status) {
        //
        // create internalerror fault message
        //
		AppLogger::Write(ERR_ERROR,"%s=(%s)"
						"MsgDrv::invoke: invoke thread failed. fileId"
						,requestMessage_.getStorageId().c_str() 
						);
        
        //exclusive unlock
        Thread::unlockMutex(timerMutex_);
        
        //exit routing
        throw runtime_error("pthread_cond_timedwait failed");
    }

    //exclusive unlock
    Thread::unlockMutex(timerMutex_);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG7,"%s%s",METHOD_LABEL
                    ,"--- pthread_cond_timedwait done. ---");
#endif //DEBUG

    //join thread resource
#if defined(WIN32)
/*
    if (WaitForSingleObject(thrId, INFINITE) != STATUS_WAIT_0) {
        LOGPRINT(TAG_ERR)
            << "MsgDrv::invoke: thread join failed." 
            << endl;
        
        //exit routing
        throw runtime_error("thread join failed");
    }
*/
#else //defined(WIN32)
    status = pthread_join(thrId, NULL);
    if (0 != status) {
		AppLogger::Write(ERR_ERROR,"%s"
						"MsgDrv::invoke: pthread_join failed."
						);

        //exit routing
        throw runtime_error("pthread_join failed");
    }
#endif //defined(WIN32)/else

    return OPENSOAP_NO_ERROR;
}

//------------------
// thread function call from MsgDrv::invoke()
//------------------
ThrFuncReturnType 
MsgDrv::processingThreadOnTimer(ThrFuncArgType arg)
{
    MsgDrv* that = (MsgDrv*)arg;

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG7
                    ,"##### processingThreadOnTimer <<START>> #####");
#endif //DEBUG    

    that->invokeFunc_(that);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG7
                    ,"##### processingThreadOnTimer <<FINISH>> #####");
    AppLogger::Write(APLOG_DEBUG7,"%s=[%s]"
                    ,"##### fileIdOfResponseSoapMsg"
                    ,that->fileIdOfResponseSoapMsg.c_str()
                    ,"#####");
#endif //DEBUG    

    //send signal
#if defined(WIN32)
    SetEvent(that->timerCond_);
#else //defined(WIN32)
    pthread_cond_signal(&(that->timerCond_));
#endif //defined(WIN32)/else

    ReturnThread(NULL);
}

//-------------------------------------
//each next processor invoke operation 
//-------------------------------------

// SrvDrv(binding service)
int 
MsgDrv::srvDrvInvoke(MsgDrv* that)
{
    static char METHOD_LABEL[] = "MsgDrv::srvDrvInvoke: ";

    int ret = OPENSOAP_NO_ERROR;

    //set responseProcessingMetho type
    //if (that->requestMessage_.isAsync()) {}
//2003/08/25
    if (that->requestMessage_.hasTTLEntry()) {
        if (!(that->requestMessage_.getBackwardPathInTTL().empty())) {
            that->responseProcessingMethodType_ = 
                REGIST_FWDQUEUE_AND_RESULT_RETURN;
        }
        else {
            that->responseProcessingMethodType_ = 
                REGIST_SPOOL_AND_RESULT_RETURN;
        }
    }
    else {
        that->responseProcessingMethodType_ = DIRECT_RETURN;
    }

    try {
        // add signature if needed
        if (that->srvConf_.isAddSignatureTrue()) {
            ret = 
              that->requestMessage_.addSignature(that->srvConf_.getSecKeyPath()
                                                 + SECURITY_DATA_PRIV_KEY);
        }
            
        SrvDrv* srvDrv = new SrvDrv(that->srvConf_, that->ssmlInfo);
        srvDrv->setMyselfUrl(that->localURLs[0]);
        ret = srvDrv->invoke(that->requestMessage_, that->responseSoapMsg);
        if (OPENSOAP_FAILED(ret)) {
			AppLogger::Write(ERR_ERROR,"%s%s=(%d)"
							,METHOD_LABEL
							,"SrvDrv::invoke failed. ret"
							,ret
							);
            delete srvDrv;
            throw runtime_error("SrvDrv::invoke failed");
        }
        delete srvDrv;

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG7,"%s%s %s=[%s]",METHOD_LABEL
                    ,"invoke result:","response",that->responseSoapMsg.c_str());
#endif //DEBUG

        //branch
        //hasEntryOfTTLTable
        
        if (that->requestMessage_.isAsync()) {
            ret = that->attachExtHeaderInfoToResponse(that->responseSoapMsg);
            if (OPENSOAP_FAILED(ret)) {
                throw runtime_error("edit ext-header failed");
            }
        }
        //convert to FileId
        that->fileIdOfResponseSoapMsg = 
            convertToFileID(that->responseSoapMsg,
                            that->srvConf_.getSoapMessageSpoolPath());

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG7,"%s%s=[%s]",METHOD_LABEL
                    ,"convrt response to FileID"
                    ,that->fileIdOfResponseSoapMsg.c_str());
#endif //DEBUG

        //async request
        //regist to spool
        //return result

        //async request message poped from queue -> 
        //  response spooling and result reply to Q.M.
        if (that->requestMessage_.isAsync()) {
            //attach message_id into response
            
            //regist to spool
            string result;

            if (that->requestMessage_.hasTTLEntry() && 
                !(that->requestMessage_.getBackwardPathInTTL().empty())) {
                //regist to res spool
                ret = 
                    that->entryQueueAndTTLTable(FWD_QUEUE, 
                                                that->fileIdOfResponseSoapMsg);
            }
            else {
                ret = 
                    that->entryToResponseSpool(that->fileIdOfResponseSoapMsg);
                
            }
            if (OPENSOAP_FAILED(ret)) {
                return ret;
            }
            //release fileId
            that->fileIdOfResponseSoapMsg = ""; //.clear();
            //recreate fileId for response
            that->createFileIdOfResponseSoapMsgAsResult(Result::SUCCESS);
        }
        if (0 != deleteFileID(that->requestMessage_.getStorageId(),
                              that->srvConf_.getSoapMessageSpoolPath())) {
			AppLogger::Write(ERR_ERROR,"%s=[%s]"
				,"request message delete failed. after SrvDrv::invoke id"
				,that->requestMessage_.getStorageId().c_str()
							);
        }
    }
    catch (runtime_error re) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Runtime Internal Error",
            re.what());
    }
    catch (exception e) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Unknown Internal Error",
            "");
    }

    return OPENSOAP_NO_ERROR;
}


// Forwader(forwarding message)
int 
MsgDrv::fwderInvoke(MsgDrv* that)
{
    //check forward allowing
    if (!that->isAllowedToForwarding()) {
        //create fault and return now
        return OPENSOAP_NO_ERROR;
    }

    try {
        Forwarder* fwder = new Forwarder(that->srvConf_, that->ssmlInfo);
        //fwder->setMyselfUrl(that->localURLs[0]);

        //set up endpoint url is givend priority
        vector<string>& fwdPathArray = 
            that->requestMessage_.getForwardPathArray();

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG7,"%s%s=(%d) %s=[%s]"
                    ,"CHECK: fwderInvoke: fwdPathArray:"
					,"size",fwdPathArray.size()
					,"path"
					,((fwdPathArray.size()>0)?fwdPathArray[0].c_str():"(nil)"));
#endif //DEBUG

        if (fwdPathArray.size() > 0) {
            fwder->setForwardPath(fwdPathArray[0]);
        }
        //invoke
        fwder->invoke(that->requestMessage_, that->responseSoapMsg);
        delete fwder;
        //convert msg to fileId
        that->fileIdOfResponseSoapMsg = 
            convertToFileID(that->responseSoapMsg,
                            that->srvConf_.getSoapMessageSpoolPath());
        //when complete request, delete request message
        if (0 != deleteFileID(that->requestMessage_.getStorageId(),
                              that->srvConf_.getSoapMessageSpoolPath())) {
			AppLogger::Write(ERR_ERROR,"%s%s=[%s]"
							,"request message delete failed. "
							,"after Forwarder::invoke id"
							,that->requestMessage_.getStorageId().c_str() 
							);
        }
    }
    catch (runtime_error re) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Internal Error",
            re.what());
        return OPENSOAP_IO_ERROR;
    }
    catch (exception e) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Unknown Internal Error",
            "");
        return OPENSOAP_NOT_CATEGORIZE_ERROR;
    }
    return OPENSOAP_NO_ERROR;
}

// Local Queue Manager(push message)
int 
MsgDrv::entryLocalQueueInvoke(MsgDrv* that)
{
    try {
        //check forward allowing
        if (!that->isAllowedToForwarding()) {
            //create fault and return now
            return OPENSOAP_NO_ERROR;
        }
        
        int ret = OPENSOAP_NO_ERROR;
        //check message_id content and response create 
        bool hasGeneratedNewMessageId = that->generateAndAttachMessageId();
        //regist to queue
        ret = that->entryQueueAndTTLTable(LOCAL_QUEUE);
        if (OPENSOAP_FAILED(ret)) {
            return ret;
        }
        
        if (hasGeneratedNewMessageId) {
            //case new message_id created
            // return message_id
            // convert response to FileId
            that->createFileIdOfResponseSoapMsgAsMessageId(
                that->requestMessage_.getMessageId());
        }
        else {
            //case already message_id contained
            // return success result 
            that->createFileIdOfResponseSoapMsgAsResult(Result::SUCCESS);
        }
        return ret;


    }
    catch (runtime_error re) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Internal Error",
            re.what());
        return OPENSOAP_IO_ERROR;
    }
    catch (exception e) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Unknown Internal Error",
            "");
        return OPENSOAP_NOT_CATEGORIZE_ERROR;
    }

    return OPENSOAP_NO_ERROR;
}

// Forward Queue Manager(push message)
int 
MsgDrv::entryFwdQueueInvoke(MsgDrv* that)
{
    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - check message_id into request soap message
    // -   not contained: create new messge_id and 
    //                    attach to request soap message
    //     already contained: keep one
    // - write fileIdOfRequestSoapMsg to ResponseSpoolManager
    // - read result queuing("success"or"failure") from ResponseSpoolManager
    // -   result is success: create response for TransI/F contained message_id
    //            is failure: create fault message for TransI/F
    //
    // - even when error occured, create FaultMessage and 
    //   convert to fileIdOfResponseSoapMsg
    //-----------------------------------------------------

    try {
        //check forward allowing
        if (!that->isAllowedToForwarding()) {
            //create fault and return now
            return OPENSOAP_NO_ERROR;
        }
        
        int ret = OPENSOAP_NO_ERROR;
        //check message_id content and response create 
        bool hasGeneratedNewMessageId = that->generateAndAttachMessageId();
        //regist to queue
        ret = that->entryQueueAndTTLTable(FWD_QUEUE);
        if (OPENSOAP_FAILED(ret)) {
            return ret;
        }
        
        if (hasGeneratedNewMessageId) {
            //case new message_id created
            // return message_id
            // convert response to FileId
            that->createFileIdOfResponseSoapMsgAsMessageId(
                that->requestMessage_.getMessageId());
        }
        else {
            //case already message_id contained
            // return success result 
            that->createFileIdOfResponseSoapMsgAsResult(Result::SUCCESS);
        }
        return ret;
    }
    catch (runtime_error re) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Internal Error",
            re.what());
        return OPENSOAP_IO_ERROR;
    }
    catch (exception e) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Unknown Internal Error",
            "");
        return OPENSOAP_NOT_CATEGORIZE_ERROR;
    }
}

bool
MsgDrv::generateAndAttachMessageId()
{
    static char METHOD_LABEL[] = "MsgDrv::generateAndAttachMessageId: ";

    int ret = OPENSOAP_NO_ERROR;
    
    if (requestMessage_.getMessageId().empty()) {
        //generate  message_id
        
        //
        //create new <message_id>
        //
        const string DOT(".");
        
        string serverName = getLocalhostName();
        
        //use fileIDOfMsgFromIf for message received timestamp 
        // fileId format is yyyymmddhhmmssxxxxx(=timestamp + 5byte sequence)
        //CAUTION!! if changed create fileID logic, modify this...
        const string& receivedDateTime = requestMessage_.getStorageId();
        
        string messageId = requestMessage_.getMethodName() + DOT +
            getLocalhostName() + DOT + receivedDateTime;
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG7,"%s%s=[%s]",METHOD_LABEL
                    ,"new message_id",messageId.c_str());
#endif //DEBUG        

        ret = requestMessage_.attachMessageIdInHeader(messageId);
        if (OPENSOAP_FAILED(ret)) {
            throw runtime_error("attach message_id failed");
        }

        return true;
    }
    return false;
}

int
MsgDrv::entryQueueAndTTLTable(TargetQueueType qtype, 
                              const string& responseFileId)
{
    static char METHOD_LABEL[] = "MsgDrv::entryQueue: ";

    int ret = OPENSOAP_NO_ERROR;

    if (responseFileId.empty()) {
        ret = requestMessage_.storeStorage();
        if (OPENSOAP_FAILED(ret)) {
            throw runtime_error("data storage fault");
        }

        ret = pushToTTLTable();
        if (!ret) {
            //create fault and return.
            createFileIdOfResponseSoapMsgAsFaultMessage(
                "Server",
                "Internal Error",
                "Entry to TTLManager Failed");
            return OPENSOAP_FILE_ERROR;
        }
    }

    //standby communication with QueueManager
    ChannelSelector* queueEntryChnlSelector = NULL;

    if (qtype == LOCAL_QUEUE) {
        queueEntryChnlSelector = new ReqQueuePushChnlSelector();
    }
    else if (qtype == FWD_QUEUE) {
        queueEntryChnlSelector = new FwdQueuePushChnlSelector();
    }

    if (!queueEntryChnlSelector) {
        //memory fault
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                    ,"ChannelSelector create failed.");
        throw runtime_error("Memory Fault");
    }

    //connection open
    ChannelDescriptor queueEntryChnl;
    if (0 != queueEntryChnlSelector->open(queueEntryChnl)) {
		AppLogger::Write(ERR_ERROR,"%s%s%s"
						,METHOD_LABEL
						,(qtype == LOCAL_QUEUE) ? "Local" : "Fwd"
						," QueueManager connection open failed."
						);
        throw runtime_error("I/O Error");
    }
    
    //send fileId 
    string sendFileId = (responseFileId.empty() ?
                         requestMessage_.getStorageId() :
                         responseFileId);
    if (0 > queueEntryChnl.write(sendFileId)) {
		AppLogger::Write(ERR_ERROR,"%s%s%s%s"
						,METHOD_LABEL
						,"write failed to "
						,(qtype == LOCAL_QUEUE) ? "Local" : "Fwd"
						," QueueManager"
						);
        throw runtime_error("I/O Write Error");
    }
  
    //recv. result from Q.M.
    string result;
    if (0 > queueEntryChnl.read(result)) {
		AppLogger::Write(ERR_ERROR,"%s%s%s%s"
						,METHOD_LABEL
						,"read failed from "
						,(qtype == LOCAL_QUEUE) ? "Local" : "Fwd"
						," QueueManager"
						);
        throw runtime_error("I/O Read Error");
    }
    
#ifdef DEBUG
    AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
                    ,"READ Result From QueueManager",result.c_str());
#endif
    
    //ultimate
    queueEntryChnlSelector->close(queueEntryChnl);
    delete queueEntryChnlSelector;

    if (result != OpenSOAP::Result::SUCCESS) {
        //create fault and return.
        createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Internal Error",
            "Entry to QueueManager Failed");
        return OPENSOAP_FILE_ERROR;
    }


    return OPENSOAP_NO_ERROR;
    
} // End of entryQueueAndTTLTable()


// Response Spool Manager(push message)
int 
MsgDrv::entryResponseInvoke(MsgDrv* that)
{
    int ret = OPENSOAP_NO_ERROR;
    try {
        ret = that->entryToResponseSpool(that->requestMessage_.getStorageId());
        if (OPENSOAP_FAILED(ret)) {
            return ret;
        }
        //recreate fileId for response
        that->createFileIdOfResponseSoapMsgAsResult(Result::SUCCESS);

        //do not delete until pop response from client
#if 0
        if (0 != deleteFileID(that->requestMessage_.getStorageId(),
                              that->srvConf_.getSoapMessageSpoolPath())) {
			AppLogger::Write(ERR_ERROR,"%s%s=[%s]"
							,"request message delete failed."
							," after SrvDrv::invoke id"
							,that->requestMessage_.getStorageId().c_str()
							);
        }
#endif //if 0

    }
    catch (runtime_error re) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Internal Error",
            re.what());
        return OPENSOAP_IO_ERROR;
    }
    catch (exception e) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Unknown Internal Error",
            "");
        return OPENSOAP_NOT_CATEGORIZE_ERROR;
    }

    return OPENSOAP_NO_ERROR;
}

// Response Spool Manager(pop message)
int 
MsgDrv::extractResponseInvoke(MsgDrv* that)
{
    static char METHOD_LABEL[] = "MsgDrv::extractResponseInvoke: ";

    int ret = OPENSOAP_NO_ERROR;
    try {
        that->popFromResponseSpool();
        //clean up request message
        if (0 != deleteFileID(that->requestMessage_.getStorageId(),
                              that->srvConf_.getSoapMessageSpoolPath())) {
			AppLogger::Write(ERR_ERROR,"%s%s%s=[%s]"
							,METHOD_LABEL
							,"request message delete failed."
							," after SrvDrv::invoke id"
							,that->requestMessage_.getStorageId().c_str()
							);
        }
    }
    catch (runtime_error re) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Runtime Internal Error",
            re.what());
    }
    catch (exception e) {
        that->createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server",
            "Unknown Internal Error",
            "");
    }
    return OPENSOAP_NO_ERROR;
}

bool
MsgDrv::isAllowedToForwarding()
{
    static char METHOD_LABEL[] = "MsgDrv::isAllowedToForwarding: ";

    //check : Header::hopcount <= 0 is not allowed

    //check : server.conf::ttl/hoptimes and Header::ttl type=hoptimes
    //  compare with Header::received_path array size 
    //  (ttl::hoptimes) < (received_path array size) is not allowed

    // if not allowed, create fault message and convert fileID

    //1st. check
    if (requestMessage_.hasHopcount() && requestMessage_.getHopcount() == 0) {
        //not allowed
		AppLogger::Write(ERR_INFO,"%s%s%s"
						,METHOD_LABEL
						,"isAllowedToForwarding: Not forward message."
						," limit hopcount=0"
						);

        createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server.LimitHop",
            "Not Forward by Limit of hopcount",
            "");
        return false;
    }
    
    long limitHoptimes = srvConf_.getLimitTTLHoptimes();
    //update limitHoptimes by request-header when smaller than limitHoptimes
    if (requestMessage_.hasTTLHoptimes()) {
        long headerHoptimes = requestMessage_.getTTLHoptimes();
        if (/*headerHoptimes > 0 &&*/ headerHoptimes < limitHoptimes) {
            limitHoptimes = headerHoptimes;
#ifdef DEBUG
            AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                    ,"update limitHoptimes by header"
					,"limitHoptimes",limitHoptimes);
#endif //DEBUG
        }
    }

    //check allowing
    if (limitHoptimes < requestMessage_.getReceivedPathArray().size()) {
        //not allowed
		AppLogger::Write(ERR_INFO,"%s%s%s"
						,METHOD_LABEL
						,"isAllowedToForwarding: Not forward message. "
						,"limit hoptimes < <received_path> count"
						);

        createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server.LimitHop",
            "Not Forward by Limit of ttl-Hoptimes",
            "");
        return false;
    }

    return true;
}

bool
MsgDrv::isAllowedToInvokeService()
{
    static char METHOD_LABEL[] = "MsgDrv::isAllowedToInvokeService: ";
    
    //check : SSML::ttl type=hoptimes and Header::ttl type=hoptimes
    //  compare with Header::received_path array size 
    //  - (ttl::hoptimes) < (received_path array size) is not allowed
    //
    // if not allowed, create fault message and convert fileID

    long limitHoptimes = ssmlInfo.getTTLHoptimes();

    //update limitHoptimes by request-header when smaller than limitHoptimes
    if (requestMessage_.hasTTLHoptimes()) {
        long headerHoptimes = requestMessage_.getTTLHoptimes();
        if (/* headerHoptimes > 0 && */ headerHoptimes < limitHoptimes) {
            limitHoptimes = headerHoptimes;
#ifdef DEBUG
            AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                    ,"update limitHoptimes by header"
					,"limitHoptimes",limitHoptimes);
#endif //DEBUG
        }
    }
    //check allowing
    if (limitHoptimes < requestMessage_.getReceivedPathArray().size()) {
        //not allowed
		AppLogger::Write(ERR_INFO,"%s%s%s"
						,METHOD_LABEL
						,"isAllowedToForwarding: Not forward message. "
						,"limit hoptimes < <received_path> count"
						);

        createFileIdOfResponseSoapMsgAsFaultMessage(
            "Server.LimitHop",
            "Not Invoke Service by Limit of ttl-Hoptimes",
            "");
        return false;
    }

    return true;
}

void 
MsgDrv::setInvokeTTL()
{
    //check ttl

    // 1st. priority in SSML
    invokeTTL_ = ssmlInfo.getSyncTTL();
    // when value is zero, 2nd. priority in server.conf
    if (invokeTTL_ <= 0) {
        invokeTTL_ = srvConf_.getLimitTTLSecond();
    }
    // request-header value smaller than that value
    // update by request-header value
    int headerTTL = requestMessage_.getTTLSecond();
    if (headerTTL > 0 && headerTTL < invokeTTL_) {
        invokeTTL_ = headerTTL;
    }
}

int
MsgDrv::attachExtHeaderInfoToResponse(string& response)
{
    static char METHOD_LABEL[] = "MsgDrv::attachExtHeaderInfoToResponse: ";

    int ret = int();
    //attach message_id into response
    MsgAttrHandler msgAttrHndl(response);
    XmlModifier xmlMod(response);
    vector<string> values;
    string baseFmt = "/Envelope/Header";
    string fmt = baseFmt + "=?";
    values.clear();
    msgAttrHndl.queryXml(fmt, values);
    if (values.empty()) {
        ret = xmlMod.attach(fmt, "", NULL);
        if (ret != 0) {
			AppLogger::Write(ERR_ERROR,"%s%s=(%d)"
						,METHOD_LABEL
						,"Header attach failed. rc"
						,ret
						);
            return OPENSOAP_XML_ERROR;
        }
    }
    
    baseFmt += "/opensoap-header-block";
    fmt = baseFmt + "=?";
    values.clear();
    msgAttrHndl.queryXml(fmt, values);
    if (values.empty()) {
        XmlModifier::NameSpaceDef nsDef;
        nsDef.href = "http://header.opensoap.jp/1.0/";
        nsDef.prefix = "opensoap-header";
        ret = xmlMod.attach(fmt, "", &nsDef);
        if (ret != 0) {
			AppLogger::Write(ERR_ERROR,"%s%s=(%d)"
							,METHOD_LABEL
							,"opensoap-header-block attach failed. rc"
							,ret
							);
            return OPENSOAP_XML_ERROR;
        }
    }
    
    baseFmt += "/";
    fmt = baseFmt + "message_id=?";
    values.clear();
    msgAttrHndl.queryXml(fmt, values);
    if (values.empty()) {
        ret = xmlMod.attach(fmt, requestMessage_.getMessageId(), NULL);
        if (ret != 0) {
			AppLogger::Write(ERR_ERROR,"%s%s=(%d)"
							,METHOD_LABEL
							,"message_id attach failed. rc"
							,ret
							);
            return OPENSOAP_XML_ERROR;
        }
    }
        
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG7,"%s%s=(%d)",METHOD_LABEL
                    ,"forFinalReceiver",requestMessage_.forFinalReceiver());
    AppLogger::Write(APLOG_DEBUG7,"%s%s=(%d)",METHOD_LABEL
                    ,"hasTTLEntry",requestMessage_.hasTTLEntry());
    AppLogger::Write(APLOG_DEBUG7,"%s%s=(%d)",METHOD_LABEL
                    ,"getBackwardPathInTTL"
                    ,requestMessage_.getBackwardPathInTTL().c_str());
#endif //DEBUG

    if (requestMessage_.forFinalReceiver()||
        (requestMessage_.hasTTLEntry()&&
         !(requestMessage_.getBackwardPathInTTL().empty()))) {
        //attach response_msg is true
        // add <response_msg> with value of "true"
        fmt = baseFmt + "response_msg=?";
        string val = "true";
        values.clear();
        msgAttrHndl.queryXml(fmt, values);
        if (values.empty()) {
            ret = xmlMod.attach(fmt, val, NULL);
            if (ret != 0) {
				AppLogger::Write(ERR_ERROR,"%s%s=(%d)"
								,METHOD_LABEL
								,"response_msg attach failed. rc"
								,ret
								);
                return OPENSOAP_XML_ERROR;
            }
        }
    }
    response = xmlMod.toString();
    return OPENSOAP_NO_ERROR;
}

//=========================================
#endif //if 0
//=========================================
// End of MsgDrv.cpp
