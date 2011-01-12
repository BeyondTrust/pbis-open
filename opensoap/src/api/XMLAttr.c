/*-----------------------------------------------------------------------------
 * $RCSfile: XMLAttr.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: XMLAttr.c,v 1.24 2002/11/06 06:41:31 bandou Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/Serializer.h>
#include "XMLAttr.h"
#include "XMLNamespace.h"

/*
=begin
= OpenSOAP XML Attribute class
=end
 */

static
int
OpenSOAPXMLAttrReleaseMembers(/* [in, out] */ OpenSOAPXMLAttrPtr attr) {
    int ret = OpenSOAPXMLNodeReleaseMembers((OpenSOAPXMLNodePtr)attr);

    return ret;
}

static
int
OpenSOAPXMLAttrFree(/* [in, out] */ OpenSOAPObjectPtr obj) {
    int ret = OpenSOAPXMLAttrReleaseMembers((OpenSOAPXMLAttrPtr)obj);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

static
int
OpenSOAPXMLAttrInitialize(/* [in, out] */ OpenSOAPXMLAttrPtr attr) {
    int ret = OpenSOAPXMLNodeInitialize((OpenSOAPXMLNodePtr)attr,
                                        OpenSOAPXMLAttrFree);

    if (attr) {
        attr->definedElement = NULL;
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrCreate(attr)
    OpenSOAP XML Attribute instance create(OpenSOAPString)

    :Parameters
      :[out] OpenSOAPXMLAttrPtr * ((|attr|))
        XML Attribute pointer
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OpenSOAPXMLAttrCreate(/* [out] */ OpenSOAPXMLAttrPtr *attr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (attr) {
        ret = OPENSOAP_MEM_BADALLOC;
        *attr = malloc(sizeof(OpenSOAPXMLAttr));
        if (*attr) {
            ret = OpenSOAPXMLAttrInitialize(*attr);
            if (OPENSOAP_FAILED(ret)) {
                free(*attr);
                *attr = NULL;
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrCreateString(attr_name, attr)
    OpenSOAP XML Attribute instance create(OpenSOAPString)

    :Parameters
      :[in]  OpenSOAPStringPtr ((|attrName|))
        XML Attribute name.
      :[in]  int ((|isDup|))
        String duplicate flag.
      :[out] OpenSOAPXMLAttrPtr * ((|attr|))
        XML Attribute pointer
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
/* OPENSOAP_API */
OpenSOAPXMLAttrCreateString(/* [in]  */ OpenSOAPStringPtr attrName,
							/* [in]  */ int isDup,
                            /* [out] */ OpenSOAPXMLAttrPtr *attr) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (attr && attrName) {
        ret = OpenSOAPXMLAttrCreate(attr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)*attr;
			ret = OpenSOAPXMLNodeSetName(node,
										 attrName,
										 isDup);
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPXMLNodeRelease(node);
				*attr = NULL;
			}
		}
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrCreateMB(attrName, attr)
    OpenSOAP XML Attribute instance create(MB)

    :Parameters
      :[in]  const char * ((|attrName|))
        XML Attribute name.
      :[out] OpenSOAPXMLAttrPtr * ((|attr|))
        XML Attribute pointer
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLAttrCreateMB(/* [in]  */ const char *attrName,
                        /* [out] */ OpenSOAPXMLAttrPtr *attr) {
    OpenSOAPStringPtr attrNameStr = NULL;
    int ret = OpenSOAPStringCreateWithMB(attrName, &attrNameStr);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPXMLAttrCreateString(attrNameStr, 0, attr);
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPStringRelease(attrNameStr);
		}
    }
	
    return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrCreateWC(attrName, attr)
    OpenSOAP XML Attribute instance create(WC)

    :Parameters
      :[in]  const wchar_t * ((|attrName|))
        XML Attribute name.
      :[out] OpenSOAPXMLAttrPtr * ((|attr|))
        XML Attribute pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLAttrCreateWC(/* [in]  */ const wchar_t *attrName,
                        /* [out] */ OpenSOAPXMLAttrPtr *attr) {
    OpenSOAPStringPtr attrNameStr = NULL;
    int ret = OpenSOAPStringCreateWithWC(attrName, &attrNameStr);

    if (OPENSOAP_SUCCEEDED(ret)) {
        ret = OpenSOAPXMLAttrCreateString(attrNameStr, 0, attr);
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPStringRelease(attrNameStr);
		}
    }
    return ret;
}

/*
 */
/*
 */
extern
int
/* OPENSOAP_API */
OpenSOAPXMLAttrCreateLibxmlAttr(/* [in]  */ xmlAttrPtr libxmlAttr,
								/* [out] */ OpenSOAPXMLAttrPtr *attr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (libxmlAttr && attr) {
		OpenSOAPStringPtr name = NULL;
		ret = OpenSOAPStringCreateWithUTF8(libxmlAttr->name, &name);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr val = NULL;
			if (libxmlAttr->children && libxmlAttr->children->content) {
				ret = OpenSOAPStringCreateWithUTF8(libxmlAttr
												   ->children->content,
												   &val);
			}
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLAttrCreateString(name,
												  0,
												  attr);
				if (OPENSOAP_SUCCEEDED(ret) && val) {
					OpenSOAPXMLNodePtr node
						= (OpenSOAPXMLNodePtr)*attr;
					ret = OpenSOAPXMLNodeSetValueAsString(node,
														  val,
														  0);
					if (OPENSOAP_FAILED(ret)) {
						/* release */
					}
				}
			}
			
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPStringRelease(val);
				OpenSOAPStringRelease(name);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrSetNamespaceString(attr, nsUri, nsPrefix)
    Set Namespace of XML Element(OpenSOAPString)

    :Parameters
      :[out] OpenSOAPXMLAttrPtr ((|attr|))
        XML Attribute.
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        Namespace URI
      :[in]  OpenSOAPStringPtr ((|nsPrefix|))
        Namespace Prefix

    :Return
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
=end
 */
extern
int
/* OPENSOAP_API */
OpenSOAPXMLAttrSetNamespaceString(/* [out] */ OpenSOAPXMLAttrPtr attr,
                                  /* [in]  */ OpenSOAPStringPtr nsUri,
                                  /* [in]  */ OpenSOAPStringPtr nsPrefix) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (attr) {
		OpenSOAPXMLElmPtr elm = attr->definedElement;
		OpenSOAPXMLNamespacePtr ns = NULL;
		ret = OpenSOAPXMLElmDefineNamespaceString(elm,
												  nsUri,
												  nsPrefix,
												  &ns);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)attr;
			node->thisNamespace = ns;
		}
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrSetNamespaceMB(attr, nsUri, nsPrefix)
    Set Namespace of XML Element(MB)

    :Parameters
      :[out] OpenSOAPXMLAttrPtr ((|attr|))
        XML Attribute.
      :[in]  const char * ((|nsUri|))
        Namespace URI
      :[in]  const char * ((|nsPrefix|))
        Namespace Prefix

    :Return
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLAttrSetNamespaceMB(/* [out] */ OpenSOAPXMLAttrPtr attr,
                              /* [in]  */ const char *nsUri,
                              /* [in]  */ const char *nsPrefix) {
    OpenSOAPStringPtr nsUriStr = NULL;
    int ret = OpenSOAPStringCreateWithMB(nsUri, &nsUriStr);

    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPStringPtr nsPrefixStr = NULL;
        int ret = OpenSOAPStringCreateWithMB(nsPrefix, &nsPrefixStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPXMLAttrSetNamespaceString(attr,
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
--- function#OpenSOAPXMLAttrSetNamespaceWC(attr, nsUri, nsPrefix)
    Set Namespace of XML Element(WC)

    :Parameters
      :[out[ OpenSOAPXMLAttrPtr ((|attr|))
        XML Attribute.
      :[in]  const wchar_t * ((|nsUri|))
        Namespace URI
      :[in]  const wchar_t * ((|nsPrefix|))
        Namespace Prefix

    :Return
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLAttrSetNamespaceWC(/* [out] */ OpenSOAPXMLAttrPtr attr,
                              /* [in]  */ const wchar_t *nsUri,
                              /* [in]  */ const wchar_t *nsPrefix) {
    OpenSOAPStringPtr nsUriStr = NULL;
    int ret = OpenSOAPStringCreateWithWC(nsUri, &nsUriStr);

    if (OPENSOAP_SUCCEEDED(ret)) {
        OpenSOAPStringPtr nsPrefixStr = NULL;
        int ret = OpenSOAPStringCreateWithWC(nsPrefix, &nsPrefixStr);
        if (OPENSOAP_SUCCEEDED(ret)) {
            ret = OpenSOAPXMLAttrSetNamespaceString(attr,
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
--- function#OpenSOAPXMLAttrGetNamespace(attr, ns)
    Get Namespace of XML Element

    :Parameters
      :OpenSOAPXMLAttrPtr [in, out] ((|attr|))
        XML Attribute
      :OpenSOAPXMLNamespacePtr * [out] ((|ns|))
        Namespace 
    :Return
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLAttrGetNamespace(/* [in]  */ OpenSOAPXMLAttrPtr attr,
                            /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (attr && ns) {
		OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)attr;
		*ns = node->thisNamespace;
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrGetValueMB(attr, typeName, value)
    Get Value of XML Attribute(MB)

    :Parameters
      :[in]  OpenSOAPXMLAttrPtr ((|attr|))
        OpenSOAP XML Attribute.
      :[in]  const char * ((|typeName|))
        Type Name
      :[out] void * ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLAttrGetValueMB(/* [in]  */ OpenSOAPXMLAttrPtr attr,
						  /* [in]  */ const char *typeName,
						  /* [out] */ void *value) {
	int ret = OpenSOAPXMLNodeGetValueMB((OpenSOAPXMLNodePtr)attr,
										typeName,
										value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrGetValueWC(elm, type_name, value)
    Get Value of XML Attribute(WC)

    :Parameters
      :[in]  OpenSOAPXMLAttrPtr ((|elm|))
        OpenSOAP XML Attribute.
      :[in]  const wchar_t * ((|type_name|))
        Type Name 
	  :[out] void *  ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLAttrGetValueWC(/* [in]  */ OpenSOAPXMLAttrPtr attr,
						  /* [in]  */ const wchar_t *typeName,
						  /* [out] */ void *value) {
    int ret = OpenSOAPXMLNodeGetValueWC((OpenSOAPXMLNodePtr)attr,
										typeName,
										value);

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrSetValueString(attr, typeName, value)
    Set Value of XML Attribute(OpenSOAPString)

    :Parameters
      :OpenSOAPXMLAttrPtr [in] ((|attr|))
        OpenSOAP XML Attribute.
      :OpenSOAPStringPtr [in] ((|typeName|))
        Type Name
      :void * [in] ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
/* OPENSOAP_API */
OpenSOAPXMLAttrSetValueString(/* [out] */ OpenSOAPXMLAttrPtr attr,
							  /* [in]  */ OpenSOAPStringPtr typeName,
                              /* [in]  */ void *value) {
	int ret = OpenSOAPXMLNodeSetValueString((OpenSOAPXMLNodePtr)attr,
											typeName,
											value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrSetValueMB(attr, typeName, value)
    Set Value of XML Attribute(MB)

    :Parameters
      :[out] OpenSOAPXMLAttrPtr ((|attr|))
        OpenSOAP XML Attribute.
      :[in]  const char * ((|typeName|))
        Type Name
      :[in]  void * ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLAttrSetValueMB(/* [out] */ OpenSOAPXMLAttrPtr attr,
						  /* [in]  */ const char *typeName,
                          /* [in]  */ void *value) {
	int ret = OpenSOAPXMLNodeSetValueMB((OpenSOAPXMLNodePtr)attr,
										typeName,
										value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrSetValueWC(elm, type_name, value)
    Set Value of XML Attribute(WC)

    :Parameters
      :OpenSOAPXMLAttrPtr [in] ((|elm|))
        OpenSOAP XML Attribute.
      :const wchar_t * [in] ((|type_name|))
        Type Name 
      :void * [in] ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLAttrSetValueWC(/* [out] */ OpenSOAPXMLAttrPtr attr,
						  /* [in]  */ const wchar_t *typeName,
                          /* [in]  */ void *value) {
	int ret = OpenSOAPXMLNodeSetValueWC((OpenSOAPXMLNodePtr)attr,
										typeName,
										value);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLAttrGetName(attr, name)
    Getting of SOAP Attribute Name

    :Parameters
      :OpenSOAPXMLAttrPtr [in] ((|attr|))
        SOAP Attribute Pointer.
      :OpenSOAPStringPtr * [out] ((|name|))
        Result Name of SOAP Attribute.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLAttrGetName(/* [in]  */ OpenSOAPXMLAttrPtr attr,
                       /* [out] */ OpenSOAPStringPtr *name) {
    int ret = OpenSOAPXMLNodeGetName((OpenSOAPXMLNodePtr)attr,
                                     name);

    return ret;
}
