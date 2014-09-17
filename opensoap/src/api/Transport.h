/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Transport.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OpenSOAP_IMPL_Transport_H
#define OpenSOAP_IMPL_Transport_H

#include <OpenSOAP/Transport.h>
#include <OpenSOAP/Stream.h>

#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct tagOpenSOAPConnectInfo OpenSOAPConnectInfo;
    typedef OpenSOAPConnectInfo *OpenSOAPConnectInfoPtr;
    
    typedef struct tagOpenSOAPTransportHeader OpenSOAPTransportHeader;
    typedef OpenSOAPTransportHeader *OpenSOAPTransportHeaderPtr;

    typedef struct tagOpenSOAPTransportAuth OpenSOAPTransportAuth;
    typedef OpenSOAPTransportAuth *OpenSOAPTransportAuthPtr;
    
    typedef struct tagOpenSOAPTransportProxy OpenSOAPTransportProxy;
    typedef OpenSOAPTransportProxy *OpenSOAPTransportProxyPtr;
    
    struct tagOpenSOAPTransport {
        OpenSOAPObject super;

        OpenSOAPTransportHeaderPtr requestHeaders;
        OpenSOAPTransportHeaderPtr responseHeaders;

        OpenSOAPTransportAuthPtr auth;
        OpenSOAPTransportProxyPtr proxy;
        
        OpenSOAPStreamPtr   transportStream;

        OpenSOAPByteArrayPtr    serviceEndPoint;
        OpenSOAPConnectInfoPtr  serviceConnectInfo;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Transport_H */
