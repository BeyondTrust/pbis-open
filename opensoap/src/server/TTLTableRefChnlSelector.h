/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTLTableRefChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef TTLTableRefChnlSelector_H
#define TTLTableRefChnlSelector_H

#include <string>

#include "ChannelSelector.h"

namespace OpenSOAP {

  //ChannelManagerÇ∆ÇÃÇ‚ÇËÇ∆ÇËÇ…ÇÊÇËDescriptorèÓïÒÇìæÇÈ
  class TTLTableRefChnlSelector : public ChannelSelector {

  public:
    TTLTableRefChnlSelector();
    virtual ~TTLTableRefChnlSelector();

  protected:
    virtual std::string makeSendOpenMessage(const int waitSecond);
    virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* TTLTableRefChnlSelector_H */
