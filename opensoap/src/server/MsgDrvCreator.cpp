/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrvCreator.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#include <iostream>
#include <stdexcept>
#include <string>

#include "StringUtil.h"
#include "MapUtil.h"
#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "MsgDrvCreator.h"
#include "MsgDrv.h"
#include "Timeout.h"
#include "SoapMessageSerializer.h"

#include "AppLogger.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"
#include "TraceLog.h"

//using namespace
#include "ServerDef.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/ErrorCode.h>

using namespace OpenSOAP;
using namespace std;

extern TraceLog		*tlog;
/*
  MsgDrvCreator::
*/

//for LOG
static const std::string CLASS_SIG = "MsgDrvCreator";

//#define DEBUG

MsgDrvCreator::MsgDrvCreator()
    : invalidMyselfUrl_(false)
{
    static char METHOD_LABEL[] = "MsgDrvCreator::MsgDrvCreator: ";

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s::%s"
					,CLASS_SIG.c_str(),CLASS_SIG.c_str());
#endif /* DEBUG */  
  
    setSocketAddr(MSGDRV_SOCKET_ADDR);
  
    //edited 2003/06/16
    int ret = OpenSOAPInitialize(NULL);
    if (OPENSOAP_FAILED(ret)) {
		AppLogger::Write(ERR_ERROR,"%s%s=(%d)"
						,METHOD_LABEL
						,"OpenSOAPInitialize failed. code"
						,ret
						);
    }

    //check backward_path[0] using myself url to reverse from forwading host
    vector<string> urls = srvConf.getBackwardUrls();
      
    //myself urls not found or invalid urls[0]
    if (urls.size() <= 0 || !checkUrl(urls[0]) ) {
		AppLogger::Write(ERR_WARN,"%s%s%s=(%s)"
						,METHOD_LABEL
						,"Init check: "
						,"server.conf::backward_path[0] is invalid url. url"
						,((urls.size() > 0) ? urls[0].c_str() : "(none)") 
						);
    }
#ifdef DEBUG
    else {
		AppLogger::Write(APLOG_DEBUG9,"%s%s %s=[%s] %s",METHOD_LABEL
						,"Init Check:","myself url",urls[0].c_str(),"ok!");
    }
#endif //DEBUG


    //added 2004/01/04 //temp.
    rule = new Rule(srvConf);
    //added 2004/01/28
    SoapMessageSerializer::setSoapMessageSpoolPath(
        srvConf.getSoapMessageSpoolPath()
        );
    SoapException::initMyUri();
}

MsgDrvCreator::~MsgDrvCreator()
{
    static char METHOD_LABEL[] = "MsgDrvCreator::~MsgDrvCreator: ";

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s::~%s"
					,CLASS_SIG.c_str(),CLASS_SIG.c_str());
#endif /* DEBUG */  

    //edited 2003/06/16
    int ret = OpenSOAPUltimate();
    if (OPENSOAP_FAILED(ret)) {
		AppLogger::Write(ERR_WARN,"%s%s=(%d)"
						,METHOD_LABEL
						,"OpenSOAPUltimate failed. code"
						,ret
						);
    }

    //added 2004/01/04
    delete rule;
}

//transport connect exec for check myself url is valied.
//this method exec on timeout checked thread 
ThrFuncReturnType
MsgDrvCreator::timeredConnectCheck(ThrFuncArgType arg) 
{
    static char METHOD_LABEL[] = "MsgDrvCreator::timeredConnectCheck: ";

    MsgDrvCreator* that = (MsgDrvCreator*)arg;
    int ret = OPENSOAP_NO_ERROR;

    //init. value is invalid
    //set valid after connect/disconnect check passed.
    that->invalidMyselfUrl_ = true;

    ret = OpenSOAPTransportConnect(that->transport);
    if (OPENSOAP_FAILED(ret)) {
		AppLogger::Write(ERR_WARN,"%s%s"
						,METHOD_LABEL
						,"OpenSOAPTransportConnect failed."
						);
        ReturnThread(NULL);
    }
#ifdef DEBUG
    else {
		AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
						,"OpenSOAPTransportConnect() done.");
    }
#endif //DEBUG
    
    ret = OpenSOAPTransportDisconnect(that->transport);
    if (OPENSOAP_FAILED(ret)) {
		AppLogger::Write(ERR_ERROR,"%s%s"
						,METHOD_LABEL
						,"OpenSOAPTransportDisonnect failed."
						);
        ReturnThread(NULL);
    }
#ifdef DEBUG
    else {
		AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
						,"OpenSOAPTransportDisconnect() done.");
    }
