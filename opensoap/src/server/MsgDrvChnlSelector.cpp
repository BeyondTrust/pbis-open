/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrvChnlSelector.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/un.h>
//#include <unistd.h>
//#include <fcntl.h>

#include "MapUtil.h"
#include "StringUtil.h"
#include "ServerCommon.h"
#include "MsgDrvChnlSelector.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

//for LOG
static const std::string CLASS_SIG = "MsgDrvChnlSelector";

MsgDrvChnlSelector::MsgDrvChnlSelector()
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
#endif /* DEBUG */  

  setSocketAddr(MSGDRV_SOCKET_ADDR);
}

MsgDrvChnlSelector::~MsgDrvChnlSelector()
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::~%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
#endif /* DEBUG */  

}

std::string MsgDrvChnlSelector::makeSendOpenMessage(const int waitSecond)
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

  return mapUtil.toString();
}

std::string MsgDrvChnlSelector::makeSendCloseMessage()
{
  MapUtil mapUtil(OpenSOAP::ChnlCommon::ITEM_DELMIT, 
		  OpenSOAP::ChnlCommon::KEY_DELMIT);
  std::string s1(OpenSOAP::ChnlCommon::CMD);
  std::string s2(OpenSOAP::ChnlCommon::CLOSE);
  mapUtil.insert(make_pair(s1,s2));
  return mapUtil.toString();
}
  
// End of MsgDrvChnlSelector.cpp
