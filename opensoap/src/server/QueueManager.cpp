/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueManager.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#else
#include <unistd.h>
#endif

#include <iostream>
#include <string>

#include <stdexcept>

#include "StringUtil.h"
#include "MapUtil.h"
#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "QueueManager.h"
#include "Queue.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/ErrorCode.h>

using namespace OpenSOAP;
using namespace std;

//#define DEBUG

//for LOG
static const string CLASS_SIG = "QueueManager";

QueueManager::QueueManager(bool isFwdQueue)
{
    static char METHOD_LABEL[] = "QueueManager::QueueManager: ";

    isFwdQueue_ = isFwdQueue;
    
    setSocketAddr(isFwdQueue_ ? FWDQUEUE_SOCKET_ADDR : REQQUEUE_SOCKET_ADDR);

    //edited 2003/08/16
    int ret = OpenSOAPInitialize(NULL);
    if (OPENSOAP_FAILED(ret)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL 
            << "OpenSOAPInitialize failed. code=(" << ret << ")" 
            << endl;
    }

    //create instance
    //インスタンス生成
    queuePtr = new Queue(isFwdQueue_ ? 
                         FWDQUEUE_TABLE_FILE : REQQUEUE_TABLE_FILE,
                         isFwdQueue_);
}

QueueManager::~QueueManager()
{
    static char METHOD_LABEL[] = "QueueManager::~QueueManager: ";

    //edited 2003/08/16
    int ret = OpenSOAPUltimate();
    if (OPENSOAP_FAILED(ret)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL 
            << "OpenSOAPUltimate failed. code=(" << ret << ")" 
            << endl;
    }

    delete queuePtr;
}

void 
QueueManager::doProc(int sockfd)
{
    static char METHOD_LABEL[] = "QueueManager::doProc: ";

    //read data
    string readBuf;
    if (0 > read(sockfd, readBuf)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL
            << "read failed" << endl;
        return;
    }

#ifdef DEBUG
    cerr << METHOD_LABEL << "read=["
         << readBuf << "]" << endl;
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
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL
            << "read failed" << endl;
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
            LOGPRINT(TAG_ERR)
                << METHOD_LABEL
                << "write failed" << endl;
            return;
        }

        //exec proc.
        queuePtr->setChnlDesc(chnlDesc);

        //switch cmd case
        if (readDataMap.exists(OpenSOAP::ChnlCommon::OPERATION)) {
            //operation type...
            // push
            // expire
            // ref
            string opr = readDataMap[OpenSOAP::ChnlCommon::OPERATION];
#ifdef DEBUG
            cerr << METHOD_LABEL << "read OPERATION=[" 
                 << opr << "]" << endl;
#endif //DEBUG
            if (opr == OpenSOAP::ChnlCommon::PUSH) {
                queuePtr->execPushOpr();
            }
            else if (opr == OpenSOAP::ChnlCommon::EXPIRE) {
                queuePtr->execExpireOpr();
            }
            else if (opr == OpenSOAP::ChnlCommon::REF) {
                //queuePtr->execRefOpr();
            }
            else {
                LOGPRINT(TAG_ERR)
                    << METHOD_LABEL
                    << "invalid operation[" << opr << "]" 
                    << endl;
            }
        }// end of if (readDataMap.exists(OpenSOAP::ChnlCommon::OPERATION))
        else {
            LOGPRINT(TAG_ERR)
                << METHOD_LABEL
                << "read failed. operation not found"
                << endl;
        }
    }
    else if (OpenSOAP::ChnlCommon::CLOSE == cmd) {
        //release resouce
#ifdef DEBUG
        cerr << METHOD_LABEL << "CLOSE proc." << endl;
#endif /* DEBUG */
    
        string s1(OpenSOAP::ChnlCommon::RESULT);
        string s2(OpenSOAP::ChnlCommon::SUCCESS);
        writeDataMap.insert(make_pair(s1,s2));
        response = writeDataMap.toString();
  
        if (0 > write(sockfd, response)) {
            LOGPRINT(TAG_ERR)
                << METHOD_LABEL
                << "write failed" << endl;
            return;
        }
    }

    //CloseSocket(sockfd);
}
