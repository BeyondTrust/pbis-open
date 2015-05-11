/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XMLElm.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_IMPL_XMLElm_H
#define OpenSOAP_IMPL_XMLElm_H

#include <OpenSOAP/XMLElm.h>
#include <OpenSOAP/XMLAttr.h>
#include <OpenSOAP/XMLNamespace.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#if defined(HAVE_LIBXML2_XMLVERSION_H) 
#include <libxml2/tree.h>
#include <libxml2/parser.h>
#include <libxml2/xmlmemory.h>
#else
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#endif /* !HAVE_LIBXML2_XMLVERSION_H */

#include "XMLNode.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	struct tagOpenSOAPXMLElm {
		OpenSOAPXMLNode super;                      /* Node               */
		OpenSOAPXMLAttrPtr  xmlAttrList;            /* ex.) EncodingStyle */
		OpenSOAPXMLNamespacePtr definedXMLNsList;   /* ex.) SOAP-ENV      */
	};

	int
	OpenSOAPXMLElmInitialize(/* [in, out] */ OpenSOAPXMLElmPtr elm,
							 /* [in] */       OpenSOAPObjectFreeFunc free_func);

	int
	OpenSOAPXMLElmReleaseMembers(/* [in, out] */ OpenSOAPXMLElmPtr elm);

	int
	OpenSOAPXMLElmRelease(/* [in, out] */ OpenSOAPXMLElmPtr elm);

	int
	OpenSOAPXMLElmAddAttrFromLibxmlNode(/* [out] */ OpenSOAPXMLElmPtr elm,
										/* [in]  */ xmlNodePtr node);
	
	int
	OpenSOAPXMLElmCreateChildNodeFromLibxml(/* [out] */ OpenSOAPXMLElmPtr elm, 
											/* [in]  */ xmlNodePtr node);

	int
	OPENSOAP_API
	OpenSOAPXMLElmAddAttributeString(OpenSOAPXMLElmPtr elm,
									 OpenSOAPStringPtr attr_name,
									 OpenSOAPStringPtr attr_type,
									 void *attr_value,
									 OpenSOAPXMLAttrPtr *attr);
	int
	OPENSOAP_API
	OpenSOAPXMLElmGetAttributeString(OpenSOAPXMLElmPtr elm,
									 OpenSOAPStringPtr attr_name,
									 OpenSOAPXMLAttrPtr * attr);
	int
	OPENSOAP_API
	OpenSOAPXMLElmGetValueString(OpenSOAPXMLElmPtr elm,
								 OpenSOAPStringPtr type_name,
								 void * value);
	int
	OPENSOAP_API
	OpenSOAPXMLElmSetValueString(OpenSOAPXMLElmPtr elm,
								 OpenSOAPStringPtr type_name,
								 void * value);

	int
    /* OPENSOAP_API */
	OpenSOAPXMLElmDefineNamespaceString(/* [in, out] */ OpenSOAPXMLElmPtr elm,
										/* [in]  */ OpenSOAPStringPtr nsUri,
										/* [in]  */
										OpenSOAPStringPtr nsPrefix,
										/* [out] */
										OpenSOAPXMLNamespacePtr *ns);

	int
	/* OPENSOAP_API */
	OpenSOAPXMLElmSetAttributeValueAsString(/* [out] */ OpenSOAPXMLElmPtr elm,
											/* [in]  */
											OpenSOAPStringPtr attrName,
											/* [in]  */
											OpenSOAPStringPtr value,
											/* [in]  */ int isDup);

	int
	/* OPENSOAP_API */
	OpenSOAPXMLElmRemoveAttributeString(/* [out] */ OpenSOAPXMLElmPtr elm,
										/* [in]  */ OpenSOAPStringPtr attrName,
										/* [in]  */ int isValueRelease,
										/* [out] */ OpenSOAPStringPtr *value);

	int
	OpenSOAPLibxmlDocCreateCharEncoding(const char *chEnc,
										OpenSOAPByteArrayPtr bAry,
										xmlDocPtr	*doc,
										xmlNodePtr	*rootNode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_IMPL_XMLElm_H */
