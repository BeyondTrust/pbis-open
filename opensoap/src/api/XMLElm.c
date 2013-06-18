/*-----------------------------------------------------------------------------
 * $RCSfile: XMLElm.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: XMLElm.c,v 1.72 2003/06/30 08:36:08 okada Exp $";
#endif  /* _DEBUG */

#include <string.h>
#include <ctype.h>

#include <OpenSOAP/Serializer.h>

#include "XMLElm.h"
#include "XMLAttr.h"
#include "XMLNamespaceList.h"

/*
=begin
= OpenSOAP XML Element class
=end
 */
extern
int
OpenSOAPXMLElmReleaseMembers(/* [in, out] */ OpenSOAPXMLElmPtr elm) {
    int ret = OpenSOAPXMLNodeReleaseMembers((OpenSOAPXMLNodePtr)elm);

    if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLNodeListRelease((OpenSOAPXMLNodePtr)(elm->
															  xmlAttrList));
        if (OPENSOAP_SUCCEEDED(ret) && elm->definedXMLNsList) {
            ret = OpenSOAPXMLNamespaceListRelease(elm->definedXMLNsList);
        }
    }

    return ret;
}

extern
int
OpenSOAPXMLElmRelease(/* [in, out] */ OpenSOAPXMLElmPtr elm) {
    int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)elm);

    return ret;
}

