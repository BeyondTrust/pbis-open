/*-----------------------------------------------------------------------------
 * $RCSfile: sec_encryptEnvelope.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include <OpenSOAP/Envelope.h>
#include <OpenSOAP/Security.h>
#include "security_defs.h"

/* Encrypt attribute string */
static const char* s_szEncryptAttr = "encrypt";

/* Namespace of encrypt attribute string */
static const char* s_szEncAttrNS = "http://security.opensoap.jp/1.0/";

/* Data Type */
static const char* s_szTypBool    = "boolean";
static const char* s_szTypeString = "string";

/* Encoding Style */
static const char* s_szEncStyle = "UTF-8";

/***************************************************************************** 
    Function      : Encrypt XML Element
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.22 *****/
static int encryptXMLElement
(OpenSOAPXMLElmPtr elm,       /* (i)  XLM Element */
 FILE*             fpPubKey,  /* (i)  Public Key File stream */
 OpenSOAPStringPtr strOut)    /* (io) Encrypted String */
{
    int                  nRet;
    OpenSOAPStringPtr    str = NULL;
    OpenSOAPByteArrayPtr barrayEnc = NULL;
    size_t               sizData;
    const unsigned char* szDataTop;
    char*                szEncData = NULL; /* Encrypted Data(base64) */

    /* Initialize */
    nRet = OpenSOAPStringCreate(&str);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayCreate(&barrayEnc);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Get Data Array of XML Element */
    nRet = OpenSOAPXMLElmGetValueMB(elm, s_szTypeString, &str);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringGetCharEncodingString(str, s_szEncStyle, barrayEnc);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Get Size and Top Pointer */
    nRet = OpenSOAPByteArrayGetSize(barrayEnc, &sizData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayBeginConst(barrayEnc, &szDataTop);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Encrypt */
    nRet = openSOAPSecEncData(szDataTop, sizData, fpPubKey,
                              &szEncData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set To OpenSOAPString */
    nRet = OpenSOAPStringSetStringMB(strOut, szEncData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
FuncEnd:
    if (barrayEnc != NULL) {
        OpenSOAPByteArrayRelease(barrayEnc);
    }
    if (str != NULL) {
        OpenSOAPStringRelease(str);
    }
    if (szEncData != NULL) {
        free(szEncData);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Encrypt Envelope Using Public Key File Stream
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.22 *****/
int
OPENSOAP_API
OpenSOAPSecEncWithStream
(OpenSOAPEnvelopePtr env,       /* (io) Envelope */
 FILE*               fpPubKey)  /* (i)  Public Key File stream */
{
    int nRet = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPBlockPtr blk = NULL;        /* Block in Body */
    OpenSOAPStringPtr strWk = NULL; /* String */
    OpenSOAPStringPtr strNS = NULL; /* Namespace String */
    /* Check Arguments */
    if (!env || !fpPubKey) {
        goto FuncEnd;
    }
    /* Execute */
    for (;;) {
        OpenSOAPXMLElmPtr elm = NULL;       /* Element in Block */
        /* Get Next Block */
        nRet = OpenSOAPEnvelopeGetNextBodyBlock(env, &blk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (blk == NULL)
            break;              /* Next Block Not Found */
        for (;;) {
            OpenSOAPXMLAttrPtr      attr;   /* Attribute in Element */
            OpenSOAPXMLNamespacePtr nsAttr; /* Attribute's NameSpace  */
            int                     iRes;
            /* Get Next Element */
            nRet = OpenSOAPBlockGetNextChild(blk, &elm);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (elm == NULL) {
                break;          /* Next Element Not Found */
            }
            /* Search Encrypt Attribute */
            nRet = OpenSOAPXMLElmGetAttributeMB(elm, s_szEncryptAttr, &attr);
            if (OPENSOAP_FAILED(nRet)) {
                return nRet;
            }
            if (attr == NULL) {
                continue;       /* No Encrypt Attribute */
            }
            /* Get Namespace of Encrypt Attribute */
            nRet = OpenSOAPXMLAttrGetNamespace(attr, &nsAttr);
            if (OPENSOAP_FAILED(nRet)) {
                return nRet;
            }
            if (nsAttr == NULL) {
                continue;       /* No Namespace */
            }
            /* Get Namespace String */
            nRet = OpenSOAPXMLNamespaceGetURI(nsAttr, &strNS);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (strNS == NULL) {
                continue;       /* No Namespace String */
            }
            /* Compare Namespace string */
            nRet = OpenSOAPStringCompareMB(strNS, s_szEncAttrNS, &iRes);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (iRes != 0) {
                continue;       /* Different Namespace */
            }
            /* Get Value of Encrypt Attribute */
            nRet = OpenSOAPXMLAttrGetValueMB(attr, s_szTypBool, &iRes);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (iRes == 0) {
                continue;       /* No Need To Encrypt */
            }
            /* Encrypt (to strWk) */
            nRet = OpenSOAPStringCreate(&strWk);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            nRet = encryptXMLElement(elm, fpPubKey, strWk);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            /* Set Encrypted Data */
            nRet = OpenSOAPXMLElmSetValueMB(elm, s_szTypeString, &strWk);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            nRet = OpenSOAPStringRelease(strWk);
            strWk = NULL;
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
        }
    }
FuncEnd:
    if (strWk) {
        OpenSOAPStringRelease(strWk);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Encrypt Envelope Using Public Key File
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.22 *****/
int
OPENSOAP_API
OpenSOAPSecEncWithFile
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
    /* Encrypt */
    nRet = OpenSOAPSecEncWithStream(env, fp);
FuncEnd:
    if (fp != NULL) {
        fclose(fp);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Decrypt XML Element
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
static int decryptXMLElement
(OpenSOAPXMLElmPtr elm,       /* (i)  XLM Element */
 FILE*             fpPrivKey, /* (i)  Private Key File stream */
 OpenSOAPStringPtr strOut)    /* (io) Decrypted String */
{
    int                  nRet;
    OpenSOAPStringPtr    str = NULL;
    OpenSOAPByteArrayPtr barrayEnc = NULL;
    size_t               sizData;
    const unsigned char* szDataTop;
    char*                szDecData = NULL; /* Decrypted Data */
    unsigned long        ulDecData; /* Length of Decrypted Data */
    char*                szWkArea = NULL;
    /* Initialize */
    nRet = OpenSOAPStringCreate(&str);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayCreate(&barrayEnc);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Get Encrypted Data Array of XML Element */
    nRet = OpenSOAPXMLElmGetValueMB(elm, s_szTypeString, &str);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPStringGetCharEncodingString(str, s_szEncStyle, barrayEnc);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Get Size and Top Pointer */
    nRet = OpenSOAPByteArrayGetSize(barrayEnc, &sizData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    nRet = OpenSOAPByteArrayBeginConst(barrayEnc, &szDataTop);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Convert NULL Terminate String */
    szWkArea = (char*) malloc(sizData + 1);
    if (szWkArea == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    strncpy(szWkArea, szDataTop, sizData);
    szWkArea[sizData] = '\0';
    /* Decrypt */
    nRet = openSOAPSecDecData(szWkArea,
                              fpPrivKey,
                              &ulDecData,
                              (unsigned char**)&szDecData);
    free(szWkArea);
    szWkArea = NULL;
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Set To OpenSOAPString */
    szWkArea = (char*)malloc(ulDecData + 1);
    if (szWkArea == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    strncpy(szWkArea, szDecData, ulDecData);
    szWkArea[ulDecData] = '\0';
    nRet = OpenSOAPStringSetStringMB(strOut, szWkArea);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }

FuncEnd:
    if (barrayEnc != NULL) {
        OpenSOAPByteArrayRelease(barrayEnc);
    }
    if (str != NULL) {
        OpenSOAPStringRelease(str);
    }
    if (szDecData != NULL) {
        free(szDecData);
    }
    if (szWkArea != NULL) {
        free(szWkArea);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Decrypt Envelope Using Public Key File Stream
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
int
OPENSOAP_API
OpenSOAPSecDecWithStream
(OpenSOAPEnvelopePtr env,       /* (i)  Envelope */
 FILE*               fpPrivKey) /* (i)  Public Key File stream */
{
    int nRet = OPENSOAP_PARAMETER_BADVALUE;
    OpenSOAPBlockPtr blk = NULL;        /* Block in Body */
    OpenSOAPStringPtr strWk = NULL; /* String */
    OpenSOAPStringPtr strNS = NULL; /* Namespace String */
    /* Check Arguments */
    if (!env || !fpPrivKey) {
        goto FuncEnd;
    }
    /* Execute */
    for (;;) {
        OpenSOAPXMLElmPtr elm = NULL;       /* Element in Block */
        /* Get Next Block */
        nRet = OpenSOAPEnvelopeGetNextBodyBlock(env, &blk);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
        if (blk == NULL)
            break;              /* Next Block Not Found */
        for (;;) {
            OpenSOAPXMLAttrPtr      attr;   /* Attribute in Element */
            OpenSOAPXMLNamespacePtr nsAttr; /* Attribute's NameSpace  */
            int                     iRes;
            /* Get Next Element */
            nRet = OpenSOAPBlockGetNextChild(blk, &elm);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (elm == NULL) {
                break;          /* Next Element Not Found */
            }
            /* Search Encrypt Attribute */
            nRet = OpenSOAPXMLElmGetAttributeMB(elm, s_szEncryptAttr, &attr);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (attr == NULL) {
                continue;       /* No Encrypt Attribute */
            }
            /* Get Namespace of Encrypt Attribute */
            nRet = OpenSOAPXMLAttrGetNamespace(attr, &nsAttr);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (nsAttr == NULL) {
                continue;       /* No Namespace */
            }
            /* Get Namespace String */
            nRet = OpenSOAPXMLNamespaceGetURI(nsAttr, &strNS);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (strNS == NULL) {
                continue;       /* No Namespace String */
            }
            /* Compare Namespace string */
            nRet = OpenSOAPStringCompareMB(strNS, s_szEncAttrNS, &iRes);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (iRes != 0) {
                continue;       /* Different Namespace */
            }
            /* Get Value of Encrypt Attribute */
            nRet = OpenSOAPXMLAttrGetValueMB(attr, s_szTypBool, &iRes);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            if (iRes == 0) {
                continue;       /* No Need To Encrypt */
            }
            /* Decrypt (to strDecrypt) */
            nRet = OpenSOAPStringCreate(&strWk);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            nRet = decryptXMLElement(elm, fpPrivKey, strWk);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            /* Set Decrypted Data */
            nRet = OpenSOAPXMLElmSetValueMB(elm, s_szTypeString, &strWk);
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
            nRet = OpenSOAPStringRelease(strWk);
            strWk = NULL;
            if (OPENSOAP_FAILED(nRet)) {
                goto FuncEnd;
            }
        }
    }
FuncEnd:
    if (strWk) {
        OpenSOAPStringRelease(strWk);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Decrypt Envelope Using Private Key File
    Return        : int
 ************************************************ Yuji Yamawaki 01.11.26 *****/
int
OPENSOAP_API
OpenSOAPSecDecWithFile
(OpenSOAPEnvelopePtr env,         /* (i)  Envelope */
 const char*         szPrivKName) /* (i)  Private Key File Name */
{
    int nRet = OPENSOAP_PARAMETER_BADVALUE;
    FILE* fp = NULL;            /* Public Key File Stream */
    /* Open Public Key File */
    fp = fopen(szPrivKName, "rb");
    if (fp == NULL) {
        nRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Decrypt */
    nRet = OpenSOAPSecDecWithStream(env, fp);
FuncEnd:
    if (fp != NULL) {
        fclose(fp);
    }
    return nRet;
}
