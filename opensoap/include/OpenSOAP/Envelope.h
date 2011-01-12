/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Envelope.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Envelope_H
#define OpenSOAP_Envelope_H

#include <OpenSOAP/Block.h>
#include <OpenSOAP/ByteArray.h>
#include <OpenSOAP/String.h>

#include <stdlib.h>

/**
 * @file OpenSOAP/Envelope.h
 * @brief OpenSOAP API Envelope Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * @typedef struct tagOpenSOAPEnvelope OpenSOAPEnvelope
     * @brief OpenSOAPEnvelope Structure Type Definition
     */
    typedef struct tagOpenSOAPEnvelope OpenSOAPEnvelope;

    /**
     * @typedef OpenSOAPEnvelope    *OpenSOAPEnvelopePtr
     * @brief OpenSOAPEnvelope Pointer Type Definition
     */
    typedef OpenSOAPEnvelope    *OpenSOAPEnvelopePtr;

    /**
      * @fn int OpenSOAPEnvelopeCreate(OpenSOAPEnvelopePtr *soap_env)
      * @brief OpenSOAP Envelope Instance Create
      * @param
      *    soap_env OpenSOAPEnvelopePtr * [out] ((|soap_env|)) Storage Buffer of OpenSOAP Envelope Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeCreate(/* [out] */ OpenSOAPEnvelopePtr *soap_env);

    /**
      * @fn int OpenSOAPEnvelopeCreateMB(const char *soapVer, const char *envPrefix, OpenSOAPEnvelopePtr *soapEnv)
      * @brief OpenSOAP Envelope Instance Create(MB)
      * @param
      *    soapVer const char * [in] ((||soapVer|)) SOAP Version
      * @param
      *    envPrefix const char * [in] ((|envPrefix|)) SOAP Envelope namespace prefix
      * @param
      *    soapEnv OpenSOAPEnvelopePtr * [out] ((|soapEnv|)) Storage Buffer of OpenSOAP Envelope Pointer
      * @return
      *    Error Code
      * @note
      *    soapVer This should be "1.1" or "1.2". If NULL, this defaults to "1.1"
      * @note
      *    envPrefix If NULL, this defaults as follows according to soapVer; "1.1": SOAP-ENV, "1.2": env
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeCreateMB(/* [in]  */ const char *soapVer,
                             /* [in]  */ const char *envPrefix,
                             /* [out] */ OpenSOAPEnvelopePtr *soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeCreateWC(const wchar_t *soapVer, const wchar_t *envPrefix, OpenSOAPEnvelopePtr *soapEnv)
      * @brief OpenSOAP Envelope instance create(WC)
      * @param
      *    soapVer const char * [in] ((||soapVer|)) SOAP Version
      * @param
      *    envPrefix const char * [in] ((|envPrefix|)) SOAP Envelope namespace prefix
      * @param
      *    soapEnv OpenSOAPEnvelopePtr * [out] ((|soapEnv|)) Storage Buffer of OpenSOAP Envelope Pointer
      * @return
      *    Error Code
      * @note
      *    soapVer This should be "1.1" or "1.2". If NULL, this defaults to "1.1"
      *    envPrefix If NULL, this defaults as follows according to soapVer; "1.1": SOAP-ENV, "1.2": env
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeCreateWC(/* [in]  */ const wchar_t *soapVer,
                             /* [in]  */ const wchar_t *envPrefix,
                             /* [out] */ OpenSOAPEnvelopePtr *soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeCreateString(OpenSOAPStringPtr soapVer, OpenSOAPStringPtr envPrefix, OpenSOAPEnvelopePtr *soapEnv)
      * @brief OpenSOAP Envelope instance create(OpenSOAPString)
      * @param
      *    soapVer OpenSOAPStringPtr [in] ((||soapVer|)) SOAP Version
      * @param
      *    envPrefix OpenSOAPStringPtr [in] ((|envPrefix|)) SOAP Envelope namespace prefix
      * @param
      *    soapEnv OpenSOAPEnvelopePtr * [out] ((|soapEnv|)) Storage Buffer of OpenSOAP Envelope Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeCreateString(/* [in]  */ OpenSOAPStringPtr soapVer,
                                 /* [in]  */ OpenSOAPStringPtr envPrefix,
                                 /* [out] */ OpenSOAPEnvelopePtr *soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeCreateFaultMB(const char *soapVer, const char *envPrefix, const char *faultCode, const char *faultString, OpenSOAPBlockPtr *faultBlock, OpenSOAPEnvelopePtr *soapEnv)
      * @brief Create OpenSOAP Fault Block(MB)
      * @param
      *    soapVer const char * [in] ((||soapVer|)) SOAP Version
      * @param
      *    envPrefix  const char * [in] ((|envPrefix|)) SOAP Envelope namespace prefix
      * @param
      *    faultCode const char * [in] ((|faultCode|)) SOAP Fault's faultcode
      * @param
      *    faultString const char * [in] ((|faultString|)) SOAP Fault's faultstring
      * @param
      *    faultBlock OpenSOAPBlockPtr * [out] ((|faultBlock|)) Storage Buffer of OpenSOAP Fault Block Pointer
      * @param
      *    soapEnv OpenSOAPEnvelopePtr * [out] ((|soapEnv|)) Storage Buffer of OpenSOAP Envelope Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeCreateFaultMB(/* [in]  */ const char *soapVer,
                                  /* [in]  */ const char *envPrefix,
                                  /* [in]  */ const char *faultCode,
                                  /* [in]  */ const char *faultString,
                                  /* [out] */ OpenSOAPBlockPtr *faultBlock,
                                  /* [out] */ OpenSOAPEnvelopePtr *soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeCreateFaultWC(const wchar_t *soapVer, const wchar_t *envPrefix, const wchar_t *faultCode, const wchar_t *faultString, OpenSOAPBlockPtr *faultBlock, OpenSOAPEnvelopePtr *soapEnv)
      * @brief Create OpenSOAP Fault Block(WC)
      * @param
      *    soapVer const wchar_t * [in] ((||soapVer|)) SOAP Version
      * @param
      *    envPrefix const wchar_t * [in] ((|envPrefix|)) SOAP Envelope namespace prefix
      * @param
      *    faultCode const wchar_t * [in] ((|faultCode|)) SOAP Fault's faultcode
      * @param
      *    faultString const wchar_t * [in] ((|faultString|)) SOAP Fault's faultstring
      * @param
      *    faultBlock OpenSOAPBlockPtr * [out] ((|faultBlock|)) Storage Buffer of OpenSOAP Fault Block Pointer
      * @param
      *    soapEnv OpenSOAPEnvelopePtr * [out] ((|soapEnv|)) Storage Buffer of OpenSOAP Envelope Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeCreateFaultWC(/* [in]  */ const wchar_t *soapVer,
                                  /* [in]  */ const wchar_t *envPrefix,
                                  /* [in]  */ const wchar_t *faultCode,
                                  /* [in]  */ const wchar_t *faultString,
                                  /* [out] */ OpenSOAPBlockPtr *faultBlock,
                                  /* [out] */ OpenSOAPEnvelopePtr *soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeCreateCharEncoding(const char *chEnc, OpenSOAPByteArrayPtr b_ary, OpenSOAPEnvelopePtr *soapEnv)
      * @brief Create Char Encoding
      * @param
      *    chEnc const char * [in] ((|chEnc|)) character encoding
      * @param
      *    b_ary OpenSOAPByteArrayPtr [in] ((|b_ary|)) OpenSOAP ByteArray
      * @param
      *    soapEnv OpenSOAPEnvelopePtr * [out] ((|soapEnv|)) Storage Buffer of OpenSOAP Envelope Pointer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeCreateCharEncoding(/* [in]  */ const char *chEnc,
                                       /* [in]  */ OpenSOAPByteArrayPtr b_ary,
                                       /* [out] */ OpenSOAPEnvelopePtr *soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeRetain(OpenSOAPEnvelopePtr soapEnv)
      * @brief *NOT IMPLEMENTED* Add Reference To OpenSOAP Envelope
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [in] ((|soapEnv|)) OpenSOAP Envelope
      * @return
      *    Error Code (OPENSOAP_YET_IMPLEMENTATION)
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeRetain(/* [in, out] */ OpenSOAPEnvelopePtr soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeRelease(OpenSOAPEnvelopePtr soapEnv)
      * @brief Release OpenSOAP Envelope Buffer
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [in] ((|soapEnv|)) OpenSOAP Envelope
      * @return
      *    Error Code
      * @note
      *    Release Resources of Created OpenSOAP Resource
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeRelease(/* [in, out] */ OpenSOAPEnvelopePtr soapEnv);

    /**
      * @fn int OpenSOAPEnvelopeAddHeaderBlockMB(OpenSOAPEnvelopePtr soapEnv, const char *block_name, OpenSOAPBlockPtr *h_block)
      * @brief Add SOAP Header Block(MB)
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [out] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const char * [in] ((|block_name|)) SOAP Block Name
      * @param
      *    h_block OpenSOAPBlockPtr * [out] ((|h_block|)) OpenSOAP Header Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeAddHeaderBlockMB(/* [out]  */ OpenSOAPEnvelopePtr soapEnv,
                                     /* [in]  */ const char *block_name,
                                     /* [out] */ OpenSOAPBlockPtr *h_block);

    /**
      * @fn int OpenSOAPEnvelopeAddHeaderBlockWC(OpenSOAPEnvelopePtr soapEnv, const wchar_t *block_name, OpenSOAPBlockPtr *h_block)
      * @brief Add SOAP Header Block(WC)
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [out] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const wchar_t * [in] ((|block_name|)) SOAP Block Name
      * @param
      *    h_block OpenSOAPBlockPtr * [out] ((|h_block|)) OpenSOAP Header Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeAddHeaderBlockWC(/* [out]  */ OpenSOAPEnvelopePtr soapEnv,
                                     /* [in]  */ const wchar_t *block_name,
                                     /* [out] */ OpenSOAPBlockPtr *h_block);

    /**
      * @fn int OpenSOAPEnvelopeGetNextHeaderBlock(OpenSOAPEnvelopePtr soap_env, OpenSOAPBlockPtr *h_block)
      * @brief Get SOAP Header Block
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    h_block OpenSOAPBlockPtr * [in, out] ((|h_block|)) OpenSOAP Header Block. If NULL, return first header block.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetNextHeaderBlock(/* [in] */ OpenSOAPEnvelopePtr soap_env,
                                       /* [in, out] */ OpenSOAPBlockPtr *h_block);

    /**
      * @fn int OpenSOAPEnvelopeGetHeaderBlockMB(OpenSOAPEnvelopePtr soap_env, const char *block_name, OpenSOAPBlockPtr *h_block)
      * @brief Get SOAP Header Block(MB)
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const char * [in] ((|block_name|)) SOAP Header Block name
      * @param
      *    h_block OpenSOAPBlockPtr * [out] ((|h_block|)) Header Block return buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetHeaderBlockMB(/* [in]  */ OpenSOAPEnvelopePtr soap_env,
                                     /* [in]  */ const char *block_name,
                                     /* [out] */ OpenSOAPBlockPtr *h_block);

    /**
      * @fn int OpenSOAPEnvelopeGetHeaderBlockWC(OpenSOAPEnvelopePtr soap_env, const wchar_t *block_name, OpenSOAPBlockPtr *h_block)
      * @brief Get SOAP Header Block(WC)
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const wchar_t * [in] ((|block_name|)) SOAP Header Block name
      * @param
      *    h_block OpenSOAPBlockPtr * [out] ((|h_block|)) Header Block return buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetHeaderBlockWC(/* [in]  */ OpenSOAPEnvelopePtr soap_env,
                                     /* [in]  */ const wchar_t *block_name,
                                     /* [out] */ OpenSOAPBlockPtr *h_block);

    /**
      * @fn int OpenSOAPEnvelopeAddBodyBlockMB(OpenSOAPEnvelopePtr soap_env, const char *block_name, OpenSOAPBlockPtr *b_block)
      * @brief Add SOAP Body Block(MB)
      * @param
      *    soap_env OpenSOAPEnvelopePtr [out] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const char * [in] ((|block_name|)) SOAP Block Name. If block_name==NULL, add body element only (no body block).
      * @param
      *    b_block OpenSOAPBlockPtr * [out] ((|b_block|)) OpenSOAP Body Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeAddBodyBlockMB(/* [out]  */ OpenSOAPEnvelopePtr soap_env,
                                   /* [in]  */ const char *block_name,
                                   /* [out] */ OpenSOAPBlockPtr *b_block);

    /**
      * @fn int OpenSOAPEnvelopeAddBodyBlockWC(OpenSOAPEnvelopePtr soap_env, const wchar_t *block_name, OpenSOAPBlockPtr *b_block)
      * @brief Add SOAP Body Block(WC)
      * @param
      *    soap_env OpenSOAPEnvelopePtr [out] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const wchar_t * [in] ((|block_name|)) SOAP Block Name
      * @param
      *    b_block OpenSOAPBlockPtr * [out] ((|b_block|)) OpenSOAP Body Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeAddBodyBlockWC(/* [out]  */ OpenSOAPEnvelopePtr soap_env,
                                   /* [in]  */ const wchar_t *block_name,
                                   /* [out] */ OpenSOAPBlockPtr *b_block);

    /**
      * @fn int OpenSOAPEnvelopeGetNextBodyBlock(OpenSOAPEnvelopePtr soap_env, OpenSOAPBlockPtr *b_block)
      * @brief Get SOAP Body Block
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    b_block OpenSOAPBlockPtr * [in, out] ((|b_block|)) OpenSOAP Body Block. If *b_block is NULL, then return first Body Block.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetNextBodyBlock(/* [in]  */ OpenSOAPEnvelopePtr soap_env,
                                     /* [in, out] */ OpenSOAPBlockPtr *b_block);

    /**
      * @fn int OpenSOAPEnvelopeGetBodyBlockMB(OpenSOAPEnvelopePtr soap_env, const char *block_name, OpenSOAPBlockPtr *b_block)
      * @brief Get SOAP Body Block(MB)
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const char * [in] ((|block_name|)) SOAP Body Block name
      * @param
      *    b_block OpenSOAPBlockPtr * [out] ((|b_block|)) Body Block return buffer
      * @return
      *    Error Code
      * @note
      *    Search for 'block_name' and return the corresponding Body Block in 'b_block'.
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetBodyBlockMB(/* [in]  */ OpenSOAPEnvelopePtr soap_env,
                                   /* [in]  */ const char *block_name,
                                   /* [out] */ OpenSOAPBlockPtr *b_block);

    /**
      * @fn int OpenSOAPEnvelopeGetBodyBlockWC(OpenSOAPEnvelopePtr soap_env, const wchar_t *block_name, OpenSOAPBlockPtr *b_block)
      * @brief Get SOAP Body Block(WC)
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    block_name const wchar_t * [in] ((|block_name|)) SOAP Body Block name
      * @param
      *    b_block OpenSOAPBlockPtr * [out] ((|b_block|)) Body Block return buffer
      * @return
      *    Error Code
      * @note
      *    Search for 'block_name' and return the corresponding Body Block in 'b_block'.
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetBodyBlockWC(/* [in]  */ OpenSOAPEnvelopePtr soap_env,
                                   /* [in]  */ const wchar_t *block_name,
                                   /* [out] */ OpenSOAPBlockPtr *b_block);

    /**
      * @fn int OpenSOAPEnvelopeGetCharEncodingString(OpenSOAPEnvelopePtr soapEnv, const char *chEnc, OpenSOAPByteArrayPtr b_ary)
      * @brief SOAP Envelope Character Encoding Output
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [in] ((|soapEnv|)) OpenSOAP Envelope
      * @param
      *    chEnc const char  * [in] ((|chEnc|)) character encoding. (i.e. "EUC-JP", "Shift_JIS", "UTF-8")
      * @param
      *    b_ary OpenSOAPByteArrayPtr [out] ((|bAry|)) Storage Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetCharEncodingString(/* [in]  */ OpenSOAPEnvelopePtr soapEnv,
                                          /* [in]  */ const char *chEnc,
                                          /* [out] */ OpenSOAPByteArrayPtr b_ary);
    
    /**
      * @fn int OpenSOAPEnvelopeGetHeaderCharEncodingString(OpenSOAPEnvelopePtr soap_env, const char *ch_enc, OpenSOAPByteArrayPtr b_ary)
      * @brief Soap Header character encoding output
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    ch_enc const char  * [in] ((|ch_enc|)) character encoding. (i.e. "EUC-JP", "Shift_JIS", "UTF-8")
      * @param
      *    b_ary OpenSOAPByteArrayPtr [out] ((|b_ary|)) Result Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetHeaderCharEncodingString(/* [in]  */ OpenSOAPEnvelopePtr soap_env,
                                                /* [in]  */ const char *ch_enc,
                                                /* [out] */ OpenSOAPByteArrayPtr  b_ary);

    /**
      * @fn int OpenSOAPEnvelopeGetBodyCharEncodingString(OpenSOAPEnvelopePtr soap_env, const char *ch_enc, OpenSOAPByteArrayPtr b_ary)
      * @brief Soap Body Character Encoding Output
      * @param
      *    soap_env OpenSOAPEnvelopePtr [in] ((|soap_env|)) OpenSOAP Envelope
      * @param
      *    ch_enc const char  * [in] ((|ch_enc|)) character encoding. (i.e. "EUC-JP", "Shift_JIS", "UTF-8")
      * @param
      *    b_ary OpenSOAPByteArrayPtr [out] ((|b_ary|)) Result Buffer
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetBodyCharEncodingString(/* [in]  */ OpenSOAPEnvelopePtr soap_env,
                                              /* [in]  */ const char *ch_enc,
                                              /* [out] */ OpenSOAPByteArrayPtr b_ary);

    /**
      * @fn int OpenSOAPEnvelopeAddFaultString(OpenSOAPEnvelopePtr soapEnv, OpenSOAPStringPtr faultCode, OpenSOAPStringPtr faultString, int isValueDup, OpenSOAPBlockPtr *faultBlock)
      * @brief Add Fault String
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [out] ((|soapEnv|)) OpenSOAP Envelope Pointer
      * @param
      *    faultCode OpenSOAPStringPtr [in] ((|faultCode|)) SOAP Fault's faultcode
      * @param
      *    faultString OpenSOAPStringPtr [in] ((|faultString|)) SOAP Fault's faultstring
      * @param
      *    isValueDup int [in] ((|isValueDup|)) faultCode and faultString duplicate flag
      * @param
      *    faultBlock OpenSOAPBlockPtr * [out] ((|faultBlock|)) Storage Buffer of OpenSOAP Fault Block
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeAddFaultString(/* [out] */ OpenSOAPEnvelopePtr soapEnv,
                                   /* [in]  */ OpenSOAPStringPtr faultCode,
                                   /* [in]  */ OpenSOAPStringPtr faultString,
                                   /* [in]  */ int isValueDup,
                                   /* [out] */ OpenSOAPBlockPtr *faultBlock);

    /**
      * @fn int OpenSOAPEnvelopeGetActorNameString(OpenSOAPEnvelopePtr soapEnv, OpenSOAPStringPtr *actorName)
      * @brief Get Actor Name String
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [in] ((|soapEnv|)) OpenSOAP Envelope Pointer
      * @param
      *    actorName OpenSOAPStringPtr * [out] ((|actorName|)) Actor Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetActorNameString(/* [in]  */ OpenSOAPEnvelopePtr soapEnv,
                                       /* [out] */ OpenSOAPStringPtr *actorName);

    /**
      * @fn int OpenSOAPEnvelopeGetActorNextString(OpenSOAPEnvelopePtr soapEnv, OpenSOAPStringPtr *actorNext)
      * @brief Get Actor Next String
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [in] ((|soapEnv|)) OpenSOAP Envelope Pointer
      * @param
      *    actorNext OpenSOAPStringPtr * [out] ((|actorNext|)) Actor Next
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeGetActorNextString(/* [in]  */ OpenSOAPEnvelopePtr soapEnv,
                                       /* [out] */ OpenSOAPStringPtr *actorNext);

    /**
      * @fn int OpenSOAPEnvelopeDefineNamespaceMB(OpenSOAPEnvelopePtr elm, const char *ns_uri, const char *ns_prefix)
      * @brief Set Namespace of XML Element(MB)
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [in] ((|soapEnv|)) OpenSOAP Envelope Pointer
      * @param
      *    ns_uri const char * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const char * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeDefineNamespaceMB(/* [in] */ OpenSOAPEnvelopePtr soapEnv,
                                   /* [in]  */ const char *ns_uri,
                                   /* [in]  */ const char *ns_prefix,
                                   /* [out] */ OpenSOAPXMLNamespacePtr *ns);

    /**
      * @fn int OpenSOAPEnvelopeDefineNamespaceWC(OpenSOAPEnvelopePtr elm, const wchar_t *ns_uri, const wchar_t *ns_prefix)
      * @brief Set Namespace of XML Element(WC)
      * @param
      *    soapEnv OpenSOAPEnvelopePtr [in] ((|soapEnv|)) OpenSOAP Envelope Pointer
      * @param
      *    ns_uri const wchar_t * [in] ((|ns_uri|)) Namespace URI
      * @param
      *    ns_prefix const wchar_t * [in] ((|ns_prefix|)) Namespace Prefix
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPEnvelopeDefineNamespaceWC(/* [in] */ OpenSOAPEnvelopePtr soapEnv,
                                   /* [in]  */ const wchar_t *ns_uri,
                                   /* [in]  */ const wchar_t *ns_prefix,
                                   /* [out] */ OpenSOAPXMLNamespacePtr *ns);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Envelope_H */
