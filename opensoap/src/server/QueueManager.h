/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: QueueManager.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef QueueManager_H
#define QueueManager_H

#include <string>

#include "ChannelManager.h"

namespace OpenSOAP {

  //prototype def.
  class ChannelDescriptor;
  class Queue;
  
  class QueueManager : public ChannelManager {
  private:
    Queue* queuePtr;
    bool isFwdQueue_;
  public:
    
    QueueManager(bool isFwdQueue);
    virtual ~QueueManager();
    
  protected:

    //このメソッドをサブクラス毎に実装する．
    virtual void doProc(int sockfd);

    //実行中のインスタンス数
    int execCount_;

  };

} // end of namespace OpenSOAP


#endif /* QueueManager_H */
