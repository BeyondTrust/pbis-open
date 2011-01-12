/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLAdopter.cpp,v $
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

#include "TTLAdopter.h"
#include "ServerCommon.h"
#include "TTLTablePushChnlSelector.h"
#include "TTLTableRefChnlSelector.h"

#include "AppLogger.h"

//#define DEBUG

using namespace std;
using namespace OpenSOAP;

TTLAdopter::TTLAdopter()
    : chnl_(0)
    , chnlDesc_(0)
{
}

TTLAdopter::~TTLAdopter()
{
}

//regist entry
string 
TTLAdopter::push(const std::string& fileId)
{
    //connect
    openChnl(TTLAdopter::PUSH);

    //send&recv
    string pushResult;
    invoke(fileId, pushResult);

    //disconnect
    closeChnl();

    return pushResult;
}

//check existance of entry and extract backward_path
//  entry not exists: return false
//  entry exists: return true and set backwardPath(=url or empty)
bool 
TTLAdopter::ref(const std::string& messageId, std::string& backwardPath)
{
    static char METHOD_LABEL[] = "TTLAdopter::ref:";

    //connect
    openChnl(TTLAdopter::REF);

    //send&recv
    invoke(messageId, backwardPath);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                     METHOD_LABEL,
                     "invoke: backwardPath",
                     backwardPath.c_str());
#endif //DEBUG

    //disconnect
    closeChnl();

    if (OpenSOAP::EntryStatus::NOENTRY == backwardPath) {
        //entry not exists.
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG, "%s %s",
                         METHOD_LABEL,
                         "return false");
#endif //DEBUG

        return false;
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG, "%s %s",
                     METHOD_LABEL,
                     "return true");
#endif //DEBUG

    //process finish
    return true;
}

void
TTLAdopter::openChnl(AccessType type)
{
    static char METHOD_LABEL[] = "TTLAdopter::openChnl:";

    //establish connection to TTL manager
    switch (type) {
    case TTLAdopter::PUSH:
        chnl_ = new TTLTablePushChnlSelector();        
        break;
    case TTLAdopter::REF:
        chnl_ = new TTLTableRefChnlSelector();
        break;
    default:
        AppLogger::Write(APLOG_ERROR, "%s %s=(%d)",
                         METHOD_LABEL,
                         "AccessType invalid. type",
                         type);
        break;
    }
    //create descriptor
    chnlDesc_ = new ChannelDescriptor();

    if (!chnl_ || !chnlDesc_) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "memory allocate failed.");
        throw runtime_error("memory fault");
    }
    
    if (0 != chnl_->open(*chnlDesc_)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "TTLManager open failed.");
        throw runtime_error("i/o error");
    }
    return;
}


void
TTLAdopter::closeChnl()
{
    static char METHOD_LABEL[] = "TTLAdopter::closeChnl:";

    int ret = chnl_->close(*chnlDesc_);
    delete chnlDesc_;
    delete chnl_;
    chnlDesc_ = 0;
    chnl_ = 0;
    if (0 != ret) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "TTLManager close failed.");
        throw runtime_error("i/o error");
    }
    return;
}

void
TTLAdopter::invoke(const string& sendData, string& recvData)
{
    static char METHOD_LABEL[] = "TTLAdopter::invoke:";

    //send.
    if (0 > chnlDesc_->write(sendData)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "write failed to TTLManager");
        throw runtime_error("i/o error");
    }

    //recv.
    if (0 > chnlDesc_->read(recvData)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "read failed from TTLManager");
        throw runtime_error("i/o error");
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                     METHOD_LABEL,
                     "recv",
                     recvData.c_str());
#endif //DEBUG

    return;
}

// End of TTLAdopter.cpp
