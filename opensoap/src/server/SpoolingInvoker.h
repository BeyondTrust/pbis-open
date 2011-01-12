/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SpoolingInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SpoolingInvoker_H
#define SpoolingInvoker_H

#include <string>
#include "Invoker.h"
#include "SoapMessage.h"
#include "SrvConf.h"

namespace OpenSOAPv1_2_00 {

    //----------------------
    //for OpenSOAP Asynchronous logic
    //----------------------

    class SpoolingInvoker : public OpenSOAP::Invoker {
      public:
        SpoolingInvoker(OpenSOAP::SrvConf& aSrvConf) 
            : OpenSOAP::Invoker(aSrvConf) {}
        virtual ~SpoolingInvoker() {}
        virtual void invoke(OpenSOAP::SoapMessage& request, 
                            OpenSOAP::SoapMessage& response);
    protected:
        bool attachResponseMsgIntoHeader(OpenSOAP::SoapMessage& response);
    };
}

#endif //SpoolingInvoker_H
