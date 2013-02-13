/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ResSpoolExpireChnlSelector.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ResSpoolExpireChnlSelector_H
#define ResSpoolExpireChnlSelector_H

#include <string>
#include <OpenSOAP/Defines.h>
#include "ChannelSelector.h"

namespace OpenSOAP {

    //call expire operation 
    class OPENSOAP_CLASS ResSpoolExpireChnlSelector : public ChannelSelector {
        
    public:
        ResSpoolExpireChnlSelector();
        virtual ~ResSpoolExpireChnlSelector();
        
    protected:
        virtual std::string makeSendOpenMessage(const int waitSecond);
        virtual std::string makeSendCloseMessage();

  };

} // end of namespace OpenSOAP


#endif // ResSpoolExpireChnlSelector_H