#endif //DEBUG

    //check passed.
    that->invalidMyselfUrl_ = false;

    ReturnThread(NULL);
}

//check myself url is valid. 
//myself url use for received_path or so 
bool
MsgDrvCreator::checkUrl(const string& url)
{
    static char METHOD_LABEL[] = "MsgDrvCreator::checkUrl: ";

    //invalidMyselfUrl_ init. value is true
    //all check proc. passed set false value to invalidMyselfUrl_
    myselfUrl_ = url;
    
    //prepare connect check
    int ret = OPENSOAP_NO_ERROR;
    ret = OpenSOAPTransportCreate(&transport);
    if (OPENSOAP_FAILED(ret)) {
		AppLogger::Write(ERR_ERROR,"%s%s"
						,METHOD_LABEL
						,"checkMyselfUrl: OpenSOAPTransportCreate() failed."
						);
        return false;
    }

#ifdef DEBUG
    else {
		AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
						,"OpenSOAPTransportCreate() done.");
    }
#endif //DEBUG
    
    ret = OpenSOAPTransportSetURL(transport, myselfUrl_.c_str());
    if (OPENSOAP_FAILED(ret)) {
		AppLogger::Write(ERR_ERROR,"%s%s=[%s]"
						,METHOD_LABEL
						,"checkMyselfUrl: OpenSOAPTransportSetURL() failed. URL"
						,myselfUrl_.c_str()
						);
        return false;
    }

#ifdef DEBUG
    else {
		AppLogger::Write(APLOG_DEBUG9,"%s%s %s=[%s]",METHOD_LABEL
						,"OpenSOAPTransportSetURL() done.","URL",url.c_str());
    }
#endif //DEBUG

    //exec transport connect proc on timeout checked thread
    int status = 0;
#if defined(WIN32)
    HANDLE thread_t = (HANDLE)_beginthread(timeredConnectCheck, 0, this);
#else
    pthread_t thread_t;
    status = pthread_create(&thread_t, NULL, timeredConnectCheck, this);
#endif

    //when checkUrl thread not returned until 2 second, url is invalid
    //create timer 
    OpenSOAP::Timeout timeout(2, thread_t);

    //start timer
    timeout.start();

    //join thread branch
#if defined(WIN32)
    if (WaitForSingleObject(thread_t, INFINITE) != STATUS_WAIT_0) {
		AppLogger::Write(ERR_ERROR,"%s%s"
						,METHOD_LABEL
						,"Init check: checkUrl thread join error"
						);
    }
#else //else WIN32
    status = pthread_join(thread_t, NULL);
    if (status != 0) {
		AppLogger::Write(ERR_ERROR,"%s%s"
						,METHOD_LABEL
						,"Init check: checkUrl pthread join error"
						);
    }
#endif

    //stop timer
    timeout.finish();

#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s %s=(%d)",METHOD_LABEL
						,"timer finish checkUrl:","invalidMyselfUrl_"
						,invalidMyselfUrl_);
#endif

    OpenSOAPTransportRelease(transport);

    //check status of invalidMyselfUrl_ after thread
    return !(invalidMyselfUrl_);
}


//main proc 
void 
MsgDrvCreator::doProc(int sockfd)
{
    static char METHOD_LABEL[] = "MsgDrvCreator::doProc: ";

    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(CLASS_SIG + __func__);
#endif //DEBUG

//-------------------------------------------------------
//for performance check
    tlog_local.SetComment("IN:OPENSOAP_SERVER");
    //tlog_local.TraceWrite();
    tlog_local.TraceUpdate();
//-------------------------------------------------------

    try {

#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s %s=(%d)",METHOD_LABEL
						,"doProc()","arg",sockfd);
#endif /* DEBUG */

        //read command data from client
        std::string readBuf;
        if (0 > read(sockfd, readBuf)) {
			AppLogger::Write(ERR_ERROR,"%s%s"
							,METHOD_LABEL
							,"read cmd data failed from TransI/F"
							);
            Exception e(-1, OPENSOAPSERVER_NETWORK_SOCKET_READ_ERROR,
                        ERR_ERROR, __FILE__, __LINE__);
//            e.SetErrText("read failed");
            throw e;
            //return;
        }

#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
						,"read",readBuf.c_str());
#endif
        
        //convert received command to map data
        MapUtil readDataMap(OpenSOAP::ChnlCommon::ITEM_DELMIT,
                            OpenSOAP::ChnlCommon::KEY_DELMIT);
        readDataMap.insertItem(readBuf);
        
#ifdef DEBUG
        readDataMap.spy();
