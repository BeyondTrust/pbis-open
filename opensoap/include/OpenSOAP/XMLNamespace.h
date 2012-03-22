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
#ifndef OpenSOAP_XMLNamespace_H
#define OpenSOAP_XMLNamespace_H

#include <OpenSOAP/String.h>

/**
 * @file OpenSOAP/XMLNamespace.h
 * @brief OpenSOAP API XML Namespace Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPXMLNamespace OpenSOAPXMLNamespace
     * @brief OpenSOAPXMLNamespace Structure Type Definition
     */
    typedef struct tagOpenSOAPXMLNamespace OpenSOAPXMLNamespace;

    /**
     * @typedef OpenSOAPXMLNamespace *OpenSOAPXMLNamespacePtr
     * @brief OpenSOAPXMLNamespace Pointer Type Definition
     */
    typedef OpenSOAPXMLNamespace *OpenSOAPXMLNamespacePtr;

    /**
     * @typedef struct tagOpenSOAPXMLElm OpenSOAPXMLElm
     * @brief OpenSOAPXMLElm Structure Type Definition
     */
    typedef struct tagOpenSOAPXMLElm OpenSOAPXMLElm;

    /**
     * @typedef OpenSOAPXMLElm *OpenSOAPXMLElmPtr
     * @brief OpenSOAPXMLElm Pointer Type Definition
     */
    typedef OpenSOAPXMLElm *OpenSOAPXMLElmPtr;

    /**
      * @fn int OpenSOAPXMLNamespaceCreateMB(const char * ns_uri, const char * ns_prefix, OpenSOAPXMLNamespacePtr * ns)
      * @brief OpenSOAP XML Namespace Element Instance Create(MB)
      * @param
      *    ns_uri const char * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const char * [in] ((|ns_prefix|)) Namespace prefix
      * @param
      *    ns OpenSOAPXMLElmPtr * [out] ((|ns|)) XML Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceCreateMB(/* [in]  */ const char *ns_uri,
                                 /* [in]  */ const char *ns_prefix,
                                 /* [out] */ OpenSOAPXMLNamespacePtr *ns);

    /**
      * @fn int OpenSOAPXMLNamespaceCreateWC(const wchar_t * ns_uri, const wchar_t * ns_prefix, OpenSOAPXMLNamespacePtr * ns)
      * @brief OpenSOAP XML Namespace Element Instance Create(WC)
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const wchar_t * [in] ((|ns_prefix|)) Namespace prefix
      * @param
      *    ns OpenSOAPXMLElmPtr * [out] ((|ns|)) XML Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceCreateWC(/* [in]  */ const wchar_t *ns_uri,
                                 /* [in]  */ const wchar_t *ns_prefix,
                                 /* [out] */ OpenSOAPXMLNamespacePtr *ns);

    /**
      * @fn int OpenSOAPXMLNamespaceRelease(OpenSOAPXMLNamespacePtr ns)
      * @brief Release XML Namespace area
      * @param
      *    ns OpenSOAPXMLNamespacePtr [out] ((|ns|)) XML Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceRelease(/* [out] */ OpenSOAPXMLNamespacePtr ns);

    /**
      * @fn int OpenSOAPXMLNamespaceCreateString(OpenSOAPStringPtr nsUri, OpenSOAPStringPtr nsPrefix, OpenSOAPXMLNamespacePtr * ns)
      * @brief XML Namespace Create String
      * @param
      *    nsUri OpenSOAPStringPtr [in] ((|nsUri|)) Namespace URI
      * @param
      *    nsPrefix OpenSOAPStringPtr [in] ((|nsPrefix|)) Namespace Prefix
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) XML Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceCreateString(/* [in]  */  OpenSOAPStringPtr nsUri,
                                     /* [in]  */  OpenSOAPStringPtr nsPrefix,
                                     /* [out] */ OpenSOAPXMLNamespacePtr *ns);


    /**
      * @fn int OpenSOAPXMLNamespaceGetURI(OpenSOAPXMLNamespacePtr ns, OpenSOAPStringPtr * ns_uri)
      * @brief Get XML Namespace URI
      * @param
      *    ns OpenSOAPXMLNamespacePtr [in] ((|ns|)) XML Namespace
      * @param
      *    ns_uri OpenSOAPStringPtr * [out] ((|ns_uri|)) Namespace URI
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceGetURI(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
                               /* [out] */ OpenSOAPStringPtr *ns_uri);

    /**
      * @fn int OpenSOAPXMLNamespaceDuplicate(OpenSOAPXMLNamespacePtr ns, OpenSOAPXMLNamespacePtr * dupNs)
      * @brief Duplicate XML Namespace
      * @param
      *    ns OpenSOAPXMLNamespacePtr [in] ((|ns|)) XML Namespace
      * @param
      *    dupNs OpenSOAPXMLNamespacePtr * [out] ((|dupNs|)) XML Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceDuplicate(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
                                  /* [out] */ OpenSOAPXMLNamespacePtr *dupNs);

    /**
      * @fn int OpenSOAPXMLNamespaceGetDefinedXMLElm(OpenSOAPXMLNamespacePtr ns, OpenSOAPXMLElmPtr * elm)
      * @brief Get Defined XML Namespace Element
      * @param
      *    ns OpenSOAPXMLNamespacePtr [in] ((|ns|)) XML Namespace
      * @param
      *    elm OpenSOAPXMLElmPtr * [out] ((|elm|)) XML Element
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceGetDefinedXMLElm(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
                                         /* [out] */ OpenSOAPXMLElmPtr *elm);

    /**
      * @fn int OpenSOAPXMLNamespaceSetDefinedXMLElm(OpenSOAPXMLNamespacePtr ns, OpenSOAPXMLElmPtr elm, OpenSOAPXMLElmPtr * oldElm)
      * @brief Set Defined XML Namespace Element
      * @param
      *    ns OpenSOAPXMLNamespacePtr [out] ((|ns|)) XML Namespace
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) XML Element
      * @param
      *    oldElm OpenSOAPXMLElmPtr * [out] ((|oldElm|)) XML Element
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceSetDefinedXMLElm(/* [out] */ OpenSOAPXMLNamespacePtr ns,
                                         /* [in]  */ OpenSOAPXMLElmPtr elm,
                                         /* [out] */ OpenSOAPXMLElmPtr *oldElm);

    /**
      * @fn int OpenSOAPXMLNamespaceGetPrefix(OpenSOAPXMLNamespacePtr ns, OpenSOAPStringPtr * ns_prefix)
      * @brief Get Namespace Prefix
      * @param
      *    ns OpenSOAPXMLNamespacePtr [in]  ((|ns|)) XML Namespace
      * @param
      *    ns_prefix OpenSOAPStringPtr * [out] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceGetPrefix(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
                                  /* [out] */ OpenSOAPStringPtr *ns_prefix);

    /**
      * @fn int OpenSOAPXMLNamespaceGetPropertiesUTF8WithAllocator(OpenSOAPXMLNamespacePtr ns, char * (*memAllocator)(size_t), char **utf8NsURI, char **utf8NsPrefix)
      * @brief Get XML Namespace Properties
      * @param
      *    ns OpenSOAPXMLNamespacePtr [in]  ((|ns|)) XML Namespace
      * @param
      *    memAllocator char * [in]  ( * ((|memAllocator|)) )(size_t) Character String
      * @param
      *    utf8NsURI char ** [out]  ((|utf8NsURI|)) Character String Array UTF8 Encoded Namespace URI
      * @param
      *    utf8NsPrefix char ** [out]  ((|utf8NsPrefix|)) Character String Array UTF8 Encoded Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceGetPropertiesUTF8WithAllocator(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
                                                       /* [in]  */ char * (*memAllocator)(size_t),
                                                       /* [out] */ char **utf8NsURI,
                                                       /* [out] */ char **utf8NsPrefix);

    /**
      * @fn int OpenSOAPXMLNamespaceIsSameUriString(OpenSOAPXMLNamespacePtr ns, OpenSOAPStringPtr nsUri, int *isSame)
      * @brief Same XML Namespace URI ?
      * @param
      *    ns OpenSOAPXMLNamespacePtr [in]  ((|ns|)) XML Namespace
      * @param
      *    nsUri OpenSOAPStringPtr [in] ((|nsUri|)) Namespace URI
      * @param
      *    isSame int * [out] ((|isSame|)) Is same URI result
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceIsSameUriString(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
                                        /* [in]  */ OpenSOAPStringPtr nsUri,
                                        /* [out] */ int *isSame);

    /**
      * @fn int OpenSOAPXMLNamespaceIsSameString(OpenSOAPXMLNamespacePtr ns, OpenSOAPStringPtr nsUri, OpenSOAPStringPtr nsPrefix, int *isSame)
      * @brief Same XML Namespace URI And Prefix ?
      * @param
      *    ns OpenSOAPXMLNamespacePtr [in]  ((|ns|)) XML Namespace
      * @param
      *    nsUri OpenSOAPStringPtr [in] ((|nsUri|)) Namespace URI
      * @param
      *    nsPrefix OpenSOAPStringPtr [in] ((|nsPrefix|)) Namespace Prefix
      * @param
      *    isSame int * [out] ((|isSame|)) Is same result
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLNamespaceIsSameString(/* [in]  */ OpenSOAPXMLNamespacePtr ns,
                                     /* [in]  */ OpenSOAPStringPtr nsUri,
                                     /* [in]  */ OpenSOAPStringPtr nsPrefix,
                                     /* [out] */ int *isSame);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_XMLNamespace_H */
