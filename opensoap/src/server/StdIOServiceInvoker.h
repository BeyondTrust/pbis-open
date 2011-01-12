/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: StdIOServiceInvoker.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef StdIOServiceInvoker_H
#define StdIOServiceInvoker_H

#include <string>
#include "ServiceInvoker.h"

namespace OpenSOAP {
    class StdIOServiceInvoker : public ServiceInvoker {
    public:
        StdIOServiceInvoker(SrvConf& aSrvConf) : ServiceInvoker(aSrvConf) {}
        virtual ~StdIOServiceInvoker() {}
        void setExecProg(const std::string& progPath) {
            execProg = progPath;
        }

    protected:
        virtual void invokeImpl(SoapMessage* request, SoapMessage* response);
        std::string execProg;
    };
}

#endif //StdIOServiceInvoker_H
