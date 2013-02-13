/*-----------------------------------------------------------------------------
 * $RCSfile: sec_signEnvelope.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/Security.h>
#include <memory.h>
#include "security_defs.h"

/* Signature Block's Name */
static const char* s_szSigBlkName =
"Signature";
/* Child's Name */
static const char* s_szChldSignature =
"Signature";
static const char* s_szChldSignedBy =
"SignedBy";
static const char* s_szChldSignedInfo = 
"SignedInfo";
static const char* s_szChldCanon =
"CanonicalizationMethod";
static const char* s_szChldSigMethod = 
"SignatureMethod";
static const char* s_szChldReference =
"Reference";
static const char* s_szChldTransforms =
"Transforms";
static const char* s_szChldTransform =
"Transform";
static const char* s_szChldDigestMethod =
"DigestMethod";
static const char* s_szChldDigestValue = 
"DigestValue";
static const char* s_szChldSignatureValue = 
"SignatureValue";

/* Block's Namespace */
static const char* s_szNSBlk =
"http://schemas.xmlsoap.org/soap/security/2000-12";
/* Child "Signature"'s Namespace */
static const char* s_szNSChldSignature =
"http://www.w3.org/2000/09/xmldsig#";

/* Actor Attribute */
static const char* s_szActorAttr = 
"some-URI";

/* Attribute "Algorithm" */
static const char* s_szAttrAlgorithm = 
"Algorithm";
/* Attribute "Reference" */
static const char* s_szAttrReference =
"URI";

/* Child "CanonicalizationMethod"'s Attribute Value */
static const char* s_szCanonAttrVal =
"http://www.w3.org/TR/2000/CR-xml-c14n-20001026";

/* "SignatureMethod" Attribute Values */
static const char* s_szSigMethodAttrValRsaSha1 = 
"http://www.w3.org/2000/09/xmldsig#rsa-sha1"; /* SHA1 */
static const char* s_szSigMethodAttrValRsaMd5 = 
"http://www.w3.org/2000/09/xmldsig#rsa-md5";  /* MD5 */
static const char* s_szSigMethodAttrValRsaRipemd = 
"http://www.w3.org/2000/09/xmldsig#rsa-ripemd";  /* RIPEMD160 */

/* "DigestMethod" Attribute Values */
static const char* s_szDigestMethodAttrValRsaSha1 = 
"http://www.w3.org/2000/09/xmldsig#sha1"; /* SHA1 */
static const char* s_szDigestMethodAttrValRsaMd5 = 
"http://www.w3.org/2000/09/xmldsig#md5";  /* MD5 */
static const char* s_szDigestMethodAttrValRsaRipemd = 
"http://www.w3.org/2000/09/xmldsig#ripemd";  /* RIPEMD160 */

/* Child "Reference" Attribute Value */
static const char* s_szReferenceAttrVal =
"#Body";

/* Child "Transform" Attribute Value */
static const char* s_szTransformAttrVal = 
"http://www.w3.org/TR/2000/CR-xml-c14n-20001026";

/* Data Type */
static const char* s_szTypeString = "string";

/* Encoding Style */
static const char* s_szEncStyle = "UTF-8";