static
int
OpenSOAPXMLElmFree(/* [in, out] */ OpenSOAPObjectPtr obj) {
    int ret = OpenSOAPXMLElmReleaseMembers((OpenSOAPXMLElmPtr)obj);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmInitialize(elm)
    Initialize XML Element Struct.

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OpenSOAPXMLElmInitialize(/* [out] */ OpenSOAPXMLElmPtr elm,
						 /* [in]  */ OpenSOAPObjectFreeFunc free_func) {

    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (elm) {
		elm->xmlAttrList = NULL;
		elm->definedXMLNsList = NULL;
        
        ret = OpenSOAPXMLNodeInitialize((OpenSOAPXMLNodePtr)elm,
                                        free_func ? free_func
                                        : OpenSOAPXMLElmFree);
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmCreate(elm)
    OpenSOAP XML Element instance create

    :Parameters
      :OpenSOAPXMLElmPtr * [out] ((|elm|))
        XML Element pointer
    :Return Values
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmCreate(/* [out] */ OpenSOAPXMLElmPtr *elm) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm) {
		ret = OPENSOAP_MEM_BADALLOC;

		*elm = malloc(sizeof(OpenSOAPXMLElm));

		if (*elm) {
			ret = OpenSOAPXMLElmInitialize(*elm, NULL);
			if (OPENSOAP_FAILED(ret)) {
				free(*elm);
				*elm = NULL;
			}
		}
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetNamespaceString(elm, nsUri, nsPrefix)
    Set Namespace of XML Element(OpenSOAPString)

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        XML Element.
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        Namespace URI
      :[in]  OpenSOAPStringPtr ((|nsPrefix|))
        Namespace Prefix

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPXMLElmSetNamespaceString(/* [out] */ OpenSOAPXMLElmPtr elm,
								 /* [in]  */ OpenSOAPStringPtr nsUri,
								 /* [in]  */ OpenSOAPStringPtr nsPrefix) {
	OpenSOAPXMLNamespacePtr ns = NULL;
	int ret = OpenSOAPXMLElmDefineNamespaceString(elm,
												  nsUri,
												  nsPrefix,
												  &ns);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)elm;
		node->thisNamespace = ns;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetNamespaceMB(elm, nsUri, nsPrefix)
    Set Namespace of XML Element(MB)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
      :const char * [in] ((|nsUri|))
        Namespace URI
      :const char * [in] ((|nsPrefix|))
        Namespace Prefix
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetNamespaceMB(/* [in, out] */ OpenSOAPXMLElmPtr elm,
                             /* [in]      */ const char *nsUri,
                             /* [in]      */ const char *nsPrefix) {
	OpenSOAPStringPtr nsUriStr = NULL;
    int ret = OpenSOAPStringCreateWithMB(nsUri, &nsUriStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr nsPrefixStr = NULL;
		ret = nsPrefix ? OpenSOAPStringCreateWithMB(nsPrefix, &nsPrefixStr)
			: OPENSOAP_NO_ERROR;
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmSetNamespaceString(elm,
												   nsUriStr,
												   nsPrefixStr);

			OpenSOAPStringRelease(nsPrefixStr);
		}

		OpenSOAPStringRelease(nsUriStr);
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetNamespaceWC(elm, nsUri, nsPrefix)
    Set Namespace of XML Element(WC)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
      :const wchar_t * [in] ((|nsUri|))
        Namespace URI
      :const wchar_t * [in] ((|nsPrefix|))
        Namespace Prefix

    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetNamespaceWC(/* [in, out] */ OpenSOAPXMLElmPtr elm,
                             /* [in]      */ const wchar_t *nsUri,
                             /* [in]      */ const wchar_t *nsPrefix) {
	OpenSOAPStringPtr nsUriStr = NULL;
    int ret = OpenSOAPStringCreateWithWC(nsUri, &nsUriStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr nsPrefixStr = NULL;
		ret = nsPrefix ? OpenSOAPStringCreateWithWC(nsPrefix, &nsPrefixStr)
			: OPENSOAP_NO_ERROR;
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmSetNamespaceString(elm,
												   nsUriStr,
												   nsPrefixStr);

			OpenSOAPStringRelease(nsPrefixStr);
		}

		OpenSOAPStringRelease(nsUriStr);
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSearchNamespaceString(elm, nsUri, nsPrefix, ns)
    Search Namespace of XML Element(OpenSOAPString).

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        XML Element.
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        Namespace URI
      :[in]  OpenSOAPStringPtr ((|nsPrefix|))
        Namespace Prefix
      :[out] OpenSOAPXMLNamespacePtr * ((|ns|))
        Namespace search result storead buffer.
		If *ns return NULL, then namespace is not found.
      :[out] OpenSOAPXMLElmPtr * ((|defElm|))
        Defined element stored buffer. If NULL, then no effect.
		If *ns equal NULL, then *defElm un
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSearchNamespaceString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                    /* [in]  */ OpenSOAPStringPtr nsUri,
                                    /* [in]  */ OpenSOAPStringPtr nsPrefix,
                                    /* [out] */ OpenSOAPXMLNamespacePtr *ns,
									/* [out] */ OpenSOAPXMLElmPtr *defElm) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && ns) {
		OpenSOAPXMLElmPtr parent = elm;
		ret = OPENSOAP_NO_ERROR;
		*ns = NULL;
		for (; OPENSOAP_SUCCEEDED(ret) && parent && !*ns;
			 parent
				 = (OpenSOAPXMLElmPtr)((OpenSOAPXMLNodePtr)parent)->parent) {
			OpenSOAPDLinkListItemPtr nextItem
				= (OpenSOAPDLinkListItemPtr)(parent->definedXMLNsList);
			for (; OPENSOAP_SUCCEEDED(ret) && !*ns && nextItem;
				 nextItem = nextItem->next) {
				OpenSOAPXMLNamespacePtr nextNs
					= (OpenSOAPXMLNamespacePtr)nextItem;
				int isSame = 0;
				ret = OpenSOAPXMLNamespaceIsSameString(nextNs,
													   nsUri,
													   nsPrefix,
													   &isSame);
				if (OPENSOAP_SUCCEEDED(ret) && isSame) {
					*ns = nextNs;
				}
			}
		}
		if (defElm) {
			*defElm = parent;
		}
	}
	
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSearchNamespaceMB(elm, nsUri, nsPrefix, ns)
    Search Namespace of XML Element(MB).

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        XML Element.
      :const char * [in] ((|nsUri|))
        Namespace URI
      :const char * [in] ((|nsPrefix|))
        Namespace Prefix
      :OpenSOAPXMLNamespacePtr * [out] ((|ns|))
        Namespace
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSearchNamespaceMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                /* [in]  */ const char *nsUri,
                                /* [in]  */ const char *nsPrefix,
                                /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
    OpenSOAPStringPtr nsUriStr = NULL;
    int ret = OpenSOAPStringCreateWithMB(nsUri, &nsUriStr);
    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPStringPtr nsPrefixStr = NULL;
        ret = OpenSOAPStringCreateWithMB(nsPrefix, &nsPrefixStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPXMLElmSearchNamespaceString(elm,
                                                      nsUriStr,
                                                      nsPrefixStr,
                                                      ns,
													  NULL);
            
            OpenSOAPStringRelease(nsPrefixStr);
        }
        OpenSOAPStringRelease(nsUriStr);
    }
    
    return ret;
}


/*
=begin
--- function#OpenSOAPXMLElmSearchNamespaceWC(elm, nsUri, nsPrefix, ns)
    Search Namespace of XML Element(WC).

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
      :const wchar_t * [in] ((|nsUri|))
        Namespace URI
      :const wchar_t * [in] ((|nsPrefix|))
        Namespace Prefix
      :OpenSOAPXMLNamespacePtr * [out] ((|ns|))
        Namespace
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSearchNamespaceWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                /* [in]  */ const wchar_t *nsUri,
                                /* [in]  */ const wchar_t *nsPrefix,
                                /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
    OpenSOAPStringPtr nsUri_str = NULL;
    int ret = OpenSOAPStringCreateWithWC(nsUri, &nsUri_str);
    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPStringPtr nsPrefix_str = NULL;
        ret = OpenSOAPStringCreateWithWC(nsPrefix, &nsPrefix_str);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPXMLElmSearchNamespaceString(elm,
                                                      nsUri_str,
                                                      nsPrefix_str,
                                                      ns,
													  NULL);
            
            OpenSOAPStringRelease(nsPrefix_str);
        }
        OpenSOAPStringRelease(nsUri_str);
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetNamespace(elm, ns)
    Get Namespace Pointer

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element
      :OpenSOAPXMLNamespacePtr * [out] ((|ns|))
        Namespace 
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetNamespace(/* [in]  */ OpenSOAPXMLElmPtr elm,
                           /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && ns) {
		*ns = ((OpenSOAPXMLNodePtr)elm)->thisNamespace;

		ret = OPENSOAP_NO_ERROR;
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmDefineNamespaceString(elm, nsUri, nsPrefix, ns)
    Define XML namespace.(OpenSOAPString)

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        XML Element.
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        Namespace URI
      :[in]  OpenSOAPStringPtr ((|nsPrefix|))
        Namespace Prefix
      :[out] OpenSOAPXMLNamespacePtr * ((|ns|))
        OpenSOAP XML Namespace. If ns is NULL, then no return value.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
/* static */
extern
int
/* OPENSOAP_API */
OpenSOAPXMLElmDefineNamespaceString(/* [out] */ OpenSOAPXMLElmPtr elm,
									/* [in]  */ OpenSOAPStringPtr nsUri,
									/* [in]  */ OpenSOAPStringPtr nsPrefix,
									/* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm) {
		OpenSOAPXMLNamespacePtr dummyNs = NULL;
		if (!ns) {
			ns = &dummyNs;
		}
		*ns = NULL;
		ret = OpenSOAPXMLElmSearchNamespaceString(elm,
												  nsUri,
												  nsPrefix,
												  ns,
												  NULL);

		if (OPENSOAP_SUCCEEDED(ret)) {
			/*
			  OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)elm;
			*/
			if (!*ns) {
				/* If there is no nsUri and nsPrefix,
				   make a new XMLNamespace,
				   input the new XMLNamespace to the definedXMLNsList,
				   input itself XMLElmPtr to definedElment */
				ret = OpenSOAPXMLNamespaceCreateString(nsUri,
													   nsPrefix,
													   ns);
				if (OPENSOAP_SUCCEEDED(ret)) {
					OpenSOAPDLinkListItemPtr *nsList
						= (OpenSOAPDLinkListItemPtr *)&elm->definedXMLNsList;
					OpenSOAPDLinkListItemPtr item
						= (OpenSOAPDLinkListItemPtr)*ns;
					ret = OpenSOAPDLinkListItemPushBack(nsList,
														item);
					if (OPENSOAP_SUCCEEDED(ret)) {
						ret = OpenSOAPXMLNamespaceSetDefinedXMLElm(*ns,
																   elm,
																   NULL);
						if (OPENSOAP_FAILED(ret)) {
							/* remove link */
							OpenSOAPDLinkListItemPopBack(nsList,
														 NULL);
						}
					}

					if (OPENSOAP_FAILED(ret)) {
						/* release XMLNamespace */
						OpenSOAPXMLNamespaceRelease(*ns);
						*ns = NULL;
					}
				}
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmDefineNamespaceMB(elm, nsUri, nsPrefix, ns)
    Define XML namespace.(MB)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
      :const char * [in] ((|nsUri|))
        Namespace URI
      :const char * [in] ((|nsPrefix|))
        Namespace Prefix
      :OpenSOAPXMLNamespacePtr * [out] ((|ns|))
        OpenSOAP XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmDefineNamespaceMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                                /* [in]  */ const char *nsUri,
                                /* [in]  */ const char *nsPrefix,
                                /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	OpenSOAPStringPtr nsUriStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(nsUri,
										 &nsUriStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr nsPrefixStr = NULL;
		int ret = OpenSOAPStringCreateWithMB(nsPrefix,
											 &nsPrefixStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmDefineNamespaceString(elm,
													  nsUriStr,
													  nsPrefixStr,
													  ns);
			
			OpenSOAPStringRelease(nsPrefixStr);
		}
		
		OpenSOAPStringRelease(nsUriStr);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmDefineNamespaceWC(elm, nsUri, nsPrefix, ns)
    Define XML namespace.(WC)

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        XML Element.
      :[in]  const wchar_t * ((|nsUri|))
        Namespace URI
      :[in]  const wchar_t * ((|nsPrefix|))
        Namespace Prefix
      :[out] OpenSOAPXMLNamespacePtr * ((|ns|))
        OpenSOAP XML Namespace.
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmDefineNamespaceWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                                /* [in]  */ const wchar_t *nsUri,
                                /* [in]  */ const wchar_t *nsPrefix,
                                /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	OpenSOAPStringPtr nsUriStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(nsUri,
										 &nsUriStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr nsPrefixStr = NULL;
		int ret = OpenSOAPStringCreateWithWC(nsPrefix,
											 &nsPrefixStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmDefineNamespaceString(elm,
													  nsUriStr,
													  nsPrefixStr,
													  ns);
			
			OpenSOAPStringRelease(nsPrefixStr);
		}
		
		OpenSOAPStringRelease(nsUriStr);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddAttributeString(elm, attrName, attrType, attrValue, attr)
    Add Attribute(OpenSOAPString)

    :Parameters
      :[in, out] OpenSOAPXMLElmPtr ((|elm|))
        SOAP Element.
      :[in]  OpenSOAPStringPtr ((|attrName|))
        Attribute Name
      :[in]  OpenSOAPStringPtr ((|attrType|))
        Attribute Type
	  :[in]  void * ((|attrValue|))
        Attribute Value
      :[out] OpenSOAPXMLAttrPtr * ((|attr|))
        OpenSOAP XML Attribute
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmAddAttributeString(/* [out] */ OpenSOAPXMLElmPtr elm,
								 /* [in]  */ OpenSOAPStringPtr attrName,
								 /* [in]  */ OpenSOAPStringPtr attrType,
								 /* [in]  */ void *attrValue,
								 /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm) {
		OpenSOAPXMLAttrPtr tmpAttr = NULL;

		ret = OpenSOAPXMLAttrCreateString(attrName,
										  1,
										  &tmpAttr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLAttrSetValueString(tmpAttr,
												attrType,
												attrValue);
			if (OPENSOAP_SUCCEEDED(ret)) {
				OpenSOAPXMLNodePtr addNode
					= (OpenSOAPXMLNodePtr)tmpAttr;
				OpenSOAPXMLNodePtr *attrList
					= (OpenSOAPXMLNodePtr *)&elm->xmlAttrList;
				ret = OpenSOAPXMLNodeListPushBack(attrList,
												  addNode);
				if (OPENSOAP_SUCCEEDED(ret)) {
					/* set defined element */
					tmpAttr->definedElement = elm;

					if (attr) {
						/* return attribute pointer */
						*attr = tmpAttr;
					}
					tmpAttr = NULL;
				}
			}

			if (OPENSOAP_FAILED(ret)) {
				/* release tmpAttr */
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddAttributeMB(elm, attrName, attrType, attrValue, attr)
    Add Attribute(MB)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        SOAP Element.
      :const char * [in] ((|attrName|))
        Attribute Name
      :const char * [in] ((|attrType|))
        Attribute Type
      :void * [in] ((|attrValue|))
        Attribute Value
      :OpenSOAPXMLAttrPtr * [out] ((|attr|))
        OpenSOAP XML Attribute
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmAddAttributeMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const char *attrName,
                             /* [in]  */ const char *attrType,
                             /* [in]  */ void *attrValue,
                             /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPStringPtr attrNameStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(attrName, &attrNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr attrTypeStr = NULL;
		ret = OpenSOAPStringCreateWithMB(attrType, &attrTypeStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmAddAttributeString(elm,
												   attrNameStr,
												   attrTypeStr,
												   attrValue, attr);
			OpenSOAPStringRelease(attrTypeStr);
		}
		OpenSOAPStringRelease(attrNameStr);
	}

	return ret;
}


/*
=begin
--- function#OpenSOAPXMLElmAddAttributeWC(elm, attrName, attrType, attrValue, attr)
    Add Attribute(WC)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        SOAP Element.
      :const wcharT * [in] ((|attrName|))
        Attribute Name
      :const wcharT * [in] ((|attrType|))
        Attribute Type
	  :void * [in] ((|attrValue|))
        Attribute Value
      :OpenSOAPXMLAttrPtr * [out] ((|attr|))
        OpenSOAP XML Attribute
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmAddAttributeWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const wchar_t *attrName,
                             /* [in]  */ const wchar_t *attrType,
                             /* [in]  */ void *attrValue,
                             /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPStringPtr attrNameStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(attrName, &attrNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr attrTypeStr = NULL;
		ret = OpenSOAPStringCreateWithWC(attrType, &attrTypeStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmAddAttributeString(elm,
												   attrNameStr,
												   attrTypeStr,
												   attrValue, attr);
			OpenSOAPStringRelease(attrTypeStr);
		}
		OpenSOAPStringRelease(attrNameStr);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetAttributeString(elm, attrName, attr)
    Get Attribute Pointer of Assignment Attribute Name(OpenSOAPString)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
      :OpenSOAPStringPtr [in] ((|attrName|))
        Attribute Name
      :OpenSOAPXMLAttrPtr * [out] ((|attr|))
        OpenSOAP XML Attribute
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetAttributeString(/* [in]  */ OpenSOAPXMLElmPtr elm,
								 /* [in]  */ OpenSOAPStringPtr attrName,
								 /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && attr) {
		OpenSOAPXMLNodePtr node = NULL;
		OpenSOAPXMLNodePtr *nodeList = (OpenSOAPXMLNodePtr *)&elm->xmlAttrList;
		ret = OpenSOAPXMLNodeListSearchNamedNodeString(nodeList,
													   attrName,
													   &node);
		*attr = (OpenSOAPXMLAttrPtr)node;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetAttributeMB(elm, attrName, attr)
    Get Attribute Pointer of Assignment Attribute Name(MB)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
      :const char * [in] ((|attrName|))
        Attribute Name
      :OpenSOAPXMLAttrPtr * [out] ((|attr|))
        OpenSOAP XML Attribute
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetAttributeMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const char *attrName,
                             /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPStringPtr attrNameStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(attrName, &attrNameStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmGetAttributeString(elm, attrNameStr, attr);
		OpenSOAPStringRelease(attrNameStr);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetAttributeWC(elm, attr_name, attr)
    Get Attribute Pointer of Assignment Attribute Name(WC)

    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        XML Element.
      :const wchar_t * [in] ((|attr_name|))
        Attribute Name
      :OpenSOAPXMLAttrPtr * [out] ((|attr|))
        OpenSOAP XML Attribute
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetAttributeWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const wchar_t *attrName,
                             /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPStringPtr attrNameStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(attrName, &attrNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmGetAttributeString(elm, attrNameStr, attr);
		OpenSOAPStringRelease(attrNameStr);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetNextChild(elm, cldElm)
    Get next child XML Element.

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :OpenSOAPXMLElmPtr * [in, out] ((|cldElm|))
        Next XML Element Pointer of Assignment XML Element
        But If *cldElm as in NULL then Return To First XML Element Pointer 
    :Return Value value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetNextChild(/* [in]      */ OpenSOAPXMLElmPtr elm,
                           /* [in, out] */ OpenSOAPXMLElmPtr *cldElm) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && cldElm) {
		OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)*cldElm;
		OpenSOAPXMLNodePtr nextNode = NULL;
		OpenSOAPXMLNodePtr *nodeList = &((OpenSOAPXMLNodePtr)elm)->children;
		
		ret = OpenSOAPXMLNodeListGetNextNode(nodeList,
											 node,
											 &nextNode);

		*cldElm = (OpenSOAPXMLElmPtr)nextNode;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddChildString(elm, cldName, cldElm)
    Add Child XML Element(OpenSOAPString)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :OpenSOAPStringPtr [in] ((|cldName|))
        Add Child Element Name
      :OpenSOAPXMLElmPtr * [out] ((|cldElm|))
        Add Child Element Pointer
    :Return Value value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmAddChildString(/* [in]  */ OpenSOAPXMLElmPtr elm,
							 /* [in]  */ OpenSOAPStringPtr childName,
							 /* [in, out] */ OpenSOAPXMLElmPtr *childElm) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (elm && childName && childElm) {
		ret = (*childElm) ? OPENSOAP_NO_ERROR
			: OpenSOAPXMLElmCreate(childElm);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)elm;
			OpenSOAPXMLNodePtr childNode = (OpenSOAPXMLNodePtr)*childElm;
			OpenSOAPStringPtr childElmName = NULL;
			ret = OpenSOAPStringDuplicate(childName, &childElmName);
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLNodeListPushBack(&node->children,
												  childNode);
				
				if (OPENSOAP_SUCCEEDED(ret)) {
					/* swap */
					OpenSOAPStringPtr prevChildName = childNode->name;
					childNode->name = childElmName;
					childElmName = prevChildName;
					/* parent setting */
					childNode->parent = node;
				}
				OpenSOAPStringRelease(childElmName);
			}

			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPXMLElmRelease(*childElm);
			}
		}
	}
	
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddChildMB(elm, cldName, cldElm)
    Add Child XML Element(MB)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :cosnt char * [in] ((|cldName|))
        Add Child Element Name
      :OpenSOAPXMLElmPtr * [out] ((|cldElm|))
        Add Child Element Pointer
    :Return Value value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmAddChildMB(/* [out] */ OpenSOAPXMLElmPtr elm,
						 /* [in]  */ const char *cldName,
						 /* [out] */ OpenSOAPXMLElmPtr *cldElm) {
	OpenSOAPStringPtr cldNameStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(cldName, &cldNameStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmAddChildString(elm, cldNameStr, cldElm);

		OpenSOAPStringRelease(cldNameStr);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddChildWC(elm, cldName, cldElm)
    Add Child XML Element(WC)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :cosnt wchar_t * [in] ((|cldName|))
        Add Child Element Name
      :OpenSOAPXMLElmPtr * [out] ((|cldElm|))
        Add Child Element Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmAddChildWC(/* [out] */ OpenSOAPXMLElmPtr elm,
						 /* [in]  */ const wchar_t *cldName,
						 /* [out] */ OpenSOAPXMLElmPtr *cldElm) {
	OpenSOAPStringPtr cldNameStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(cldName, &cldNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmAddChildString(elm, cldNameStr, cldElm);
		OpenSOAPStringRelease(cldNameStr);
	}
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddChildXMLDocument(elm, elmname, document, charEnc)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  char * ((|elmname|))
        New OpenSOAP XML Element name
      :[in]  OpenSOAPByteArrayPtr ((|document|))
        XML Document
      :[in]  const char * ((|charEnc|))
        Character Encoding
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmAddChildXMLDocument(OpenSOAPXMLElmPtr elm,
					char *elmname,
					OpenSOAPByteArrayPtr document,
					const char *charEnc) {
	OpenSOAPXMLElmPtr	child_elm = NULL;
	xmlDocPtr		doc = NULL;
	xmlNodePtr		node = NULL;

	int ret = OpenSOAPXMLElmCreate(&child_elm);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmAddChildMB(elm, elmname, &child_elm);
	}

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPLibxmlDocCreateCharEncoding(charEnc, document, &doc, &node);
	}

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmCreateChildNodeFromLibxml((OpenSOAPXMLElmPtr)child_elm, node);
	}

	xmlFreeDoc(doc);
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetChildString(elm, chldName, chldElm)
    Get Child XML Element(OpenSOAPString)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :OpenSOAPStringPtr [in] ((|chldName|))
        Assignment Element Name 
      :OpenSOAPXMLElmPtr * [out] ((|chldElm|))
        XML Element Pointer of Assignment Element Name 
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetChildString(/* [in]  */ OpenSOAPXMLElmPtr elm,
							 /* [in]  */ OpenSOAPStringPtr chldName,
							 /* [out] */ OpenSOAPXMLElmPtr *chldElm) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && chldElm) {
		OpenSOAPXMLNodePtr node = NULL;
		OpenSOAPXMLNodePtr *nodeList = &((OpenSOAPXMLNodePtr)elm)->children;
		ret = OpenSOAPXMLNodeListSearchNamedNodeString(nodeList,
													   chldName,
													   &node);
		*chldElm = (OpenSOAPXMLElmPtr)node;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetChildMB(elm, cldName, cldElm)
    Get Child XML Element(MB)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :cosnt char * [in] ((|cldName|))
        Assignment Element Name 
      :OpenSOAPXMLElmPtr * [out] ((|cldElm|))
        XML Element Pointer of Assignment Element Name 
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetChildMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
						 /* [in]  */ const char *cldName,
						 /* [out] */ OpenSOAPXMLElmPtr *cldElm) {
	OpenSOAPStringPtr cldNameStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(cldName, &cldNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmGetChildString(elm, cldNameStr, cldElm);
		OpenSOAPStringRelease(cldNameStr);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetChildWC(elm, cldName, cldElm)
    Get Child XML Element(WC)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :cosnt wchar_t * [in] ((|cldName|))
        Assignment Element Name 
      :OpenSOAPXMLElmPtr * [out] ((|cldElm|))
        XML Element Pointer of Assignment Element Name 
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetChildWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
						 /* [in]  */ const wchar_t *cldName,
						 /* [out] */ OpenSOAPXMLElmPtr *cldElm) {
	OpenSOAPStringPtr cldNameStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(cldName, &cldNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmGetChildString(elm, cldNameStr, cldElm);
		OpenSOAPStringRelease(cldNameStr);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetValueString(elm, typeName, value)
    Get Value of XML Element(OpenSOAPString)

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  OpenSOAPString ((|typeName|))
        Type Name
      :[out] void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetValueString(/* [in]  */ OpenSOAPXMLElmPtr elm,
							 /* [in]  */ OpenSOAPStringPtr typeName,
							 /* [out] */ void *value) {
	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)elm;
	int ret = OpenSOAPXMLNodeGetValueString(node,
											typeName,
											value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetValueMB(elm, typeName, value)
    Get Value of XML Element(MB)

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|type_name|))
        Type Name
      :[out] void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetValueMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                         /* [in]  */ const char *typeName,
                         /* [out] */ void *value) {
	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)elm;
	int ret = OpenSOAPXMLNodeGetValueMB(node,
										typeName,
										value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetValueWC(elm, typeName, value)
    Get Value of XML Element(WC)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :const wchar_t * [in] ((|typeName|))
        Type Name
      :void * [out] ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                         /* [in]  */ const wchar_t *typeName,
                         /* [out] */ void *value) {
	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)elm;
	int ret = OpenSOAPXMLNodeGetValueWC(node,
										typeName,
										value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetValueString(elm, typeName, value)
    Setting Value of XML Element(OpenSOAPString)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :OpenSOAPStringPtr [in] ((|typeName|))
        Type Name
      :void * [in] ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetValueString(/* [out] */ OpenSOAPXMLElmPtr elm,
							 /* [in]  */ OpenSOAPStringPtr typeName,
							 /* [in]  */ void *value) {
	int ret = OpenSOAPXMLNodeSetValueString((OpenSOAPXMLNodePtr)elm,
											typeName,
											value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetValueMB(elm, typeName, value)
    Setting Value of XML Element(MB)

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :const char * [in] ((|typeName|))
        Type Name
      :void * [in] ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetValueMB(/* [in] */ OpenSOAPXMLElmPtr elm,
                         /* [in] */ const char *typeName,
                         /* [in] */ void *value) {
	int ret = OpenSOAPXMLNodeSetValueMB((OpenSOAPXMLNodePtr)elm,
										typeName,
										value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetValueWC(elm, typeName, value)
    Setting Value of XML Element(WC)

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|typeName|))
        Type Name
      :[in]  void * ((|value|))
        Setting Buffer Pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetValueWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                         /* [in]  */ const wchar_t *typeName,
                         /* [in]  */ void *value) {
	int ret = OpenSOAPXMLNodeSetValueWC((OpenSOAPXMLNodePtr)elm,
										typeName,
										value);

	return ret;
}

/*
=begin
--- function#OpenSOAPLibxmlDocDumpByteArray(doc, chEnc, bAry)
    Libxml's doc dump to OpenSOAPByteArray.

    :Parameters
      :[in]  xmlDocPtr ((|doc|))
        libxml document pointer.
      :[in]  const char  *((|chEnc|))
        character encoding. i.e. "EUC-JP", "Shift_JIS", "UTF-8"
      :[out] OpenSOAPByteArrayPtr ((|bAry|))
        Result Buffer        
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
static
int
OpenSOAPLibxmlDocDumpByteArray(/* [in]  */ xmlDocPtr	doc,
							   /* [in]  */ const char *chEnc,
							   /* [out] */ OpenSOAPByteArrayPtr bAry) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (doc) {
		const char XML_PROC_BEGIN[] = "<?xml";
		const char XML_PROC_END[] = "?>";
		xmlChar *xmlText = NULL;
		xmlChar *xmlTextEnd = NULL;
		size_t xmlTextLen = 0;
		xmlChar *xmlBodyBegin = NULL;
		
		/* change xmlDoc to text */
#if 1
		xmlKeepBlanksDefault(0);
		xmlDocDumpFormatMemory(doc,
							   &xmlText,
							   &xmlTextLen,
							   1);
#else		
		xmlDocDumpMemory(doc, &xmlText, &xmlTextLen);
#endif
		if (!xmlText) {
			ret = OPENSOAP_MEM_BADALLOC; /* ? */
			return ret;
		}
		ret = OPENSOAP_NO_ERROR;
		/* */
		xmlTextEnd = xmlText + xmlTextLen;
		/* */
		xmlBodyBegin = xmlText;

		/* delete the processing line <?xml ... ?> */
		if (strncmp(xmlBodyBegin,
					XML_PROC_BEGIN, sizeof(XML_PROC_BEGIN) - 1) == 0) {
			/* have xml processing line */
			char *xmlProcEnd = strstr(xmlBodyBegin, XML_PROC_END);
			if (!xmlProcEnd) {
				ret = OPENSOAP_XML_BADMAKEDOCUMENT;
			}
			else {
				const char OMITTED_CHARS[] = "\r\n";
				xmlBodyBegin = xmlProcEnd + sizeof(XML_PROC_END) - 1;

				/* remove CR/LF */
				for (; xmlBodyBegin != xmlTextEnd
						 && strchr(OMITTED_CHARS, *xmlBodyBegin);
					 ++xmlBodyBegin) {
				}
			}
		}

		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPByteArrayPtr utf8XmlText = NULL;
			/* create ByteArray */
			ret = OpenSOAPByteArrayCreate(&utf8XmlText);
			if (OPENSOAP_SUCCEEDED(ret)) {
				/* set UTF-8 encode character data */
				ret = OpenSOAPStringConvertXMLCharRefToUTF8(xmlBodyBegin,
															utf8XmlText);
				if (OPENSOAP_SUCCEEDED(ret)) {
					/* convert UTF-8 to chEnc character encoding */
					ret = OpenSOAPStringConvertCharEncoding("UTF-8",
															utf8XmlText,
															chEnc,
															bAry);
				}
			}
			/* release temporary byte array */
			OpenSOAPByteArrayRelease(utf8XmlText);
		}
		/* */
		xmlFree(xmlText);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrListAddToLibxml(attrList, doc, elm)
    Add attribute to the element node in libxml structure from opensoap structure 
    
    :Parameters
      :[in]  OpenSOAPXMLAttrPtr ((|attrList|))
        OpenSOAP XML Attribute list
      :[out] xmlDocPtr ((|doc|))
        Libxml Document Pointer
      :[out] xmlNodePtr ((|elm|))
        Libxml Node Pointer
    :Return value
      :type int
        error code.
=end
 */
static
int
OpenSOAPXMLAttrListAddToLibxml(/* [in]  */ OpenSOAPXMLAttrPtr attrList,
							   /* [out] */ xmlDocPtr doc,
							   /* [out] */ xmlNodePtr elm) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && doc) {
		char *utf8AttrName = NULL;
		char *utf8AttrValue = NULL;
		char *utf8NsPrefix = NULL;
		char *utf8NsURI = NULL;
		OpenSOAPXMLNodePtr attr = (OpenSOAPXMLNodePtr)attrList;
		for (ret = OPENSOAP_NO_ERROR; attr; attr = attr->next) {
			OpenSOAPXMLNamespacePtr attrNs = attr->thisNamespace;
			xmlNsPtr libxmlAttrNs = NULL;
			xmlAttrPtr libxmlAttr = NULL;
			/* get name */
			ret = OpenSOAPStringGetStringUTF8WithAllocator(attr->name,
														   NULL,
														   NULL,
														   &utf8AttrName);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}
			/* get value */
			ret = OpenSOAPStringGetStringUTF8WithAllocator(attr->value,
														   NULL,
														   NULL,
														   &utf8AttrValue);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}
			
			if (attrNs) {
				/* get URI and prefix */
				ret = OpenSOAPXMLNamespaceGetPropertiesUTF8WithAllocator(attrNs,
																		 NULL,
																		 &utf8NsURI,
																		 &utf8NsPrefix);
				if (OPENSOAP_FAILED(ret)) {
					break;
				}

				/* search Namespace */
				libxmlAttrNs = xmlSearchNs(doc, elm, utf8NsPrefix);
				if (!libxmlAttrNs
					|| (!libxmlAttrNs->href && !utf8NsURI)
					|| !(libxmlAttrNs->href && utf8NsURI
						&& (strcmp(libxmlAttrNs->href, utf8NsURI) == 0))) {
					libxmlAttrNs = xmlNewNs(elm,
											utf8NsURI,
											utf8NsPrefix);
				}
			}

			libxmlAttr = libxmlAttrNs
				? xmlNewNsProp(elm, libxmlAttrNs, utf8AttrName, utf8AttrValue)
				: xmlSetProp(elm, utf8AttrName, utf8AttrValue);

			free(utf8NsURI);
			utf8NsURI = NULL;
			free(utf8NsPrefix);
			utf8NsPrefix = NULL;
			free(utf8AttrValue);
			utf8AttrValue = NULL;
			free(utf8AttrName);
			utf8AttrName = NULL;
		}
		free(utf8NsURI);
		free(utf8NsPrefix);
		free(utf8AttrValue);
		free(utf8AttrName);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmIsParentDefinedNs(elm, ns, result)
    check parent element's defined namespace
    
    :Parameters
      :[in]  OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP Element Pointer
      :[in]  OpenSOAPXMLNamespacePtr ((|ns|))
        OpenSOAP XML Namespace Pointer
      :[out] int * ((|result|))
        result return buffer. If *result is non zero,
		then ns is defined at parents.
    :Return value
      :type int
        error code.
      
=end
*/
static
int
OpenSOAPXMLElmIsParentDefinedNs(/* [in]  */ OpenSOAPXMLElmPtr elm,
								/* [in]  */ OpenSOAPXMLNamespacePtr ns,
								/* [out] */ int *result) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && ns && result) {
		OpenSOAPXMLNodePtr parent = (OpenSOAPXMLNodePtr)elm;

		ret = OPENSOAP_NO_ERROR;
		*result = 0;

		for (parent = parent->parent;
			parent; parent = parent->parent) {
			OpenSOAPDLinkListItemPtr nsItr
				= (OpenSOAPDLinkListItemPtr)
				(((OpenSOAPXMLElmPtr)parent)->definedXMLNsList);
			for (; nsItr && (OpenSOAPXMLNamespacePtr)nsItr != ns;
				 nsItr = nsItr->next) {
			}
			if (nsItr) {
				*result = 1;
				break;
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetNsToLibxmlElm(childElm, childNode, doc)
    Add namespace to the element node in libxml structure from opensoap structure 
    
    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm_child|))
        OpenSOAP Element Pointer
      :[out] xmlDocPtr ((|doc|))
        Libxml Document Pointer
      :[out] xmlNodePtr ((|childNode|))
        Libxml Node Pointer
    :Return value
      :type int
        error code.
      
=end
*/
static
int
OpenSOAPXMLElmSetNsToLibxmlElm(/* [in]  */ OpenSOAPXMLElmPtr elm,
							   /* [out] */ xmlDocPtr  doc,
							   /* [out] */ xmlNodePtr childNode) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (elm && childNode && doc) {
		xmlNsPtr childNs = NULL;
		char *utf8NsURI = NULL;
		char *utf8NsPrefix = NULL;
		OpenSOAPXMLNamespacePtr elmsNamespace
			= ((OpenSOAPXMLNodePtr)elm)->thisNamespace;

		if (elmsNamespace) {
			int isParentDefinedNs = 0;
			ret = OpenSOAPXMLElmIsParentDefinedNs(elm,
												  elmsNamespace,
												  &isParentDefinedNs);
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLNamespaceGetPropertiesUTF8WithAllocator(elmsNamespace,
															NULL,
															&utf8NsURI,
															&utf8NsPrefix);
				if (OPENSOAP_SUCCEEDED(ret)) {
					xmlNsPtr	childNs = NULL;
					/* check weather there is a namespace defined */
					xmlNsPtr searchNs = xmlSearchNsByHref(doc,
														  childNode,
														  utf8NsURI);
					/* NS is parent Defined, NsURI set to NULL */
					if (isParentDefinedNs) {
						free(utf8NsURI);
						utf8NsURI = NULL;
					}

					if (searchNs
						&& (!searchNs->prefix || !utf8NsPrefix
							 || ((strcmp(searchNs->prefix, utf8NsPrefix) == 0)
								  && !isParentDefinedNs))) {
						childNs = searchNs;
					}
				}
			}
		}
		else {
			/* add NULL namespace to this element */
			ret = OPENSOAP_NO_ERROR;
		}

		if (OPENSOAP_SUCCEEDED(ret)) {
			if (!childNs) {
				childNs = xmlNewNs(childNode, utf8NsURI, utf8NsPrefix);
			}

			xmlSetNs(childNode, childNs);
		}
		/* free utf8 string */
		free(utf8NsURI);
		free(utf8NsPrefix);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddNsDefsToLibxmlElm(childElm, doc, childNode)
    Add namespace definition to the element node in libxml structure from opensoap structure 
    
    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm_child|))
        OpenSOAP Element Pointer
      :[out] xmlDocPtr ((|doc|))
        Libxml Document Pointer
      :[out] xmlNodePtr ((|childNode|))
        Libxml Node Pointer
    :Return value
      :type int
        error code.
      
=end
*/
static
int
OpenSOAPXMLElmAddNsDefsToLibxmlElm(/* [in]  */ OpenSOAPXMLElmPtr elm,
								   /* [out] */ xmlDocPtr  doc,
								   /* [out] */ xmlNodePtr childNode) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (elm && childNode && doc) {
		char *utf8NsURI = NULL;
		char *utf8NsPrefix = NULL;
		OpenSOAPDLinkListItemPtr nsItr
			= (OpenSOAPDLinkListItemPtr)(elm->definedXMLNsList);

		for (ret = OPENSOAP_NO_ERROR; nsItr; nsItr = nsItr->next) {
			OpenSOAPXMLNamespacePtr ns
				= (OpenSOAPXMLNamespacePtr)nsItr;
			ret = OpenSOAPXMLNamespaceGetPropertiesUTF8WithAllocator(ns,
														NULL,
														&utf8NsURI,
														&utf8NsPrefix);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}

			if (utf8NsURI) {
				/* check weather there is a namespace defined */
				xmlNsPtr searchNs = xmlSearchNsByHref(doc,
													  childNode,
													  utf8NsURI);
				if (!searchNs
					|| (searchNs->prefix && utf8NsPrefix
						&& (strcmp(searchNs->prefix, utf8NsPrefix)
							!= 0))) {
					xmlNsPtr childNs = xmlNewNs(childNode,
												utf8NsURI,
												utf8NsPrefix);
				}
			}
			/* */
			free(utf8NsURI);
			utf8NsURI = NULL;
			free(utf8NsPrefix);
			utf8NsPrefix = NULL;
		}
		/* */
		free(utf8NsURI);
		free(utf8NsPrefix);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddNsToLibxml(doc, childElm, childNode)
    Add namespace to the element node in libxml structure from opensoap structure 
    
    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|childElm|))
        OpenSOAP Element Pointer
      :[in] xmlDocPtr ((|doc|))
        Libxml Document Pointer
      :[out] xmlNodePtr ((|childNode|))
        Libxml Node Pointer
    :Return value
      :type int
        error code.
      
=end
*/
static
int
OpenSOAPXMLElmAddNsToLibxml(/* [in]  */ OpenSOAPXMLElmPtr elm,
							/* [out] */ xmlDocPtr doc,
							/* [out] */ xmlNodePtr childNode) {
	int ret = OpenSOAPXMLElmSetNsToLibxmlElm(elm, doc, childNode);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmAddNsDefsToLibxmlElm(elm, doc, childNode);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddNodeToLibxml(elm, doc, node)
    create libxml structure from opensoap structure 
    
    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP Element Pointer
      :[out] xmlDocPtr ((|doc|))
        Libxml Document Pointer
      :[out] xmlNodePtr ((|node|))
        Libxml Node Pointer
    :Return value
      :type int
        error code.
      
=end
*/
static
int
OpenSOAPXMLElmAddNodeToLibxml(/* [in]  */ OpenSOAPXMLElmPtr elm,
							  /* [out] */ xmlDocPtr doc,
							  /* [out] */ xmlNodePtr node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	/* parameter check */
	if (elm && node && doc) {
		char *utf8NodeName = NULL;
		char *utf8NodeValue = NULL;
		OpenSOAPXMLNodePtr	nodeItr = ((OpenSOAPXMLNodePtr)elm)->children;
		for (ret = OPENSOAP_NO_ERROR; nodeItr; nodeItr = nodeItr->next) {
			OpenSOAPXMLElmPtr childElm = (OpenSOAPXMLElmPtr)nodeItr;
			xmlNodePtr	childNode = NULL;
			if (nodeItr->value) {
				ret = OpenSOAPStringGetStringUTF8WithAllocator(nodeItr->value,
															   NULL,
															   NULL,
															   &utf8NodeValue);
				if (OPENSOAP_FAILED(ret)) {
					break;
				}
			}
			/* */
			ret = OpenSOAPStringGetStringUTF8WithAllocator(nodeItr->name,
														   NULL,
														   NULL,
														   &utf8NodeName);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}
			/* */
			childNode = xmlNewTextChild(node,
										NULL,
										utf8NodeName,
										utf8NodeValue);
			
			/* add element's namespace */
			ret = OpenSOAPXMLElmAddNsToLibxml(childElm,
											  doc,
											  childNode);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}

			/* add attributes */
			ret = OpenSOAPXMLAttrListAddToLibxml(childElm->xmlAttrList,
												 doc,
												 childNode);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}

			/* add node */
			ret = OpenSOAPXMLElmAddNodeToLibxml(childElm, doc, childNode);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}

			free(utf8NodeName);
			utf8NodeName = NULL;
			free(utf8NodeValue);
			utf8NodeValue = NULL;
		}

		free(utf8NodeName);
		free(utf8NodeValue);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetCharEncodingString(elm, chEnc, bAry)
    XML Element character encoding output

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char  *((|chEnc|))
        character encoding. i.e. "EUC-JP", "Shift_JIS", "UTF-8"
      :[out] OpenSOAPByteArrayPtr ((|bAry|))
        Result Buffer        
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetCharEncodingString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                    /* [in]  */ const char *chEnc,
                                    /* [out] */ OpenSOAPByteArrayPtr bAry) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (elm) {
		xmlDocPtr	doc = NULL;
		OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)elm;
		char *utf8NodeName = NULL;
		char *utf8NodeValue = NULL;

		/* make a new xmlDoc */ 
		doc = xmlNewDoc("1.0");

		ret = OpenSOAPStringGetStringUTF8WithAllocator(node->name,
													   NULL,
													   NULL,
													   &utf8NodeName);
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (node->value) {
				ret = OpenSOAPStringGetStringUTF8WithAllocator(node->value,
															   NULL,
															   NULL,
															   &utf8NodeValue);
			}
			if (OPENSOAP_SUCCEEDED(ret)) {
				xmlNodePtr	elmNode = xmlNewDocRawNode(doc, 
													   NULL, 
													   utf8NodeName,
													   utf8NodeValue);
				doc->children = elmNode;
				
				/* add root node's namespace */
				ret = OpenSOAPXMLElmAddNsToLibxml(elm,
												  doc,
												  elmNode);
				if (OPENSOAP_SUCCEEDED(ret)) {
					/* add root node's attributes */
					ret = OpenSOAPXMLAttrListAddToLibxml(elm->xmlAttrList,
														 doc,
														 elmNode);
					if (OPENSOAP_SUCCEEDED(ret)) {
						/* add children node */
						ret =  OpenSOAPXMLElmAddNodeToLibxml(elm,
															 doc,
															 elmNode);
						if (OPENSOAP_SUCCEEDED(ret)) {
							/* libxml document to byte array */
							ret = OpenSOAPLibxmlDocDumpByteArray(doc,
																 chEnc,
																 bAry);
						}
					}
				}
			}
		}

		/* free utf8 strings */
		free(utf8NodeValue);
		free(utf8NodeName);
		/* free libxml document */
		xmlFreeDoc(doc);
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddAttrFromLibxmlNode(elm, node)
    Add attribute to the element in opensoap structure from libxml structure  
    
    :Parameters
      :xmlNodePtr [in, out] ((|childNode|))
        Libxml Node Pointer
      :OpenSOAPXMLElmPtr [in] ((|elm_cld|))
        OpenSOAP Element Pointer
    :Return value
      :type int
        error code.
      
