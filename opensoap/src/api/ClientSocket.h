/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ClientSocket.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OpenSOAP_IMPL_ClientSocket_H
#define OpenSOAP_IMPL_ClientSocket_H

#include <OpenSOAP/ClientSocket.h>

#include "Socket.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
    struct tagOpenSOAPClientSocket {
        OpenSOAPSocket  super;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_ClientSocket_H */
