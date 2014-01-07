/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLTableRefChnlSelector.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#else
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <iostream>

#include "MapUtil.h"
#include "StringUtil.h"
#include "ServerCommon.h"
#include "TTLTableRefChnlSelector.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

//for LOG
static const std::string CLASS_SIG = "TTLTableRefChnlSelector";


TTLTableRefChnlSelector::TTLTableRefChnlSelector()
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str(),__func__);
//  cerr << CLASS_SIG << "::" << CLASS_SIG << endl;
#endif /* DEBUG */  
  
  //std::string sockAddr = TTLMGR_SOCKET_ADDR;
  //setSocketAddr(sockAddr);
  setSocketAddr(TTLMGR_SOCKET_ADDR);
}

TTLTableRefChnlSelector::~TTLTableRefChnlSelector()
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str(),__func__);
//  cerr << CLASS_SIG << "::~" << CLASS_SIG << endl;
#endif /* DEBUG */  

}

std::string TTLTableRefChnlSelector::makeSendOpenMessage(const int waitSecond)
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
  s2 = OpenSOAP::ChnlCommon::REF;
  mapUtil.insert(make_pair(s1,s2));
  
  return mapUtil.toString();
}

std::string TTLTableRefChnlSelector::makeSendCloseMessage()
{
  MapUtil mapUtil(OpenSOAP::ChnlCommon::ITEM_DELMIT,
		  OpenSOAP::ChnlCommon::KEY_DELMIT);
  std::string s1(OpenSOAP::ChnlCommon::CMD);
  std::string s2(OpenSOAP::ChnlCommon::CLOSE);
  mapUtil.insert(make_pair(s1,s2));
  return mapUtil.toString();
}
  
// End of TTLTableRefChnlSelector.cpp

