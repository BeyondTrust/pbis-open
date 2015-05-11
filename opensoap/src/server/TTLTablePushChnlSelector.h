/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLTablePushChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef TTLTablePushChnlSelector_H
#define TTLTablePushChnlSelector_H

#include <string>

#include "ChannelSelector.h"

namespace OpenSOAP {

  //ChannelManagerÇ∆ÇÃÇ‚ÇËÇ∆ÇËÇ…ÇÊÇËDescriptorèÓïÒÇìæÇÈ
  class TTLTablePushChnlSelector : public ChannelSelector {

  public:
    TTLTablePushChnlSelector();
    virtual ~TTLTablePushChnlSelector();

  protected:
    virtual std::string makeSendOpenMessage(const int waitSecond);
    virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* TTLTablePushChnlSelector_H */
