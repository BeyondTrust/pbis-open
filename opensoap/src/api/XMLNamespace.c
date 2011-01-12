/*-----------------------------------------------------------------------------
 * $RCSfile: XMLNamespace.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: XMLNamespace.c,v 1.26 2003/01/08 09:03:07 bandou Exp $";
#endif  /* _DEBUG */

#include "XMLNamespace.h"

/*
=begin
= OpenSOAP XML namespace class
=end
 */
int
OpenSOAPXMLNamespaceReleaseMembers(/* [in, out] */ OpenSOAPXMLNamespacePtr
								   ns) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ns) {
        OpenSOAPStringRelease(ns->prefix);
        OpenSOAPStringRelease(ns->uri);

        ret = OpenSOAPDLinkListItemReleaseMembers((OpenSOAPDLinkListItemPtr)ns);
    }

    return ret;
}

int
OpenSOAPXMLNamespaceFree(/* [in, out] */ OpenSOAPObjectPtr obj) {
    int ret = OpenSOAPXMLNamespaceReleaseMembers((OpenSOAPXMLNamespacePtr)obj);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

int
OpenSOAPXMLNamespaceListRelease(/* [in, out] */ OpenSOAPXMLNamespacePtr ns) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
	OpenSOAPDLinkListItemPtr nsItr = (OpenSOAPDLinkListItemPtr)ns;

    while (nsItr) {
        OpenSOAPDLinkListItemPtr next = nsItr->next;
            
        OpenSOAPXMLNamespaceRelease((OpenSOAPXMLNamespacePtr)nsItr);

        nsItr = next;
        ret = OPENSOAP_NO_ERROR;
    }

    return ret;
}

int
OpenSOAPXMLNamespaceInitialize(/* [in, out] */ OpenSOAPXMLNamespacePtr ns) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ns) {
        ns->prefix = NULL;
        ns->uri    = NULL;
        ns->definedElement = NULL;

        ret = OpenSOAPDLinkListItemInitialize((OpenSOAPDLinkListItemPtr)ns,
											  OpenSOAPXMLNamespaceFree);
        
    }

    return ret;
}


/*
=begin
--- function#OpenSOAPXMLNamespaceCreate(ns)
    OpenSOAP XML Namespace Element instance create(OpenSOAPString)

    :Parameters
      :[out] OpenSOAPXMLElmPtr * ((|ns|))
        XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
int
OpenSOAPXMLNamespaceCreate(/* [out] */ OpenSOAPXMLNamespacePtr *ns) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (ns) {
		ret = OPENSOAP_MEM_BADALLOC;
		*ns = malloc(sizeof(OpenSOAPXMLNamespace));
		if (*ns) {
			ret = OpenSOAPXMLNamespaceInitialize(*ns);
			if (OPENSOAP_FAILED(ret)) {
				free(*ns);
				*ns = NULL;
			}
		}
	}

	return ret;
}

