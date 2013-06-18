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
#ifndef OpenSOAP_XMLElm_H
#define OpenSOAP_XMLElm_H

#include <OpenSOAP/XMLAttr.h>

/**
 * @file OpenSOAP/XMLElm.h
 * @brief OpenSOAP API XML Element Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
      * @fn int OpenSOAPXMLElmCreate(OpenSOAPXMLElmPtr *elm)
      * @brief OpenSOAP XML Element Instance Create
      * @param
      *    elm OpenSOAPXMLElmPtr * [out] ((|elm|)) XML Element pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmCreate(/* [out] */ OpenSOAPXMLElmPtr *elm);

    /**
      * @fn int OpenSOAPXMLElmSetNamespaceMB(OpenSOAPXMLElmPtr elm, const char *ns_uri, const char *ns_prefix)
      * @brief Set Namespace of XML Element(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    ns_uri const char * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const char * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetNamespaceMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ const char *ns_uri,
                                 /* [in]  */ const char *ns_prefix);

    /**
      * @fn int OpenSOAPXMLElmSetNamespaceWC(OpenSOAPXMLElmPtr elm, const wchar_t *ns_uri, const wchar_t *ns_prefix)
      * @brief Set Namespace of XML Element(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const wchar_t * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetNamespaceWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ const wchar_t *ns_uri,
                                 /* [in]  */ const wchar_t *ns_prefix);

    /**
      * @fn int OpenSOAPXMLElmSearchNamespaceMB(OpenSOAPXMLElmPtr elm, const char *ns_uri, const char *ns_prefix, OpenSOAPXMLNamespacePtr *ns)
      * @brief Search Namespace of XML Element(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) XML Element
      * @param
      *    ns_uri const char * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const char * [in] ((|ns_prefix|)) Namespace Prefix
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSearchNamespaceMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                    /* [in]  */ const char *ns_uri,
                                    /* [in]  */ const char *ns_prefix,
                                    /* [out] */ OpenSOAPXMLNamespacePtr *ns);
    
    /**
      * @fn int OpenSOAPXMLElmSearchNamespaceWC(OpenSOAPXMLElmPtr elm, const wchar_t *ns_uri, const wchar_t *ns_prefix, OpenSOAPXMLNamespacePtr *ns)
      * @brief Search Namespace of XML Element(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const wchar_t * [in] ((|ns_prefix|)) Namespace Prefix
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSearchNamespaceWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                    /* [in]  */ const wchar_t *ns_uri,
                                    /* [in]  */ const wchar_t *ns_prefix,
                                    /* [out] */ OpenSOAPXMLNamespacePtr *ns);

    /**
      * @fn int OpenSOAPXMLElmGetNamespace(OpenSOAPXMLElmPtr elm, OpenSOAPXMLNamespacePtr *ns)
      * @brief Get Namespace Pointer
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetNamespace(/* [in]  */ OpenSOAPXMLElmPtr elm,
                               /* [out] */ OpenSOAPXMLNamespacePtr *ns);

    /**
      * @fn int OpenSOAPXMLElmSearchNamespaceString(OpenSOAPXMLElmPtr elm, OpenSOAPStringPtr nsUri, OpenSOAPStringPtr nsPrefix, OpenSOAPXMLNamespacePtr *ns, OpenSOAPXMLElmPtr *defElm)
      * @brief Search Namespace String
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) XML Element
      * @param
      *    nsUri OpenSOAPStringPtr [in] ((|nsUri|)) Namespace URI
      * @param
      *    nsPrefix OpenSOAPStringPtr [in] ((|nsPrefix|)) Namespace Prefix
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) Namespace
      * @param
      *    defElm OpenSOAPXMLElmPtr * [out] ((|defElm|)) XML Element
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSearchNamespaceString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                        /* [in]  */ OpenSOAPStringPtr nsUri,
                                        /* [in]  */ OpenSOAPStringPtr nsPrefix,
                                        /* [out] */ OpenSOAPXMLNamespacePtr *ns,
                                        /* [out] */ OpenSOAPXMLElmPtr *defElm);

    /**
      * @fn int OpenSOAPXMLElmDefineNamespaceMB(OpenSOAPXMLElmPtr elm, const char *ns_uri, const char *ns_prefix, OpenSOAPXMLNamespacePtr *ns)
      * @brief Define XML namespace.(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    ns_uri const char * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const char * [in] ((|ns_prefix|)) Namespace Prefix
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) OpenSOAP XML Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmDefineNamespaceMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                                    /* [in]  */ const char *ns_uri,
                                    /* [in]  */ const char *ns_prefix,
                                    /* [out] */ OpenSOAPXMLNamespacePtr *ns);

    /**
      * @fn int OpenSOAPXMLElmDefineNamespaceWC(OpenSOAPXMLElmPtr elm, const wchar_t *ns_uri, const wchar_t *ns_prefix, OpenSOAPXMLNamespacePtr *ns)
      * @brief Define XML namespace.(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const wchar_t * [in] ((|ns_prefix|)) Namespace Prefix
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) OpenSOAP XML Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmDefineNamespaceWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                                    /* [in]  */ const wchar_t *ns_uri,
                                    /* [in]  */ const wchar_t *ns_prefix,
                                    /* [out] */ OpenSOAPXMLNamespacePtr *ns);

    /**
      * @fn int OpenSOAPXMLElmAddAttributeMB(OpenSOAPXMLElmPtr elm, const char *attr_name, const char *attr_type, void *attr_value, OpenSOAPXMLAttrPtr *attr)
      * @brief Add Attribute(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) SOAP Element
      * @param
      *    attr_name const char * [in] ((|attr_name|)) Attribute Name
      * @param
      *    attr_type const char * [in] ((|attr_type|)) Attribute Type
      * @param
      *    attr_value void * [in] ((|attr_value|)) Attribute Value
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) OpenSOAP XML Attribute
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmAddAttributeMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ const char *attr_name,
                                 /* [in]  */ const char *attr_type,
                                 /* [in]  */ void *attr_value,
                                 /* [out] */ OpenSOAPXMLAttrPtr *attr);

    /**
      * @fn int OpenSOAPXMLElmAddAttributeWC(OpenSOAPXMLElmPtr elm, const wchar_t *attr_name, const wchar_t *attr_type, void *attr_value, OpenSOAPXMLAttrPtr *attr)
      * @brief Add Attribute(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) SOAP Element
      * @param
      *    attr_name const wchar_t * [in] ((|attr_name|)) Attribute Name
      * @param
      *    attr_type const wchar_t * [in] ((|attr_type|)) Attribute Type
      * @param
      *    attr_value void * [in] ((|attr_value|)) Attribute Value
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) OpenSOAP XML Attribute
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmAddAttributeWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ const wchar_t *attr_name,
                                 /* [in]  */ const wchar_t *attr_type,
                                 /* [in]  */ void *attr_value,
                                 /* [out] */ OpenSOAPXMLAttrPtr *attr);

    /**
      * @fn int OpenSOAPXMLElmGetAttributeMB(OpenSOAPXMLElmPtr elm, const char *attr_name, OpenSOAPXMLAttrPtr *attr)
      * @brief Get Attribute Pointer of Assignment Attribute Name(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    attr_name const char * [in] ((|attr_name|)) Attribute Name
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) OpenSOAP XML Attribute
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetAttributeMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ const char *attr_name,
                                 /* [out] */ OpenSOAPXMLAttrPtr *attr);

    /**
      * @fn int OpenSOAPXMLElmGetAttributeWC(OpenSOAPXMLElmPtr elm, const wchar_t *attr_name, OpenSOAPXMLAttrPtr *attr)
      * @brief Get Attribute Pointer of Assignment Attribute Name(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in, out] ((|elm|)) XML Element
      * @param
      *    attr_name const wchar_t * [in] ((|attr_name|)) Attribute Name
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) OpenSOAP XML Attribute
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetAttributeWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ const wchar_t *attr_name,
                                 /* [out] */ OpenSOAPXMLAttrPtr *attr);

    /**
      * @fn int OpenSOAPXMLElmGetNextChild(OpenSOAPXMLElmPtr elm, OpenSOAPXMLElmPtr *cld_elm)
      * @brief Get next child XML Element
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    cld_elm OOpenSOAPXMLElmPtr * [in, out] ((|cld_elm|)) XML Element Pointer. Return the next XML Element. If 'cld_elm' is NULL, return to first XML Element.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetNextChild(/* [in]  */ OpenSOAPXMLElmPtr elm,
                               /* [in, out] */ OpenSOAPXMLElmPtr *cld_elm);

    /**
      * @fn int OpenSOAPXMLElmAddChildString(OpenSOAPXMLElmPtr elm, OpenSOAPStringPtr childName, OpenSOAPXMLElmPtr *childElm)
      * @brief Add Child XML Element(OpenSOAPString)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName OpenSOAPStringPtr [in] ((|cld_name|)) Add Child Element Name
      * @param
      *    childElm OpenSOAPXMLElmPtr * [out] ((|cld_elm|)) Add Child Element Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmAddChildString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ OpenSOAPStringPtr childName,
                                 /* [out] */ OpenSOAPXMLElmPtr *childElm);

    /**
      * @fn int OpenSOAPXMLElmAddChildMB(OpenSOAPXMLElmPtr elm, const char *cld_name, OpenSOAPXMLElmPtr *cld_elm)
      * @brief Add Child XML Element(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    cld_name cosnt char * [in] ((|cld_name|)) Add Child Element Name
      * @param
      *    cld_elm OpenSOAPXMLElmPtr * [out] ((|cld_elm|)) Add Child Element Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmAddChildMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const char *cld_name,
                             /* [out] */ OpenSOAPXMLElmPtr *cld_elm);

    /**
      * @fn int OpenSOAPXMLElmAddChildWC(OpenSOAPXMLElmPtr elm, const wchar_t *cld_name, OpenSOAPXMLElmPtr *cld_elm)
      * @brief Add Child XML Element(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    cld_name cosnt wchar_t * [in] ((|cld_name|)) Add Child Element Name
      * @param
      *    cld_elm OpenSOAPXMLElmPtr * [out] ((|cld_elm|)) Add Child Element Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmAddChildWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const wchar_t *cld_name,
                             /* [out] */ OpenSOAPXMLElmPtr *cld_elm);

    /**
      * @fn int OpenSOAPXMLElmAddChildXMLDocument(OpenSOAPXMLElmPtr elm, char *elmname, OpenSOAPByteArrayPtr document, const char *charEnc)
      * @brief Add Child XML Element(XML Document)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    elmname char * [in] ((elmname|)) OpenSOAP XML Element Name
      * @param
      *    document OpenSOAPByteArrayPtr [in] ((document|)) OpenSOAPByteArrayPtr
      * @param
      *    charEnc const char * [in] ((|charEnc|)) Characeter Encoding
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmAddChildXMLDocument(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                      /* [in]  */ char *elmname,
                                      /* [in]  */ OpenSOAPByteArrayPtr document,
                                      /* [in]  */ const char *charEnc);

    /**
      * @fn int OpenSOAPXMLElmGetChildString(OpenSOAPXMLElmPtr elm, OpenSOAPStringPtr childName, OpenSOAPXMLElmPtr *childElm)
      * @brief Get Child XML Element(OpenSOAPString)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName OpenSOAPStringPtr [in] ((|chldName|)) Assignment Element Name
      * @param
      *    childElm OpenSOAPXMLElmPtr * [out] ((|chldElm|)) XML Element Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetChildString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                 /* [in]  */ OpenSOAPStringPtr childName,
                                 /* [out] */ OpenSOAPXMLElmPtr *childElm);
	
    /**
      * @fn int OpenSOAPXMLElmGetChildMB(OpenSOAPXMLElmPtr elm, const char *cld_name, OpenSOAPXMLElmPtr *cld_elm)
      * @brief Get Child XML Element(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    cld_name cosnt char * [in] ((|cld_name|)) Assignment Element Name
      * @param
      *    cld_elm OpenSOAPXMLElmPtr * [out] ((|cld_elm|)) XML Element Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetChildMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const char *cld_name,
                             /* [out] */ OpenSOAPXMLElmPtr *cld_elm);

    /**
      * @fn int OpenSOAPXMLElmGetChildWC(OpenSOAPXMLElmPtr elm, const wchar_t *cld_name, OpenSOAPXMLElmPtr *cld_elm)
      * @brief Get Child XML Element(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    cld_name cosnt wchar_t * [in] ((|cld_name|)) Assignment Element Name
      * @param
      *    cld_elm OpenSOAPXMLElmPtr * [out] ((|cld_elm|)) XML Element Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetChildWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const wchar_t *cld_name,
                             /* [out] */ OpenSOAPXMLElmPtr *cld_elm);

    /**
      * @fn int OpenSOAPXMLElmGetValueMB(OpenSOAPXMLElmPtr elm, const char *type_name, void *value)
      * @brief Get Value of XML Element(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    type_name const char * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Setting Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetValueMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const char *type_name,
                             /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmGetValueWC(OpenSOAPXMLElmPtr elm, const wchar_t *type_name, void *value)
      * @brief Get Value of XML Element(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    type_name const wchar_t * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Setting Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const wchar_t *type_name,
                             /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmSetValueMB(OpenSOAPXMLElmPtr elm, const char *type_name, void *value)
      * @brief Set Value of XML Element(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    type_name const char * [in] ((|typeName|)) Type Name
      * @param
      *    value void * [in] ((|value|)) Setting Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetValueMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const char *type_name,
                             /* [in]  */ void *value);

    /**
      * @fn int OpenSOAPXMLElmSetValueWC(OpenSOAPXMLElmPtr elm, const wchar_t *type_name, void *value)
      * @brief Set Value of XML Element(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    type_name const wchar_t * [in] ((|typeName|)) Type Name
      * @param
      *    value void * [in] ((|value|)) Setting Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetValueWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                             /* [in]  */ const wchar_t *type_name,
                             /* [in]  */ void *value);

    /**
      * @fn int OpenSOAPXMLElmGetCharEncodingString(OpenSOAPXMLElmPtr elm, const char *chEnc, OpenSOAPByteArrayPtr bAry)
      * @brief XML Element Character Encoding Output
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    chEnc const char  * [in] ((|chEnc|)) character encoding (i.e. "EUC-JP", "Shift_JIS", "UTF-8").
      * @param
      *    bAry OpenSOAPByteArrayPtr [out] ((|bAry|)) Result Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetCharEncodingString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                        /* [in]  */ const char *chEnc,
                                        /* [out] */ OpenSOAPByteArrayPtr bAry);

    /**
      * @fn int OpenSOAPXMLElmGetNameString(OpenSOAPXMLElmPtr elm, OpenSOAPStringPtr *name)
      * @brief Get XML Element's Name as OpenSOAPString
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) XML Element Pointer
      * @param
      *    name OpenSOAPStringPtr * [out] ((|name|)) Result Name of Node
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetNameString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                /* [out] */ OpenSOAPStringPtr *name);

    /**
      * @fn int OpenSOAPXMLElmGetNextAttr(OpenSOAPXMLElmPtr elm, OpenSOAPXMLAttrPtr *attr_elm)
      * @brief Get next attr XML Element
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    attr_elm OpenSOAPXMLAttrPtr * [in, out] ((|attr_elm|)) Next Attr Element Pointer. If 'attr_elm' is NULL, then return to first XML Element pointer.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetNextAttr(/* [in] */ OpenSOAPXMLElmPtr elm,
                              /* [in, out] */ OpenSOAPXMLAttrPtr *attr_elm);

    /**
      * @fn int OpenSOAPXMLElmGetChildValueMB(OpenSOAPXMLElmPtr elm, const char *childName, const char *typeName, void *value)
      * @brief Get Child Value
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName const char * [in] ((|childName|)) child element name
      * @param
      *    typeName const char * [in] ((|typeName|)) value's type name
      * @param
      *    value void * [out] ((|value|)) value buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetChildValueMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                  /* [in]  */ const char *childName,
                                  /* [in]  */ const char *typeName,
                                  /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmGetChildValueWC(OpenSOAPXMLElmPtr elm, const wchar_t *childName, const wchar_t *typeName, void *value)
      * @brief Get Child Value
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName const wchar_t * [in] ((|childName|)) child element name
      * @param
      *    typeName const wchar_t * [in] ((|typeName|)) value's type name
      * @param
      *    value void * [out] ((|value|)) value buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetChildValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                  /* [in]  */ const wchar_t *childName,
                                  /* [in]  */ const wchar_t *typeName,
                                  /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmSetChildValueMB(OpenSOAPXMLElmPtr elm, const char *childName, const char *typeName, void *value)
      * @brief Set Child Value
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName const char * [in] ((|childName|)) child element name
      * @param
      *    typeName const char * [in] ((|typeName|)) value's type name
      * @param
      *    value void * [in] ((|value|)) value buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetChildValueMB(/* [out]  */ OpenSOAPXMLElmPtr elm,
                                  /* [in]  */ const char *childName,
                                  /* [in]  */ const char *typeName,
                                  /* [in] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmSetChildValueWC(OpenSOAPXMLElmPtr elm, const wchar_t *childName, const wchar_t *typeName, void *value)
      * @brief Set Child Value
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName const wchar_t * [in] ((|childName|)) child element name
      * @param
      *    typeName const wchar_t * [in] ((|typeName|)) value's type name
      * @param
      *    value void * [in] ((|value|)) value buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetChildValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                  /* [in]  */ const wchar_t *childName,
                                  /* [in]  */ const wchar_t *typeName,
                                  /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmSetChildValueXMLDocument(OpenSOAPXMLElmPtr elm, OpenSOAPByteArrayPtr document, const char *charEnc)
      * @brief Add Child XML Element(XML Document)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    document OpenSOAPByteArrayPtr [in] ((document|)) OpenSOAPByteArrayPtr
      * @param
      *    charEnc const char * [in] ((|charEnc|)) Characeter Encoding
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetChildValueXMLDocument(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                           /* [in]  */ OpenSOAPByteArrayPtr document,
                                           /* [in]  */ const char *charEnc);

    /**
      * @fn int OpenSOAPXMLElmSetValueAsString(OpenSOAPXMLElmPtr elm, OpenSOAPStringPtr value, int isDup)
      * @brief XML Element value set as string
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) XML Elm Pointer
      * @param
      *    value OpenSOAPStringPtr [in] ((|value|)) Setting Elm Name
      * @param
      *    isDup int [in] ((|isDup|)) duplicate flag. If non zero, then value is duplicate.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetValueAsString(/* [out] */ OpenSOAPXMLElmPtr elm,
                                   /* [in]  */ OpenSOAPStringPtr value,
                                   /* [in]  */ int isDup);

    /**
      * @fn int OpenSOAPXMLElmSetValueAsStringMB(OpenSOAPXMLElmPtr elm, const char *value)
      * @brief XML Element value set as string
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) XML Elm Pointer
      * @param
      *    value const char * [in] ((|value|)) value as string
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetValueAsStringMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                                     /* [in]  */ const char *value);
	
    /**
      * @fn int OpenSOAPXMLElmSetValueAsStringWC(OpenSOAPXMLElmPtr elm, const wchar_t *value)
      * @brief XML Element value set as string
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) XML Elm Pointer
      * @param
      *    value const wchar_t * [in] ((|value|)) value as string
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetValueAsStringWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                                     /* [in]  */ const wchar_t *value);

    /**
      * @fn int OpenSOAPXMLElmSetChildValueAsStringMB(OpenSOAPXMLElmPtr elm, const char *childName, const char *value)
      * @brief Set Child Value
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName const char * [in] ((|childName|)) child element name
      * @param
      *    value const char * [in] ((|value|)) value buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetChildValueAsStringMB(/* [out] */ OpenSOAPXMLElmPtr elm,
                                          /* [in]  */ const char *childName,
                                          /* [in]  */ const char *value);

    /**
      * @fn int OpenSOAPXMLElmSetChildValueAsStringWC(OpenSOAPXMLElmPtr elm, const wchar_t *childName, const wchar_t *value)
      * @brief Set Child Value
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName const wchar_t * [in] ((|childName|)) child element name
      * @param
      *    value const wchar_t * [in] ((|value|)) value buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetChildValueAsStringWC(/* [out] */ OpenSOAPXMLElmPtr elm,
                                          /* [in]  */ const wchar_t *childName,
                                          /* [in]  */ const wchar_t *value);

    /**
      * @fn int OpenSOAPXMLElmSetChildValueAsString(OpenSOAPXMLElmPtr elm, OpenSOAPStringPtr childName, int isValueDup, OpenSOAPStringPtr value)
      * @brief Set Child Value
      * @param
      *    elm OpenSOAPXMLElmPtr [out] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName OpenSOAPStringPtr [in] ((|childName|)) child element name
      * @param
      *    isValueDup int [in] ((|isValueDup|)) value duplicate flag
      * @param
      *    value OpenSOAPStringPtr [in, out] ((|value|)) value buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetChildValueAsString(/* [out] */ OpenSOAPXMLElmPtr elm,
                                        /* [in]  */ OpenSOAPStringPtr childName,
                                        /* [in]  */ int isValueDup,
                                        /* [in, out] */ OpenSOAPStringPtr value);
	
    /**
      * @fn int OpenSOAPXMLElmRemoveChildString(OpenSOAPXMLElmPtr elm, OpenSOAPStringPtr childName, int isValueRelease, OpenSOAPStringPtr *childValue)
      * @brief Remove Child XML Element(OpenSOAPString)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    childName OpenSOAPStringPtr [in] ((|childName|)) Child Element Name
      * @param
      *    isValueRelease int [in] ((|isValueRelease|)) Value Released or not
      * @param
      *    childValue OpenSOAPStringPtr * [out] ((|childValue|)) Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmRemoveChildString(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                    /* [in]  */ OpenSOAPStringPtr childName,
                                    /* [in]  */ int isValueRelease,
                                    /* [out] */ OpenSOAPStringPtr *childValue);	

    /**
      * @fn int OpenSOAPXMLElmGetAttributeValueMB(OpenSOAPXMLElmPtr elm, const char *attrName, const char *typeName, void *value)
      * @brief Get Attribute Value(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    attrName const char * [in] ((|attrName|)) Attribute Name
      * @param
      *    typeName const char * [in] ((|typeName|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetAttributeValueMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                      /* [in]  */ const char *attrName,
                                      /* [in]  */ const char *typeName,
                                      /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmGetAttributeValueWC(OpenSOAPXMLElmPtr elm, const wchar_t *attrName, const wchar_t *typeName, void *value)
      * @brief Get Attribute Value(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    attrName const wchar_t * [in] ((|attrName|)) Attribute Name
      * @param
      *    typeName const wchar_t * [in] ((|typeName|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmGetAttributeValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                      /* [in]  */ const wchar_t *attrName,
                                      /* [in]  */ const wchar_t *typeName,
                                      /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmSetAttributeValueMB(OpenSOAPXMLElmPtr elm, const char *attrName, const char *typeName, void *value)
      * @brief Set Attribute Value(MB)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    attrName const char * [in] ((|attrName|)) Attribute Name
      * @param
      *    typeName const char * [in] ((|typeName|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetAttributeValueMB(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                      /* [in]  */ const char *attrName,
                                      /* [in]  */ const char *typeName,
                                      /* [out] */ void *value);

    /**
      * @fn int OpenSOAPXMLElmSetAttributeValueWC(OpenSOAPXMLElmPtr elm, const wchar_t *attrName, const wchar_t *typeName, void *value)
      * @brief Set Attribute Value(WC)
      * @param
      *    elm OpenSOAPXMLElmPtr [in] ((|elm|)) OpenSOAP XML Element
      * @param
      *    attrName const wchar_t * [in] ((|attrName|)) Attribute Name
      * @param
      *    typeName const wchar_t * [in] ((|typeName|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPXMLElmSetAttributeValueWC(/* [in]  */ OpenSOAPXMLElmPtr elm,
                                      /* [in]  */ const wchar_t *attrName,
                                      /* [in]  */ const wchar_t *typeName,
                                      /* [out] */ void *value);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_XMLElm_H */
