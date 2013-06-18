/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrvCreator.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef MsgDrvCreator_H
#define MsgDrvCreator_H

#include <string>

#include "ChannelManager.h"
#include "SrvConf.h"
#include "Rule.h"
//using namespace
#include "ServerDef.h"

#include <OpenSOAP/Transport.h>

namespace OpenSOAP {

    //prototype def.
    class ChannelDescriptor;

  class MsgDrvCreator : public ChannelManager {

  private:
      //added 2004/01/04
      Rule* rule;

  public:

    MsgDrvCreator();
    virtual ~MsgDrvCreator();

  public:

    //socket address 指定 (run()実行前条件)
    //bool setSocketAddr(const std::string& addr);

    //受信待ち実行開始
    //void run();

  protected:
    SrvConf srvConf;
    //std::string socketAddr_;

      bool invalidMyselfUrl_;
      std::string myselfUrl_;
      OpenSOAPTransportPtr transport;

      bool checkUrl(const std::string& url);
      static
#if defined(WIN32)
      void
#else
      void*
#endif
      timeredConnectCheck(void* arg);



    //通信毎のサブスレッド
    //※スレッド関数の仕様によりC＋＋クラスでは
    //  スタティックメソッドである必要がある．
    //static void* connectionThread(void* arg);

    //このメソッドをサブクラス毎に実装する．
    virtual void doProc(int sockfd);
#if 0
    //virtual std::string parseReadData(const std::string& aReadData);
    virtual int parseAndOpen(const std::string& aReadData,
			     std::string& response,
			     ChannelDescriptor& chnlDesc);
#endif



    //実行中のインスタンス数
    int execCount_;

  };

} // end of namespace OpenSOAP


#endif /* MsgDrvCreator_H */
