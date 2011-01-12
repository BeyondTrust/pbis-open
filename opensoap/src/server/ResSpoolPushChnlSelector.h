/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ResSpoolPushChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ResSpoolPushChnlSelector_H
#define ResSpoolPushChnlSelector_H

#include <string>
#include <OpenSOAP/Defines.h>
#include "ChannelSelector.h"

namespace OpenSOAP {

    //ChannelManagerÇ∆ÇÃÇ‚ÇËÇ∆ÇËÇ…ÇÊÇËDescriptorèÓïÒÇìæÇÈ
    class OPENSOAP_CLASS ResSpoolPushChnlSelector : public ChannelSelector {

  public:
        ResSpoolPushChnlSelector();
        virtual ~ResSpoolPushChnlSelector();

  protected:
        virtual std::string makeSendOpenMessage(const int waitSecond);
        virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* ResSpoolPushChnlSelector_H */
