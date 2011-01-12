/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ReqQueueExpireChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ReqQueueExpireChnlSelector_H
#define ReqQueueExpireChnlSelector_H

#include <string>

#include "ChannelSelector.h"

namespace OpenSOAP {

  class ReqQueueExpireChnlSelector : public ChannelSelector {

  public:
    ReqQueueExpireChnlSelector();
    virtual ~ReqQueueExpireChnlSelector();

  protected:
    virtual std::string makeSendOpenMessage(const int waitSecond);
    virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* ReqQueueExpireChnlSelector_H */
