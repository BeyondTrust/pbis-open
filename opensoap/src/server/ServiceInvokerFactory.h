/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ServiceInvokerFactory.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef ServiceInvokerFactory_H
#define ServiceInvokerFactory_H

#include <string>
#include "ServiceInvoker.h"
#include "SSMLAttrMgrProtocol.h"

namespace OpenSOAP {

    class ServiceInvokerFactory {
        
      public:
        ServiceInvokerFactory();
        virtual ~ServiceInvokerFactory();
        ServiceInvoker* createServiceInvoker(const std::string& methodName,
                                             const std::string& methodNs,
                                             const SSMLType ssmlType,
                                             SrvConf& srvConf);
    };
}

#endif //ServiceInvokerFactory_H