=end
*/
static
int
OpenSOAPLibxmlNsGetPropatiesString(/* [in]  */ xmlNsPtr ns,
								   /* [out] */ OpenSOAPStringPtr *uri,
								   /* [out] */ OpenSOAPStringPtr *prefix) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (ns && uri && prefix) {
		*uri = NULL;
		*prefix = NULL;
		ret = OPENSOAP_NO_ERROR;
		if (ns->href) {
			ret = OpenSOAPStringCreateWithUTF8(ns->href,
											   uri);
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (ns->prefix) {
				ret = OpenSOAPStringCreateWithUTF8(ns->prefix,
												   prefix);
			}
		}
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPStringRelease(*uri);
			*uri = NULL;
			OpenSOAPStringRelease(*prefix);
			*prefix = NULL;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddAttrFromLibxmlNode(elm, node)
    Add attribute to the element in opensoap structure from libxml structure  
    
    :Parameters
      :xmlNodePtr [in, out] ((|childNode|))
        Libxml Node Pointer
      :OpenSOAPXMLElmPtr [in] ((|elm_cld|))
        OpenSOAP Element Pointer
    :Return value
      :type int
        error code.
      
=end
*/
int
OpenSOAPXMLElmAddAttrFromLibxmlNode(/* [out] */ OpenSOAPXMLElmPtr elm,
									/* [in]  */ xmlNodePtr node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (elm && node) {
		xmlAttrPtr attrItr = node->properties;
		OpenSOAPXMLNodePtr *next
			= (OpenSOAPXMLNodePtr *)&elm->xmlAttrList;
		OpenSOAPXMLNodePtr last = *next;
		if (last) {
			next = &(*next)->next;
		}
		/* search last of list */
		for (; *next; last = *next, next = &last->next) {
		}
		
		ret = OPENSOAP_NO_ERROR;
		for (;OPENSOAP_SUCCEEDED(ret) && attrItr;
			 attrItr = attrItr->next, last = *next, next = &last->next) {
			OpenSOAPXMLAttrPtr addAttr = NULL;
			OpenSOAPXMLNodePtr addNode = NULL;
			ret = OpenSOAPXMLAttrCreateLibxmlAttr(attrItr,
												  &addAttr);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}
			addAttr->definedElement = elm;
			addNode = (OpenSOAPXMLNodePtr)addAttr;
			if (attrItr->ns) {
				OpenSOAPStringPtr uri = NULL;
				OpenSOAPStringPtr prefix = NULL;
				
				ret = OpenSOAPLibxmlNsGetPropatiesString(attrItr->ns,
														 &uri,
														 &prefix);
				if (OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPXMLAttrSetNamespaceString(addAttr,
															uri,
															prefix);
					
					OpenSOAPStringRelease(prefix);
					OpenSOAPStringRelease(uri);
				}
			}
			if (OPENSOAP_FAILED(ret)) {
				/* */
				break;
			}
			*next = addNode;
			addNode->prev = last;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmAddAttrFromLibxmlNode(elm, node)
    Add attribute to the element in opensoap structure from libxml structure  
    
    :Parameters
      :xmlNodePtr [in, out] ((|childNode|))
        Libxml Node Pointer
      :OpenSOAPXMLElmPtr [in] ((|elm_cld|))
        OpenSOAP Element Pointer
    :Return value
      :type int
        error code.
      
=end
*/
int
OpenSOAPXMLElmAddNsFromLibxmlNode(/* [out] */ OpenSOAPXMLElmPtr elm,
								  /* [in]  */ xmlNodePtr node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (elm && node) {
		ret = OPENSOAP_NO_ERROR;
		if (node->ns) {
			/* set namespace */
			OpenSOAPStringPtr uri = NULL;
			OpenSOAPStringPtr prefix = NULL;

			ret = OpenSOAPLibxmlNsGetPropatiesString(node->ns,
													 &uri,
													 &prefix);
			
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLElmSetNamespaceString(elm,
													   uri,
													   prefix);

				OpenSOAPStringRelease(prefix);
				OpenSOAPStringRelease(uri);
			}
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* add defined namespaces */
			OpenSOAPStringPtr uri = NULL;
			OpenSOAPStringPtr prefix = NULL;
			xmlNsPtr nsItr = node->nsDef;
			for (; OPENSOAP_SUCCEEDED(ret) && nsItr; nsItr = nsItr->next) {
				ret = OpenSOAPLibxmlNsGetPropatiesString(nsItr,
														 &uri,
														 &prefix);
				if (OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPXMLElmDefineNamespaceString(elm,
															  uri,
															  prefix,
															  NULL);
				}
				/* release */
				OpenSOAPStringRelease(prefix);
				prefix = NULL;
				OpenSOAPStringRelease(uri);
				uri = NULL;
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmCreateChildNodeFromLibxml(elm, node)
    create opensoap structure from libxml structure 
    
    :Parameters
      :OpenSOAPXMLElmPtr [in, out] ((|elm|))
        OpenSOAP Element Pointer
      :xmlNodePtr [in] ((|node|))
        Libxml Node Pointer
    :Return value
      :type int
        error code.
      
=end
*/
int
OpenSOAPXMLElmCreateChildNodeFromLibxml(/* [in, out] */ OpenSOAPXMLElmPtr elm,
										/* [in]      */ xmlNodePtr node) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && node) {
		xmlNodePtr	childNode = node->children;
		for (ret = OPENSOAP_NO_ERROR; childNode; childNode = childNode->next) {
			const char XMLNODE_TYPE_TEXT[] = "text";
			if (xmlIsBlankNode(childNode)) {
				continue;
			}
			if (childNode->name
				&& strcmp(childNode->name, XMLNODE_TYPE_TEXT) == 0) {
				/* text node */
				OpenSOAPStringPtr contentStr = NULL;
				ret = OpenSOAPStringCreateWithUTF8(childNode->content,
												   &contentStr);
				if (OPENSOAP_SUCCEEDED(ret)) {
					/* */
					OpenSOAPStringRelease(((OpenSOAPXMLNodePtr)elm)->value);
					((OpenSOAPXMLNodePtr)elm)->value = contentStr;
				}
			}
			else {
				/* not text node */
				OpenSOAPStringPtr childName = NULL;
				OpenSOAPXMLElmPtr childElm = NULL;
				/* create node name */
				ret = OpenSOAPStringCreateWithUTF8(childNode->name,
												   &childName);
				if (OPENSOAP_FAILED(ret)) {
					break;
				}
				
				/* add child element */
				ret = OpenSOAPXMLElmAddChildString(elm,
												   childName,
												   &childElm);
				OpenSOAPStringRelease(childName);
				if (OPENSOAP_FAILED(ret)) {
					break;
				}
				
				/* add element namespace */
				ret = OpenSOAPXMLElmAddNsFromLibxmlNode(childElm, childNode);
				if (OPENSOAP_FAILED(ret)) {
					break;
				}

				/* add element's attributes */
				ret = OpenSOAPXMLElmAddAttrFromLibxmlNode(childElm,
														  childNode);
				if (OPENSOAP_FAILED(ret)) {
					break;
				}
			
				/* add element's child */
				ret = OpenSOAPXMLElmCreateChildNodeFromLibxml(childElm,
															  childNode);
				if (OPENSOAP_FAILED(ret)) {
					break;
				}
			}
		}
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetNameString(elm, name)
    Getting of XML Element's Name as OpenSOAPString

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        XML Element Pointer.
      :[out] OpenSOAPStringPtr * ((|name|))
        Result Name of Node.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
int
OPENSOAP_API
OpenSOAPXMLElmGetNameString(/* [in]  */ OpenSOAPXMLElmPtr elm,
							/* [out] */ OpenSOAPStringPtr *name) {
	int ret = OpenSOAPXMLNodeGetName((OpenSOAPXMLNodePtr)elm, name);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetNextAttr(elm, attr)
    Get next attr XML Element.

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :OpenSOAPXMLAttrPtr * [in, out] ((|attr|))
        Next Attr Element Pointer of Assignment XML Element
        But If *attr as in NULL then Return To First XML Element Pointer 
    :Return Value value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetNextAttr(/* [in] */ OpenSOAPXMLElmPtr elm,
                          /* [in, out] */ OpenSOAPXMLAttrPtr *attr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attr) {
		OpenSOAPXMLNodePtr list = (OpenSOAPXMLNodePtr)elm->xmlAttrList;
		OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)*attr;
		ret = OPENSOAP_NO_ERROR;
		if (node) {
			OpenSOAPXMLNodePtr itr = list;
			for (; itr && itr != node; itr = itr->next) {
			}
			if (itr) {
				/* */
				*attr = (OpenSOAPXMLAttrPtr)node->next;
			}
			else {
				/* not include attribute */
				ret = OPENSOAP_PARAMETER_BADVALUE;
			}
		}
		else {
			/* first item */
			*attr = (OpenSOAPXMLAttrPtr)list;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetChildValueString(elm, pName, typeName, value)
    Get Child Value

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|childName|))
        child element name
      :[in]  const char * ((|typeName|))
        value's type name
      :[out] void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
static
int
OpenSOAPXMLElmGetChildValueString(/* [in]  */ OpenSOAPXMLElmPtr elm,
								  /* [in]  */ OpenSOAPStringPtr childName,
								  /* [in]  */ OpenSOAPStringPtr typeName,
								  /* [out] */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && childName && typeName && value) {
		OpenSOAPXMLElmPtr childElm = NULL;
		ret = OpenSOAPXMLElmGetChildString(elm, childName, &childElm);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = childElm ?
				OpenSOAPXMLElmGetValueString(childElm, typeName, value)
				: OPENSOAP_XMLNODE_NOT_FOUND;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetChildValueString(elm, pName, typeName, value)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|childName|))
        child element name
      :[in]  const wchar_t * ((|typeName|))
        value's type name
      :[in]  void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
static
int
OpenSOAPXMLElmSetChildValueString(/* [out] */ OpenSOAPXMLElmPtr elm,
								  /* [in]  */ OpenSOAPStringPtr childName,
								  /* [in]  */ OpenSOAPStringPtr typeName,
								  /* [in]  */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && childName && typeName && value) {
		OpenSOAPXMLElmPtr childElm = NULL;
		ret = OpenSOAPXMLElmGetChildString(elm, childName, &childElm);
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (!childElm) {
				ret = OpenSOAPXMLElmAddChildString(elm, childName, &childElm);
			}
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLElmSetValueString(childElm, typeName, value);
			}
		}
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetChildValueMB(elm, pName, typeName, value)
    Get Child Value

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|childName|))
        child element name
      :[in]  const char * ((|typeName|))
        value's type name
      :[out] void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetChildValueMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
							  /* [in]  */ const char *childName,
							  /* [in]  */ const char *typeName,
							  /* [out] */ void *value) {
	OpenSOAPStringPtr childNameStr = NULL;
	/* create child name string */
	int ret = OpenSOAPStringCreateWithMB(childName, &childNameStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr typeNameStr = NULL;
		/* create type name string */
		ret = OpenSOAPStringCreateWithMB(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* get value */
			ret = OpenSOAPXMLElmGetChildValueString(elm,
													childNameStr,
													typeNameStr,
													value);
			/* release type name string */
			OpenSOAPStringRelease(typeNameStr);
		}
		/* release child name string */
		OpenSOAPStringRelease(childNameStr);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetChildValueWC(elm, pName, typeName, value)
    Get Child Value

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|childName|))
        child element name
      :[in]  const wchar_t * ((|typeName|))
        value's type name
      :[out] void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetChildValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
							  /* [in]  */ const wchar_t *childName,
							  /* [in]  */ const wchar_t *typeName,
							  /* [out] */ void *value) {
	OpenSOAPStringPtr childNameStr = NULL;
	/* create child name string */
	int ret = OpenSOAPStringCreateWithWC(childName, &childNameStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr typeNameStr = NULL;
		/* create type name string */
		ret = OpenSOAPStringCreateWithWC(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* get value */
			ret = OpenSOAPXMLElmGetChildValueString(elm,
													childNameStr,
													typeNameStr,
													value);
			/* release type name string */
			OpenSOAPStringRelease(typeNameStr);
		}
		/* release child name string */
		OpenSOAPStringRelease(childNameStr);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetChildValueMB(elm, pName, typeName, value)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|childName|))
        child element name
      :[in]  const char * ((|typeName|))
        value's type name
      :[in]  void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetChildValueMB(/* [out] */ OpenSOAPXMLElmPtr elm,
							  /* [in]  */ const char *childName,
							  /* [in]  */ const char *typeName,
							  /* [in]  */ void *value) {
	OpenSOAPStringPtr childNameStr = NULL;
	/* create child name string */
	int ret = OpenSOAPStringCreateWithMB(childName, &childNameStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr typeNameStr = NULL;
		/* create type name string */
		ret = OpenSOAPStringCreateWithMB(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* set value */
			ret = OpenSOAPXMLElmSetChildValueString(elm,
													childNameStr,
													typeNameStr,
													value);
			/* release type name string */
			OpenSOAPStringRelease(typeNameStr);
		}
		/* release child name string */
		OpenSOAPStringRelease(childNameStr);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetChildValueWC(elm, pName, typeName, value)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|childName|))
        child element name
      :[in]  const wchar_t * ((|typeName|))
        value's type name
      :[in]  void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetChildValueWC(/* [out] */ OpenSOAPXMLElmPtr elm,
							  /* [in]  */ const wchar_t *childName,
							  /* [in]  */ const wchar_t *typeName,
							  /* [in]  */ void *value) {
	OpenSOAPStringPtr childNameStr = NULL;
	/* create child name string */
	int ret = OpenSOAPStringCreateWithWC(childName, &childNameStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr typeNameStr = NULL;
		/* create type name string */
		ret = OpenSOAPStringCreateWithWC(typeName, &typeNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* set value */
			ret = OpenSOAPXMLElmSetChildValueString(elm,
													childNameStr,
													typeNameStr,
													value);
			/* release type name string */
			OpenSOAPStringRelease(typeNameStr);
		}
		/* release child name string */
		OpenSOAPStringRelease(childNameStr);
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetChildValueXMLDocument(elm, document, charEnc)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  OpenSOAPByteArrayPtr ((|document|))
        XML Document
      :[in]  const char * ((|charEnc|))
        Character Encoding
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetChildValueXMLDocument(OpenSOAPXMLElmPtr elm,
						OpenSOAPByteArrayPtr document,
						const char *charEnc) {
	xmlDocPtr		doc = NULL;
	xmlNodePtr		node = NULL;

	int ret = OpenSOAPLibxmlDocCreateCharEncoding(charEnc, document, &doc, &node);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPXMLElmCreateChildNodeFromLibxml((OpenSOAPXMLElmPtr)elm, node);
	}

	xmlFreeDoc(doc);
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetValueAsString(elm, name, isDup)
    XML Element value set as string

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        XML Elm Pointer.
      :[in]  OpenSOAPStringPtr ((|value|))
        Setting Elm Name
      :[in]  int ((|isDup|))
        duplicate flag. If non zero, then value is duplicate.
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetValueAsString(/* [out] */ OpenSOAPXMLElmPtr elm,
							   /* [in]  */ OpenSOAPStringPtr value,
							   /* [in]  */ int isDup) {
    int ret = OpenSOAPXMLNodeSetValueAsString((OpenSOAPXMLNodePtr)elm,
											  value,
											  isDup);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetValueAsStringMB(elm, name)
    XML Element value set as string

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        XML Elm Pointer.
      :[in]  OpenSOAPStringPtr ((|value|))
        value as string
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetValueAsStringMB(/* [out] */ OpenSOAPXMLElmPtr elm,
								 /* [in]  */ const char *value) {
    int ret = OpenSOAPXMLNodeSetValueAsStringMB((OpenSOAPXMLNodePtr)elm,
												value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetValueAsStringWC(elm, name)
    XML Element value set as string

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        XML Elm Pointer.
      :[in]  const wchar_t * ((|value|))
        value as string
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetValueAsStringWC(/* [out] */ OpenSOAPXMLElmPtr elm,
								 /* [in]  */ const wchar_t *value) {
    int ret = OpenSOAPXMLNodeSetValueAsStringWC((OpenSOAPXMLNodePtr)elm,
												value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetChildValueAsString(elm, childName, value, isValueDup)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  OpenSOAPStringPtr ((|childName|))
        child element name
      :[in]  int ((|isValueDup|))
        value duplicate flag.
      :[in, out]  OpenSOAPStringPtr ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetChildValueAsString(/* [out] */ OpenSOAPXMLElmPtr elm,
									/* [in]  */ OpenSOAPStringPtr childName,
									/* [in]  */ int isValueDup,
									/* [in, out] */ OpenSOAPStringPtr value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && childName && value) {
		OpenSOAPXMLElmPtr childElm = NULL;
		ret = OpenSOAPXMLElmGetChildString(elm, childName, &childElm);
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (!childElm) {
				ret = OpenSOAPXMLElmAddChildString(elm, childName, &childElm);
			}
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLElmSetValueAsString(childElm,
													 value,
													 isValueDup);
			}
		}
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetChildValueAsStringMB(elm, childName, value)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|childName|))
        child element name
      :[in]  const char * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetChildValueAsStringMB(/* [out] */ OpenSOAPXMLElmPtr elm,
									  /* [in]  */ const char *childName,
									  /* [in]  */ const char *value) {
	OpenSOAPStringPtr childNameStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(childName, &childNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr valueStr = NULL;
		ret = OpenSOAPStringCreateWithMB(value, &valueStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmSetChildValueAsString(elm,
													  childNameStr,
													  0,
													  valueStr);
			
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPStringRelease(valueStr);
			}
		}

		OpenSOAPStringRelease(childNameStr);
	}
	
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetChildValueAsStringWC(elm, childName, value)
    Set Child Value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|childName|))
        child element name
      :[in]  const wchar_t * ((|value|))
        value buffer.
    :Return Value value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetChildValueAsStringWC(/* [out] */ OpenSOAPXMLElmPtr elm,
									  /* [in]  */ const wchar_t *childName,
									  /* [in]  */ const wchar_t *value) {
	OpenSOAPStringPtr childNameStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(childName, &childNameStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr valueStr = NULL;
		ret = OpenSOAPStringCreateWithWC(value, &valueStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmSetChildValueAsString(elm,
													  childNameStr,
													  0,
													  valueStr);
			
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPStringRelease(valueStr);
			}
		}

		OpenSOAPStringRelease(childNameStr);
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmRemoveChildString(elm, childName, isValueRelease, childValue)
    Remove Child XML Element

    :Parameters
      :OpenSOAPXMLElmPtr [in] ((|elm|))
        OpenSOAP XML Element.
      :OpenSOAPStringPtr [in] ((|childName|))
        Remove child element name
	  :[in]  int * ((|isValueRelease|))
	    Child value release flag.
      :[out] OpenSOAPStringPtr * ((|childValue|))
        Removed child value stored buffer. If isValueRelease is not true,
		return NULL. If childValue equal NULL, then no return value.
    :Return Value value
      :int
	    Error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLElmRemoveChildString(/* [out] */ OpenSOAPXMLElmPtr elm,
								/* [in]  */ OpenSOAPStringPtr childName,
								/* [in]  */ int isValueRelease,
								/* [out] */ OpenSOAPStringPtr *childValue) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (elm) {
		OpenSOAPXMLNodePtr *nodeList
			= &((OpenSOAPXMLNodePtr)elm)->children;
		ret = OpenSOAPXMLNodeListRemoveNamedNodeString(nodeList,
													   childName,
													   isValueRelease,
													   childValue);
	}
	
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetAttributeValueMB(elm, attrName, typeName, value)
    Get XML attribute value

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|attrName|))
        attribute name
      :[in]  const char * ((|typeName|))
        value's type name
      :[out] void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetAttributeValueMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
								  /* [in]  */ const char *attrName,
								  /* [in]  */ const char *typeName,
								  /* [out] */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && typeName && value) {
		OpenSOAPXMLAttrPtr attr = NULL;
		ret = OpenSOAPXMLElmGetAttributeMB(elm, attrName, &attr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLAttrGetValueMB(attr, typeName, value);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmGetAttributeValueWC(elm, attrName, typeName, value)
    Get XML attribute value

    :Parameters
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|attrName|))
        attribute name
      :[in]  const wchar_t * ((|typeName|))
        value's type name
      :[out] void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmGetAttributeValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
								  /* [in]  */ const wchar_t *attrName,
								  /* [in]  */ const wchar_t *typeName,
								  /* [out] */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && typeName && value) {
		OpenSOAPXMLAttrPtr attr = NULL;
		ret = OpenSOAPXMLElmGetAttributeWC(elm, attrName, &attr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLAttrGetValueWC(attr, typeName, value);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetAttributeValueMB(elm, attrName, typeName, value)
    Set XML attribute value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|attrName|))
        attribute name
      :[in]  const char * ((|typeName|))
        value's type name
      :[in]  void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetAttributeValueMB(/* [out] */ OpenSOAPXMLElmPtr elm,
								  /* [in]  */ const char *attrName,
								  /* [in]  */ const char *typeName,
								  /* [in]  */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && typeName && value) {
		OpenSOAPXMLAttrPtr attr = NULL;
		ret = OpenSOAPXMLElmGetAttributeMB(elm, attrName, &attr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = attr
				? OpenSOAPXMLAttrSetValueMB(attr, typeName, value) /* found */
				: OpenSOAPXMLElmAddAttributeMB(elm,
											   attrName,
											   typeName,
											   value,
											   &attr); /* not found */
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetAttributeValueWC(elm, attrName, typeName, value)
    Set XML attribute value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|attrName|))
        attribute name
      :[in]  const wchar_t * ((|typeName|))
        value's type name
      :[in]  void * ((|value|))
        value buffer.
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetAttributeValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
								  /* [in]  */ const wchar_t *attrName,
								  /* [in]  */ const wchar_t *typeName,
								  /* [out] */ void *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && typeName && value) {
		OpenSOAPXMLAttrPtr attr = NULL;
		ret = OpenSOAPXMLElmGetAttributeWC(elm, attrName, &attr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = attr
				? OpenSOAPXMLAttrSetValueWC(attr, typeName, value) /* found */
				: OpenSOAPXMLElmAddAttributeWC(elm,
											   attrName,
											   typeName,
											   value,
											   &attr); /* not found */
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetAttributeValueAsString(elm, attrName, value, isDup)
    Set XML attribute value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  OpenSOAPStringPtr ((|attrName|))
        attribute name
      :[in]  void * ((|value|))
        value buffer.
	  :[in]  int isDup
	    Duplicate flag
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
/* OPENSOAP_API */
OpenSOAPXMLElmSetAttributeValueAsString(/* [out] */ OpenSOAPXMLElmPtr elm,
										/* [in]  */ OpenSOAPStringPtr attrName,
										/* [in]  */ OpenSOAPStringPtr value,
										/* [in]  */ int isDup) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && value) {
		OpenSOAPXMLNodePtr *nodeList
			= (OpenSOAPXMLNodePtr *)&elm->xmlAttrList;
		OpenSOAPXMLNodePtr attr = NULL;
		ret = OpenSOAPXMLNodeListGetNamedNodeString(nodeList,
													attrName,
													isDup,
													(int (*)
													 (OpenSOAPXMLNodePtr *))
													OpenSOAPXMLAttrCreate,
													&attr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNodeSetValueAsString(attr,
												  value,
												  isDup);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetAttributeValueAsStringMB(elm, attrName, typeName, value)
    Set XML attribute value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|attrName|))
        attribute name
      :[in]  const char * ((|value|))
        value buffer.
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetAttributeValueAsStringMB(/* [out] */ OpenSOAPXMLElmPtr elm,
										  /* [in]  */ const char *attrName,
										  /* [in]  */ const char *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && value) {
		OpenSOAPStringPtr attrNameStr = NULL;
		ret = OpenSOAPStringCreateWithMB(attrName,
										 &attrNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr valueStr = NULL;
			ret = OpenSOAPStringCreateWithMB(value,
											 &valueStr);
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLElmSetAttributeValueAsString(elm,
															  attrNameStr,
															  valueStr,
															  0);
				if (OPENSOAP_FAILED(ret)) {
					OpenSOAPStringRelease(valueStr);
				}
			}
			/* release */
			OpenSOAPStringRelease(attrNameStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmSetAttributeValueAsStringWC(elm, attrName, typeName, value)
    Set XML attribute value

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const wchar_t * ((|attrName|))
        attribute name
      :[in]  const wchar_t * ((|value|))
        value buffer.
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLElmSetAttributeValueAsStringWC(/* [out] */ OpenSOAPXMLElmPtr elm,
										  /* [in]  */ const wchar_t *attrName,
										  /* [in]  */ const wchar_t *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm && attrName && value) {
		OpenSOAPStringPtr attrNameStr = NULL;
		ret = OpenSOAPStringCreateWithWC(attrName,
										 &attrNameStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr valueStr = NULL;
			ret = OpenSOAPStringCreateWithWC(value,
											 &valueStr);
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLElmSetAttributeValueAsString(elm,
															  attrNameStr,
															  valueStr,
															  0);
				if (OPENSOAP_FAILED(ret)) {
					OpenSOAPStringRelease(valueStr);
				}
			}
			/* release */
			OpenSOAPStringRelease(attrNameStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLElmRemoveAttributeString(elm, attrName, isValueRelease, value)
    Remove XML attribute with name

    :Parameters
      :[out] OpenSOAPXMLElmPtr ((|elm|))
        OpenSOAP XML Element.
      :[in]  const char * ((|attrName|))
        attribute name
	  :[in]  int ((|isValueRelease|))
	    Value release flag.
	  :[out] OpenSOAPStringPtr * ((|value|))
	    Value stored buffer.
    :Return Value value
      :int
	    Error code.
=end
 */
extern
int
/* OPENSOAP_API */
OpenSOAPXMLElmRemoveAttributeString(/* [out] */ OpenSOAPXMLElmPtr elm,
									/* [in]  */ OpenSOAPStringPtr attrName,
									/* [in]  */ int isValueRelease,
									/* [out] */ OpenSOAPStringPtr *value) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm) {
		OpenSOAPXMLNodePtr *attrList
			= (OpenSOAPXMLNodePtr *)&elm->xmlAttrList;

		ret = OpenSOAPXMLNodeListRemoveNamedNodeString(attrList,
													   attrName,
													   isValueRelease,
													   value);
	}

	return ret;
}


/*
 */
static
const unsigned char *
searchXMLDocBody(/* [in] */ const unsigned char *xmldocBegin,
				 /* [in] */ size_t xmldocSize) {
	static const unsigned char XML_PROC_END[] = "?>";
	static const unsigned char *XML_PROC_END_END
		= XML_PROC_END + sizeof(XML_PROC_END) - 1;

	const unsigned char *ret = xmldocBegin;
	if (xmldocBegin && xmldocSize) {
		const unsigned char *xmldocEnd = xmldocBegin + xmldocSize;
		const unsigned char *xmlProcEndI = XML_PROC_END;
		for (; xmlProcEndI != XML_PROC_END_END
				 && ret != xmldocEnd; ++ret) {
			if (*ret == *xmlProcEndI) {
				++xmlProcEndI;
			}
			else {
				xmlProcEndI = XML_PROC_END;
			}
		}
		if (ret == xmldocEnd) {
			ret = xmldocBegin;
		}
		for (; ret != xmldocEnd && isspace(*ret); ++ret) {
		}
	}

	return ret;
}

/*
 */
static
int
OpenSOAPByteArrayRemoveXMLProcAttr(/* [in]  */ OpenSOAPByteArrayPtr xmldoc,
								   /* [out] */ char **buf,
								   /* [out] */ size_t *bufSz) {
	static const char DEFAULT_XML_PROC[] = "<?xml version=\"1.0\"?>";
	static const size_t DEFAULT_XML_PROC_SIZE
		= sizeof(DEFAULT_XML_PROC) - 1;
	static const char SGML_TAG_BEGIN = '<';
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (xmldoc && buf && bufSz) {
		const unsigned char *xmldocBegin = NULL;
		size_t xmldocSize = 0;
		*buf = NULL;
		*bufSz = 0;
		ret = OpenSOAPByteArrayGetBeginSizeConst(xmldoc,
												 &xmldocBegin,
												 &xmldocSize);
		if (OPENSOAP_SUCCEEDED(ret)) {
			const unsigned char *xmldocBody 
				= searchXMLDocBody(xmldocBegin, xmldocSize);
			size_t xmldocBodySize = xmldocSize - (xmldocBody - xmldocBegin);
			if (xmldocBodySize == 0
				|| *xmldocBody != SGML_TAG_BEGIN) {
				ret = OPENSOAP_XML_NOTXMLDOCUMENT;
			} 
			else {
				*bufSz = DEFAULT_XML_PROC_SIZE + xmldocBodySize;
				*buf = malloc(*bufSz);
				if (*buf) {
					memcpy(*buf, DEFAULT_XML_PROC, DEFAULT_XML_PROC_SIZE);
					memcpy(*buf + DEFAULT_XML_PROC_SIZE,
						   xmldocBody, xmldocBodySize);
				}
				else {
					*bufSz = 0;
					ret = OPENSOAP_MEM_BADALLOC;
				}
			}
		}
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPLibxmlDocCreateCharEncoding(chEnc, bAry, doc, rootNode)
    create char encoding
    
    :Parameters
      :[in]  const cahr * ((|chEnc|))
        character encoding
      :[in]  OpenSOAPByteArrayPtr ((|bAry|))
        OpenSOAP ByteArray
      :[out] xmlDocPtr * ((|doc|))
        libxml document buffer.
      :[out] xmlNodePtr * ((|rootNode|))
        libxml document root node buffer.
    :Return value
      :type int
        error code.
    :Note
      
=end
*/
extern
int
OpenSOAPLibxmlDocCreateCharEncoding(const char *chEnc,
									OpenSOAPByteArrayPtr bAry,
									xmlDocPtr	*doc,
									xmlNodePtr	*rootNode) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (bAry && doc && rootNode) {
		OpenSOAPByteArrayPtr utf8BAry = NULL;
		*doc = NULL;
		*rootNode = NULL;
		/* change b_arry to "UTF-8" */
		ret = OpenSOAPByteArrayCreate(&utf8BAry);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPStringConvertCharEncoding(chEnc,
													bAry,
													"UTF-8",
													utf8BAry);
			if (OPENSOAP_SUCCEEDED(ret)) {
				char *utf8Buf = NULL;
				size_t utf8BufSize = 0;
				ret = OpenSOAPByteArrayRemoveXMLProcAttr(utf8BAry,
														 &utf8Buf,
														 &utf8BufSize);
				if (OPENSOAP_SUCCEEDED(ret)) {
					*doc = xmlParseMemory(utf8Buf, utf8BufSize);
					if (!*doc) {
						ret = OPENSOAP_XML_BADMAKEDOCUMENT;
					}
					else {
						/* check document (root node) */
						*rootNode = xmlDocGetRootElement(*doc);
						if (!*rootNode) {
							xmlFreeDoc(*doc);
							*doc = NULL;
							ret = OPENSOAP_XML_BADMAKEDOCUMENT;
						}
					}
					free(utf8Buf);
				}
			}
			OpenSOAPByteArrayRelease(utf8BAry);
		}
	}
	
	return ret;
}
