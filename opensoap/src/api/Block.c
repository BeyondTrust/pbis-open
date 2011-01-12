/*-----------------------------------------------------------------------------
 * $RCSfile: Block.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Block.c,v 1.52 2003/11/20 07:03:16 okada Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/Envelope.h>
#include "Block.h"
#include "XMLAttr.h"
#include "XMLNamespace.h"

/* for temporary implementation */
#include <string.h>

/*
=begin
= OpenSOAP SOAP Block class
=end
 */

/*
=begin
--- function#OpenSOAPBlockCreate(soapBlock)
    OpenSOAP XML Element instance create

    :Parameters
      :[out] OpenSOAPBlockPtr * ((|soapBlock|))
        XML Element pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
=end
 */
extern
int
OpenSOAPBlockCreate(/* [out] */ OpenSOAPBlockPtr *soapBlock) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (soapBlock) {
		ret = OpenSOAPXMLElmCreate((OpenSOAPXMLElmPtr*)soapBlock);
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockReplaceXMLElmChildren(elm)
    replace XML Element's children element to OpenSOAP Block

    :Parameters
      :[in, out] OpenSOAPXMLElmPtr * ((|elm|))
        Parent XML Element pointer
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
=end
 */
extern
int
OpenSOAPBlockReplaceXMLElmChildren(/* [in, out] */ OpenSOAPXMLElmPtr elm) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (elm) {
		ret = OPENSOAP_NO_ERROR;
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetValueString(soapBlock, typeName, value)
    Get Value of SOAP Block(OpenSOAPString)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        OpenSOAP SOAP Block.
      :OpenSOAPStringPtr [in] ((|typeName|))
        Type Name
      :void * [out] ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPBlockGetValueString(/* [in]  */ OpenSOAPBlockPtr soapBlock,
							/* [in]  */ OpenSOAPStringPtr typeName,
							/* [out] */ void *value) {
	int ret = OpenSOAPXMLElmGetValueString((OpenSOAPXMLElmPtr)soapBlock,
									   typeName,
									   value);

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetValueMB(soapBlock, typeName, value)
    Get Value of SOAP Block(MB)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        OpenSOAP SOAP Block.
      :const char * [in] ((|typeName|))
        Type Name
      :void * [out] ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPBlockGetValueMB(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                        /* [in]  */ const char *typeName,
                        /* [out] */ void *value) {
	int ret = OpenSOAPXMLElmGetValueMB((OpenSOAPXMLElmPtr)soapBlock,
									   typeName,
									   value);

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetValueWC(soapBlock, typeName, value)
    Get Value of SOAP Block(WC)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        OpenSOAP SOAP Block.
      :const wchar_t * [in] ((|typeName|))
        Type Name
      :void * [out] ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPBlockGetValueWC(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                        /* [in]  */ const wchar_t *typeName,
                        /* [out] */ void *value) {
	int ret = OpenSOAPXMLElmGetValueWC((OpenSOAPXMLElmPtr)soapBlock,
									   typeName,
									   value);

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetValueMB(soapBlock, typeName, value)
    Set Value of SOAP Block(MB)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        OpenSOAP SOAP Block.
      :const char * [in] ((|typeName|))
        Type Name
      :void * [in] ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      (2001/08/28)
      Store the result of Value serialized by typeName into soapBlock.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPBlockSetValueMB(/* [out] */ OpenSOAPBlockPtr soapBlock,
                        /* [in]  */ const char *typeName,
                        /* [in]  */ void *value) {
	int ret = OpenSOAPXMLElmSetValueMB((OpenSOAPXMLElmPtr)soapBlock,
									   typeName,
									   value);

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetValueWC(soapBlock, typeName, value)
    Set Value of SOAP Block(WC)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        OpenSOAP SOAP Block.
      :const wchar_t * [in] ((|typeName|))
        Type Name
      :void * [in] ((|value|))
        Strage Buffer Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      (2001/08/28)
      Store the result of Value serialized by typeName into soapBlock.
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPBlockSetValueWC(/* [out] */ OpenSOAPBlockPtr soapBlock,
						/* [in]  */ const wchar_t *typeName,
						/* [in]  */ void *value) {
	int ret = OpenSOAPXMLElmSetValueWC((OpenSOAPXMLElmPtr)soapBlock,
									   typeName,
									   value);

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetNamespaceMB(soapBlock, nsUri, nsPrefix)
    Set Namespace(MB)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const char * [in] ((|nsUri|))
        Namespace URI
      :const char * [in] ((|nsPrefix|))
        Namespace Prefix
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      (2001/08/28)
      sotre nsPrifix/nsUri into soapBlock.
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetNamespaceMB(/* [out] */ OpenSOAPBlockPtr soapBlock,
                            /* [in]  */ const char *nsUri,
                            /* [in]  */ const char *nsPrefix) {
	int ret = OpenSOAPXMLElmSetNamespaceMB((OpenSOAPXMLElmPtr)soapBlock,
										   nsUri,
										   nsPrefix);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetNamespaceWC(soapBlock, nsUri, nsPrefix)
    Set Namespace(WC)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|nsUri|))
        Namespace URI
      :const wchar_t * [in] ((|nsPrefix|))
        Namespace Prefix
    :Return Value
      :int
	    Return Code Is ErrorCode
    :Note
      (2001/08/28)
      store nsPrifix/nsUri into soapBlock.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetNamespaceWC(/* [out] */ OpenSOAPBlockPtr soapBlock,
                            /* [in]  */ const wchar_t *nsUri,
                            /* [in]  */ const wchar_t *nsPrefix) {
	int ret = OpenSOAPXMLElmSetNamespaceWC((OpenSOAPXMLElmPtr)soapBlock,
										   nsUri,
										   nsPrefix);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetNamespace(soapBlock, ns)
    Get Namespace

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
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
OpenSOAPBlockGetNamespace(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                          /* [out] */ OpenSOAPXMLNamespacePtr *ns) {
	int ret = OpenSOAPXMLElmGetNamespace((OpenSOAPXMLElmPtr)soapBlock,
										 ns);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockIsSameNamespaceString(soapBlock, nsUri, isSame)
    Judge Namespace(OpenSOAPString)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        SOAP Block.
      :OpenSOAPStringPtr [in] ((|nsUri|))
        Namespace URI.
      :int * [out] ((|isSame|))
        judge result buffer.
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
      
=end
 */
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPBlockIsSameNamespaceString(/* [in]  */ OpenSOAPBlockPtr soapBlock,
								   /* [in]  */ OpenSOAPStringPtr nsUri,
								   /* [out] */ int *isSame) {
	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)soapBlock;
	int ret = OpenSOAPXMLNamespaceIsSameUriString(node->thisNamespace,
												  nsUri,
												  isSame);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockIsSameNamespaceMB(soapBlock, nsUri, isSame)
    Judge Namespace(MB)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        SOAP Block.
      :const char * [in] ((|nsUri|))
        Namespace URI.
      :int * [out] ((|isSame|))
        judge result buffer.
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockIsSameNamespaceMB(/* [in]  */ OpenSOAPBlockPtr soapBlock,
							   /* [in]  */ const char *nsUri,
							   /* [out] */ int *isSame) {
	OpenSOAPStringPtr nsUriStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(nsUri, &nsUriStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPBlockIsSameNamespaceString(soapBlock, nsUriStr, isSame);
		OpenSOAPStringRelease(nsUriStr);
	}
	return ret;
}

/*
=begin
--- function#OpenSOAPBlockIsSameNamespaceWC(soapBlock, nsUri, isSame)
    Judge Namespace(WC)

    :Parameters
      :OpenSOAPBlockPtr [in] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|nsUri|))
        Namespace URI.
      :int * [out] ((|isSame|))
        judge result buffer.
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockIsSameNamespaceWC(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                               /* [in]  */ const wchar_t *nsUri,
                               /* [out] */ int *isSame) {
	OpenSOAPStringPtr nsUriStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(nsUri, &nsUriStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPBlockIsSameNamespaceString(soapBlock, nsUriStr, isSame);
		OpenSOAPStringRelease(nsUriStr);
	}
	return ret;
}

/*
=begin
--- function#OpenSOAPBlockAddAttributeMB(soapBlock, attrName, attrType, attrValue, attr)
    Add and Set Attribute(MB)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
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
OpenSOAPBlockAddAttributeMB(/* [out] */ OpenSOAPBlockPtr soapBlock,
                            /* [in]  */ const char *attrName,
                            /* [in]  */ const char *attrType,
                            /* [in]  */ void *attrValue,
                            /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmAddAttributeMB(elm,
										   attrName,
										   attrType,
										   attrValue,
										   attr);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockAddAttributeWC(soapBlock, attrName, attrType, attrValue, attr)
    Add and Set Attribute(WC)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|attrName|))
        Attribute Name
      :const wchar_t * [in] ((|attrType|))
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
OpenSOAPBlockAddAttributeWC(/* [out] */ OpenSOAPBlockPtr soapBlock,
                            /* [in]  */ const wchar_t *attrName,
                            /* [in]  */ const wchar_t *attrType,
                            /* [in]  */ void *attrValue,
                            /* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmAddAttributeWC(elm,
										   attrName,
										   attrType,
										   attrValue,
										   attr);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetAttributeMB(soapBlock, attrName, attr)
    Get Value of Attribute(MB)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
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
OpenSOAPBlockGetAttributeMB(/* [in]  */ OpenSOAPBlockPtr soapBlock,
							/* [in]  */ const char *attrName,
							/* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmGetAttributeMB(elm,
										   attrName,
										   attr);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetAttributeWC(soapBlock, attrName, attr)
    Get Value of Attribute

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|attrName|))
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
OpenSOAPBlockGetAttributeWC(/* [in]  */ OpenSOAPBlockPtr soapBlock,
							/* [in]  */ const wchar_t *attrName,
							/* [out] */ OpenSOAPXMLAttrPtr *attr) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmGetAttributeWC(elm,
										   attrName,
										   attr);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetChildMB(soapBlock, childName, childElm)
    Get The Child XML Element with Matching Name.
    Get first if more than one.

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const char * [in] ((|childName|))
        Name of XML Element
      :OpenSOAPXMLElmPtr * [out] ((|childElm|))
        OpenSOAP XML Element.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetChildMB(/* [in]  */ OpenSOAPBlockPtr soapBlock,
						/* [in]  */ const char *childName,
						/* [out] */ OpenSOAPXMLElmPtr *childElm) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmGetChildMB(elm,
									   childName,
									   childElm);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetChildWC(soapBlock, childName, childElm)
    Get The Child XML Element with Matching Name.
    Get first if more than one.

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|childName|))
        Name of Child XML Element
      :OpenSOAPXMLElmPtr * [out] ((|childElm|))
        OpenSOAP XML Element.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetChildWC(/* [in]  */ OpenSOAPBlockPtr soapBlock,
						/* [in]  */ const wchar_t *childName,
						/* [out] */ OpenSOAPXMLElmPtr *childElm) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmGetChildWC(elm,
									   childName,
									   childElm);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetChildValueMB(soapBlock, pName, typeName, value)
    Get Parameter Value.

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const char * [in] ((|pName|))
        Parameter Name
      :const char * [in] ((|typeName|))
        Parameter Type
      :void * [out] ((|value|))
        Pointer to the storing buffer of the value
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetChildValueMB(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                             /* [in]  */ const char *pName,
                             /* [in]  */ const char *typeName,
                             /* [out] */ void *value) {
	int ret = OpenSOAPXMLElmGetChildValueMB((OpenSOAPXMLElmPtr)soapBlock,
											pName,
											typeName,
											value);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetChildValueWC(soapBlock, pName, typeName, value)
    Get Parameter Value.

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|pName|))
        Parameter Name
      :const wchar_t * [in] ((|typeName|))
        Parameter Type
      :void * [out] ((|value|))
        Pointer to the storing buffer of the value
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetChildValueWC(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                             /* [in]  */ const wchar_t *pName,
                             /* [in]  */ const wchar_t *typeName,
                             /* [out] */ void *value) {
	int ret = OpenSOAPXMLElmGetChildValueWC((OpenSOAPXMLElmPtr)soapBlock,
											pName,
											typeName,
											value);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetChildValueMB(soapBlock, pName, typeName, value)
    Set Parameter Value

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const char * [in] ((|pName|))
        Parameter Name
      :const char * [in] ((|typeName|))
        Parameter Type
      :void * [in] ((|value|))
        Pointer to the buffer of Parameter Value
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetChildValueMB(/* [out] */ OpenSOAPBlockPtr soapBlock,
                             /* [in]  */ const char *pName,
                             /* [in]  */ const char *typeName,
                             /* [in]  */ void *value) {
	int ret = OpenSOAPXMLElmSetChildValueMB((OpenSOAPXMLElmPtr)soapBlock,
											pName,
											typeName,
											value);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetChildValueWC(soapBlock, pName, typeName, value)
    Set Parameter Value

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|pName|))
        Parameter Name
      :const wchar_t * [in] ((|typeName|))
        Parameter Type
      :void * [in] ((|value|))
        Pointer to the buffer of Parameter Value
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetChildValueWC(/* [out] */ OpenSOAPBlockPtr soapBlock,
                             /* [in]  */ const wchar_t *pName,
                             /* [in]  */ const wchar_t *typeName,
                             /* [in]  */ void *value) {
	int ret = OpenSOAPXMLElmSetChildValueWC((OpenSOAPXMLElmPtr)soapBlock,
											pName,
											typeName,
											value);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetNextChild(soapBlock, childElm)
    Get next child Block.

    :Parameters
      :[in, out] OpenSOAPBlockPtr ((|soapBlock|))
        SOAP Block.
      :[in, out] OpenSOAPXMLElmPtr * ((|childElm|))
        next XML Element
        if *childElm is NULL, the first child is returned
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetNextChild(/* [in]      */ OpenSOAPBlockPtr soapBlock,
                          /* [in, out] */ OpenSOAPXMLElmPtr *childElm) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmGetNextChild(elm, childElm);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockAddChildMB(soapBlock, childName, childElm)
    Add of Child XML Element(MB)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const char * [in] ((|childName|))
        Name of child XML Element
      :OpenSOAPXMLElmPtr * [out] ((|childElm|))
        OpenSOAP XML Element.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockAddChildMB(/* [out] */ OpenSOAPBlockPtr soapBlock,
                        /* [in]  */ const char *childName,
                        /* [out] */ OpenSOAPXMLElmPtr *childElm) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmAddChildMB(elm,
									   childName,
									   childElm);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockAddChildWC(soapBlock, childName, childElm)
    Add of Child XML Element(WC)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|childName|))
        Name of XML Element
      :OpenSOAPXMLElmPtr * [out] ((|childElm|))
        OpenSOAP XML Element.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockAddChildWC(/* [out] */ OpenSOAPBlockPtr soapBlock,
                        /* [in]  */ const wchar_t *childName,
                        /* [out] */ OpenSOAPXMLElmPtr *childElm) {
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmAddChildWC(elm,
									   childName,
									   childElm);

	return ret;
}

static
const wchar_t
mustUnderstandName[] = L"mustUnderstand";

static
const wchar_t
mustUnderstandType[] = L"int";


/*
=begin
--- function#OpenSOAPBlockGetMustunderstandAttr(soapBlock, mustUnderstand)
    Get mustunderstand attribute.

    :Parameters
      :[in]  OpenSOAPBlockPtr ((|soapBlock|))
        SOAP Block.
      :[out] int * ((|mustUnderstand|))
        mustunderstand return buffer pointer
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetMustunderstandAttr(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                                   /* [out] */ int *mustUnderstand) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (soapBlock && mustUnderstand) {
		long val = 0;
		OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
		ret = OpenSOAPXMLElmGetAttributeValueWC(elm,
												mustUnderstandName,
												mustUnderstandType,
												&val);
		if (OPENSOAP_SUCCEEDED(ret)) {
			*mustUnderstand = val;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetMustunderstandAttr(soapBlock)
    Set mustunderstand attribute.

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetMustunderstandAttr(/* [out] */ OpenSOAPBlockPtr soapBlock) {
	long val = 1;
	OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
	int ret = OpenSOAPXMLElmSetAttributeValueWC(elm,
												mustUnderstandName,
												mustUnderstandType,
												&val);

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockClearMustunderstandAttr(soapBlock)
    Clear mustunderstand attribute.

    :Parameters
      :[out] OpenSOAPBlockPtr ((|soapBlock|))
        SOAP Block.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockClearMustunderstandAttr(/* [out] */ OpenSOAPBlockPtr soapBlock) {
	OpenSOAPStringPtr mustUnderstandNameStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(mustUnderstandName,
										 &mustUnderstandNameStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;

		ret = OpenSOAPXMLElmRemoveAttributeString(elm,
												  mustUnderstandNameStr,
												  1,
												  NULL);
		
		OpenSOAPStringRelease(mustUnderstandNameStr);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetActorAttr(soapBlock, actorUri)
    Set mustunderstand attribute.

    :Parameters
      :[in]  OpenSOAPBlockPtr ((|soapBlock|))
        SOAP Block.
      :[out] OpenSOAPStringPtr *((|actorUri|))
        actor attribute value.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetActorAttr(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                          /* [out] */ OpenSOAPStringPtr *actorUri) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (soapBlock && actorUri) {
		OpenSOAPEnvelopePtr env = NULL;
		ret = OpenSOAPXMLNodeGetRoot((OpenSOAPXMLNodePtr)soapBlock,
									 (OpenSOAPXMLNodePtr *)&env);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr actorName = NULL;
			ret = OpenSOAPEnvelopeGetActorNameString(env, &actorName);
			if (OPENSOAP_SUCCEEDED(ret)) {
				OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
				OpenSOAPXMLAttrPtr attr = NULL;
				ret = OpenSOAPXMLElmGetAttributeString(elm,
													   actorName,
													   &attr);
				if (OPENSOAP_SUCCEEDED(ret)) {
					if (!*actorUri) {
						ret = OpenSOAPStringCreate(actorUri);
					}
					if (OPENSOAP_SUCCEEDED(ret)) {
						ret = OpenSOAPXMLAttrGetValueMB(attr,
														"string",
														actorUri);
					}
				}

				OpenSOAPStringRelease(actorName);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetActorAttrString(soapBlock, actorUrl)
    Set actor attribute.(OpenSOAPString)

    :Parameters
      :[out] OpenSOAPBlockPtr ((|soapBlock|))
        SOAP Block.
      :[in]  OpenSOAPStringPtr ((|actorUrl|))
        actor attribute value.
    :Return Value
      :int
	    Return Code Is ErrorCode
	:Note
	  This Function Is Static Function
      
=end
 */
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPBlockSetActorAttrString(/* [out] */ OpenSOAPBlockPtr soapBlock,
								/* [in]  */ OpenSOAPStringPtr actorUrl,
								/* [in]  */ int isDup) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (soapBlock && actorUrl) {
		OpenSOAPEnvelopePtr env = NULL;
		ret = OpenSOAPXMLNodeGetRoot((OpenSOAPXMLNodePtr)soapBlock,
									 (OpenSOAPXMLNodePtr *)&env);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr actorName = NULL;
			ret = OpenSOAPEnvelopeGetActorNameString(env, &actorName);
			if (OPENSOAP_SUCCEEDED(ret)) {
				OpenSOAPXMLElmPtr elm = (OpenSOAPXMLElmPtr)soapBlock;
				ret = OpenSOAPXMLElmSetAttributeValueAsString(elm,
															  actorName,
															  actorUrl,
															  isDup);

				if (OPENSOAP_FAILED(ret) || isDup) {
					OpenSOAPStringRelease(actorName);
				}
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetActorAttrMB(soapBlock, actorUrl)
    Set actor attribute.(MB)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const char * [in] ((|actorUrl|))
        actor attribute value.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetActorAttrMB(/* [out] */ OpenSOAPBlockPtr soapBlock,
                            /* [in]  */ const char *actorUrl) {
	OpenSOAPStringPtr actorUrlStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(actorUrl,
										 &actorUrlStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPBlockSetActorAttrString(soapBlock,
											  actorUrlStr,
											  0);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetActorAttrWC(soapBlock, actorUrl)
    Set actor attribute.(WC)

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
      :const wchar_t * [in] ((|actorUrl|))
        actor attribute value.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetActorAttrWC(/* [out] */ OpenSOAPBlockPtr soapBlock,
                            /* [in]  */ const wchar_t *actorUrl) {
	OpenSOAPStringPtr actorUrlStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(actorUrl,
										 &actorUrlStr);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPBlockSetActorAttrString(soapBlock,
											  actorUrlStr,
											  0);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockClearActorAttr(soapBlock)
    Clear actor attribute.

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockClearActorAttr(/* [out] */ OpenSOAPBlockPtr soapBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetActorNextString(soapEnv, actorNext)
    Get actor next url value.

    :Parameters
      :[in]  OpenSOAPBlockPtr ((|soapBlock|))
        OpenSOAP Block.
      :[out] OpenSOAPStringPtr * ((|actorNext|))
	    Next actor value stored buffer. If *actorNext is NULL,
		then create OpenSOAPString.
    :Return value
      :int
	    Error code.
      
=end
*/
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPBlockGetActorNextString(/* [in]  */ OpenSOAPBlockPtr soapBlock,
								/* [out] */ OpenSOAPStringPtr *actorNext) {
	OpenSOAPEnvelopePtr env = NULL;
	/* get Envelope */
	int ret = OpenSOAPXMLNodeGetRoot((OpenSOAPXMLNodePtr)soapBlock,
									 (OpenSOAPXMLNodePtr *)&env);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPEnvelopeGetActorNextString(env,
												 actorNext);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockSetActorAttrNext(soapBlock)
    Set actor attribute to next.

    :Parameters
      :OpenSOAPBlockPtr [in, out] ((|soapBlock|))
        SOAP Block.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockSetActorAttrNext(/* [in, out] */ OpenSOAPBlockPtr soapBlock) {
	OpenSOAPStringPtr actorNext = NULL;
	int ret = OpenSOAPBlockGetActorNextString(soapBlock, &actorNext);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPBlockSetActorAttrString(soapBlock,
											  actorNext,
											  0);
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPStringRelease(actorNext);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockIsActorAttrNext(soapBlock, isActorNext)
    Is actor attribute to next.

    :Parameters
      :[in]  OpenSOAPBlockPtr ((|soapBlock|))
        SOAP Block.
      :[out] int * ((|isActorNext|))
        judge result.
    :Return Value
      :int
	    Return Code Is ErrorCode
      
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockIsActorAttrNext(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                             /* [out] */ int *isActorNext) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (soapBlock && isActorNext) {
		OpenSOAPStringPtr actorValue = NULL;

		*isActorNext = 0;
		ret = OpenSOAPBlockGetActorAttr(soapBlock, &actorValue);

		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr actorNext = NULL;
			ret = OpenSOAPBlockGetActorNextString(soapBlock,
												  &actorNext);

			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPStringCompare(actorValue,
											actorNext,
											isActorNext);

				OpenSOAPStringRelease(actorNext);
			}

			OpenSOAPStringRelease(actorValue);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetName(block, name)
    Getting of SOAP Block Name

    :Parameters
      :[in]  OpenSOAPBlockPtr ((|block|))
        SOAP Block Pointer.
      :[out] OpenSOAPStringPtr * ((|name|))
        Result Name of SOAP Block.
    :Return Value
      :int
	    Error code.
=end
 */
extern
int
OPENSOAP_API
OpenSOAPBlockGetName(/* [in]  */ OpenSOAPBlockPtr soapBlock,
                     /* [out] */ OpenSOAPStringPtr *name) {
    int ret = OpenSOAPXMLNodeGetName((OpenSOAPXMLNodePtr)soapBlock,
                                     name);

    return ret;
}

/*
=begin
--- function#OpenSOAPBlockGetCharEncodingString(soapBlock, chEnc, bAry)
    Soap block character encoding output

    :Parameters
      :[in]  OpenSOAPBlockPtr ((|soapBlock|))
        OpenSOAP Block.
      :[in]  const char  * ((|chEnc|))
        character encoding. i.e. "EUC-JP", "Shift_JIS", "UTF-8"
      :[out] OpenSOAPByteArrayPtr ((|bAry|))
        Result Buffer        
    :Return Value
      :int
	    Error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPBlockGetCharEncodingString(/* [in]  */ OpenSOAPBlockPtr soapBlock,
								   /* [in]  */ const char *chEnc,
								   /* [out] */ OpenSOAPByteArrayPtr bAry) {
	int ret = OpenSOAPXMLElmGetCharEncodingString((OpenSOAPXMLElmPtr)soapBlock,
												  chEnc,
												  bAry);

    return ret;
}
