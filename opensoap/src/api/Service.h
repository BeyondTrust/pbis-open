/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Service.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef OpenSOAP_IMPL_Service_H
#define OpenSOAP_IMPL_Service_H

#include <OpenSOAP/Service.h>
#include <OpenSOAP/StringHash.h>
#include <OpenSOAP/Stream.h>

#include "Object.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct tagOpenSOAPServiceMapItem OpenSOAPServiceMapItem;
    typedef OpenSOAPServiceMapItem *OpenSOAPServiceMapItemPtr;
    
    typedef struct tagOpenSOAPServiceConnecter OpenSOAPServiceConnecter;
    typedef OpenSOAPServiceConnecter *OpenSOAPServiceConnecterPtr;

    struct tagOpenSOAPService {
        OpenSOAPObject super;

        OpenSOAPStringPtr   serviceName;
        OpenSOAPStringHashPtr serviceMap;
        int     isLoop;
        OpenSOAPServiceMapItemPtr defaultService;

        OpenSOAPServiceConnecterPtr serviceConnecter;
        OpenSOAPByteArrayPtr requestBuffer;
        OpenSOAPByteArrayPtr responseBuffer;
        OpenSOAPByteArrayPtr charsetBuffer;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Service_H */
