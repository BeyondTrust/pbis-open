/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgDrvChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef MsgDrvChnlSelector_H
#define MsgDrvChnlSelector_H

#include <string>

#include <OpenSOAP/Defines.h>
#include "ChannelSelector.h"

namespace OpenSOAP {

    //ChannelManagerÇ∆ÇÃÇ‚ÇËÇ∆ÇËÇ…ÇÊÇËDescriptorèÓïÒÇìæÇÈ
    class OPENSOAP_CLASS MsgDrvChnlSelector : public ChannelSelector {

  public:
    MsgDrvChnlSelector();
    virtual ~MsgDrvChnlSelector();

  protected:
    virtual std::string makeSendOpenMessage(const int waitSecond);
    virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif /* MsgDrvChnlSelector_H */
