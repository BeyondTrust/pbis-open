/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ForwardingInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef ForwardingInvoker_H
#define ForwardingInvoker_H

#include <string>
#include "Invoker.h"
#include "SoapMessage.h"
#include "SrvConf.h"

namespace OpenSOAPv1_2_00 {

    //----------------------
    //for OpenSOAP Asynchronous logic
    //----------------------

    class ForwardingInvoker : public OpenSOAP::Invoker {
      public:
        ForwardingInvoker(OpenSOAP::SrvConf& aSrvConf) 
            : OpenSOAP::Invoker(aSrvConf) {}
        virtual ~ForwardingInvoker() {}
        virtual void invoke(OpenSOAP::SoapMessage& request, 
                            OpenSOAP::SoapMessage& response);

    protected:
        long getInvokeTTL() const;
    };
}

#endif //ForwardingInvoker_H
