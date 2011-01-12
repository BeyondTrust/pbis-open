/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: FwdQueuePushChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef FwdQueuePushChnlSelector_H
#define FwdQueuePushChnlSelector_H

#include <string>

#include "ChannelSelector.h"

namespace OpenSOAP {

  //ChannelManagerÇ∆ÇÃÇ‚ÇËÇ∆ÇËÇ…ÇÊÇËDescriptorèÓïÒÇìæÇÈ
  class FwdQueuePushChnlSelector : public ChannelSelector {

  public:
    FwdQueuePushChnlSelector();
    virtual ~FwdQueuePushChnlSelector();

  protected:
    virtual std::string makeSendOpenMessage(const int waitSecond);
    virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* FwdQueuePushChnlSelector_H */
