/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ResponseSpoolingOrForwardingInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef ResponseSpoolingOrForwardingInvoker_H
#define ResponseSpoolingOrForwardingInvoker_H

#include <string>
#include "Invoker.h"
#include "SoapMessage.h"
#include "SrvConf.h"

namespace OpenSOAPv1_2_00 {

    //----------------------
    //for OpenSOAP Asynchronous logic
    //----------------------

    class ResponseSpoolingOrForwardingInvoker : public OpenSOAP::Invoker {
      public:
        ResponseSpoolingOrForwardingInvoker(OpenSOAP::SrvConf& aSrvConf) 
            : OpenSOAP::Invoker(aSrvConf) {}
        virtual ~ResponseSpoolingOrForwardingInvoker() {}
        virtual void invoke(OpenSOAP::SoapMessage& request, 
                            OpenSOAP::SoapMessage& response);
    protected:
        //bool attachResponseMsgIntoHeader(OpenSOAP::SoapMessage& response);
    };
}

#endif //ResponseSpoolingOrForwardingInvoker_H
