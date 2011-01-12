/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ReqQueuePushChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ReqQueuePushChnlSelector_H
#define ReqQueuePushChnlSelector_H

#include <string>

#include "ChannelSelector.h"

namespace OpenSOAP {

  //ChannelManagerÇ∆ÇÃÇ‚ÇËÇ∆ÇËÇ…ÇÊÇËDescriptorèÓïÒÇìæÇÈ
  class ReqQueuePushChnlSelector : public ChannelSelector {

  public:
    ReqQueuePushChnlSelector();
    virtual ~ReqQueuePushChnlSelector();

  protected:
    virtual std::string makeSendOpenMessage(const int waitSecond);
    virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* ReqQueuePushChnlSelector_H */
