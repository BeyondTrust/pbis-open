/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrv.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef MSG_DRV_H
#define MSG_DRV_H

#include <string>
#include <vector>

#include "ThreadDef.h"

//#include "RequestMessage.h"
#include "SSMLInfo.h"

//added 2004/01/04
#include "Rule.h"

//using namespace
#include "ServerDef.h"

//thread function ptr.
typedef ThrFuncReturnType (*ThrFuncPtr)(ThrFuncArgType);

namespace OpenSOAP {
    class ChannelDescriptor;
    class SrvConf;
    class MsgAttrHandler;
//    class RequestMessage;

  
    class MsgDrv {

        //common timer thread functionptr
        typedef int (*InvokeFuncPtr)(MsgDrv*);

        typedef enum { 
            LOCAL_QUEUE, 
            FWD_QUEUE,
        } TargetQueueType;

        typedef enum {
            //for sync
            DIRECT_RETURN,
            //for local async or returned response
            REGIST_SPOOL_AND_RESULT_RETURN,    
            //for forwarded async
            REGIST_FWDQUEUE_AND_RESULT_RETURN, 
        } ResponseProcessingMethodType;

    public:
        Rule* rule;
        void setRule(Rule* aRule) { rule = aRule; }
        
    public:
        MsgDrv(const ChannelDescriptor& chnlDesc, SrvConf& refSrvConf);
        ~MsgDrv ();
        //void setSrvConf(SrvConf* insSrvConf) { srvConf = insSrvConf; }
        void setInvalidMyselfUrl(bool invalid) {invalidMyselfUrl_ = invalid;}

        //-----------------------
        //run() is main proc.
        //-----------------------
        //read fileID of request SOAP message from TransI/F
        //convert fileID to request SOAP message
        //parse request SOAP message
        //decide proc. by parsing
        //  direct access to SrvDrv and Forwarder
        //   or
        //  write fileID of request SOAP message
        //    to Queue Manager(localQ/fwdQ)
        //    to ResponseSpool Manager
        //  read fileID of response SOAP message as a result
        //write fileID of response SOAP message to TransI/F
        //
        //even when error occured, create FaultMessage and convert to fileID
        //write to TransI/F
        int run();
        
    private:
        SrvConf& srvConf_;
        bool invalidMyselfUrl_;

        long invokeTTL_;

        SSMLInfo ssmlInfo;

        //substance SOAP Message
//        RequestMessage requestMessage_;
        //std::string requestSoapMsg;       //[transI/F]->[MsgDrv]->[SrvDrv]
        std::string responseSoapMsg;      //[transI/F]<-[MsgDrv]<-[SrvDrv]
        
        //FileID relate to stored SOAP Message 
        //std::string fileIdOfRequestSoapMsg; //[transI/F]->[MsgDrv]->[SrvDrv]
        std::string fileIdOfResponseSoapMsg;  //[transI/F]<-[MsgDrv]<-[SrvDrv]

        ThrMutexHandle timerMutex_;
        ThrCondHandle timerCond_;

        //common exclusive lock func. ===> ThreadDef
        //void lockMutex(ThrMutexHandle mt);
        //void unlockMutex(ThrMutexHandle mt);

        //check ttl : server.conf/ssml/request-header
        void setInvokeTTL();

#if 0
        //check TTLTable
        bool hasEntryIntoTTLTable(RequestMessage& requet, 
                                  std::string& backwardPath);

        //check hopcount and ttl-hoptimes
        bool isAllowedToForwarding();
        bool isAllowedToInvokeService();

        //common invoke
        int invoke(InvokeFuncPtr invokeFunc);

        //
        //thread functions...
        //
        static
        ThrFuncReturnType processingThreadOnTimer(ThrFuncArgType arg);

        //function ptr.
        InvokeFuncPtr invokeFunc_;
        static
        int srvDrvInvoke(MsgDrv*);
        static
        int fwderInvoke(MsgDrv*);
        static
        int entryLocalQueueInvoke(MsgDrv*);
        static
        int entryFwdQueueInvoke(MsgDrv*);
        static
        int entryResponseInvoke(MsgDrv*);
        static
        int extractResponseInvoke(MsgDrv*);

        ResponseProcessingMethodType responseProcessingMethodType_;
#endif //if 0

        //bool defaultForwarding;
        //bool operationExist;
    
        //communicator with TransI/F
        ChannelDescriptor transIFchnlDesc_;
    
        //unsigned int downsideTimeout, ttl;

        //use for FalutMessage
        //std::string faultMessage;

    
        //used in order to store request method
        //std::vector<std::string> operationNames;
        //std::string operationName;

        //    std::string localURL;
        std::vector<std::string> localURLs;

    private:

        //common use when error occured
        //write fileIdOfResponseSoapMsg to TransI/F after this proc.
        void createFileIdOfResponseSoapMsgAsFaultMessage(const std::string,
                                                         const std::string,
                                                         const std::string);
        void createFileIdOfResponseSoapMsgAsMessageId(const std::string& messageId);
        void createFileIdOfResponseSoapMsgAsResult(const std::string& result);

        //write to TransI/F final proc.
        int returnToTransIF(std::string);

        bool generateAndAttachMessageId();
        int entryQueueAndTTLTable(TargetQueueType qtype,
                                  const std::string& responseFileId = "");

        int attachExtHeaderInfoToResponse(std::string& response);


//-----------------------------------------------------------------
        // regist entry to TTL Table
        bool pushToTTLTable();

        //--- get response message FileID from response spool ---//
        //---    by message_id ---//
        //call case...
        //recv. request message contained message_id from client 
        void popFromResponseSpool();

        //--- push response message by FileID to spool and get result ---//
        //call case...
        // 1) already queuing and poped async request send to SrvDrv
        //    returned response
        // 2) recv. response message returned transmission server
        //    this server final receiver...
        int entryToResponseSpool(const std::string& fileIDOfResponse);

        bool isThisMyselfUrl(std::string targetUrl);
    };
}

#endif /* MSG_DRV_H */
