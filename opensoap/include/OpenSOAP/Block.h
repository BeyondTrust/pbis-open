/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Block.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Block_H
#define OpenSOAP_Block_H

#include <OpenSOAP/XMLElm.h>
#include <OpenSOAP/XMLNamespace.h>

/**
 * @file OpenSOAP/Block.h
 * @brief OpenSOAP API Block Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPBlock OpenSOAPBlock
     * @brief OpenSOAPBlock Structure Type Definition
     */
    typedef struct tagOpenSOAPBlock OpenSOAPBlock;

    /**
     * @typedef OpenSOAPBlock *OpenSOAPBlockPtr
     * @brief OpenSOAPBlock Pointer Type Definition
     */
    typedef OpenSOAPBlock *OpenSOAPBlockPtr;

    /**
      * @fn int OpenSOAPBlockGetValueMB(OpenSOAPBlockPtr soap_block, const char * type_name, void * value)
      * @brief Get Value of SOAP Block(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) OpenSOAP SOAP Block
      * @param
      *    type_name const char * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetValueMB(OpenSOAPBlockPtr /* [in] */ soap_block,
                            const char * /* [in] */ type_name,
                            void * /* [out] */ value);

    /**
      * @fn int OpenSOAPBlockGetValueWC(OpenSOAPBlockPtr soap_block, const wchar_t * type_name, void * value)
      * @brief Get Value of SOAP Block(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) OpenSOAP SOAP Block
      * @param
      *    type_name const wchar_t * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [out] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetValueWC(OpenSOAPBlockPtr /* [in] */ soap_block,
                            const wchar_t * /* [in] */ type_name,
                            void * /* [out] */ value);

    /**
      * @fn int OpenSOAPBlockSetValueMB(OpenSOAPBlockPtr soap_block, const char * type_name, void * value)
      * @brief Set Value of SOAP Block(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) OpenSOAP SOAP Block
      * @param
      *    type_name const char * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [in] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetValueMB(OpenSOAPBlockPtr /* [in] */ soap_block,
                            const char * /* [in] */ type_name,
                            void * /* [in] */ value);

    /**
      * @fn int OpenSOAPBlockSetValueWC(OpenSOAPBlockPtr soap_block, const wchar_t * type_name, void * value)
      * @brief Set Value of SOAP Block(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) OpenSOAP SOAP Block
      * @param
      *    type_name const wchar_t * [in] ((|type_name|)) Type Name
      * @param
      *    value void * [in] ((|value|)) Storage Buffer Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetValueWC(OpenSOAPBlockPtr /* [in] */ soap_block,
                            const wchar_t * /* [in] */ type_name,
                            void * /* [in] */ value);

    /**
      * @fn int OpenSOAPBlockSetNamespaceMB(OpenSOAPBlockPtr soap_block, const char * ns_uri, const char * ns_prefix)
      * @brief Set Namespace(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    ns_uri const char * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const char * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetNamespaceMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const char * /* [in] */ ns_uri,
                                const char * /* [in] */ ns_prefix);

    /**
      * @fn int OpenSOAPBlockSetNamespaceWC(OpenSOAPBlockPtr soap_block, const wchar_t * ns_uri, const wchar_t * ns_prefix)
      * @brief Set Namespace(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const wchar_t * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetNamespaceWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const wchar_t * /* [in] */ ns_uri,
                                const wchar_t * /* [in] */ ns_prefix);

    /**
      * @fn int OpenSOAPBlockGetNamespace(OpenSOAPBlockPtr soap_block, OpenSOAPXMLNamespacePtr * ns)
      * @brief Get Namespace
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    ns OpenSOAPXMLNamespacePtr * [out] ((|ns|)) Namespace
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetNamespace(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                              OpenSOAPXMLNamespacePtr * /* [out] */ ns);

    /**
      * @fn int OpenSOAPBlockIsSameNamespaceMB(OpenSOAPBlockPtr soap_block, const char * ns_uri, int * is_same_uri)
      * @brief Judge Namespace(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) SOAP Block
      * @param
      *    ns_uri OpenSOAPStringPtr [in] ((|ns_uri|)) Namespace URI
      * @param
      *    is_same_uri int * [out] ((|is_same_uri|)) judge result buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockIsSameNamespaceMB(OpenSOAPBlockPtr /* [in] */ soap_block,
                                   const char * /* [in] */ ns_uri,
                                   int * /* [out] */ is_same_uri);

    /**
      * @fn int OpenSOAPBlockIsSameNamespaceWC(OpenSOAPBlockPtr soap_block, const wchar_t * ns_uri, int * is_same_uri)
      * @brief Judge Namespace(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) SOAP Block
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    is_same_uri int * [out] ((|is_same_uri|)) judge result buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockIsSameNamespaceWC(OpenSOAPBlockPtr /* [in] */ soap_block,
                                   const wchar_t * /* [in] */ ns_uri,
                                   int * /* [out] */ is_same_uri);

    /**
      * @fn int OpenSOAPBlockAddAttributeMB(OpenSOAPBlockPtr soap_block, const char * attr_name, const char * attr_type, void * attr_value, OpenSOAPXMLAttrPtr * attr)
      * @brief Add and Set Attribute(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|))
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
    OpenSOAPBlockAddAttributeMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const char * /* [in] */ attr_name,
                                const char * /* [in] */ attr_type,
                                void * /* [in] */ attr_value,
                                OpenSOAPXMLAttrPtr * /* [out] */ attr);

    /**
      * @fn int OpenSOAPBlockAddAttributeWC(OpenSOAPBlockPtr soap_block, const wchar_t * attr_name, const wchar_t * attr_type, void * attr_value, OpenSOAPXMLAttrPtr * attr)
      * @brief Add and Set Attribute(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
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
    OpenSOAPBlockAddAttributeWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const wchar_t * /* [in] */ attr_name,
                                const wchar_t * /* [in] */ attr_type,
                                void * /* [in] */ attr_value,
                                OpenSOAPXMLAttrPtr * /* [out] */ attr);

    /**
      * @fn int OpenSOAPBlockGetAttributeMB(OpenSOAPBlockPtr soap_block, const char * attr_name, OpenSOAPXMLAttrPtr * attr)
      * @brief Get Value of Attribute(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    attr_name const char * [in] ((|attr_name|)) Attribute Name
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) OpenSOAP XML Attribute
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetAttributeMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const char * /* [in] */ attr_name,
                                OpenSOAPXMLAttrPtr * /* [out] */ attr);
    
    /**
      * @fn int OpenSOAPBlockGetAttributeWC(OpenSOAPBlockPtr soap_block, const wchar_t * attr_name, OpenSOAPXMLAttrPtr * attr)
      * @brief Get Value of Attribute
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    attr_name const wchar_t * [in] ((|attr_name|)) Attribute Name
      * @param
      *    attr OpenSOAPXMLAttrPtr * [out] ((|attr|)) OpenSOAP XML Attribute
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetAttributeWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const wchar_t * /* [in] */ attr_name,
                                OpenSOAPXMLAttrPtr * /* [out] */ attr);
    
    /**
      * @fn int OpenSOAPBlockGetChildValueMB(OpenSOAPBlockPtr soap_block, const char * p_name, const char * type_name, void * value)
      * @brief Get Parameter Value(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    p_name const char * [in] ((|p_name|)) Parameter Name
      * @param
      *    type_name const char * [in] ((|type_name|)) Parameter Type
      * @param
      *    value void * [out] ((|value|)) Parameter Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetChildValueMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                 const char * /* [in] */ p_name,
                                 const char * /* [in] */ type_name,
                                 void * /* [out] */ value);

    /**
      * @fn int OpenSOAPBlockGetChildValueWC(OpenSOAPBlockPtr soap_block, const wchar_t * p_name, const wchar_t * type_name, void * value)
      * @brief Get Child Value(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    p_name const wchar_t * [in] ((|p_name|)) Parameter Name
      * @param
      *    type_name const wchar_t * [in] ((|type_name|)) Parameter Type
      * @param
      *    value void * [out] ((|value|)) Parameter Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetChildValueWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                 const wchar_t * /* [in] */ p_name,
                                 const wchar_t * /* [in] */ type_name,
                                 void * /* [out] */ value);

    /**
      * @fn int OpenSOAPBlockSetChildValueMB(OpenSOAPBlockPtr soap_block, const char * p_name, const char * type_name, void * value)
      * @brief Set Parameter Value(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    p_name const char * [in] ((|p_name|)) Parameter Name
      * @param
      *    type_name const char * [in] ((|type_name|)) Parameter Type
      * @param
      *    value void * [in] ((|value|)) Parameter Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetChildValueMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                 const char * /* [in] */ p_name,
                                 const char * /* [in] */ type_name,
                                 void * /* [in] */ value);

    /**
      * @fn int OpenSOAPBlockSetChildValueWC(OpenSOAPBlockPtr soap_block, const wchar_t * p_name, const wchar_t * type_name, void * value)
      * @brief Set Parameter Value(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    p_name const wchar_t * [in] ((|p_name|)) Parameter Name
      * @param
      *    type_name const wchar_t * [in] ((|type_name|)) Parameter Type
      * @param
      *    value void * [in] ((|value|)) Parameter Value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetChildValueWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                 const wchar_t * /* [in] */ p_name,
                                 const wchar_t * /* [in] */ type_name,
                                 void * /* [in] */ value);

    /**
      * @fn int OpenSOAPBlockGetNextChild(OpenSOAPBlockPtr soap_block, OpenSOAPXMLElmPtr * xml_elm)
      * @brief Get next child Block
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    xml_elm OpenSOAPXMLElmPtr * [in, out] ((|xml_elm|)) OpenSOAP XML Element.
      *    If *xml_elm is NULL, the first child is returned
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetNextChild(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                              OpenSOAPXMLElmPtr * /* [in, out] */ xml_elm);

    /**
      * @fn int OpenSOAPBlockAddChildMB(OpenSOAPBlockPtr soap_block, const char * elm_name, OpenSOAPXMLElmPtr * xml_elm)
      * @brief Add Child XML Element(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    elm_name const char * [in] ((|elm_name|)) Child XML Element Name
      * @param
      *    xml_elm OpenSOAPXMLElmPtr * [out] ((|xml_elm|)) OpenSOAP XML Element
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockAddChildMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                            const char * /* [in] */ elm_name,
                            OpenSOAPXMLElmPtr * /* [out] */ xml_elm);

    /**
      * @fn int OpenSOAPBlockAddChildWC(OpenSOAPBlockPtr soap_block, const wchar_t * elm_name, OpenSOAPXMLElmPtr * xml_elm)
      * @brief Add Child XML Element(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    elm_name const wchar_t * [in] ((|elm_name|)) Child XML Element Name
      * @param
      *    xml_elm OpenSOAPXMLElmPtr * [out] ((|xml_elm|)) OpenSOAP XML Element
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockAddChildWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                            const wchar_t * /* [in] */ elm_name,
                            OpenSOAPXMLElmPtr * /* [out] */ xml_elm);

    /**
      * @fn int OpenSOAPBlockGetChildMB(OpenSOAPBlockPtr soap_block, const char * elm_name, OpenSOAPXMLElmPtr * xml_elm)
      * @brief Get The Child with Matching Name(MB). Get first if more than one.
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    elm_name const char * [in] ((|elm_name|)) Name of XML Element
      * @param
      *    xml_elm OpenSOAPXMLElmPtr * [out] ((|xml_elm|)) OpenSOAP XML Element
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetChildMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                            const char * /* [in] */ elm_name,
                            OpenSOAPXMLElmPtr * /* [out] */ xml_elm);

    /**
      * @fn int OpenSOAPBlockGetChildWC(OpenSOAPBlockPtr soap_block, const wchar_t * elm_name, OpenSOAPXMLElmPtr * xml_elm)
      * @brief Get The Child with Matching Name(WC). Get first if more than one.
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    elm_name const wchar_t * [in] ((|elm_name|)) Name of Child XML Element
      * @param
      *    xml_elm OpenSOAPXMLElmPtr * [out] ((|xml_elm|)) OpenSOAP XML Element
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetChildWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                            const wchar_t * /* [in] */ elm_name,
                            OpenSOAPXMLElmPtr * /* [out] */ xml_elm);

    /**
      * @fn int OpenSOAPBlockGetMustunderstandAttr(OpenSOAPBlockPtr soap_block, int * must_std)
      * @brief Get mustunderstand attribute
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) SOAP Block
      * @param
      *    must_std int * [out] ((|must_std|)) mustunderstand return buffer pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetMustunderstandAttr(OpenSOAPBlockPtr /* [in] */ soap_block,
                                       int * /* [out] */ must_std);

    /**
      * @fn int OpenSOAPBlockSetMustunderstandAttr(OpenSOAPBlockPtr soap_block)
      * @brief Set mustunderstand attribute
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetMustunderstandAttr(OpenSOAPBlockPtr /* [in, out] */ soap_block);

    /**
      * @fn int OpenSOAPBlockClearMustunderstandAttr(OpenSOAPBlockPtr soap_block)
      * @brief Clear mustunderstand attribute
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockClearMustunderstandAttr(OpenSOAPBlockPtr /* [in, out] */ soap_block);

    /**
      * @fn int OpenSOAPBlockGetActorAttr(OpenSOAPBlockPtr soap_block, OpenSOAPStringPtr * actor_url)
      * @brief Get actor attribute
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soapBlock|)) SOAP Block
      * @param
      *    actor_url OpenSOAPStringPtr * [out] ((|actorUri|)) actor attribute value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetActorAttr(OpenSOAPBlockPtr /* [in] */ soap_block,
                              OpenSOAPStringPtr * /* [out] */ actor_url);

    /**
      * @fn int OpenSOAPBlockSetActorAttrMB(OpenSOAPBlockPtr soap_block, const char * actor_url)
      * @brief Set actor attribute.(MB)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    actor_url const char * [in] ((|actor_url|)) actor attribute value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetActorAttrMB(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const char * /* [in] */ actor_url);

    /**
      * @fn int OpenSOAPBlockSetActorAttrWC(OpenSOAPBlockPtr soap_block, const wchar_t * actor_url)
      * @brief Set actor attribute.(WC)
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    actor_url const wchar_t * [in] ((|actor_url|)) actor attribute value
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetActorAttrWC(OpenSOAPBlockPtr /* [in, out] */ soap_block,
                                const wchar_t * /* [in] */ actor_url);

    /**
      * @fn int OpenSOAPBlockClearActorAttr(OpenSOAPBlockPtr soap_block)
      * @brief Clear actor attribute
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockClearActorAttr(OpenSOAPBlockPtr /* [in, out] */ soap_block);

    /**
      * @fn int OpenSOAPBlockSetActorAttrNext(OpenSOAPBlockPtr soap_block)
      * @brief Set actor attribute to next
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soapBlock|)) SOAP Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockSetActorAttrNext(OpenSOAPBlockPtr /* [in, out] */ soap_block);
    
    /**
      * @fn int OpenSOAPBlockIsActorAttrNext(OpenSOAPBlockPtr soap_block, int * is_actor_next)
      * @brief Is actor attribute to next
      * @param
      *    soap_block OpenSOAPBlockPtr [in, out] ((|soap_block|)) SOAP Block
      * @param
      *    is_actor_next int * [out] ((|is_actor_next|)) judge result
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockIsActorAttrNext(OpenSOAPBlockPtr /* [in] */ soap_block,
                                 int * /* [out] */ is_actor_next);

    /**
      * @fn int OpenSOAPBlockGetName(OpenSOAPBlockPtr block, OpenSOAPStringPtr * name)
      * @brief Get SOAP Block Name
      * @param
      *    block OpenSOAPBlockPtr [in] ((|block|)) SOAP Block Pointer
      * @param
      *    name OpenSOAPStringPtr * [out] ((|name|)) Result Name of SOAP Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetName(OpenSOAPBlockPtr /* [in] */ block,
                         OpenSOAPStringPtr * /* [out] */ name);
    
    /**
      * @fn int OpenSOAPBlockGetCharEncodingString(OpenSOAPBlockPtr soap_block, const char * ch_enc, OpenSOAPByteArrayPtr b_ary)
      * @brief Soap block character encoding output
      * @param
      *    soap_block OpenSOAPBlockPtr [in] ((|soap_block|)) OpenSOAP Block
      * @param
      *    ch_enc const char  * [in] ((|ch_enc|)) character encoding. (i.e. "EUC-JP", "Shift_JIS", "UTF-8")
      * @param
      *    b_ary OpenSOAPByteArrayPtr [out] ((|b_ary|)) Result Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPBlockGetCharEncodingString(OpenSOAPBlockPtr /* [in] */ soap_block,
                                        const char * /* [in] */ ch_enc,
                                        OpenSOAPByteArrayPtr /* [out] */ b_ary);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Block_H */
