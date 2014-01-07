/*-----------------------------------------------------------------------------
 * $RCSfile: Envelope.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: Envelope.c,v 1.85 2003/06/30 08:36:10 okada Exp $";
#endif  /* _DEBUG */

#include "Envelope.h"
#include "Block.h"
#include "XMLNamespace.h"

#include <OpenSOAP/Locale.h>

#include <string.h>
#include <ctype.h>

/* SOAP Envelope Element Name */
static
const char SOAPEnvelopeElementName[] = "Envelope";
/* */
static
const char SOAPHeaderElementName[] = "Header";
/* */
static
const char SOAPBodyElementName[] = "Body";

/*
 */
typedef struct {
	const char *soapVersion;
	const char *soapEnvURI;
	const char *soapEnvDefPrefix;
	const char *soapActorName;
	const char *soapActorNext;
} SoapVersionEnvValues;

typedef const SoapVersionEnvValues *SoapVersionEnvValuesMapConstIterator;

static
const
SoapVersionEnvValues SOAP_VERSION_ENVURI_MAP[] = {
	{ "1.1",
	  "http://schemas.xmlsoap.org/soap/envelope/",
	  "SOAP-ENV",
	  "actor",
	  "http://schemas.xmlsoap.org/soap/actor/next"},
	{ "1.2",
	  "http://www.w3.org/2003/05/soap-envelope",
	  "env",
	  "role",
	  "http://www.w3.org/2002/06/soap-envelope/role/next"},
	{ NULL, NULL, NULL, NULL, NULL}
};

/*
=begin
= OpenSOAP Envelope class
=end
 */

static
int
OpenSOAPEnvelopeGetEnvValues(/* [in]  */ OpenSOAPEnvelopePtr env,
							 /* [out] */
							 SoapVersionEnvValuesMapConstIterator *vals) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (env && vals) {
		SoapVersionEnvValuesMapConstIterator i = SOAP_VERSION_ENVURI_MAP;
		for (; i->soapVersion; ++i) {
			int cmpResult = 0;
			ret = OpenSOAPStringCompareMB(env->version,
										  i->soapVersion,
										  &cmpResult);
			if (OPENSOAP_FAILED(ret)) {
				break;
			}
			if (cmpResult == 0) {
				break;
			}
		}
		if (OPENSOAP_SUCCEEDED(ret)) {
			*vals = i;
		}
	}

	return ret;
}

static
int
OpenSOAPEnvelopeReleaseMembers(/* [in, out] */ OpenSOAPEnvelopePtr soapEnv) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (soapEnv) {
        OpenSOAPStringRelease(soapEnv->version);

        ret = OpenSOAPXMLElmReleaseMembers((OpenSOAPXMLElmPtr)soapEnv);
    }

    return ret;
}

static
int
OpenSOAPEnvelopeFree(OpenSOAPObjectPtr /* [in, out] */ obj) {
    int ret = OpenSOAPEnvelopeReleaseMembers((OpenSOAPEnvelopePtr)obj);

    if (OPENSOAP_SUCCEEDED(ret)) {
        free(obj);
    }

    return ret;
}

static
int
OpenSOAPEnvelopeInitialize(OpenSOAPEnvelopePtr /* [in, out] */ soapEnv) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (soapEnv) {
        soapEnv->version = NULL;
        soapEnv->header  = NULL;
        soapEnv->body    = NULL;

        ret = OpenSOAPXMLElmInitialize((OpenSOAPXMLElmPtr)soapEnv,
                                       OpenSOAPEnvelopeFree);
    }

    return ret;
}



/*
=begin
--- function#OpenSOAPEnvelopeCreate(soapEnv)
    OpenSOAP Envelope instance create
    
    :Parameters
      :OpenSOAPEnvelopePtr * [out] ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      SOAP Envelope 領域の確保と構造体の初期化を行います。
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeCreate(OpenSOAPEnvelopePtr *soapEnv) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (soapEnv) {
        ret = OPENSOAP_MEM_BADALLOC;
        *soapEnv = malloc(sizeof(OpenSOAPEnvelope));
        if (*soapEnv) {
            ret = OpenSOAPEnvelopeInitialize(*soapEnv);

            if (OPENSOAP_FAILED(ret)) {
                free(*soapEnv);
                *soapEnv = NULL;
            }
        }
    }
    
    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeSetValuesMB(soapEnv, soapVer, envURI, envPrefix)
    OpenSOAP Envelope set variables.

    :Parameters
      :[out] const cahr * ((||soapEnv|))
        OpenSOAP Envelope Pointer.
      :[in]  const char * ((|soapVer|))
	    SOAP Version
      :[in]  const char * ((|envURI|))
        SOAP Envelope namespace URI.
      :[in]  const char * ((|envPrefix|))
        SOAP Envelope namespace prefix.
    :Return value
      :int 
	    Error code.
=end
 */
