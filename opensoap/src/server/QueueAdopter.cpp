/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueAdopter.cpp,v $
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

#include "QueueAdopter.h"
#include "ServerCommon.h"
#include "ReqQueuePushChnlSelector.h"
#include "FwdQueuePushChnlSelector.h"

//#define DEBUG

using namespace std;
using namespace OpenSOAP;

QueueAdopter::QueueAdopter()
    : chnl_(0)
    , chnlDesc_(0)
    , qType(LOCAL)
{
}

QueueAdopter::QueueAdopter(const QueueType& type)
    : chnl_(0)
    , chnlDesc_(0)
    , qType(type)
{
}

QueueAdopter::~QueueAdopter()
{
}

//regist entry
string 
QueueAdopter::push(const std::string& fileId)
{
    //connect
    openChnl(QueueAdopter::PUSH);

    //send&recv
    string pushResult;
    invoke(fileId, pushResult);

    //disconnect
    closeChnl();

    return pushResult;
}

//extract entry
string 
QueueAdopter::pop(const std::string& fileId)
{
    //connect
    openChnl(QueueAdopter::POP);

    //send&recv
    string popResult;
    invoke(fileId, popResult);

    //disconnect
    closeChnl();

    return popResult;
}

//extract entry
string 
QueueAdopter::expire(const std::string& fileId)
{
    //connect
    openChnl(QueueAdopter::EXPIRE);

    //send&recv
    string expireResult;
    invoke(fileId, expireResult);

    //disconnect
    closeChnl();

    return expireResult;
}

void
QueueAdopter::openChnl(AccessType type)
{
    //establish connection to TTL manager
    switch (type) {
    case QueueAdopter::PUSH:
        if (qType == LOCAL) {
            chnl_ = new ReqQueuePushChnlSelector();
        }
        else {
            chnl_ = new FwdQueuePushChnlSelector();
        }
        break;
    case QueueAdopter::POP:
        //chnl_ = new ReqQueuePushChnlSelector();
        break;
    default:
        LOGPRINT(TAG_ERR)
            << "QueueAdopter::openChnl: AccessType invalid. type=("
            << type << ")" 
            << endl;
        break;
    }
    //create descriptor
    chnlDesc_ = new ChannelDescriptor();

    if (!chnl_ || !chnlDesc_) {
        LOGPRINT(TAG_ERR)
            << "QueueAdopter::openChnl: "
            << "memory allocate failed."
            << endl;
        throw runtime_error("memory fault");
    }
    
    if (0 != chnl_->open(*chnlDesc_)) {
        LOGPRINT(TAG_ERR)
            << "QueueAdopter::openChnl: "
            << "TTLManager open failed." 
            << endl;
        throw runtime_error("i/o error");
    }
    return;
}


void
QueueAdopter::closeChnl()
{
    int ret = chnl_->close(*chnlDesc_);
    delete chnlDesc_;
    delete chnl_;
    chnlDesc_ = 0;
    chnl_ = 0;
    if (0 != ret) {
        LOGPRINT(TAG_ERR)
            << "~QueueAdopter::closeChnl: "
            << "TTLManager close failed."
            << endl;
        throw runtime_error("i/o error");
    }
    return;
}

void
QueueAdopter::invoke(const string& sendData, string& recvData)
{
    //send.
    if (0 > chnlDesc_->write(sendData)) {
        LOGPRINT(TAG_ERR)
            << "QueueAdopter::invoke: "
            << "write failed to TTLManager"
            << endl;
        throw runtime_error("i/o error");
    }

    //recv.
    if (0 > chnlDesc_->read(recvData)) {
        LOGPRINT(TAG_ERR)
            << "QueueAdopter::invoke: "
            << "read failed from TTLManager"
            << endl;
        throw runtime_error("i/o error");
    }
#ifdef DEBUG
    cerr << "### QueueAdopter::recv=[" << recvData << "] ###" << endl;
#endif //DEBUG

    return;
}

// End of QueueAdopter.cpp
