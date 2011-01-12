/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLManager.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef TTLManager_H
#define TTLManager_H

#include <string>

#include "ChannelManager.h"

namespace OpenSOAP {

  //prototype def.
  class ChannelDescriptor;
  class TTL;
  
  class TTLManager : public ChannelManager {
  private:
    TTL* ttlPtr;
    
  public:

    TTLManager();
    virtual ~TTLManager();

  protected:

    //このメソッドをサブクラス毎に実装する．
    virtual void doProc(int sockfd);

    //実行中のインスタンス数
    int execCount_;

  };

} // end of namespace OpenSOAP


#endif /* TTLManager_H */
