/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: HttpServiceInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef HttpServiceInvoker_H
#define HttpServiceInvoker_H

#include <string>
#include "ServiceInvoker.h"

namespace OpenSOAP {
    class HttpServiceInvoker : public ServiceInvoker {
    public:
        HttpServiceInvoker(SrvConf& aSrvConf) : ServiceInvoker(aSrvConf) {}
        virtual ~HttpServiceInvoker() {}
        void setEndPointUrl(const std::string& url) {
            endPointUrl = url;
        }
    protected:
        virtual void invokeImpl(SoapMessage* request, SoapMessage* response);
        void OpenSOAPAPITransport(SoapMessage* request, SoapMessage* response);

        std::string endPointUrl;
    };
}

#endif //HttpServiceInvoker_H
