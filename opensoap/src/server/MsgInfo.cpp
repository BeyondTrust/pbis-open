
#include <stdio.h>
#include <stdlib.h>
#include "MsgInfo.h"
#include "SrvErrorCode.h"
#include "StringUtil.h"

using namespace OpenSOAPv1_2_00;
using namespace std;

//init. static member
const string MsgInfo::DELIMITER(",");

MsgInfo::MsgInfo()
    : async(false)
    , hasForwardPath(false)
    , hasHopcount(false)
    , hopcount(0)
    , hasTTLSecond(false)
    , ttlSecond(0)
    , hasTTLHoptimes(false)
    , ttlHoptimes(0)
    , hasTTLAsyncSecond(false)
    , ttlAsyncSecond(0)
    , isResponseMsg(false)
    , undelete(false)
    , in(false)
    , operationExist(false)
    , hasExtHeader(false)
    , hasExtHeaderBlock(false)
{
}

MsgInfo::~MsgInfo(){
}

MsgInfo::MsgInfo(MsgInfo& mi){
    RequestID=mi.RequestID;
    ResponseID=mi.ResponseID;
    //header
    messageId = mi.messageId;
    async = mi.async;
    hasForwardPath = mi.hasForwardPath;
    forwardPathArray = mi.forwardPathArray;
    hopcount = mi.hopcount;
    receivedPathUrlArray = mi.receivedPathUrlArray;
    receivedPathTimeArray = mi.receivedPathTimeArray;
    ttlSecond = mi.ttlSecond;
    ttlHoptimes = mi.ttlHoptimes;
    ttlAsyncSecond = mi.ttlAsyncSecond;
    backwardPath = mi.backwardPath;
    isResponseMsg = mi.isResponseMsg;
    undelete = mi.undelete;
    in = mi.in;
    operationExist = mi.operationExist;
    hasExtHeader = mi.hasExtHeader;
    hasExtHeaderBlock = mi.hasExtHeaderBlock;

    methodName = mi.methodName;
    methodNamespace = mi.methodNamespace;
}
//setter by string
void MsgInfo::SetRequestID(const std::string& str){
    RequestID=str;
}
void MsgInfo::SetResponseID(const std::string& str){
    ResponseID=str;
}
//setter by char*
void MsgInfo::SetRequestID(const char * str){
    RequestID=str;
}
void MsgInfo::SetResponseID(const char * str){
    ResponseID=str;
}

//getter
const string& MsgInfo::GetRequestID() const {
	return RequestID;
}
const string& MsgInfo::GetResponseID() const {
	return ResponseID;
}

void MsgInfo::setAsync(const std::string& s) {
    int n=0;
    StringUtil::fromString(s,n);
    if (n==1 || 0 == strcasecmp("true", s.c_str())) {
        async = true;
    }
}

void MsgInfo::setIn(const std::string& s) {
    int n=0;
    StringUtil::fromString(s,n);
    if (n==1 || 0 == strcasecmp("true", s.c_str())) {
        in = true;
    }
}

void MsgInfo::setUndelete(const std::string& s) {
    int n=0;
    StringUtil::fromString(s,n);
    if (n==1 || 0 == strcasecmp("true", s.c_str())) {
        undelete = true;
    }
}

string
MsgInfo::toString() const
{
    int defaultLevel = APLOG_DEBUG; //get from server.conf
#ifdef DEBUG
    defaultLevel = APLOG_DEBUG9;
#endif //DEBUG
    
    return toString(defaultLevel);
}

string
MsgInfo::toString(int level) const
{
    int i = 0;

    string retStr(RequestID);
//    if (level >= APLOG_DEBUG) {
    retStr += DELIMITER;
    retStr += ResponseID;
//    }

    retStr += DELIMITER;
    retStr += messageId;

    retStr += DELIMITER;
    retStr += (async)?"true":"false";

    retStr += DELIMITER;
    for (i=0; i<forwardPathArray.size(); i++) {
        if (i>0) {
            retStr += "|";
        }
        retStr += forwardPathArray[i];
    }

    retStr += DELIMITER;
    retStr += StringUtil::toString(hopcount);

    retStr += DELIMITER;
    for (i=0; i<receivedPathUrlArray.size(); i++) {
        if (i>0) {
            retStr += "|";
        }
        retStr += receivedPathUrlArray[i];
    }

    retStr += DELIMITER;
    retStr += StringUtil::toString(ttlSecond);

    retStr += DELIMITER;
    retStr += StringUtil::toString(ttlHoptimes);
    
    retStr += DELIMITER;
    retStr += StringUtil::toString(ttlAsyncSecond);

    retStr += DELIMITER;
    retStr += backwardPath;

    retStr += DELIMITER;
    retStr += (isResponseMsg)?"true":"false";

    retStr += DELIMITER;
    retStr += (undelete)?"true":"false";

    retStr += DELIMITER;
    retStr += (in)?"true":"false";

    retStr += DELIMITER;
    retStr += methodName;

    retStr += DELIMITER;
    retStr += methodNamespace;

    return retStr;
}
