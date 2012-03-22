/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolManager.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SpoolManager_H
#define SpoolManager_H

#include <string>

#include "ChannelManager.h"

namespace OpenSOAP {

  //prototype def.
  class ChannelDescriptor;
  class Spool;
  
  class SpoolManager : public ChannelManager {
  private:
    Spool* resSpoolPtr;
    
  public:

    SpoolManager();
    virtual ~SpoolManager();

  protected:

    //このメソッドをサブクラス毎に実装する．
    virtual void doProc(int sockfd);

    //実行中のインスタンス数
    int execCount_;

  };

} // end of namespace OpenSOAP


#endif /* SpoolManager_H */
