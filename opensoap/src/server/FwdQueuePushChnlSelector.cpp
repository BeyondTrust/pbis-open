/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: FwdQueuePushChnlSelector.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <sys/types.h>

#if defined(WIN32)
#else
#include <sys/un.h>
#include <unistd.h>
#endif
#include <iostream>
#include <fcntl.h>

#include "MapUtil.h"
#include "StringUtil.h"
#include "ServerCommon.h"
#include "FwdQueuePushChnlSelector.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

//for LOG
static const std::string CLASS_SIG = "FwdQueuePushChnlSelector";


FwdQueuePushChnlSelector::FwdQueuePushChnlSelector()
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
#endif /* DEBUG */  
  
  //std::string sockAddr = FWDQUEUE_SOCKET_ADDR;
  //setSocketAddr(sockAddr);
	setSocketAddr(FWDQUEUE_SOCKET_ADDR);
}

FwdQueuePushChnlSelector::~FwdQueuePushChnlSelector()
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::~%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
#endif /* DEBUG */  

}

std::string FwdQueuePushChnlSelector::makeSendOpenMessage(const int waitSecond)
{
  MapUtil mapUtil(OpenSOAP::ChnlCommon::ITEM_DELMIT,
		  OpenSOAP::ChnlCommon::KEY_DELMIT);
  std::string s1(OpenSOAP::ChnlCommon::CMD);
  std::string s2(OpenSOAP::ChnlCommon::OPEN);
  mapUtil.insert(make_pair(s1,s2));
  if (ChannelDescriptor::NO_WAIT != waitSecond) {
    s1 = OpenSOAP::ChnlCommon::WAIT;
    s2 = StringUtil::toString(waitSecond);
    mapUtil.insert(make_pair(s1,s2));
  }
  s1 = OpenSOAP::ChnlCommon::OPERATION;
  s2 = OpenSOAP::ChnlCommon::PUSH;
  mapUtil.insert(make_pair(s1,s2));
  return mapUtil.toString();
}

std::string FwdQueuePushChnlSelector::makeSendCloseMessage()
{
  MapUtil mapUtil(OpenSOAP::ChnlCommon::ITEM_DELMIT,
		  OpenSOAP::ChnlCommon::KEY_DELMIT);
  std::string s1(OpenSOAP::ChnlCommon::CMD);
  std::string s2(OpenSOAP::ChnlCommon::CLOSE);
  mapUtil.insert(make_pair(s1,s2));
  return mapUtil.toString();
}
  
// End of FwdQueuePushChnlSelector.cpp
