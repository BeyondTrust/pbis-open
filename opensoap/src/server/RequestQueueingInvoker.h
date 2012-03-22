/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: RequestQueueingInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef RequestQueueingInvoker_H
#define RequestQueueingInvoker_H

#include <string>
#include "Invoker.h"
#include "SoapMessage.h"
#include "SrvConf.h"

namespace OpenSOAPv1_2_00 {

    //----------------------
    //for OpenSOAP Asynchronous logic
    //----------------------

    class RequestQueueingInvoker : public OpenSOAP::Invoker {
      public:
        RequestQueueingInvoker(OpenSOAP::SrvConf& aSrvConf) 
            : OpenSOAP::Invoker(aSrvConf) {}
        virtual ~RequestQueueingInvoker() {}
        virtual void invoke(OpenSOAP::SoapMessage& request, 
                            OpenSOAP::SoapMessage& response);

    protected:
        long getInvokeTTL(/*const SoapMessage& request*/);
    };
}

#endif //RequestQueueingInvoker_H