#endif /* DEBUG */
        
        //create return command
        std::string response;  
        MapUtil writeDataMap(OpenSOAP::ChnlCommon::ITEM_DELMIT,
                             OpenSOAP::ChnlCommon::KEY_DELMIT);

        //parse received command
        if (!readDataMap.exists(OpenSOAP::ChnlCommon::CMD)) {
			AppLogger::Write(ERR_ERROR,"%s%s%s"
							,METHOD_LABEL
							,"read invalid data from TransI/F. "
							,"CMD item not found"
							);
            Exception e(-1, OPENSOAP_UNSUPPORT_PROTOCOL,
                        ERR_ERROR, __FILE__, __LINE__);
            e.SetErrText("invalid cmd");
            throw e;
            //return;
        }
        string cmd = readDataMap[OpenSOAP::ChnlCommon::CMD];

        if (OpenSOAP::ChnlCommon::OPEN == cmd) {
            //normal case
            
            //result
            string s1(OpenSOAP::ChnlCommon::RESULT);
            string s2(OpenSOAP::ChnlCommon::SUCCESS);
            writeDataMap.insert(make_pair(s1,s2));
            //communication type
            s1 = OpenSOAP::ChnlCommon::CHNL_TYPE;
            s2 = OpenSOAP::ChnlCommon::UNIX_SOCKET;
            writeDataMap.insert(make_pair(s1,s2));

            //convert command to string data for return
            response = writeDataMap.toString();

            //create descriptor for data communication 
            ChannelDescriptor chnlDesc;
            chnlDesc.setReadFd(sockfd);
            chnlDesc.setWriteFd(sockfd);

            //return result of initial command
            if (0 > write(sockfd, response)) {
				AppLogger::Write(ERR_ERROR,"%s%s"
							,METHOD_LABEL
							,"write result of cmd failed to TransI/F"
							);
                Exception e(-1, OPENSOAPSERVER_NETWORK_SOCKET_WRITE_ERROR,
                            ERR_ERROR, __FILE__, __LINE__);
//                e.SetErrText("write failed");
                throw e;
                //return;
            }

            //init rule for each thread
            rule->initSession();
            //create instance and call method
            MsgDrv* msgDrv = new MsgDrv(chnlDesc, srvConf);
            try {
                msgDrv->setInvalidMyselfUrl(invalidMyselfUrl_);
            
                //added 2004/01/04
                msgDrv->setRule(rule);
                //msgDrv->setSrvConf(&srvConf);
                int ret = msgDrv->run();
                if (OPENSOAP_FAILED(ret)) {
                    AppLogger::Write(ERR_ERROR,"%s%s"
                                     ,METHOD_LABEL
                                     ,"MsgDrv::run failed."
                        );
                    Exception e(-1, OPENSOAPSERVER_NETWORK_ERROR,
                                ERR_ERROR, __FILE__, __LINE__);
                    e.SetErrText("method failed");
                    throw e;
                }
            }
            catch (Exception& e) {
                e.AddFuncCallStack();
                AppLogger::Write(e);
            }
            delete msgDrv;
            //term rule for each thread
            rule->termSession();
            
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
						,"msgDrv->run() Finish");
#endif
        }
        else if (OpenSOAP::ChnlCommon::CLOSE == cmd) {
            //release communication resource 
            
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"CLOSE proc.");
#endif /* DEBUG */
            
            string s1(OpenSOAP::ChnlCommon::RESULT);
            string s2(OpenSOAP::ChnlCommon::SUCCESS);
            writeDataMap.insert(make_pair(s1,s2));
            response = writeDataMap.toString();
            
            if (0 > write(sockfd, response)) {
				AppLogger::Write(ERR_ERROR,"%s%s"
							,METHOD_LABEL
							,"write result of close cmd failed to TransI/F"
							);
                Exception e(-1, OPENSOAPSERVER_NETWORK_SOCKET_WRITE_ERROR,
                            ERR_ERROR, __FILE__, __LINE__);
//                e.SetErrText("write failed");
                throw e;
                //return;
            }
        }
        
        //CloseSocket(sockfd);

    }
    catch (Exception e) {
        e.AddFuncCallStack();
        AppLogger::Write(e);
    }
    catch (...) {
        AppLogger::Write(ERR_ERROR, "unknown error");
    }

//-------------------------------------------------------
//for performance check
    tlog_local.SetComment("OUT:OPENSOAP_SERVER");
    //tlog_local.TraceWrite();
    tlog_local.TraceUpdate();
//-------------------------------------------------------


    return;
}

// End of MsgDrvCreator.cpp