static
int
OpenSOAPEnvelopeSetValuesMB(/* [out] */ OpenSOAPEnvelopePtr soapEnv,
							/* [in]  */ const char *soapVer,
							/* [in]  */ const char *envURI,
							/* [in]  */ const char *envPrefix) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (soapEnv && soapVer && envURI && envPrefix) {
		OpenSOAPXMLElmPtr  rootElm  = (OpenSOAPXMLElmPtr)(soapEnv);
		OpenSOAPXMLNodePtr rootNode = (OpenSOAPXMLNodePtr)(rootElm);
		ret = OpenSOAPStringCreateWithMB(SOAPEnvelopeElementName,
										 &(rootNode->name));
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPStringCreateWithMB(soapVer,
											 &(soapEnv->version));
					
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPXMLNamespaceCreateMB(envURI,
												   envPrefix,
												   &(rootNode
													 ->thisNamespace));
				if (OPENSOAP_SUCCEEDED(ret)) {
					rootNode->thisNamespace->definedElement
						= rootElm;
					rootElm->definedXMLNsList
						= rootNode->thisNamespace;
				}
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeCreateMB(soapVer, envPrefix, soapEnv)
    OpenSOAP Envelope instance create(MB)

    :Parameters
      :const cahr * [in] ((||soapVer|))
        SOAP Virsion
      :const char * [in] ((|envPrefix|))
        SOAP Envelope namespace prefix.
      :OpenSOAPEnvelopePtr * [out] ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :int 
	    Return Code Is ErrorCode
    :Note
      (2001/08/28)
	  SOAP Envelope作成領域の確保を行い、SOAP Versionによって
	  各々定数の代入を行います。
      soapVer      NULL 以外の場合は "1.1" or "1.2"
                    NULL の場合は "1.1" と同等。
      envPrefix    NULL 以外の場合は内容に従う。
                    NULL の場合はsoapVerにより以下に従う。
                    "1.1" SOAP-ENV
                    "1.2" env
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeCreateMB(const char *soapVer,
                         const char *envPrefix,
                         OpenSOAPEnvelopePtr *soapEnv) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (soapEnv) {
		SoapVersionEnvValuesMapConstIterator i = SOAP_VERSION_ENVURI_MAP;
		if (soapVer) {
			for (; i->soapVersion && strcmp(soapVer, i->soapVersion) != 0;
				 ++i) {
			}
		}
		if (i->soapVersion) {
			ret = OpenSOAPEnvelopeCreate(soapEnv);
			if (OPENSOAP_SUCCEEDED(ret)) {
				if (!envPrefix) {
					envPrefix = i->soapEnvDefPrefix;
				}
				ret = OpenSOAPEnvelopeSetValuesMB(*soapEnv,
												  i->soapVersion,
												  i->soapEnvURI,
												  envPrefix);
			}
			if (OPENSOAP_FAILED(ret)) {
				OpenSOAPEnvelopeRelease(*soapEnv);
				*soapEnv = NULL;
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPStringConvertWCtoMB(wcStr, memAllocator, len, mbStr)
    Convert const wchar_t * to char *

    :Parameters
      :[in]  const wchar_t * ((|wcStr|))
        wide character string.
      :[in]  char * ( * ((|memAllocator|)) ) (size_t)
        memory allocator
      :[out] size_t * ((|len|))
        size of string buffer. If NULL, no effect.
      :[out] char ** ((|mbStr|))
        string stored buffer pointer.
    :Return value
      :type int
        Error code.
=end
 */
static
int
OpenSOAPStringConvertWCtoMB(/* [in]  */ const wchar_t *wcStr,
							/* [in]  */  char *(*memAllocator)(size_t),
							/* [out] */ size_t *len,
							/* [out] */ char **mbStr) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (mbStr) {
		ret = OPENSOAP_NO_ERROR;
		*mbStr = NULL;
		if (wcStr) {
			OpenSOAPStringPtr convStr = NULL;
			ret = OpenSOAPStringCreateWithWC(wcStr, &convStr);
			if (OPENSOAP_SUCCEEDED(ret)) {
				/* */
				ret = OpenSOAPStringGetStringMBWithAllocator(convStr,
															 memAllocator,
															 len,
															 mbStr);
				/* release */
				OpenSOAPStringRelease(convStr);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeCreateWC(soapVer, envPrefix, soapEnv)
    OpenSOAP Envelope instance create(WC)

    :Parameters
      :const cahr * [in] ((||soapVer|))
        SOAP Virsion
      :const char * [in] ((|envPrefix|))
        SOAP Envelope namespace prefix.
      :OpenSOAPEnvelopePtr * [out] ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :type int
        Error code.
    :Note
      (2001/08/28)
	  SOAP Envelope作成領域の確保を行い、SOAP Versionによって
	  各々定数の代入を行います。
      soapVer      NULL 以外の場合は "1.1" or "1.2"
                    NULL の場合は "1.1" と同等。
      envPrefix    NULL 以外の場合は内容に従う。
                    NULL の場合はsoapVerにより以下に従う。
                    "1.1" SOAP-ENV
                    "1.2" env
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeCreateWC(const wchar_t *soapVer,
                         const wchar_t *envPrefix,
                         OpenSOAPEnvelopePtr *soapEnv) {
	char *soapVerMB = NULL;
	int ret = OpenSOAPStringConvertWCtoMB(soapVer, NULL, NULL, &soapVerMB);
	if (OPENSOAP_SUCCEEDED(ret)) {
		char *envPrefixMB = NULL;
		ret = OpenSOAPStringConvertWCtoMB(envPrefix, NULL, NULL, &envPrefixMB);
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* */
			ret = OpenSOAPEnvelopeCreateMB(soapVerMB, envPrefixMB, soapEnv);
			/* */
			free(envPrefixMB);
		}
		/* */
		free(soapVerMB);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeCreateString(soapVer, envPrefix, soapEnv)
    OpenSOAP Envelope instance create(OpenSOAPString)

    :Parameters
      :[in]  OpenSOAPStringPtr ((||soapVer|))
        SOAP Virsion
      :[in]  OpenSOAPStringPtr ((|envPrefix|))
        SOAP Envelope namespace prefix.
      :[out] OpenSOAPEnvelopePtr * ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :type int
        Error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeCreateString(/* [in]  */ OpenSOAPStringPtr soapVer,
							 /* [in]  */ OpenSOAPStringPtr envPrefix,
							 /* [out] */ OpenSOAPEnvelopePtr *soapEnv) {
	char *soapVerMB = NULL;
	int ret = soapVer ?
		OpenSOAPStringGetStringMBWithAllocator(soapVer,
											   NULL,
											   NULL,
											   &soapVerMB)
		: OPENSOAP_NO_ERROR;
	if (OPENSOAP_SUCCEEDED(ret)) {
		char *envPrefixMB = NULL;
		ret = envPrefix ?
			OpenSOAPStringGetStringMBWithAllocator(envPrefix,
												   NULL,
												   NULL,
												   &envPrefixMB)
			: OPENSOAP_NO_ERROR;
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* */
			ret = OpenSOAPEnvelopeCreateMB(soapVerMB, envPrefixMB, soapEnv);
			/* */
			free(envPrefixMB);
		}
		/* */
		free(soapVerMB);
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeCreateWithLibxmlNode(envNode, doc, soapEnv)
=end
*/
static
int
OpenSOAPEnvelopeCreateWithLibxmlNode(/* [in]  */ xmlNodePtr	envNode,
									 /* [in]  */ xmlDocPtr	doc,
									 /* [out] */ OpenSOAPEnvelopePtr *soapEnv) {

    int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (doc && envNode && soapEnv) {
		xmlNsPtr	envNs;
		SoapVersionEnvValuesMapConstIterator i = SOAP_VERSION_ENVURI_MAP;
		for (envNs = NULL; i->soapVersion; ++i) {
			envNs = xmlSearchNsByHref(doc,
									   envNode,
									   i->soapEnvURI);
			if (envNs) {
				break;
			}
		}
		if (!i->soapVersion) {
			ret = OPENSOAP_XML_BADNAMESPACE;
		}
		else if (envNode->name && strcmp(envNode->name,
										 SOAPEnvelopeElementName) != 0) {
			/* root node name check */
			ret = OPENSOAP_XML_BADDOCUMENTTYPE;
		}
		else {
			ret = OpenSOAPEnvelopeCreateMB(i->soapVersion,
										   envNs->prefix,
										   soapEnv);
			if (OPENSOAP_SUCCEEDED(ret)) {
				OpenSOAPXMLElmPtr envElm = (OpenSOAPXMLElmPtr)(*soapEnv);
				/* add envelope's attribute */
				ret = OpenSOAPXMLElmAddAttrFromLibxmlNode(envElm,
														  envNode);
				if (OPENSOAP_FAILED(ret)) {
					OpenSOAPEnvelopeRelease(*soapEnv);
					*soapEnv = NULL;
				}
			}
		}
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeSetHeaderBodyElm(soapEnv)
    create char encoding
    
    :Parameters
      :[in, out] OpenSOAPEnvelopePtr ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :type int
        error code.
    :Note
      
=end
*/
static
int
OpenSOAPEnvelopeSetHeaderBodyElm(OpenSOAPEnvelopePtr soapEnv) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	
	if (soapEnv) {
		/* Envelope Node */
		OpenSOAPXMLNodePtr envNode = (OpenSOAPXMLNodePtr)soapEnv;
		/* SOAP-ENV namespace */
		OpenSOAPXMLNamespacePtr envNs = envNode->thisNamespace;
		/* Envelope child node */
		OpenSOAPXMLNodePtr envChild = envNode->children;
		/* */
		int cmpResult = 0;

		/* Header check */
		ret = envChild
			? OpenSOAPStringCompareMB(envChild->name,
									  SOAPHeaderElementName,
									  &cmpResult)
			: OPENSOAP_XML_NOHEADERBODY;
		if (OPENSOAP_SUCCEEDED(ret)) {
			if (cmpResult == 0) {
				if (envChild->thisNamespace != envNs) {
					ret = OPENSOAP_XML_BADNAMESPACE;
				}
				else {
					soapEnv->header = (OpenSOAPXMLElmPtr)envChild;
					ret = OpenSOAPBlockReplaceXMLElmChildren(soapEnv
															 ->header);
					envChild = envChild->next;
				}
			}
			if (OPENSOAP_SUCCEEDED(ret)) {
				/* Body check */
				ret = envChild
					? OpenSOAPStringCompareMB(envChild->name,
											  SOAPBodyElementName,
											  &cmpResult)
					: OPENSOAP_XML_NOHEADERBODY;
				if (cmpResult == 0) {
					if (envChild->thisNamespace != envNs) {
						ret = OPENSOAP_XML_BADNAMESPACE;
					}
					else {
						soapEnv->body = (OpenSOAPXMLElmPtr)envChild;
						ret = OpenSOAPBlockReplaceXMLElmChildren(soapEnv
																 ->body);
					}
				}
				else {
					ret = OPENSOAP_XML_NOHEADERBODY;
				}
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeCreateCharEncoding(chEnc, b_ary, soapEnv)
    create char encoding
    
    :Parameters
      :const cahr * [in] ((|chEnc|))
        character encoding
      :OpenSOAPByteArrayPtr [in] ((|b_ary|))
        OpenSOAP ByteArray
      :OpenSOAPEnvelopePtr * [out] ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :type int
        error code.
    :Note
      (2001/08/28)
      b_aryの内容をchEnc(EUC-JP/Shift_JIS/UTF-8...)にエンコーディングする。
      soapEnv内に各々パラメータをb_aryより設定する。
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeCreateCharEncoding(const char *chEnc,
                                   OpenSOAPByteArrayPtr bAry,
                                   OpenSOAPEnvelopePtr *soapEnv) {
    xmlDocPtr	doc = NULL;
    xmlNodePtr	envNode = NULL;

	int ret = OpenSOAPLibxmlDocCreateCharEncoding(chEnc,
												  bAry,
												  &doc,
												  &envNode);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPEnvelopeCreateWithLibxmlNode(envNode,
												   doc,
												   soapEnv);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPXMLElmCreateChildNodeFromLibxml((OpenSOAPXMLElmPtr)(*soapEnv),
														  envNode);
			
			if (OPENSOAP_SUCCEEDED(ret)) {
				ret = OpenSOAPEnvelopeSetHeaderBodyElm(*soapEnv);
			}
		}

		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPEnvelopeRelease(*soapEnv);
			*soapEnv = NULL;
		}

		xmlFreeDoc(doc);
	}

	return ret;
}


/*
=begin
--- function#OpenSOAPEnvelopeRetain(soapEnv)
    SOAP Envelope リファレンス追加。

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeRetain(OpenSOAPEnvelopePtr soapEnv) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeRelease(soapEnv)
    Release SOAP Envelope Buffer

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
	  SOAP Envelope作成領域の開放を行います。
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeRelease(OpenSOAPEnvelopePtr soapEnv) {
    int ret = OpenSOAPXMLElmRelease((OpenSOAPXMLElmPtr)soapEnv);

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddHeaderBlockString(soapEnv, block_name, hBlock)
    Add SOAP Header Block(OpenSOAPString)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :OpenSOAPStringPtr [in] ((|block_name|))
        SOAP Block Name
      :OpenSOAPBlockPtr * [out] ((|hBlock|))
        OpenSOAP Header Block
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      SOAP Envelope領域にHeader Blockを追加します。
      (2001/10/11)
      This Function Is Static Function
  
=end
*/
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPEnvelopeAddHeaderBlockString(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                 OpenSOAPStringPtr /* [in] */ block_name,
                                 OpenSOAPBlockPtr * /* [out] */ hBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	OpenSOAPXMLElmPtr second_b;
	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)soapEnv;

	if( !soapEnv || !hBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	if( !soapEnv->header && block_name == NULL ){
		OpenSOAPXMLNodePtr headerNode = NULL;
		/* add body element only ( no child---body block )*/
		ret = OpenSOAPXMLElmCreate( &soapEnv->header );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}
		headerNode = (OpenSOAPXMLNodePtr)soapEnv->header;

		ret = OpenSOAPStringCreate( &(headerNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringCreateWithMB(SOAPHeaderElementName,
										 &headerNode->name);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		if (node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
		}
		else{
			second_b = (OpenSOAPXMLElmPtr)node->children;
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
			headerNode->next = (OpenSOAPXMLNodePtr)second_b;
		}

		headerNode->parent = (OpenSOAPXMLNodePtr)soapEnv;

		*hBlock = NULL;

		ret = OPENSOAP_NO_ERROR;
		return ret;
	}

	if( !soapEnv->header && block_name != NULL ){
		OpenSOAPXMLNodePtr headerNode = NULL;
		ret = OpenSOAPXMLElmCreate( &soapEnv->header );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		headerNode = (OpenSOAPXMLNodePtr)soapEnv->header;

		ret = OpenSOAPStringCreate( &(headerNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringCreateWithMB(SOAPHeaderElementName,
										 &headerNode->name);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		
		if (node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
		}
		else{
			second_b = (OpenSOAPXMLElmPtr)node->children;
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
			headerNode->next = (OpenSOAPXMLNodePtr)second_b;
		}

		headerNode->parent = (OpenSOAPXMLNodePtr)soapEnv;
	}

	ret = OpenSOAPBlockCreate( hBlock );
	if( OPENSOAP_FAILED(ret) ){
		return ret;
	}

	ret = OpenSOAPXMLElmAddChildString( soapEnv->header, block_name, (OpenSOAPXMLElmPtr*)hBlock );

    return ret;
}


/*
=begin
--- function#OpenSOAPEnvelopeAddHeaderBlockMB(soapEnv, block_name, hBlock)
    Add SOAP Header Block(MB)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const char * [in] ((|block_name|))
        SOAP Block Name
      :OpenSOAPBlockPtr * [out] ((|hBlock|))
        OpenSOAP Header Block
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      SOAP Envelope領域にHeader Blockを追加します。
  
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeAddHeaderBlockMB(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                 const char * /* [in] */ block_name,
                                 OpenSOAPBlockPtr * /* [out] */ hBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	OpenSOAPXMLElmPtr second_b;
	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)soapEnv;

	if( !soapEnv || !hBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	if( !soapEnv->header && block_name == NULL ){
		OpenSOAPXMLNodePtr headerNode = NULL;
		/* add body element only ( no child---body block )*/
		ret = OpenSOAPXMLElmCreate( &soapEnv->header );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		headerNode = (OpenSOAPXMLNodePtr)soapEnv->header;

		ret = OpenSOAPStringCreate( &(headerNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringSetStringMB(headerNode->name,
										SOAPHeaderElementName);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		if( node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
		}
		else{
			second_b = (OpenSOAPXMLElmPtr)node->children;
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
			headerNode->next = (OpenSOAPXMLNodePtr)second_b;
		}

		headerNode->parent = (OpenSOAPXMLNodePtr)soapEnv;
		headerNode->thisNamespace = node->thisNamespace;

		*hBlock = NULL;

		ret = OPENSOAP_NO_ERROR;
		return ret;
	}

	if( !soapEnv->header && block_name != NULL ){
		OpenSOAPXMLNodePtr headerNode = NULL;

		ret = OpenSOAPXMLElmCreate( &soapEnv->header );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		headerNode = (OpenSOAPXMLNodePtr)soapEnv->header;
		
		ret = OpenSOAPStringCreate( &(headerNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringSetStringMB(headerNode->name,
										SOAPHeaderElementName);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		
		if( node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
		}
		else{
			second_b = (OpenSOAPXMLElmPtr)node->children;
			node->children = (OpenSOAPXMLNodePtr)soapEnv->header;
			headerNode->next = (OpenSOAPXMLNodePtr)second_b;
		}

		headerNode->parent = (OpenSOAPXMLNodePtr)soapEnv;
		headerNode->thisNamespace = node->thisNamespace;
	}

	ret = OpenSOAPBlockCreate( hBlock );
	if( OPENSOAP_FAILED(ret) ){
		return ret;
	}

	ret = OpenSOAPXMLElmAddChildMB( soapEnv->header, block_name, (OpenSOAPXMLElmPtr*)hBlock );

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddHeaderBlockWC(soapEnv, block_name, hBlock)
    Add SOAP Header Block(WC)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const wchar_t * [in] ((|block_name|))
        SOAP Block Name
      :OpenSOAPBlockPtr * [out] ((|hBlock|))
        OpenSOAP Header Block
    :Return value
      :int
	    Return Code Is ErrorCode
    :Note
      SOAP Envelope領域にHeader Blockを追加します。
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeAddHeaderBlockWC(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                 const wchar_t * /* [in] */ block_name,
                                 OpenSOAPBlockPtr * /* [out] */ hBlock) {
	OpenSOAPStringPtr block_name_str = NULL;
	int ret = OpenSOAPStringCreateWithWC(block_name, &block_name_str);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPEnvelopeAddHeaderBlockString(soapEnv, block_name_str, hBlock);
		OpenSOAPStringRelease(block_name_str);
	}
	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetNextHeaderBlock(soapEnv, hBlock)
    Get SOAP Header Block

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :OpenSOAPBlockPtr * [in, out] ((|hBlock|))
        if OpenSOAP Header Block. *hBlock as in NULL then
        Return to First Header Block
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetNextHeaderBlock(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                   OpenSOAPBlockPtr * /* [in, out] */ hBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !hBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	if( soapEnv->header != NULL ){
		ret = OpenSOAPXMLElmGetNextChild( soapEnv->header,
										  (OpenSOAPXMLElmPtr *)(hBlock) );
	}
	else{
		*hBlock = NULL;
		ret = OPENSOAP_NO_ERROR;
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetHeaderBlockString(soapEnv, block_name, hBlock)
    Get SOAP Header Block(OpenSOAPString)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :OpenSOAPStringPtr [in] ((|block_name|))
        SOAP Header Block name.
      :OpenSOAPBlockPtr * [out] ((|hBlock|))
        Header Block return buffer's pointer.
    :Return value
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
OpenSOAPEnvelopeGetHeaderBlockString(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                               OpenSOAPStringPtr /* [in] */ block_name,
                               OpenSOAPBlockPtr * /* [out] */ hBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !hBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	ret = OpenSOAPXMLElmGetChildString( soapEnv->header, 
									block_name, 
									(OpenSOAPXMLElmPtr*)hBlock );

    return ret;
}


/*
=begin
--- function#OpenSOAPEnvelopeGetHeaderBlockMB(soapEnv, block_name, hBlock)
    Get SOAP Header Block(MB)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const char * [in] ((|block_name|))
        SOAP Header Block name.
      :OpenSOAPBlockPtr * [out] ((|hBlock|))
        Header Block return buffer's pointer.
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetHeaderBlockMB(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                               const char * /* [in] */ block_name,
                               OpenSOAPBlockPtr * /* [out] */ hBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !hBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	ret = OpenSOAPXMLElmGetChildMB( soapEnv->header, 
									block_name, 
									(OpenSOAPXMLElmPtr*)hBlock );

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetHeaderBlockWC(soapEnv, block_name, hBlock)
    Get SOAP Header Block(WC)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const wchar_t * [in] ((|block_name|))
        SOAP Header Block name.
      :OpenSOAPBlockPtr * [out] ((|hBlock|))
        Header Block return buffer's pointer.
        
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetHeaderBlockWC(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                               const wchar_t * /* [in] */ block_name,
                               OpenSOAPBlockPtr * /* [out] */ hBlock) {
	OpenSOAPStringPtr block_name_str = NULL;
	int ret = OpenSOAPStringCreateWithWC(block_name, &block_name_str);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPEnvelopeGetHeaderBlockString(soapEnv, block_name_str, hBlock);
		OpenSOAPStringRelease(block_name_str);
	}
	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddBodyBlockString(soapEnv, block_name, bBlock)
    Add SOAP Body Block(OpenSOAPString)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :OpenSOAPStringPtr [in] ((|block_name|))
        SOAP Block Name
		If block_name==NULL, add body element only (no body block)
      :OpenSOAPBlockPtr * [out] ((|bBlock|))
        OpenSOAP Body Block
        
    :Return value
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
OpenSOAPEnvelopeAddBodyBlockString(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                 OpenSOAPStringPtr /* [in] */ block_name,
                                 OpenSOAPBlockPtr * /* [out] */ bBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;
	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)soapEnv;

	if( !soapEnv || !bBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	if( !soapEnv->body && block_name == NULL ){
		OpenSOAPXMLNodePtr bodyNode = NULL;
		/* add body element only ( no child---body block )*/
		ret = OpenSOAPXMLElmCreate( &soapEnv->body );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}
		bodyNode = (OpenSOAPXMLNodePtr)soapEnv->body;

		ret = OpenSOAPStringCreate( &(bodyNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringSetStringMB(bodyNode->name,
										SOAPBodyElementName);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		if( node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->body;
		}
		else{
			node->children->next = (OpenSOAPXMLNodePtr)soapEnv->body;
			bodyNode->prev = node->children;
		}

		bodyNode->parent = (OpenSOAPXMLNodePtr)soapEnv;

		*bBlock = NULL;
		
		ret = OPENSOAP_NO_ERROR;
		return ret;
	}
	
	if( !soapEnv->body && block_name != NULL ){
		OpenSOAPXMLNodePtr bodyNode = NULL;
		/* add body element and its child---body block named by block_name */
		ret = OpenSOAPXMLElmCreate( &soapEnv->body );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}
		
		bodyNode = (OpenSOAPXMLNodePtr)soapEnv->body;

		ret = OpenSOAPStringCreate( &(bodyNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringSetStringMB(bodyNode->name,
										SOAPBodyElementName);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		if( node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->body;
		}
		else{
			node->children->next = (OpenSOAPXMLNodePtr)soapEnv->body;
			bodyNode->prev = node->children;
		}
		bodyNode->parent = (OpenSOAPXMLNodePtr)soapEnv;
	}


	ret = OpenSOAPBlockCreate( bBlock );
	if( OPENSOAP_FAILED(ret) ){
		return ret;
	}

	ret = OpenSOAPXMLElmAddChildString( soapEnv->body, block_name, (OpenSOAPXMLElmPtr*)bBlock );

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddBodyBlockMB(soapEnv, block_name, bBlock)
    Add SOAP Body Block(MB)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const char * [in] ((|block_name|))
        SOAP Block Name
		If block_name==NULL, add body element only (no body block)
      :OpenSOAPBlockPtr * [out] ((|bBlock|))
        OpenSOAP Body Block
        
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeAddBodyBlockMB(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                 const char * /* [in] */ block_name,
                                 OpenSOAPBlockPtr * /* [out] */ bBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	OpenSOAPXMLNodePtr node = (OpenSOAPXMLNodePtr)soapEnv;

	if( !soapEnv || !bBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	if( !soapEnv->body && block_name == NULL ){
		OpenSOAPXMLNodePtr bodyNode = NULL;
		/* add body element only ( no child---body block )*/
		ret = OpenSOAPXMLElmCreate( &soapEnv->body );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		bodyNode = (OpenSOAPXMLNodePtr)soapEnv->body;
		ret = OpenSOAPStringCreate( &(bodyNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringSetStringMB(bodyNode->name,
										SOAPBodyElementName);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		if( node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->body;
		}
		else{
			node->children->next = (OpenSOAPXMLNodePtr)soapEnv->body;
			bodyNode->prev = node->children;
		}

		bodyNode->parent = (OpenSOAPXMLNodePtr)soapEnv;
		bodyNode->thisNamespace = node->thisNamespace;

		*bBlock = NULL;
		
		ret = OPENSOAP_NO_ERROR;
		return ret;
	}
	
	if( !soapEnv->body && block_name != NULL ){
		OpenSOAPXMLNodePtr bodyNode = NULL;
		/* add body element and its child---body block named by block_name */
		ret = OpenSOAPXMLElmCreate( &soapEnv->body );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		bodyNode = (OpenSOAPXMLNodePtr)soapEnv->body;
		ret = OpenSOAPStringCreate( &(bodyNode->name) );
		if( OPENSOAP_FAILED(ret) ){
			return ret;
		}

		ret = OpenSOAPStringSetStringMB(bodyNode->name,
										SOAPBodyElementName);
		if(ret == OPENSOAP_PARAMETER_BADVALUE){
			return ret;
		}

		if( node->children == NULL ){
			node->children = (OpenSOAPXMLNodePtr)soapEnv->body;
		}
		else{
			node->children->next = (OpenSOAPXMLNodePtr)soapEnv->body;
			bodyNode->prev = node->children;
		}
		bodyNode->parent = (OpenSOAPXMLNodePtr)soapEnv;
		bodyNode->thisNamespace = node->thisNamespace;
	}


	ret = OpenSOAPBlockCreate( bBlock );
	if( OPENSOAP_FAILED(ret) ){
		return ret;
	}

	ret = OpenSOAPXMLElmAddChildMB( soapEnv->body, block_name, (OpenSOAPXMLElmPtr*)bBlock );

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddBodyBlockWC(soapEnv, block_name, bBlock)
    Add SOAP Body Block(WC)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const wchar_t * [in] ((|block_name|))
        SOAP Block Name
      :OpenSOAPBlockPtr * [out] ((|bBlock|))
        OpenSOAP Body Block
        
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeAddBodyBlockWC(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                 const wchar_t * /* [in] */ block_name,
                                 OpenSOAPBlockPtr * /* [out] */ bBlock) {
	OpenSOAPStringPtr block_name_str = NULL;
	int ret = OpenSOAPStringCreateWithWC(block_name, &block_name_str);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPEnvelopeAddBodyBlockString(soapEnv, block_name_str, bBlock);
		OpenSOAPStringRelease(block_name_str);
	}
	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetNextBodyBlock(soapEnv, bBlock)
    Get SOAP Body Block

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :OpenSOAPBlockPtr * [in, out] ((|bBlock|))
        if OpenSOAP Body Block. *bBlock as in NULL then
        Return to First Body Block
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetNextBodyBlock(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                                   OpenSOAPBlockPtr * /* [in, out] */ bBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !bBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	if( soapEnv->body != NULL ){
		ret = OpenSOAPXMLElmGetNextChild( soapEnv->body,
										  (OpenSOAPXMLElmPtr *)(bBlock) );
	}
	else{
		*bBlock = NULL;
		ret = OPENSOAP_NO_ERROR;
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetBodyBlockString(soapEnv, block_name, bBlock)
    Get SOAP Body Block(OpenSOAPString)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :OpenSOAPStringPtr [in] ((|block_name|))
        SOAP Body Block name.
      :OpenSOAPBlockPtr * [out] ((|bBlock|))
        Body Block return buffer's pointer.
        
    :Return value
      :int
	    Return Code Is ErrorCode

    :Note
      (2001/08/28)
      block_nameを検索bBlockを戻す。
	  (2001/10/11)
	  This Function Is Static Function
=end
*/
/* extern */
static
int
/* OPENSOAP_API */
OpenSOAPEnvelopeGetBodyBlockString(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                               OpenSOAPStringPtr /* [in] */ block_name,
                               OpenSOAPBlockPtr * /* [out] */ bBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !bBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	ret = OpenSOAPXMLElmGetChildString( soapEnv->body, 
								  block_name, 
								 (OpenSOAPXMLElmPtr*)bBlock );

    return ret;
}


/*
=begin
--- function#OpenSOAPEnvelopeGetBodyBlockMB(soapEnv, block_name, bBlock)
    Get SOAP Body Block(MB)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const char * [in] ((|block_name|))
        SOAP Body Block name.
      :OpenSOAPBlockPtr * [out] ((|bBlock|))
        Body Block return buffer's pointer.
        
    :Return value
      :int
	    Return Code Is ErrorCode

    :Note
      (2001/08/28)
      block_nameを検索bBlockを戻す。
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetBodyBlockMB(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                               const char * /* [in] */ block_name,
                               OpenSOAPBlockPtr * /* [out] */ bBlock) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !bBlock ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
		return ret;
	}

	ret = OpenSOAPXMLElmGetChildMB( soapEnv->body, 
									block_name, 
									(OpenSOAPXMLElmPtr*)bBlock );

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetBodyBlockWC(soapEnv, block_name, bBlock)
    Get SOAP Body Block(WC)

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const wchar_t * [in] ((|block_name|))
        SOAP Body Block name.
      :OpenSOAPBlockPtr * [out] ((|bBlock|))
        Body Block return buffer's pointer.
        
    :Return value
      :int
	    Return Code Is ErrorCode

    :Note
      (2001/08/28)
      block_nameを検索bBlockを戻す。
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetBodyBlockWC(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
                               const wchar_t * /* [in] */ block_name,
                               OpenSOAPBlockPtr * /* [out] */ bBlock) {
	OpenSOAPStringPtr block_name_str = NULL;
	int ret = OpenSOAPStringCreateWithWC(block_name, &block_name_str);
	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPEnvelopeGetBodyBlockString(soapEnv, block_name_str, bBlock);
		OpenSOAPStringRelease(block_name_str);
	}
	return ret;
}

/*
=begin
--- function#OpenSOAPByteArrayAddXMLProc(bAry, chEnc)
    add XML Proc line.

    :Parameters
      :[in,out] OpenSOAPByteArrayPtr ((|bAry|))
        UTF-8 encode XML Element serialized data buffer.
      :[in]     const char  * ((|chEnc|))
        character encoding. i.e. "EUC-JP", "Shift_JIS", "UTF-8"
    :Return value
      :int
	    error code
=end
 */
static
int
OpenSOAPByteArrayAddXMLProc(/* [in,out] */ OpenSOAPByteArrayPtr bAry,
							/* [in]     */ const char *chEnc) {
	static const char XML_PROC_PRE[]
		= "<?xml version=\"1.0\" encoding=\"";
	static const char XML_PROC_POST[]
		= "\"?>\n";
	static const size_t XML_PROC_PRE_SIZE
		= sizeof(XML_PROC_PRE) - 1;
	static const size_t XML_PROC_POST_SIZE
		= sizeof(XML_PROC_POST) - 1;
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (bAry && chEnc) {
		const unsigned char *bAryBeg = NULL;
		size_t bArySize = 0;
		/* get XML element serialize data address and size */
		ret = OpenSOAPByteArrayGetBeginSizeConst(bAry,
												 &bAryBeg,
												 &bArySize);
		if (OPENSOAP_SUCCEEDED(ret)) {
			size_t chEncSize = strlen(chEnc);
			size_t bufSize
				= XML_PROC_PRE_SIZE
				+ chEncSize
				+ XML_PROC_POST_SIZE
				+ bArySize;
			char *buf = malloc(bufSize);
			if (!buf) {
				ret = OPENSOAP_MEM_BADALLOC;
			}
			else {
				char *i = buf;
				/* add XML Proc prefix for encoding value */
				memcpy(i, XML_PROC_PRE, XML_PROC_PRE_SIZE);
				i += XML_PROC_PRE_SIZE;
				/* add encoding value */
				memcpy(i, chEnc, chEncSize);
				i += chEncSize;
				/* add XML Proc postfix for encoding value */
				memcpy(i, XML_PROC_POST, XML_PROC_POST_SIZE);
				i += XML_PROC_POST_SIZE;
				/* add xml element serialize value */
				memcpy(i, bAryBeg, bArySize);

				/* set XML Proc line add data */
				ret = OpenSOAPByteArraySetData(bAry,
											   buf,
											   bufSize);
				
				
				free(buf);
			}
			
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetCharEncodingString(soapEnv, chEnc, bAry)
    SOAP Envelope character encoding output.

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope。
      :const char  * [in] ((|chEnc|))
        character encoding. i.e. "EUC-JP", "Shift_JIS", "UTF-8"
      :OpenSOAPByteArrayPtr [out] ((|bAry|))
        Strage Buffer
    :Return value
      :int
	    Return Code Is ErrorCode
 
    :Note
      (2001/08/28)
      soapEnvよりSOAP Messageを組み立てる。
     
=end
 */
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetCharEncodingString(/* [in]  */ OpenSOAPEnvelopePtr soapEnv,
                                      /* [in]  */ const char *chEnc,
                                      /* [out] */ OpenSOAPByteArrayPtr bAry) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	static const char MIDDLE_CHARSET[] = "UTF-8";

	if (soapEnv && bAry) {
		/* get current codeset */
		ret = chEnc ? OPENSOAP_NO_ERROR
			: OpenSOAPLocaleGetCurrentCodeset(&chEnc);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPByteArrayPtr xmlElmBAry = NULL;
			/* create xml element serialize buffer */
			ret = OpenSOAPByteArrayCreate(&xmlElmBAry);
			if (OPENSOAP_SUCCEEDED(ret)) {
				/* get xml element serialize data */
				ret = OpenSOAPXMLElmGetCharEncodingString((OpenSOAPXMLElmPtr)soapEnv,
														  MIDDLE_CHARSET,
														  xmlElmBAry);
				if (OPENSOAP_SUCCEEDED(ret)) {
					/* add XML Proc line */
					ret = OpenSOAPByteArrayAddXMLProc(xmlElmBAry,
													  chEnc);
					if (OPENSOAP_SUCCEEDED(ret)) {
						/* convert char-encoding */
						ret = OpenSOAPStringConvertCharEncoding(MIDDLE_CHARSET,
																xmlElmBAry,
																chEnc,
																bAry);
					}
				}
				/* release temporary buffer */
				OpenSOAPByteArrayRelease(xmlElmBAry);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetHeaderCharEncodingString(soapEnv, ch_enc, b_ary)
    Soap Header character encoding output

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope.
      :const char  * [in] ((|ch_enc|))
        character encoding. i.e. "EUC-JP", "Shift_JIS", "UTF-8"
      :OpenSOAPByteArrayPtr [out] ((|b_ary|))
        Result Buffer        
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetHeaderCharEncodingString(OpenSOAPEnvelopePtr /* [in] */ soapEnv,
										    const char * /* [in] */ ch_enc,
										    OpenSOAPByteArrayPtr /* [out] */ b_ary) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !soapEnv->header || !b_ary ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
	}
	else{
		ret = OpenSOAPXMLElmGetCharEncodingString( soapEnv->header,
												   ch_enc,
												   b_ary );
	}

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetBodyCharEncodingString(soapEnv, ch_enc, b_ary)
    Soap Body character encoding output

    :Parameters
      :OpenSOAPEnvelopePtr [in] ((|soapEnv|))
        OpenSOAP Envelope.
      :const char  * [in] ((|ch_enc|))
        character encoding. i.e. "EUC-JP", "Shift_JIS", "UTF-8"
      :OpenSOAPByteArrayPtr [out] ((|b_ary|))
        Result Buffer        
    :Return Value
      :int
	    Return Code Is ErrorCode
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetBodyCharEncodingString(/* [in]  */ OpenSOAPEnvelopePtr soapEnv,
										  /* [in]  */ const char *ch_enc,
										  /* [out] */ OpenSOAPByteArrayPtr b_ary) {
    int ret = OPENSOAP_YET_IMPLEMENTATION;

	if( !soapEnv || !soapEnv->body || !b_ary ){
		ret = OPENSOAP_PARAMETER_BADVALUE;
	}
	else {
		ret = OpenSOAPXMLElmGetCharEncodingString(soapEnv->body,
												  ch_enc,
												  b_ary);
	}

    return ret;
}

/*
 */
static
int
OpenSOAPEnvelopeAddFaultBlock(/* [out] */ OpenSOAPEnvelopePtr soapEnv,
							  /* [out] */ OpenSOAPBlockPtr *faultBlock) {
	static const char FAULT_BLOCK_NAME[] = "Fault";
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (soapEnv && faultBlock) {
		ret = OpenSOAPEnvelopeAddBodyBlockMB(soapEnv,
											 FAULT_BLOCK_NAME,
											 faultBlock);
		if (OPENSOAP_SUCCEEDED(ret)) {
			/* */
			((OpenSOAPXMLNodePtr)*faultBlock)->thisNamespace
				=	((OpenSOAPXMLNodePtr)soapEnv)->thisNamespace;
		}
	}

	return ret;
}

/*
 */
static
int
OpenSOAPBlockSetFaultValuesString(/* [out] */ OpenSOAPBlockPtr faultBlock,
								  /* [in, out]  */ OpenSOAPStringPtr faultCode,
								  /* [in, out]  */
								  OpenSOAPStringPtr faultString,
								  /* [in]  */ int isValueDup) {
	static const wchar_t FAULT_CODE_NAME[] = L"faultcode";
	static const wchar_t FAULT_STRING_NAME[] = L"faultstring";
	int ret = OPENSOAP_PARAMETER_BADVALUE;
	if (faultBlock && faultCode && faultString) {
		OpenSOAPStringPtr faultCodeName = NULL;
		ret = OpenSOAPStringCreateWithWC(FAULT_CODE_NAME,
										 &faultCodeName);
		if (OPENSOAP_SUCCEEDED(ret)) {
			OpenSOAPStringPtr faultStringName = NULL;
			ret = OpenSOAPStringCreateWithWC(FAULT_STRING_NAME,
											 &faultStringName);
			if (OPENSOAP_SUCCEEDED(ret)) {
				OpenSOAPXMLElmPtr faultElm = (OpenSOAPXMLElmPtr)faultBlock;
				ret = OpenSOAPXMLElmSetChildValueAsString(faultElm,
														  faultCodeName,
														  isValueDup,
														  faultCode);
				if (OPENSOAP_SUCCEEDED(ret)) {
					ret = OpenSOAPXMLElmSetChildValueAsString(faultElm,
															  faultStringName,
															  isValueDup,
															  faultString);
					if (OPENSOAP_FAILED(ret)) {
						OpenSOAPXMLElmRemoveChildString(faultElm,
														faultStringName,
														isValueDup,
														&faultString);
					}
				}
				if (OPENSOAP_FAILED(ret)) {
					OpenSOAPXMLElmRemoveChildString(faultElm,
													faultCodeName,
													isValueDup,
													&faultString);
				}

				OpenSOAPStringRelease(faultStringName);
			}

			OpenSOAPStringRelease(faultCodeName);
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddFaultString(soapEnv, faultCode, faultString, isValueDup, faultBlock)
    OpenSOAP Envelope instance create(MB)

    :Parameters
      :[out] OpenSOAPEnvelopePtr ((|soapEnv|))
        OpenSOAP Envelope Pointer
      :[in]  OpenSOAPStringPtr  ((|faultCode|))
        SOAP Fault's faultcode.
      :[in]  OpenSOAPStringPtr  ((|faultString|))
        SOAP Fault's faultstring.
      :[in]  int  ((|isValueDup|))
        faultCode and faultString duplicate flag.
      :[out] OpenSOAPBlockPtr * ((|faultBlock|))
        Strage Buffer of OpenSOAP Fault Block Pointer
    :Return value
      :int 
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeAddFaultString(/* [out] */ OpenSOAPEnvelopePtr soapEnv,
							   /* [in]  */ OpenSOAPStringPtr faultCode,
							   /* [in]  */ OpenSOAPStringPtr faultString,
							   /* [in]  */ int isValueDup,
							   /* [out] */ OpenSOAPBlockPtr *faultBlock) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (faultCode && faultString) {
		ret = OpenSOAPEnvelopeAddFaultBlock(soapEnv, faultBlock);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPBlockSetFaultValuesString(*faultBlock,
													faultCode,
													faultString,
													isValueDup);
		}
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddFaultMB(soapEnv, faultCode, faultString, faultBlock)
    OpenSOAP Envelope instance create(MB)

    :Parameters
      :[out] OpenSOAPEnvelopePtr ((|soapEnv|))
        OpenSOAP Envelope Pointer
      :[in]  const char * ((|faultCode|))
        SOAP Fault's faultcode.
      :[in]  const char * ((|faultString|))
        SOAP Fault's faultstring.
      :[out] OpenSOAPBlockPtr * ((|faultBlock|))
        Strage Buffer of OpenSOAP Fault Block Pointer
    :Return value
      :int 
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeAddFaultMB(/* [out] */ OpenSOAPEnvelopePtr soapEnv,
						   /* [in]  */ const char *faultCode,
						   /* [in]  */ const char *faultString,
						   /* [out] */ OpenSOAPBlockPtr *faultBlock) {
	OpenSOAPStringPtr faultCodeStr = NULL;
	OpenSOAPStringPtr faultStringStr = NULL;
	int ret = OpenSOAPStringCreateWithMB(faultCode, &faultCodeStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPStringCreateWithMB(faultString, &faultStringStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPEnvelopeAddFaultString(soapEnv,
												 faultCodeStr,
												 faultStringStr,
												 0,
												 faultBlock);
		}
		else {
			OpenSOAPStringRelease(faultCodeStr);
		}
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeAddFaultWC(soapEnv, faultCode, faultString, faultBlock)
    OpenSOAP Envelope instance create(WC)

    :Parameters
      :[out] OpenSOAPEnvelopePtr ((|soapEnv|))
        OpenSOAP Envelope Pointer
      :[in]  const wchar_t * ((|faultCode|))
        SOAP Fault's faultcode.
      :[in]  const wchar_t * ((|faultString|))
        SOAP Fault's faultstring.
      :[out] OpenSOAPBlockPtr * ((|faultBlock|))
        Strage Buffer of OpenSOAP Fault Block Pointer
    :Return value
      :int 
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeAddFaultWC(/* [out] */ OpenSOAPEnvelopePtr soapEnv,
						   /* [in]  */ const wchar_t *faultCode,
						   /* [in]  */ const wchar_t *faultString,
						   /* [out] */ OpenSOAPBlockPtr *faultBlock) {
	OpenSOAPStringPtr faultCodeStr = NULL;
	OpenSOAPStringPtr faultStringStr = NULL;
	int ret = OpenSOAPStringCreateWithWC(faultCode, &faultCodeStr);

	if (OPENSOAP_SUCCEEDED(ret)) {
		ret = OpenSOAPStringCreateWithWC(faultString, &faultStringStr);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OpenSOAPEnvelopeAddFaultString(soapEnv,
												 faultCodeStr,
												 faultStringStr,
												 0,
												 faultBlock);
		}
		else {
			OpenSOAPStringRelease(faultCodeStr);
		}
	}
	
	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeCreateMB(soapVer, envPrefix, faultCode, faultString, faultBlock, soapEnv)
    OpenSOAP Envelope instance create(MB)

    :Parameters
      :[in]  const cahr * ((||soapVer|))
        SOAP Virsion
      :[in]  const char * ((|envPrefix|))
        SOAP Envelope namespace prefix.
      :[in]  const char * ((|faultCode|))
        SOAP Fault's faultcode.
      :[in]  const char * ((|faultString|))
        SOAP Fault's faultstring.
      :[out] OpenSOAPBlockPtr * ((|faultBlock|))
        Strage Buffer of OpenSOAP Fault Block Pointer
      :[out] OpenSOAPEnvelopePtr * ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :int 
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeCreateFaultMB(/* [in]  */ const char *soapVer,
							  /* [in]  */ const char *envPrefix,
							  /* [in]  */ const char *faultCode,
							  /* [in]  */ const char *faultString,
							  /* [out] */ OpenSOAPBlockPtr *faultBlock,
							  /* [out] */ OpenSOAPEnvelopePtr *soapEnv) {
	int ret = OpenSOAPEnvelopeCreateMB(soapVer,
									   envPrefix,
									   soapEnv);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPBlockPtr dummyBlock = NULL;
		if (!faultBlock) {
			faultBlock = &dummyBlock;
		}
		ret = OpenSOAPEnvelopeAddFaultMB(*soapEnv,
										 faultCode,
										 faultString,
										 faultBlock);
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPEnvelopeRelease(*soapEnv);
			*soapEnv = NULL;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeCreateMB(soapVer, envPrefix, faultCode, faultString, faultBlock, soapEnv)
    OpenSOAP Envelope instance create(MB)

    :Parameters
      :[in]  const cahr * ((||soapVer|))
        SOAP Virsion
      :[in]  const char * ((|envPrefix|))
        SOAP Envelope namespace prefix.
      :[in]  const char * ((|faultCode|))
        SOAP Fault's faultcode.
      :[in]  const char * ((|faultString|))
        SOAP Fault's faultstring.
      :[out] OpenSOAPBlockPtr * ((|faultBlock|))
        Strage Buffer of OpenSOAP Fault Block Pointer
      :[out] OpenSOAPEnvelopePtr * ((|soapEnv|))
        Strage Buffer of OpenSOAP Envelope Pointer
    :Return value
      :int 
	    error code.
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeCreateFaultWC(/* [in]  */ const wchar_t *soapVer,
							  /* [in]  */ const wchar_t *envPrefix,
							  /* [in]  */ const wchar_t *faultCode,
							  /* [in]  */ const wchar_t *faultString,
							  /* [out] */ OpenSOAPBlockPtr *faultBlock,
							  /* [out] */ OpenSOAPEnvelopePtr *soapEnv) {
	int ret = OpenSOAPEnvelopeCreateWC(soapVer,
									   envPrefix,
									   soapEnv);
	if (OPENSOAP_SUCCEEDED(ret)) {
		OpenSOAPBlockPtr dummyBlock = NULL;
		if (!faultBlock) {
			faultBlock = &dummyBlock;
		}
		ret = OpenSOAPEnvelopeAddFaultWC(*soapEnv,
										 faultCode,
										 faultString,
										 faultBlock);
		
		if (OPENSOAP_FAILED(ret)) {
			OpenSOAPEnvelopeRelease(*soapEnv);
			*soapEnv = NULL;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetActorNameString(soapEnv, actorName)
    Get actor name.

    :Parameters
      :[in]  OpenSOAPEnvelopePtr ((|soapEnv|))
        OpenSOAP Envelope。
      :[out] OpenSOAPStringPtr * ((|actorName|))
	    Actor name.
    :Return value
      :int
	    Return Code Is ErrorCode
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetActorNameString(/* [in]  */ OpenSOAPEnvelopePtr soapEnv,
								   /* [out] */ OpenSOAPStringPtr *actorName) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (soapEnv && actorName) {
		SoapVersionEnvValuesMapConstIterator vals = NULL;
		ret = OpenSOAPEnvelopeGetEnvValues(soapEnv,
										   &vals);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = vals
				? OpenSOAPStringCreateWithMB(vals->soapActorName,
											 actorName)
				: OPENSOAP_PARAMETER_BADVALUE;
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeGetActorNextString(soapEnv, actorNext)
    Get actor next url value.

    :Parameters
      :[in]  OpenSOAPEnvelopePtr ((|soapEnv|))
        OpenSOAP Envelope。
      :[out] OpenSOAPStringPtr * ((|actorNext|))
	    Next actor value stored buffer. If *actorNext is NULL,
		then create OpenSOAPString.
    :Return value
      :int
	    Error code.
      
=end
*/
extern
int
OPENSOAP_API
OpenSOAPEnvelopeGetActorNextString(/* [in]  */ OpenSOAPEnvelopePtr soapEnv,
								   /* [out] */ OpenSOAPStringPtr *actorNext) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if (soapEnv && actorNext) {
		SoapVersionEnvValuesMapConstIterator vals = NULL;
		ret = OpenSOAPEnvelopeGetEnvValues(soapEnv,
										   &vals);
		if (OPENSOAP_SUCCEEDED(ret)) {
			ret = OPENSOAP_PARAMETER_BADVALUE;
			if (vals) {
				ret = *actorNext ?
					OpenSOAPStringSetStringMB(*actorNext,
											  vals->soapActorNext)
					: OpenSOAPStringCreateWithMB(vals->soapActorNext,
												 actorNext);
			}
		}
	}

	return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeDefineNamespaceMB(soapEnv, nsUri, nsPrefix)
    Set Namespace of XML Element(MB)

    :Parameters
      :OpenSOAPEnvelopePtr [in, out] ((|soapEnv|))
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
OpenSOAPEnvelopeDefineNamespaceMB(/* [in, out] */ OpenSOAPEnvelopePtr soapEnv,
                               /* [in]      */ const char *nsUri,
                               /* [in]      */ const char *nsPrefix,
                                  /* [out]     */ OpenSOAPXMLNamespacePtr *ns) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if ( soapEnv ){
        ret = OpenSOAPXMLElmDefineNamespaceMB(&soapEnv->super, nsUri, nsPrefix,
                                                ns);
    }

    return ret;
}

/*
=begin
--- function#OpenSOAPEnvelopeDefineNamespaceWC(soapEnv, nsUri, nsPrefix)
    Set Namespace of XML Element(WC)

    :Parameters
      :OpenSOAPEnvelopePtr [in, out] ((|soapEnv|))
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
OpenSOAPEnvelopeDefineNamespaceWC(/* [in, out] */ OpenSOAPEnvelopePtr soapEnv,
                                  /* [in]      */ const wchar_t *nsUri,
                                  /* [in]      */ const wchar_t *nsPrefix,
                                  /* [out]     */ OpenSOAPXMLNamespacePtr *ns) {
	int ret = OPENSOAP_PARAMETER_BADVALUE;

	if ( soapEnv ){
        ret = OpenSOAPXMLElmDefineNamespaceWC(&soapEnv->super, nsUri, nsPrefix,
                                                ns);
    }
}
