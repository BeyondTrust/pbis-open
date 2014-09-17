/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XMLNamespace.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_XMLNamespace_H
#define OpenSOAP_IMPL_XMLNamespace_H

#include <OpenSOAP/XMLNamespace.h>
#include <OpenSOAP/XMLElm.h>

#include "DLinkList.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    struct tagOpenSOAPXMLNamespace {
        OpenSOAPDLinkListItem  super;
        /* */
        OpenSOAPStringPtr   prefix;
        OpenSOAPStringPtr   uri;
        OpenSOAPXMLElmPtr   definedElement;
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_XMLNamespace_H */
