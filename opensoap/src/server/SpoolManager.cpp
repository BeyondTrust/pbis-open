/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolManager.cpp,v $
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
#include "SpoolManager.h"
#include "Spool.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/ErrorCode.h>

using namespace OpenSOAP;
using namespace std;

//for LOG
static const std::string CLASS_SIG = "SpoolManager";

SpoolManager::SpoolManager()
{
    static char METHOD_LABEL[] = "SpoolManager::SpoolManager: ";
  
    setSocketAddr(RESSPOOL_SOCKET_ADDR);

    //edited 2003/08/16
    int ret = OpenSOAPInitialize(NULL);
    if (OPENSOAP_FAILED(ret)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL 
            << "OpenSOAPInitialize failed. code=(" << ret << ")" 
            << endl;
    }

    //create instance
    resSpoolPtr = new Spool(RESSPOOL_TABLE_FILE);
}

SpoolManager::~SpoolManager()
{
    static char METHOD_LABEL[] = "SpoolManager::~SpoolManager: ";

    //edited 2003/08/16
    int ret = OpenSOAPUltimate();
    if (OPENSOAP_FAILED(ret)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL 
            << "OpenSOAPUltimate failed. code=(" << ret << ")" 
            << endl;
    }

    delete resSpoolPtr;
}

void 
SpoolManager::doProc(int sockfd)
{
    static char METHOD_LABEL[] = "SpoolManager::doProc: ";

    //-----------------
    //recv data
    //-----------------
    string readBuf;
    if (0 > read(sockfd, readBuf)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL
            << "read failed"
            << endl;
        return;
    }

#ifdef DEBUG
    cerr << METHOD_LABEL << "read=["
         << readBuf << "]" << endl;
#endif
    
    //parse received data
    MapUtil readDataMap(OpenSOAP::ChnlCommon::ITEM_DELMIT,
                        OpenSOAP::ChnlCommon::KEY_DELMIT);
    readDataMap.insertItem(readBuf);
    
#ifdef DEBUG
    readDataMap.spy();
#endif /* DEBUG */
  
    //for response
    string response;  
    MapUtil writeDataMap(OpenSOAP::ChnlCommon::ITEM_DELMIT,
                         OpenSOAP::ChnlCommon::KEY_DELMIT);

    //check recv.data
    if (!readDataMap.exists(OpenSOAP::ChnlCommon::CMD)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL
            << "read invalid data. "
            << OpenSOAP::ChnlCommon::CMD << " not found"
            << endl;
        return;
    }
    string cmd = readDataMap[OpenSOAP::ChnlCommon::CMD];
  
    if (OpenSOAP::ChnlCommon::OPEN == cmd) {
        //normal case
    
        //result
        string s1(OpenSOAP::ChnlCommon::RESULT);
        string s2(OpenSOAP::ChnlCommon::SUCCESS);
        writeDataMap.insert(make_pair(s1,s2));
        //com.type
        s1 = OpenSOAP::ChnlCommon::CHNL_TYPE;
        s2 = OpenSOAP::ChnlCommon::UNIX_SOCKET;
        writeDataMap.insert(make_pair(s1,s2));
        
        //create response
        response = writeDataMap.toString();
  
        //set com.chnl
        ChannelDescriptor chnlDesc;
        chnlDesc.setReadFd(sockfd);
        chnlDesc.setWriteFd(sockfd);

        //-----------------
        //send response
        //-----------------
        if (0 > write(sockfd, response)) {
            LOGPRINT(TAG_ERR)
                << METHOD_LABEL
                << "write failed"
                << endl;
            return;
        }
    
        //exec proc.
        // read main data and write main response
        resSpoolPtr->setChnlDesc(chnlDesc);
    
        //switch cmd case
        if (readDataMap.exists(OpenSOAP::ChnlCommon::OPERATION)) {
            //operation type...
            // push
            // pop
            // expire
            string opr = readDataMap[OpenSOAP::ChnlCommon::OPERATION];
            if (opr == OpenSOAP::ChnlCommon::PUSH) {
                resSpoolPtr->execPushOpr();
            }
            else if (opr == OpenSOAP::ChnlCommon::POP) {
                resSpoolPtr->execPopOpr();
            }
            else if (opr == OpenSOAP::ChnlCommon::EXPIRE) {
                resSpoolPtr->execExpireOpr();
            }
            else {
                LOGPRINT(TAG_ERR)
                    << METHOD_LABEL
                    << "invalid operation[" << opr << "]" 
                    << endl;
            }
        }// end of if (readDataMap.exists(OpenSOAP::ChnlCommon::OPERATION))
    }
    else if (OpenSOAP::ChnlCommon::CLOSE == cmd) {
        //ultimate proc.
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
                << "write failed. result of close cmd."
                << endl;
        }
    }

    //CloseSocket(sockfd);
}
