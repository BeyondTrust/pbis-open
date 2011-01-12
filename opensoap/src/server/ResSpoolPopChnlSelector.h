/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ResSpoolPopChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ResSpoolPopChnlSelector_H
#define ResSpoolPopChnlSelector_H

#include <string>
#include <OpenSOAP/Defines.h>
#include "ChannelSelector.h"

namespace OpenSOAP {

  //ChannelManagerÇ∆ÇÃÇ‚ÇËÇ∆ÇËÇ…ÇÊÇËDescriptorèÓïÒÇìæÇÈ
  class OPENSOAP_CLASS ResSpoolPopChnlSelector : public ChannelSelector {

  public:
    ResSpoolPopChnlSelector();
    virtual ~ResSpoolPopChnlSelector();

  protected:
    virtual std::string makeSendOpenMessage(const int waitSecond);
    virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* ResSpoolPopChnlSelector_H */
