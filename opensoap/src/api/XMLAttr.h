/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XMLAttr.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_XMLAttr_H
#define OpenSOAP_IMPL_XMLAttr_H

#include <OpenSOAP/XMLAttr.h>
#include <OpenSOAP/String.h>

#include "XMLNode.h"
#include "XMLElm.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    struct tagOpenSOAPXMLAttr {
        OpenSOAPXMLNode super;
        OpenSOAPXMLElmPtr	definedElement;
    };

	/* */
	int
	OpenSOAPXMLAttrCreate(/* [out] */ OpenSOAPXMLAttrPtr *attr);
	
    int
/*    OPENSOAP_API */
    OpenSOAPXMLAttrCreateString(/* [in]  */ OpenSOAPStringPtr attrName,
								/* [in]  */ int isDup,
                                /* [out] */ OpenSOAPXMLAttrPtr *attr);
	
    int
/*    OPENSOAP_API */
    OpenSOAPXMLAttrSetValueString(/* [out] */ OpenSOAPXMLAttrPtr elm,
                                  /* [in]  */ OpenSOAPStringPtr typeName,
                                  /* [in]  */ void * value);

    int
	/* OPENSOAP_API */
	OpenSOAPXMLAttrSetNamespaceString(/* [out] */ OpenSOAPXMLAttrPtr attr,
									  /* [in]  */ OpenSOAPStringPtr nsUri,
									  /* [in]  */ OpenSOAPStringPtr nsPrefix);
	
	int
	/* OPENSOAP_API */
	OpenSOAPXMLAttrCreateLibxmlAttr(/* [in]  */ xmlAttrPtr libxmlAttr,
									/* [out] */ OpenSOAPXMLAttrPtr *attr);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_XMLAttr_H */
