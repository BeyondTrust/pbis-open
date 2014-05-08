/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Invoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef Invoker_H
#define Invoker_H

#include <string>
#include "SoapMessage.h"
#include "SrvConf.h"

namespace OpenSOAP {
    class Invoker {
      public:
        Invoker(SrvConf& aSrvConf) : srvConf(aSrvConf) {}
        virtual ~Invoker() {}
        void setServiceInfo(const std::string& aMethodName,
                            const std::string& aNsUri) {
            methodName = aMethodName;
            nsUri = aNsUri;
        }
        virtual void invoke(SoapMessage& request, SoapMessage& response);

      protected:
        std::string methodName;
        std::string nsUri;
        SrvConf& srvConf;
    };
}

#endif //Invoker_H
