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
#ifndef OpenSOAP_XMLAttr_H
#define OpenSOAP_XMLAttr_H

#include <OpenSOAP/XMLNamespace.h>

/**
 * @file OpenSOAP/XMLAttr.h
 * @brief OpenSOAP API XML Attribute Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPXMLAttr OpenSOAPXMLAttr
     * @brief OpenSOAPXMLAttr Structure Type Definition
     */
    typedef struct tagOpenSOAPXMLAttr OpenSOAPXMLAttr;

    /**
     * @typedef OpenSOAPXMLAttr *OpenSOAPXMLAttrPtr
     * @brief OpenSOAPXMLAttr Pointer Type Definition
     */
    typedef OpenSOAPXMLAttr *OpenSOAPXMLAttrPtr;

    /**
      * @fn int OpenSOAPXMLAttrCreateMB(const char * attr_name, OpenSOAPXMLAttrPtr * attr)
      * @brief OpenSOAP XML Attribute Instance Create(MB)
      * @param
      *    attr_name const char * [in] ((|attr_name|)) XML Attribute name
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) XML Attribute pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrCreateMB(const char * /* [in] */ attr_name,
                            OpenSOAPXMLAttrPtr * /* [out] */ attr);

    /**
      * @fn int OpenSOAPXMLAttrCreateWC(const wchar_t * attr_name, OpenSOAPXMLAttrPtr * attr)
      * @brief OpenSOAP XML Attribute Instance Create(WC)
      * @param
      *    attr_name const wchar_t * [in] ((|attr_name|)) XML Attribute name
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) XML Attribute pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrCreateWC(const wchar_t * /* [in] */ attr_name,
                            OpenSOAPXMLAttrPtr * /* [out] */ attr);

    /**
      * @fn int OpenSOAPXMLAttrSetNamespaceMB(OpenSOAPXMLAttrPtr elm, const char * ns_uri, const char * ns_prefix)
      * @brief Set Namespace of XML Element(MB)
      * @param
      *    elm OpenSOAPXMLAttrPtr [in, out] ((|elm|)) XML Attribute
      * @param
      *    ns_uri const char * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const char * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrSetNamespaceMB(OpenSOAPXMLAttrPtr /* [in, out] */ elm,
                                  const char * /* [in] */ ns_uri,
                                  const char * /* [in] */ ns_prefix);

    /**
      * @fn int OpenSOAPXMLAttrSetNamespaceWC(OpenSOAPXMLAttrPtr elm, const wchar_t * ns_uri, const wchar_t * ns_prefix)
      * @brief Set Namespace of XML Element(WC)
      * @param
      *    elm OpenSOAPXMLAttrPtr [in, out] ((|elm|)) XML Attribute
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const wchar_t * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrSetNamespaceWC(OpenSOAPXMLAttrPtr /* [in, out] */ elm,
                                  const wchar_t * /* [in] */ ns_uri,
                                  const wchar_t * /* [in] */ ns_prefix);

    /**
      * @fn int OpenSOAPXMLAttrGetNamespace(OpenSOAPXMLAttrPtr elm, OpenSOAPXMLNamespacePtr * ns)
      * @brief Get Namespace of XML Element
      * @param
      *    elm OpenSOAPXMLAttrPtr [in, out] ((|elm|)) XML Attribute
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrGetNamespace(OpenSOAPXMLAttrPtr /* [in, out] */ elm,
                                OpenSOAPXMLNamespacePtr * /* [out] */ ns);

    /**
      * @fn int OpenSOAPXMLAttrGetValueMB(OpenSOAPXMLAttrPtr elm, const char * type_name, void * value)
      * @brief Get Value of XML Attribute(MB)
      * @param
      *    elm OpenSOAPXMLAttrPtr [in] ((|elm|)) OpenSOAP XML Attribute
      * @param
      *    type_name const char * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrGetValueMB(OpenSOAPXMLAttrPtr /* [in] */ elm,
                              const char * /* [in] */ type_name,
                              void * /* [out] */ value);

    /**
      * @fn int OpenSOAPXMLAttrGetValueWC(OpenSOAPXMLAttrPtr elm, const wchar_t * type_name, void * value)
      * @brief Get Value of XML Attribute(WC)
      * @param
      *    elm OpenSOAPXMLAttrPtr [in] ((|elm|)) OpenSOAP XML Attribute
      * @param
      *    type_name const wchar_t * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrGetValueWC(OpenSOAPXMLAttrPtr /* [in] */ elm,
                              const wchar_t * /* [in] */ type_name,
                              void * /* [out] */ value);

    /**
      * @fn int OpenSOAPXMLAttrSetValueMB(OpenSOAPXMLAttrPtr elm, const char * type_name, void * value)
      * @brief Set Value of XML Attribute(MB)
      * @param
      *    elm OpenSOAPXMLAttrPtr [in] ((|elm|)) OpenSOAP XML Attribute
      * @param
      *    type_name const char * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [in] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrSetValueMB(OpenSOAPXMLAttrPtr /* [in] */ elm,
                              const char * /* [in] */ type_name,
                              void * /* [in] */ value);

    /**
      * @fn int OpenSOAPXMLAttrSetValueWC(OpenSOAPXMLAttrPtr elm, const wchar_t * type_name, void * value)
      * @brief Set Value of XML Attribute(WC)
      * @param
      *    elm OpenSOAPXMLAttrPtr [in] ((|elm|)) OpenSOAP XML Attribute
      * @param
      *    type_name const wchar_t * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [in] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrSetValueWC(OpenSOAPXMLAttrPtr /* [in] */ elm,
                              const wchar_t * /* [in] */ type_name,
                              void * /* [in] */ value);
    
    /**
      * @fn int OpenSOAPXMLAttrGetName(OpenSOAPXMLAttrPtr attr, OpenSOAPStringPtr * name)
      * @brief Get of SOAP Attribute Name
      * @param
      *    attr OpenSOAPXMLAttrPtr [in] ((|attr|)) SOAP Attribute Pointer
      * @param
      *    name OpenSOAPStringPtr * [out] ((|name|)) Result Name of SOAP Attribute
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLAttrGetName(OpenSOAPXMLAttrPtr /* [in] */ attr,
                           OpenSOAPStringPtr * /* [out] */ name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_XMLAttr_H */
