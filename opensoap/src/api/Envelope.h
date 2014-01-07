/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Envelope.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_Envelope_H
#define OpenSOAP_IMPL_Envelope_H

#include <OpenSOAP/Envelope.h>
#include "XMLElm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    struct tagOpenSOAPEnvelope {
        OpenSOAPXMLElm          super;
        
        OpenSOAPStringPtr       version;
        OpenSOAPXMLElmPtr       header;
        OpenSOAPXMLElmPtr       body;
    };
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_Envelope_H */
