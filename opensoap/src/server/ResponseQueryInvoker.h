/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ResponseQueryInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef ResponseQueryInvoker_H
#define ResponseQueryInvoker_H

#include <string>
#include "Invoker.h"
#include "SoapMessage.h"
#include "SrvConf.h"

namespace OpenSOAPv1_2_00 {

    //----------------------
    //for OpenSOAP Asynchronous logic
    //----------------------

    class ResponseQueryInvoker : public OpenSOAP::Invoker {
      public:
        ResponseQueryInvoker(OpenSOAP::SrvConf& aSrvConf) 
            : OpenSOAP::Invoker(aSrvConf) {}
        virtual ~ResponseQueryInvoker() {}
        virtual void invoke(OpenSOAP::SoapMessage& request, 
                            OpenSOAP::SoapMessage& response);
    };
}

#endif //ResponseQueryInvoker_H
