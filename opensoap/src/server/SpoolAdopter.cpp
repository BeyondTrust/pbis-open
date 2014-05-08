/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolAdopter.cpp,v $
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

#include "SpoolAdopter.h"
#include "ServerCommon.h"
#include "ResSpoolPushChnlSelector.h"
#include "ResSpoolPopChnlSelector.h"
#include "ResSpoolExpireChnlSelector.h"
#include "AppLogger.h"

//#define DEBUG

using namespace std;
using namespace OpenSOAP;

SpoolAdopter::SpoolAdopter()
    : chnl_(0)
    , chnlDesc_(0)
{
}

SpoolAdopter::~SpoolAdopter()
{
}

//regist entry
string 
SpoolAdopter::push(const std::string& fileId)
{
    //connect
    openChnl(SpoolAdopter::PUSH);

    //send&recv
    string pushResult;
    invoke(fileId, pushResult);

    //disconnect
    closeChnl();

    return pushResult;
}

//check existance of entry and extract fileId of response
//  entry not exists: return false
//  entry exists: return true and set responseFileId
//                if send msg not contained <undelete> tag,
//                delete entry from spool table
bool 
SpoolAdopter::pop(const std::string& fileId, std::string& responseFileId)
{
    static char METHOD_LABEL[] = "SpoolAdopter::pop: ";

    //connect
    openChnl(SpoolAdopter::POP);

    //send&recv
    invoke(fileId, responseFileId);

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s%s=[%s]",METHOD_LABEL
                        ,"invoke: responseFileId",responseFileId.c_str());
#endif //DEBUG

    //disconnect
    closeChnl();

    //process finish
    return true;
}

//expire record by ttl-manager
string
SpoolAdopter::expire(const std::string& messageId)
{
    static char METHOD_LABEL[] = "SpoolAdopter::expire: ";

    //connect
    openChnl(SpoolAdopter::EXPIRE);

    string result;
    //send&recv
    invoke(messageId, result);

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s%s=[%s]",METHOD_LABEL
                        ,"invoke: result",result.c_str());
#endif //DEBUG

    //disconnect
    closeChnl();

    //process finish
    return result;
}

void
SpoolAdopter::openChnl(AccessType type)
{
    static char METHOD_LABEL[] = "SpoolAdopter::openChnl: ";
    //establish connection to TTL manager
    switch (type) {
    case SpoolAdopter::PUSH:
        chnl_ = new ResSpoolPushChnlSelector();        
        break;
    case SpoolAdopter::POP:
        chnl_ = new ResSpoolPopChnlSelector();
        break;
    case SpoolAdopter::EXPIRE:
        chnl_ = new ResSpoolExpireChnlSelector();
        break;
    default:
        AppLogger::Write(APLOG_ERROR,"%s%s %s=[%s]",METHOD_LABEL
                        ,"AccessType invalid.","type",type);
        break;
    }
    //create descriptor
    chnlDesc_ = new ChannelDescriptor();

    if (!chnl_ || !chnlDesc_) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"memory allocate failed.");
        throw runtime_error("memory fault");
    }
    
    if (0 != chnl_->open(*chnlDesc_)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"SpoolManager open failed.");
        throw runtime_error("i/o error");
    }
    return;
}

void
SpoolAdopter::closeChnl()
{
    int ret = chnl_->close(*chnlDesc_);
    delete chnlDesc_;
    delete chnl_;
    chnlDesc_ = 0;
    chnl_ = 0;
    if (0 != ret) {
        AppLogger::Write(APLOG_ERROR,"%s%s","SpoolAdopter::closeChnl: "
                        ,"SpoolManager close failed.");
        throw runtime_error("i/o error");
    }
    return;
}

void
SpoolAdopter::invoke(const string& sendData, string& recvData)
{
    static char METHOD_LABEL[] = "SpoolAdopter::invoke: ";
    //send.
    if (0 > chnlDesc_->write(sendData)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"write failed to SpoolManager");
        throw runtime_error("i/o error");
    }

    //recv.
    if (0 > chnlDesc_->read(recvData)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"read failed from SpoolManager");
        throw runtime_error("i/o error");
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s %s=[%s]",METHOD_LABEL
                    ,"recv",recvData.c_str());
#endif //DEBUG

    return;
}

// End of SpoolAdopter.cpp