/***************************************************************************** 
    Function      : Add Signature Part
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
static int addSignaturePart
(OpenSOAPEnvelopePtr env,       /* (io) Envelope */
 int                 iType,     /* (i)  Hash Type(OPENSOAP_HA_*) */
 char*               szSign,    /* (i)  Sign Value(base64 string) */
 char*               szDigest,  /* (i)  Digest Value(base64 string) */
 OpenSOAPStringPtr   name)      /* (i)  Name of Sign */
{
    OpenSOAPBlockPtr sig_block = NULL;
    OpenSOAPXMLElmPtr elmSignature = NULL;
    OpenSOAPXMLElmPtr elmSignedBy = NULL;
    OpenSOAPXMLElmPtr elmSignedInfo = NULL;
    OpenSOAPXMLElmPtr elmCanonicalizationMethod = NULL;
    OpenSOAPXMLElmPtr elmSignatureMethod = NULL;
    OpenSOAPXMLElmPtr elmReference = NULL;
    OpenSOAPXMLElmPtr elmTransforms = NULL;
    OpenSOAPXMLElmPtr elmTransform = NULL;
    OpenSOAPXMLElmPtr elmDigestMethod = NULL;
    OpenSOAPXMLElmPtr elmDigestValue = NULL;
    OpenSOAPXMLElmPtr elmSignatureValue = NULL;
    OpenSOAPStringPtr strWk = NULL;
    OpenSOAPXMLAttrPtr  attrWk = NULL;
    const char* szWk;
    int nRet;
    /* Add Signature Block */
    nRet = OpenSOAPEnvelopeAddHeaderBlockMB(env,
                                            s_szSigBlkName,
                                            &sig_block);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set Namespace to Block */
    nRet = OpenSOAPBlockSetNamespaceMB(sig_block,
                                       s_szNSBlk,
                                       "SOAP-SEC");
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set Actor Attribute */
    nRet = OpenSOAPBlockSetActorAttrMB(sig_block,
                                       s_szActorAttr);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* mustundastand Attribute */
    nRet = OpenSOAPBlockSetMustunderstandAttr(sig_block);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Child "Signature" */
    nRet = OpenSOAPBlockAddChildMB(sig_block,
                                   s_szChldSignature,
                                   &elmSignature);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Namespace to "Signature" */
    nRet = OpenSOAPXMLElmSetNamespaceMB(elmSignature,
                                        s_szNSChldSignature,
                                        "ds");
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    if (name != NULL) {
        /* Add "SignedBy" to "Signature" */
        nRet = OpenSOAPXMLElmAddChildMB(elmSignature,
                                        s_szChldSignedBy,
                                        &elmSignedBy);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        /* Add name to "SignedBy" */
        nRet = OpenSOAPXMLElmSetValueMB(elmSignedBy,
                                        s_szTypeString,
                                        &name);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
    }
    /* Add "SignedInfo" to "Signature" */
    nRet = OpenSOAPXMLElmAddChildMB(elmSignature,
                                    s_szChldSignedInfo,
                                    &elmSignedInfo);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "CanonicalizationMethod" to "SignedInfo" */
    nRet = OpenSOAPXMLElmAddChildMB(elmSignedInfo,
                                    s_szChldCanon,
                                    &elmCanonicalizationMethod);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Attribute to "CanonicalizationMethod" */
    nRet = OpenSOAPStringCreateWithMB(s_szCanonAttrVal,
                                      &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPXMLElmAddAttributeMB(elmCanonicalizationMethod,
                                        s_szAttrAlgorithm,
                                        s_szTypeString,
                                        &strWk,
                                        &attrWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringRelease(strWk);
    strWk = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "SignatureMethod" to "SignedInfo" */
    nRet = OpenSOAPXMLElmAddChildMB(elmSignedInfo,
                                    s_szChldSigMethod,
                                    &elmSignatureMethod);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Attribute To "SignatureMethod" */
    if (iType == OPENSOAP_HA_MD5) {
        szWk = s_szSigMethodAttrValRsaMd5;
    } else if (iType == OPENSOAP_HA_RIPEMD) {
        szWk = s_szSigMethodAttrValRsaRipemd;
    } else { /* SHA */
        szWk = s_szSigMethodAttrValRsaSha1;
    }
    nRet = OpenSOAPStringCreateWithMB(szWk,
                                      &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPXMLElmAddAttributeMB(elmSignatureMethod,
                                        s_szAttrAlgorithm,
                                        s_szTypeString,
                                        &strWk,
                                        &attrWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringRelease(strWk);
    strWk = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "Reference" To "SignedInfo" */
    nRet = OpenSOAPXMLElmAddChildMB(elmSignedInfo,
                                    s_szChldReference,
                                    &elmReference);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Attribute To "Reference" */
    nRet = OpenSOAPStringCreateWithMB(s_szReferenceAttrVal,
                                      &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPXMLElmAddAttributeMB(elmReference,
                                        s_szAttrReference,
                                        s_szTypeString,
                                        &strWk,
                                        &attrWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringRelease(strWk);
    strWk = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "Transforms" To "Reference" */
    nRet = OpenSOAPXMLElmAddChildMB(elmReference,
                                    s_szChldTransforms,
                                    &elmTransforms);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "Transform" To "Transforms" */
    nRet = OpenSOAPXMLElmAddChildMB(elmTransforms,
                                    s_szChldTransform,
                                    &elmTransform);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Attribute To "Transform" */
    nRet = OpenSOAPStringCreateWithMB(s_szTransformAttrVal,
                                      &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPXMLElmAddAttributeMB(elmTransform,
                                        s_szAttrAlgorithm,
                                        s_szTypeString,
                                        &strWk,
                                        &attrWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringRelease(strWk);
    strWk = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "DigestMethod" To "Reference" */
    nRet = OpenSOAPXMLElmAddChildMB(elmReference,
                                    s_szChldDigestMethod,
                                    &elmDigestMethod);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Attribute To "DigestMethod" */
    if (iType == OPENSOAP_HA_MD5) {
        szWk = s_szDigestMethodAttrValRsaMd5;
    } else if (iType == OPENSOAP_HA_RIPEMD) {
        szWk = s_szDigestMethodAttrValRsaRipemd;
    } else { /* SHA */
        szWk = s_szDigestMethodAttrValRsaSha1;
    }
    nRet = OpenSOAPStringCreateWithMB(szWk,
                                      &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPXMLElmAddAttributeMB(elmDigestMethod,
                                        s_szAttrAlgorithm,
                                        s_szTypeString,
                                        &strWk,
                                        &attrWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringRelease(strWk);
    strWk = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "DigestValue" To "Reference" */
    nRet = OpenSOAPXMLElmAddChildMB(elmReference,
                                    s_szChldDigestValue,
                                    &elmDigestValue);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set Digest Value */
    nRet = OpenSOAPStringCreateWithMB(szDigest,
                                      &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPXMLElmSetValueMB(elmDigestValue,
                                    s_szTypeString,
                                    &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringRelease(strWk);
    strWk = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add "SignatureValue" To "Signature" */
    nRet = OpenSOAPXMLElmAddChildMB(elmSignature,
                                    s_szChldSignatureValue,
                                    &elmSignatureValue);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set Signature */
    nRet = OpenSOAPStringCreateWithMB(szSign,
                                      &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPXMLElmSetValueMB(elmSignatureValue,
                                    s_szTypeString,
                                    &strWk);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringRelease(strWk);
    strWk = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (strWk) {
        OpenSOAPStringRelease(strWk);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Get Hash Type from "SignatureMethod"
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
static int getHashType
(OpenSOAPStringPtr strData,     /* (i)  OpenSOAP String */
 int*              piType)      /* (o)  Hash Type */
{
    int nRet;
    int iWk;
    *piType = OPENSOAP_HA_SHA;
    nRet = OpenSOAPStringCompareMB(strData,
                                   s_szSigMethodAttrValRsaMd5,
                                   &iWk);
    if (OPENSOAP_FAILED(nRet)) {
        return nRet;
    }
    if (iWk == 0) {
        *piType = OPENSOAP_HA_MD5;
        return nRet;
    }
    nRet = OpenSOAPStringCompareMB(strData,
                                   s_szSigMethodAttrValRsaRipemd,
                                   &iWk);
    if (OPENSOAP_FAILED(nRet)) {
        return nRet;
    }
    if (iWk == 0) {
        *piType = OPENSOAP_HA_RIPEMD;
        return nRet;
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Get Signature String
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
static int getSignatureString
(const OpenSOAPEnvelopePtr env,    /* (i)  Envelope */
 OpenSOAPBlockPtr*         pBlk,   /* (io) Current Header Block */
 int*                      piType, /* (o)  Hash Type */
 OpenSOAPStringPtr*        strSig) /* (io) Signature String */
{
    int nRet = OPENSOAP_NO_ERROR;
    OpenSOAPStringPtr strName = NULL;
    OpenSOAPStringPtr strNS = NULL;
    OpenSOAPStringPtr strVal = NULL;
    OpenSOAPXMLElmPtr elmSignature;
    OpenSOAPXMLElmPtr elmSignedInfo;
    OpenSOAPXMLElmPtr elmSignatureMethod;
    OpenSOAPXMLElmPtr elmSignatureValue;
    OpenSOAPXMLNamespacePtr nmspace;
    OpenSOAPXMLAttrPtr      attr;
    int               iWk;
    for (;;) {
        /* Get Next Header Block */
        nRet = OpenSOAPEnvelopeGetNextHeaderBlock(env, pBlk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (*pBlk == NULL)
            break;              /* No More Block */
        /* Check Block's Name */
        nRet = OpenSOAPBlockGetName(*pBlk, &strName);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        nRet = OpenSOAPStringCompareMB(strName, s_szSigBlkName, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0)
            continue;           /* Different Block Name */
        /* Check Block's Namespace */
        nRet = OpenSOAPBlockGetNamespace(*pBlk, &nmspace);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (nmspace == NULL) {
            continue;           /* Namespace not found */
        }
        /* Get Namespace String */
        nRet = OpenSOAPXMLNamespaceGetURI(nmspace, &strNS);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (strNS == NULL) {
            continue;           /* No Namespace */
        }
        /* Compare Block's Namespace String */
        nRet = OpenSOAPStringCompareMB(strNS, s_szNSBlk, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0) {
            continue;           /* Different Block's Namespace */
        }
        /* Find Child "Signature" */
        nRet = OpenSOAPBlockGetChildMB(*pBlk,
                                       s_szChldSignature,
                                       &elmSignature);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (elmSignature == NULL) {
            continue;           /* Specified Child Not Found */
        }
        /* Check Child's Namespace */
        nRet = OpenSOAPXMLElmGetNamespace(elmSignature, &nmspace);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (nmspace == NULL) {
            continue;           /* Namespace not found */
        }
        /* Get Namespace String */
        nRet = OpenSOAPXMLNamespaceGetURI(nmspace, &strNS);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (strNS == NULL) {
            continue;           /* No Namespace */
        }
        /* Compare Namespace String */
        nRet = OpenSOAPStringCompareMB(strNS, s_szNSChldSignature, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0) {
            continue;           /* Different Namespace */
        }
        /* Find Child "SignedInfo" from "Signature" */
        nRet = OpenSOAPXMLElmGetChildMB(elmSignature,
                                        s_szChldSignedInfo,
                                        &elmSignedInfo);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (elmSignedInfo == NULL) {
            continue;           /* SignedInfo Not Found */
        }
        /* Find "SignatureMethod" from "SignedInfo" */
        nRet = OpenSOAPXMLElmGetChildMB(elmSignedInfo,
                                        s_szChldSigMethod,
                                        &elmSignatureMethod);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (elmSignatureMethod == NULL) {
            continue;           /* SignatureMethod Not Found */
        }
        /* Get Signature Method */
        nRet = OpenSOAPXMLElmGetAttributeMB(elmSignatureMethod,
                                            s_szAttrAlgorithm,
                                            &attr);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (attr == NULL) {
            continue;       /* No Signature Method */
        }
        nRet = OpenSOAPXMLAttrGetValueMB(attr,
                                         s_szTypeString,
                                         &strVal);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        /* Get Hash Type */
        nRet = getHashType(strVal, piType);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        /* Find Child "SignatureValue" from "Signature" */
        nRet = OpenSOAPXMLElmGetChildMB(elmSignature,
                                        s_szChldSignatureValue,
                                        &elmSignatureValue);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (elmSignatureValue == NULL) {
            continue;           /* SignatureValue Not Found */
        }
        /* Get Signature Value */
        nRet = OpenSOAPXMLElmGetValueMB(elmSignatureValue,
                                        s_szTypeString,
                                        strSig);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        /* Normal End */
        break;
    }
FuncEnd:
    return nRet;
}
/***************************************************************************** 
    Function      : Add Signature to SOAP Message using PrivateKey File Stream
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
int
OPENSOAP_API
OpenSOAPSecAddSignWithStream
(OpenSOAPEnvelopePtr env,        /* (io) Envelope */
 int                 iType,      /* (i)  Hash Type(OPENSOAP_HA_*) */
 FILE*               fpPrivKey,  /* (i)  Private Key File stream */
 OpenSOAPStringPtr   name)       /* (i)  Name of Sign */
{
    int                  nRet = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPByteArrayPtr bodyData = NULL;
    size_t               sizBodyData;
    const unsigned char* szTop;
    char*                szSignData = NULL;
    char*                szDigest = NULL;
    /* Check Arguments */
    if (!env || !fpPrivKey) {
        return nRet;
    }
    /* Get Body As String */
    nRet = OpenSOAPByteArrayCreate(&bodyData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPEnvelopeGetBodyCharEncodingString(env,
                                                     s_szEncStyle,
                                                     bodyData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayBeginConst(bodyData,
                                       &szTop);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayGetSize(bodyData,
                                    &sizBodyData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Make Signature */
    nRet = openSOAPSecMakeRSASign(iType,
                                  szTop,
                                  (unsigned long)sizBodyData,
                                  fpPrivKey,
                                  &szSignData,
                                  &szDigest);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Add Signature */
    nRet = addSignaturePart(env,
                            iType,
                            szSignData,
                            szDigest,
                            name);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (bodyData != NULL) {
        OpenSOAPByteArrayRelease(bodyData);
    }
    if (szDigest != NULL) {
        free(szDigest);
    }
    if (szSignData != NULL) {
        free(szSignData);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Add Signature to SOAP Message using PrivateKey File
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
int
OPENSOAP_API
OpenSOAPSecAddSignWithFile
(OpenSOAPEnvelopePtr env,         /* (io) Envelope */
 int                 iType,       /* (i)  Hash Type(OPENSOAP_HA_*) */
 const char*         szPrivKName, /* (i)  Private Key File Name */
 OpenSOAPStringPtr   name)        /* (i)  Name of Sign */
{
    int nRet = OPENSOAP_PARAMETER_BADVALUE;
    FILE* fp = NULL;            /* Private Key File Stream */
    /* Open Private Key File */
    fp = fopen(szPrivKName, "rb");
    if (fp == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Add Signature */
    nRet = OpenSOAPSecAddSignWithStream(env, iType, fp, name);
FuncEnd:
    if (fp != NULL) {
        fclose(fp);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Count Signature
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
int
OPENSOAP_API
OpenSOAPSecCntSign
(const OpenSOAPEnvelopePtr env,   /* (i)  Envelope */
 int*                      pnSig) /* (o)  Signature Count */
{
    int                     nRet = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPBlockPtr        blk = NULL;
    OpenSOAPStringPtr       strBlkName = NULL;
    OpenSOAPStringPtr       strNamespace = NULL;
    OpenSOAPXMLNamespacePtr nmspace;
    int                     iWk;

    /* Check Arguments */
    if (env == NULL || pnSig == NULL) {
        goto FuncEnd;
    }
    /* Initialize */
    *pnSig = 0;
    /* Conut Block */
    for (;;) {
        /* Get Next Header Block */
        nRet = OpenSOAPEnvelopeGetNextHeaderBlock(env, &blk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (blk == NULL) {
            break;              /* No More Block */
        }
        /* Check Block's Name */
        nRet = OpenSOAPBlockGetName(blk, &strBlkName);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        nRet = OpenSOAPStringCompareMB(strBlkName, s_szSigBlkName, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0) {
            continue;           /* Different Block Name */
        }
        /* Check Block's Namespace */
        nRet = OpenSOAPBlockGetNamespace(blk, &nmspace);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (nmspace == NULL) {
            continue;           /* Namespace not found */
        }
        /* Get Namespace String */
        nRet = OpenSOAPXMLNamespaceGetURI(nmspace, &strNamespace);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (strNamespace == NULL) {
            continue;           /* No Namespace */
        }
        nRet = OpenSOAPStringCompareMB(strNamespace, s_szNSBlk, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0) {
            continue;           /* Different Block's Namespace */
        }
        /* Signature Found!! */
        (*pnSig)++;
    }

FuncEnd:
    return nRet;
}
/***************************************************************************** 
    Function      : Get "SignedBy" List of Signature
    Return        : int
 ************************************************ Yuji Yamawaki 01.12.14 *****/
int
OPENSOAP_API
OpenSOAPSecGetSignedByList
(OpenSOAPEnvelopePtr env,         /* (i)  Envelope */
 int                 nCntMax,     /* (i)  Maximum Count */
 OpenSOAPStringPtr   list[],      /* (io) SignedBy List  */
 int*                pnCntPacked) /* (o)  Packed Count */
{
    int                     nRet = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr        blk = NULL;
    OpenSOAPStringPtr       strBlkName = NULL;
    OpenSOAPStringPtr       strNSBlk = NULL;
    OpenSOAPStringPtr       strNSChld = NULL;
    OpenSOAPXMLNamespacePtr nmspace;
    OpenSOAPXMLElmPtr       elmSignature;
    OpenSOAPXMLElmPtr       elmSignedBy;
    int                     iWk;
    /* Check Arguments */
    if (!env || nCntMax < 0 || !list || !pnCntPacked) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Initialize */
    *pnCntPacked = 0;
    /* Check */
    while (*pnCntPacked < nCntMax) {
        /* Get Next Hedaer Block */
        nRet = OpenSOAPEnvelopeGetNextHeaderBlock(env, &blk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (blk == NULL) {
            break;              /* No More Block */
        }
        /* Check Block's Name */
        nRet = OpenSOAPBlockGetName(blk, &strBlkName);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        nRet = OpenSOAPStringCompareMB(strBlkName, s_szSigBlkName, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0) {
            continue;           /* Different Block Name */
        }
        /* Check Block's Namespace */
        nRet = OpenSOAPBlockGetNamespace(blk, &nmspace);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (nmspace == NULL) {
            continue;           /* Namespace not found */
        }
        /* Get Namespace String */
        nRet = OpenSOAPXMLNamespaceGetURI(nmspace, &strNSBlk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (strNSBlk == NULL) {
            continue;           /* No Namespace */
        }
        nRet = OpenSOAPStringCompareMB(strNSBlk, s_szNSBlk, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0) {
            continue;           /* Different Block's Namespace */
        }
        /* Find Child "Signature" */
        nRet = OpenSOAPBlockGetChildMB(blk,
                                       s_szChldSignature,
                                       &elmSignature);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (elmSignature == NULL) {
            continue;           /* Specified Child Not Found */
        }
        /* Check Child's Namespace */
        nRet = OpenSOAPXMLElmGetNamespace(elmSignature, &nmspace);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (nmspace == NULL) {
            continue;           /* Namespace not found */
        }
        /* Get Namespace String */
        nRet = OpenSOAPXMLNamespaceGetURI(nmspace, &strNSChld);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (strNSChld == NULL) {
            continue;           /* No Namespace */
        }
        /* Compare Namespace String */
        nRet = OpenSOAPStringCompareMB(strNSChld, s_szNSChldSignature, &iWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (iWk != 0) {
            continue;           /* Different Namespace */
        }
        /* Find Child "SignedBy" from "Signature" */
        nRet = OpenSOAPXMLElmGetChildMB(elmSignature,
                                        s_szChldSignedBy,
                                        &elmSignedBy);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (elmSignedBy == NULL) {
            continue;           /* "SignedBy" Not Found (This is NOT Error) */
        }
        /* Get "SignedBy" */
        nRet = OpenSOAPXMLElmGetValueMB(elmSignedBy,
                                        s_szTypeString,
                                        &(list[*pnCntPacked]));
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        (*pnCntPacked)++;
    }
FuncEnd:
    return nRet;
}
/***************************************************************************** 
    Function      : Verify Signature using PublicKey File Stream
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
int
OPENSOAP_API
OpenSOAPSecVerifySignWithStream
(OpenSOAPEnvelopePtr env,       /* (io) Envelope */
 FILE*               fpPubKey)  /* (i)  Public Key File stream */
{
    int                  nRet;
    OpenSOAPBlockPtr     blk = NULL;   /* Current Header Block */
    OpenSOAPStringPtr    strSig  = NULL; /* Signature String */
    OpenSOAPByteArrayPtr bodyData = NULL;
    const unsigned char* szBodyTop;
    size_t               sizBodyData, sizWk;
    OpenSOAPByteArrayPtr barray = NULL;
    const unsigned char* szTop;
    int                  iType;     /* Hash Type */
    char*                szWk = NULL;

    /* Check Arguments */
    if (!env || !fpPubKey) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Get Body Data */
    nRet = OpenSOAPByteArrayCreate(&bodyData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPEnvelopeGetBodyCharEncodingString(env,
                                                     s_szEncStyle,
                                                     bodyData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayBeginConst(bodyData,
                                       &szBodyTop);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayGetSize(bodyData,
                                    &sizBodyData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    for (;;) {
        /* Initialize String Area */
        nRet = OpenSOAPStringCreate(&strSig);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        /* Get Signature String */
        nRet = getSignatureString(env,
                                  &blk,
                                  &iType,
                                  &strSig);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (blk == NULL) {
            /* No More Signature */
            nRet = OPENSOAP_SEC_SIGNVERIFY_ERROR;
            break;
        }
        if (barray != NULL) {
            nRet = OpenSOAPByteArrayRelease(barray);
            barray = NULL;
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
        }
        nRet = OpenSOAPByteArrayCreate(&barray);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        nRet = OpenSOAPStringGetCharEncodingString(strSig,
                                                   s_szEncStyle,
                                                   barray);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        nRet = OpenSOAPByteArrayBeginConst(barray,
                                           &szTop);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        /* Make NULL Terminate String */
        nRet = OpenSOAPByteArrayGetSize(barray, &sizWk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (sizWk > 0) {
            if ((szWk = (char*) malloc(sizWk + 1)) == NULL) {
                nRet = OPENSOAP_MEM_BADALLOC;
                goto FuncEnd;
            }
            memcpy(szWk, szTop, sizWk);
            szWk[sizWk] = '\0';
        }
        /* Verify Signature */
        nRet = openSOAPSecVerifyRSASign(iType,
                                        szWk,
                                        szBodyTop,
                                        (unsigned long)sizBodyData,
                                        fpPubKey);
        free(szWk);
        szWk = NULL;
        if (OPENSOAP_FAILED(nRet) &&
            nRet != OPENSOAP_SEC_SIGNVERIFY_ERROR) {
            /* Error Except Verify Error */
            goto FuncEnd;
        }
        if (OPENSOAP_SUCCEEDED(nRet)) {
            break;
        }
        nRet = OpenSOAPStringRelease(strSig);
        strSig = NULL;
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
    }
FuncEnd:
    if (strSig != NULL) {
        OpenSOAPStringRelease(strSig);
    }
    if (bodyData != NULL) {
        OpenSOAPByteArrayRelease(bodyData);
    }
    if (barray != NULL) {
        OpenSOAPByteArrayRelease(barray);
    }
    if (szWk != NULL) {
        free(szWk);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Verify Signature using PublicKey File
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.30 *****/
int
OPENSOAP_API
OpenSOAPSecVerifySignWithFile
(OpenSOAPEnvelopePtr env,        /* (io) Envelope */
 const char*         szPubKName) /* (i)  Public Key File Name */
{
    int nRet = OPENSOAP_PARAMETER_BADVALUE;
    FILE* fp = NULL;            /* Public Key File Stream */
    /* Open Public Key File */
    fp = fopen(szPubKName, "rb");
    if (fp == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Verify Signature */
    nRet = OpenSOAPSecVerifySignWithStream(env, fp);
FuncEnd:
    if (fp != NULL) {
        fclose(fp);
    }
    return nRet;
}