static
int
OpenSOAPStringDuplicateAllowNULL(/* [in, out] */ OpenSOAPStringPtr *str) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (str) {
		OpenSOAPStringPtr src = *str;
		*str = NULL;
		ret = (src) ? OpenSOAPStringDuplicate(src, str) : OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceCreateStringWithFlag(nsUri, nsPrefix, isDup, ns)
    OpenSOAP XML Namespace Element instance create(OpenSOAPString)

    :Parameters
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        Namespace URI. this 
      :[in]  OpenSOAPStringPtr ((|nsPrefix|))
        Namespace prefix
      :[in]  int ((|isDup|))
        parameter is duplicate flag.
      :[out] OpenSOAPXMLElmPtr * ((|ns|))
        XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
static
int
OpenSOAPXMLNamespaceCreateStringWithFlag(/* [in] */  OpenSOAPStringPtr nsUri,
										 /* [in] */
										 OpenSOAPStringPtr nsPrefix,
										 /* [in] */  int isDup,
										 /* [out] */
										 OpenSOAPXMLNamespacePtr *ns) {
    int ret = OpenSOAPXMLNamespaceCreate(ns);

	if (OPENSOAP_SUCCEEDED(ret)) {
		if (isDup) {
			ret = OpenSOAPStringDuplicateAllowNULL(&nsUri);
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPStringDuplicateAllowNULL(&nsPrefix);
				if (OPENSOAP_FAILED(ret)) {
					OpenSOAPStringRelease(nsUri);
				}
			}
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			(*ns)->uri    = nsUri;
			(*ns)->prefix = nsPrefix;
		}
		else {
			OpenSOAPXMLNamespaceRelease(*ns);
			*ns = NULL;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceCreateString(nsUri, nsPrefix, ns)
    OpenSOAP XML Namespace Element instance create(OpenSOAPString)

    :Parameters
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        Namespace URI. this 
      :[in]  OpenSOAPStringPtr ((|nsPrefix|))
        Namespace prefix
      :[out] OpenSOAPXMLElmPtr * ((|ns|))
        XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceCreateString(/* [in]  */  OpenSOAPStringPtr nsUri,
								 /* [in]  */  OpenSOAPStringPtr nsPrefix,
								 /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	int ret = OpenSOAPXMLNamespaceCreateStringWithFlag(nsUri,
													   nsPrefix,
													   1,
													   ns);

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceCreateMB(nsUri, nsPrefix, ns)
    OpenSOAP XML Namespace Element instance create(MB)

    :Parameters
      :[in]  const char * ((|nsUri|))
        Namespace URI
      :[in]  const char * ((|nsPrefix|))
        Namespace prefix
      :[out] OpenSOAPXMLElmPtr * ((|ns|))
        XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceCreateMB(/* [in] */  const char *nsUri,
                             /* [in] */  const char *nsPrefix,
                             /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	OpenSOAPStringPtr nsUriStr = NULL;
	int ret = nsUri ? OpenSOAPStringCreateWithMB(nsUri, &nsUriStr)
		: OPENSOAP_NO_ERROR;

	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr nsPrefixStr = NULL;
		ret = nsPrefix ? OpenSOAPStringCreateWithMB(nsPrefix, &nsPrefixStr)
			: OPENSOAP_NO_ERROR;
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNamespaceCreateStringWithFlag(nsUriStr,
														   nsPrefixStr,
														   0,
														   ns);
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPStringRelease(nsPrefixStr);
			}
		}
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPStringRelease(nsUriStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceCreateWC(nsUri, nsPrefix, ns)
    OpenSOAP XML Namespace Element instance create(WC)

    :Parameters
      :const wchar_t * [in] ((|nsUri|))
        Namespace URI
      :const wchar_t * [in] ((|nsPrefix|))
        Namespace prefix
      :OpenSOAPXMLElmPtr * [out] ((|ns|))
        XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceCreateWC(/* [in] */  const wchar_t *nsUri,
                             /* [in] */  const wchar_t *nsPrefix,
                             /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	OpenSOAPStringPtr nsUriStr = NULL;
	int ret = nsUri ? OpenSOAPStringCreateWithWC(nsUri, &nsUriStr)
		: OPENSOAP_NO_ERROR;

	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPStringPtr nsPrefixStr = NULL;
		ret = nsPrefix ? OpenSOAPStringCreateWithWC(nsPrefix, &nsPrefixStr)
			: OPENSOAP_NO_ERROR;
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLNamespaceCreateStringWithFlag(nsUriStr,
														   nsPrefixStr,
														   0,
														   ns);
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPStringRelease(nsPrefixStr);
			}
		}
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPStringRelease(nsUriStr);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceRelease(ns)
    Relese XML Namespace area

    :Parameters
      :[in, out] OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceRelease(/* [in, out] */ OpenSOAPXMLNamespacePtr ns) {
    int ret = OpenSOAPObjectRelease((OpenSOAPObjectPtr)ns);

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceDuplicate(ns, dupNs)
    Get Namespace URI

    :Parameters
      :[in]  OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
      :[out]  OpenSOAPXMLNamespacePtr * ((|dupNs|))
        XML Namespace.
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceDuplicate(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
							  /* [out] */ OpenSOAPXMLNamespacePtr *dupNs) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (ns && dupNs) {
		ret = OpenSOAPXMLNamespaceCreateStringWithFlag(ns->uri,
													   ns->prefix,
													   1,
													   dupNs);
		
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceGetDefinedXMLElm(ns, elm)
    Set defined XML Element.

    :Parameters
      :[out] OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
      :[in]  OpenSOAPXMLElmPtr * ((|elm|))
        XML Element buffer.
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceGetDefinedXMLElm(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
									 /* [out] */ OpenSOAPXMLElmPtr *elm) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (ns && elm) {
		*elm = ns->definedElement;

		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceSetDefinedXMLElm(ns, elm, oldElm)
    Set defined XML Element.

    :Parameters
      :[out] OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
      :[in]  OpenSOAPXMLElmPtr ((|elm|))
        XML Element
      :[out] OpenSOAPXMLElmPtr * ((|oldElm|))
        Previous defined XML Element buffer. If NULL then no effect.
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceSetDefinedXMLElm(/* [out] */ OpenSOAPXMLNamespacePtr ns,
									 /* [in]  */ OpenSOAPXMLElmPtr elm,
									 /* [out] */ OpenSOAPXMLElmPtr *oldElm) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (ns) {
		OpenSOAPXMLElmPtr dummyElm = NULL;
		if (!oldElm) {
			oldElm = &dummyElm;
		}
		*oldElm = ns->definedElement;
		ns->definedElement = elm;

		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceGetURI(ns, nsUri)
    Get Namespace URI

    :Parameters
      :[in]  OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
      :[out] OpenSOAPStringPtr * ((|nsUri|))
        Namespace URI
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceGetURI(/* [in] */  OpenSOAPXMLNamespacePtr ns,
						   /* [out] */ OpenSOAPStringPtr *nsUri) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (ns && nsUri) {
		*nsUri = ns->uri;
		
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceGetPrefix(ns, nsPrefix)
    Get Namespace Prefix

    :Parameters
      :[in]  OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
      :[out] OpenSOAPStringPtr * ((|nsPrefix|))
        Namespace Prefix
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceGetPrefix(/* [in] */  OpenSOAPXMLNamespacePtr ns,
							  /* [out] */ OpenSOAPStringPtr *nsPrefix) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (ns && nsPrefix) {
		*nsPrefix = ns->prefix;
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceIsSameUriString(ns, nsUri, isSame)
    Compare URI and Prefix.

    :Parameters
      :[in]  OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        XML namespace URI.
      :[out] OpenSOAPStringPtr * ((|isSame|))
        Result return buffer.
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceIsSameUriString(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
									/* [in]  */ OpenSOAPStringPtr nsUri,
									/* [out] */ int *isSame) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (ns && isSame) {
		ret = OPENSOAP_NO_ERROR;
		*isSame = 0;
		if (!nsUri && !ns->uri) {
			/* ns->uri and nsUri both NULL */
			*isSame = 1;
		}
		else if (nsUri && ns->uri) {
			int cmpResult = 0;
			ret = OpenSOAPStringCompare(ns->uri,
										nsUri,
										&cmpResult);
			if (OPENSOAP_SUCCEEDED(ret)) {
				*isSame = (cmpResult == 0);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceIsSameString(ns, nsUri, nsPrefix, isSame)
    Compare URI and Prefix.

    :Parameters
      :[in]  OpenSOAPXMLNamespacePtr ((|ns|))
        XML Namespace.
      :[in]  OpenSOAPStringPtr ((|nsUri|))
        XML namespace URI.
      :[in]  OpenSOAPStringPtr ((|nsPrefix|))
        XML namespace prefix.
      :[out] OpenSOAPStringPtr * ((|isSame|))
        Result return buffer.
    :Return Value
      :int
	    error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceIsSameString(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
								 /* [in]  */ OpenSOAPStringPtr nsUri,
								 /* [in]  */ OpenSOAPStringPtr nsPrefix,
								 /* [out] */ int *isSame) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (ns && isSame) {
		int cmpResultUri = 0;
		*isSame = 0;
		ret = OPENSOAP_NO_ERROR;
		if (nsUri) {
			ret = OpenSOAPStringCompare(ns->uri,
										nsUri,
										&cmpResultUri);
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			int cmpResultPrefix = 0;
			if (nsPrefix && ns->prefix) {
				ret = OpenSOAPStringCompare(ns->prefix,
											nsPrefix,
											&cmpResultPrefix);
			}
			if (OPENSOAP_SUCCEEDED(ret)) {
				if (cmpResultPrefix == 0) {
					if (cmpResultUri != 0) {
						if (ns->prefix && nsPrefix) {
							ret = OPENSOAP_XML_NS_URI_UNMATCHED;
						}
					}
					else {
						*isSame = 1;
					}
				}
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPXMLNamespaceGetPropertiesUTF8WithAllocator(ns, memAllocator, utf8NsURI, utf8NsPrefix)
    get XML Namespace properties UTF-8 encoding
    
    :Parameters
      :[in]  OpenSOAPXMLNamespacePtr ((|ns|))
        OpenSOAP XML Namespace Pointer
      :[in]  char *(* ((|memAllocator|)) )(size_t)
        memory allocator
      :[out] char ** ((|utf8NsURI|))
        URI stored buffer.
      :[out] char ** ((|utf8NsPrefix|))
        prefix stored buffer.
    :Return value
      :type int
        error code.
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPXMLNamespaceGetPropertiesUTF8WithAllocator(/* [in]  */
												   OpenSOAPXMLNamespacePtr ns,
												   /* [in]  */
												   char *
												   (*memAllocator)(size_t),
												   /* [out] */
												   char **utf8NsURI,
												   /* [out] */
												   char **utf8NsPrefix) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (ns && utf8NsURI && utf8NsPrefix) {
		ret = (ns->uri)
			? OpenSOAPStringGetStringUTF8WithAllocator(ns->uri,
													   memAllocator,
													   NULL,
													   utf8NsURI)
			: OPENSOAP_NO_ERROR;
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = (ns->prefix)
				? OpenSOAPStringGetStringUTF8WithAllocator(ns->prefix,
														   memAllocator,
														   NULL,
														   utf8NsPrefix)
				: OPENSOAP_NO_ERROR;
		}
	}

	return ret;
}

