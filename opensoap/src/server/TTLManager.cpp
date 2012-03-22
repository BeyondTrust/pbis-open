/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLManager.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include <string>

#include <stdexcept>

#include "StringUtil.h"
#include "MapUtil.h"
#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "TTLManager.h"
#include "TTL.h"
#include "AppLogger.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/ErrorCode.h>

using namespace OpenSOAP;
using namespace std;

//for LOG
static const string CLASS_SIG = "TTLManager";

TTLManager::TTLManager()
{
    static char METHOD_LABEL[] = "TTLManager::TTLManager: ";

    setSocketAddr(TTLMGR_SOCKET_ADDR);

    //edited 2003/08/16
    int ret = OpenSOAPInitialize(NULL);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s=(%d)",METHOD_LABEL
                        ,"OpenSOAPInitialize failed. code",ret);
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL 
//            << "OpenSOAPInitialize failed. code=(" << ret << ")" 
//            << endl;
    }

    //create instance
    ttlPtr = new TTL(TTL_TABLE_FILE);
  
    ttlPtr->watchInterval = 1;
    //ttlPtr->ttlMaxValue = 3600;

#if 0
    //added 2004/04/24
    SoapMessageSerializer::setSoapMessageSpoolPath(
        srvConf.getSoapMessageSpoolPath()
        );
    SoapException::initMyUri();
#endif 
}

TTLManager::~TTLManager()
{
    static char METHOD_LABEL[] = "TTLManager::~TTLManager: ";

    //edited 2003/08/16
    int ret = OpenSOAPUltimate();
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s=(%d)",METHOD_LABEL
                        ,"OpenSOAPUltimate failed. code",ret);
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL 
//            << "OpenSOAPUltimate failed. code=(" << ret << ")" 
//            << endl;
    }

    delete ttlPtr;
}

void 
TTLManager::doProc(int sockfd)
{
    static char METHOD_LABEL[] = "TTLManager::doProc: ";

    //read data
    string readBuf;
    if (0 > read(sockfd, readBuf)) {
        perror(METHOD_LABEL);
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"read failed");
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "read failed" 
//            << endl;
        return;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                        ,"read",readBuf.c_str());
//    cerr << METHOD_LABEL << "read=["
//         << readBuf << "]" << endl;
#endif

    //parse recv. data
    MapUtil readDataMap(OpenSOAP::ChnlCommon::ITEM_DELMIT,
                        OpenSOAP::ChnlCommon::KEY_DELMIT);
    readDataMap.insertItem(readBuf);
  
#ifdef DEBUG
    readDataMap.spy();
#endif /* DEBUG */
  
    //analize data
    string response;  
    MapUtil writeDataMap(OpenSOAP::ChnlCommon::ITEM_DELMIT,
                         OpenSOAP::ChnlCommon::KEY_DELMIT);

    if (!readDataMap.exists(OpenSOAP::ChnlCommon::CMD)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"read failed");
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "read failed" 
//            << endl;
        return;
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
    
        //reply data
        response = writeDataMap.toString();
  
        ChannelDescriptor chnlDesc;
        chnlDesc.setReadFd(sockfd);
        chnlDesc.setWriteFd(sockfd);
        
        if (0 > write(sockfd, response)) {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"write failed");
//            LOGPRINT(TAG_ERR)
//                << METHOD_LABEL
//                << "write failed" << endl;
            return;
        }

        //exec proc.
        ttlPtr->setChnlDesc(chnlDesc);
    
        if (readDataMap.exists(OpenSOAP::ChnlCommon::OPERATION)) {
            
            string opr = readDataMap[OpenSOAP::ChnlCommon::OPERATION];
            if (opr == OpenSOAP::ChnlCommon::PUSH) {
                ttlPtr->execPushOpr();
            }
            else if (opr == OpenSOAP::ChnlCommon::REF) {
                ttlPtr->execRefOpr();
            }
            else {
                AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
                        ,"invalid operation",opr.c_str());
//                LOGPRINT(TAG_ERR)
//                    << METHOD_LABEL
//                    << "invalid operation[" << opr << "]" 
//                    << endl;
            }
        }// end of if (readDataMap.exists(OpenSOAP::ChnlCommon::OPERATION))
    
    }
    else if (OpenSOAP::ChnlCommon::CLOSE == cmd) {
        //release resouce
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"CLOSE proc.");
//        cerr << METHOD_LABEL << "CLOSE proc." << endl;
#endif /* DEBUG */

        string s1(OpenSOAP::ChnlCommon::RESULT);
        string s2(OpenSOAP::ChnlCommon::SUCCESS);
        writeDataMap.insert(make_pair(s1,s2));
        response = writeDataMap.toString();
        
        if (0 > write(sockfd, response)) {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"write failed");
//            LOGPRINT(TAG_ERR)
//                << METHOD_LABEL
//                << "write failed" << endl;
            return;
        }
    }

    //CloseSocket(sockfd);
}
