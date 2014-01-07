/*-----------------------------------------------------------------------------
 * $RCSfile: sec_rsasig.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/Security.h>
#include "security_defs.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#if defined(DEBUG)
#include <openssl/err.h>
#endif

/***************************************************************************** 
    Function      : Convert Hash Type (OPENSOAP_HA_* --> NID_*)
    Return        : 
	************************************************ Yuji Yamawaki 01.09.10 *****/
static int convType(int iTypeIn)
{
    switch (iTypeIn) {
		case OPENSOAP_HA_MD5:
			return NID_md5;
		case OPENSOAP_HA_RIPEMD:
			return NID_ripemd160;
		default:                    /* OPENSOAP_HA_SHA */
			return NID_sha1;
    }
}
/***************************************************************************** 
    Function      : Generate Digital Signature(RSA) <Binary>
    Return        : int
	************************************************ Yuji Yamawaki 02.02.06 *****/
int
openSOAPSecMakeRSABinSign
(int                  iType,     /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szData,    /* (i)  Data to be signed */
 unsigned long        ulDataLen, /* (i)  Length of Data */
 FILE*                fpPrivKey, /* (i)  RSA Private Key File stream */
 unsigned char**      pszDSig,   /* (o)  Digital Signature */
 int*                 pnSigLen,  /* (o)  Signature Length */
 unsigned char**      pszDigest, /* (o)  Digest Value(NULL Ok) */
 int*                 pnDigestLen) /* (o)  Digest Length(NULL Ok) */
{
    RSA*           pRsa = NULL;
    char*          szSig = NULL;
    unsigned char* szHash = NULL;
    unsigned long  ulLenHash;
    int            nSigLen;
    int            nRet = OPENSOAP_NO_ERROR;
    fpos_t         fposKey;
    /* Check Arguments */
    if (szData == NULL || fpPrivKey == NULL || ulDataLen <= 0 ||
        pszDSig == NULL || pnSigLen == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Backup Stream's Position */
    if (fgetpos(fpPrivKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Hash Original Data */
    nRet = openSOAPSecMakeHash(iType, szData, ulDataLen,
                               &ulLenHash, &szHash);
    if (OPENSOAP_FAILED(nRet)) {
        return nRet;
    }
    /* Generate RSA from Private key */
    if (PEM_read_RSAPrivateKey(fpPrivKey, &pRsa, NULL, NULL) == NULL) {
        nRet = OPENSOAP_SEC_SIGNGEN_ERROR;
        goto FuncEnd;
    }
    /* Recover File Position */
    if (fsetpos(fpPrivKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Allocate Signature Area */
    if ((szSig = (char*)malloc(RSA_size(pRsa))) == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Create Signature */
    if (RSA_sign(convType(iType),
                 szHash,
                 ulLenHash,
                 szSig,
                 &nSigLen,
                 pRsa) != 1) {
        free(szSig);
        szSig = NULL;
        nRet = OPENSOAP_SEC_SIGNGEN_ERROR;
        goto FuncEnd;
    }

 FuncEnd:
    /* Terminate */
    if (pRsa != NULL)
        RSA_free(pRsa);
    if (OPENSOAP_FAILED(nRet)) {
        if (szSig != NULL) {
            free(szSig);
            szSig = NULL;
        }
        if (szHash != NULL) {
            free(szHash);
            szHash = NULL;
        }
    }
    *pszDSig = szSig;
    *pnSigLen = nSigLen;
    if (pszDigest != NULL) {
        *pszDigest = szHash;
    } else {
        if (szHash != NULL) {
            free(szHash);
        }
    }
    if (pnDigestLen != NULL) {
        *pnDigestLen = (int)ulLenHash;
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Generate Digital Signature(RSA)
    Return        : int
	************************************************ Yuji Yamawaki 01.07.26 *****/
int
openSOAPSecMakeRSASign
(int                  iType,     /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szData,    /* (i)  Data to be signed */
 unsigned long        ulDataLen, /* (i)  Length of Data */
 FILE*                fpPrivKey, /* (i)  RSA Private Key File stream */
 char**               pszDSig,   /* (o)  Digital Signature<base64> */
 char**               pszDigest) /* (o)  Digest Value<base64> */
                                 /*      (NULL is allowed) */
{
    int            nRet = OPENSOAP_NO_ERROR;
    unsigned char* szDSigBin = NULL;   /* Digital Signature (Bin) */
    int            nDSigLenBin;        /* Length of Digital Signature (Bin) */
    unsigned char* szDigestBin = NULL; /* Digest Value (Bin) */
    int            nDigestLenBin;      /* Length of Digest Value (Bin) */
    char*          szDSigb64 = NULL;   /* Digital Signature (base64) */
    char*          szDigestb64 = NULL; /* Digest Value (base64) */

    /* Check Arguments */
    if (pszDSig == NULL) {
        nRet = OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Get Digital Signature <Binary> */
    nRet = openSOAPSecMakeRSABinSign(iType,
                                     szData,
                                     ulDataLen,
                                     fpPrivKey,
                                     &szDSigBin,
                                     &nDSigLenBin,
                                     &szDigestBin,
                                     &nDigestLenBin);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Encode Hash String */
    if (pszDigest != NULL) {
        nRet = openSOAPSecEncByBase64(szDigestBin,
                                      (unsigned long)nDigestLenBin,
                                      &szDigestb64);
        if (OPENSOAP_FAILED(nRet)) {
            goto FuncEnd;
        }
    }
    /* Encode Signature String */
    nRet = openSOAPSecEncByBase64(szDSigBin,
                                  (unsigned long)nDSigLenBin,
                                  &szDSigb64);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
 FuncEnd:
    if (szDSigBin != NULL) {
        free(szDSigBin);
    }
    if (szDigestBin != NULL) {
        free(szDigestBin);
    }
    if (OPENSOAP_FAILED(nRet)) {
        if (szDSigb64 != NULL) {
            free(szDSigb64);
            szDSigb64 = NULL;
        }
        if (szDigestb64 != NULL) {
            free(szDigestb64);
            szDigestb64 = NULL;
        }
    }
    *pszDSig = szDSigb64;
    if (pszDigest != NULL) {
        *pszDigest = szDigestb64;
    } else {
        if (szDigestb64 != NULL) {
            free(szDigestb64);
        }
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Generate Digital Signature(from Private Key File)(RSA)
    Return        : int
	************************************************ Yuji Yamawaki 01.07.26 *****/
int
openSOAPSecMakeRSASignFromFile
(int                  iType,      /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szData,     /* (i)  Data to be signed */
 unsigned long        ulDataLen,  /* (i)  Length of Data */
 const char*          szPrivKeyFileName, /* (i)  RSA Private Key File Name */
 char**               pszDigest,   /* (o)  Digest Value<base64> */
 char**               pszDSig)     /* (o)  Digital Signature<base64> */
{
    FILE*          fpPrivate = NULL;
    int            nRet = OPENSOAP_NO_ERROR;
    /* Check Arguments */
    if (szData == NULL || ulDataLen <= 0 || szPrivKeyFileName == NULL ||
        pszDSig == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Private Key File */
    if ((fpPrivate = fopen(szPrivKeyFileName, "r")) == NULL) {
        return OPENSOAP_FILEOPEN_ERROR;
    }
    /* Genetare Key File */
    nRet = openSOAPSecMakeRSASign(iType, szData, ulDataLen,
                                  fpPrivate, pszDigest, pszDSig);
    /* Terminate */
    fclose(fpPrivate);
    return nRet;
}

/***************************************************************************** 
    Function      : Verify Digital Signature(RSA) <Binary>
    Return        : int
	************************************************ Yuji Yamawaki 02.02.07 *****/
int
openSOAPSecVerifyRSASignBin
(int                  iType,          /* (i)  Hash Type(OPENSOAP_HA_*) */
 unsigned char*       szIn,           /* (i)  Signature */
 unsigned long        ulInSize,       /* (i)  Signature Size */
 const unsigned char* szData,         /* (i)  Original Data */
 unsigned long        ulDataSize,     /* (i)  Original Data Size */
 FILE*                fpPubKey)       /* (i)  RSA Public Key File Stream */
{
    RSA*  pRsa = NULL;
    unsigned char* szHash = NULL;
    unsigned long  ulLenHash;
    int            nRet = OPENSOAP_NO_ERROR;
    int            iRes;
    fpos_t         fposKey;
    /* Check Arguments */
    if (szIn == NULL || ulInSize == 0 || szData == NULL || ulDataSize == 0 ||
        fpPubKey == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Backup Stream's Position */
    if (fgetpos(fpPubKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Hash Original Data */
    nRet = openSOAPSecMakeHash(iType, szData, ulDataSize,
                               &ulLenHash, &szHash);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Generate RSA from Public key */
    if (PEM_read_RSAPublicKey(fpPubKey, &pRsa, NULL, NULL) == NULL) {
        nRet = OPENSOAP_SEC_SIGNVERIFY_ERROR;
        goto FuncEnd;
    }
    /* Recover File Position */
    if (fsetpos(fpPubKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Verify Signature */
    iRes = RSA_verify(convType(iType),
                      szHash,
                      ulLenHash,
                      szIn,
                      (int)ulInSize,
                      pRsa);
    if (iRes != 1) {
        nRet = OPENSOAP_SEC_SIGNVERIFY_ERROR;
    }
 FuncEnd:
    if (szHash != NULL)
        free(szHash);
    /* Terminate */
    RSA_free(pRsa);
    return nRet;
}
/***************************************************************************** 
    Function      : Verify Digital Signature(RSA)
    Return        : int
	************************************************ Yuji Yamawaki 01.07.26 *****/
int
openSOAPSecVerifyRSASign
(int                  iType,          /* (i)  Hash Type(OPENSOAP_HA_*) */
 const char*          szIn,           /* (i)  Signature(Base64 string) */
 const unsigned char* szData,         /* (i)  Original Data */
 unsigned long        ulDataSize,     /* (i)  Original Data Size */
 FILE*                fpPubKey)       /* (i)  RSA Public Key File Stream */
{
    int            nRet = OPENSOAP_NO_ERROR;
    unsigned char* szSig = NULL;  /* Signature(Binary) */
    unsigned long  ulSigLen;      /* Length of Signature(Binary) */
    /* Check Arguments */
    if (szIn == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Decode Base64 */
    nRet = openSOAPSecDecByBase64(szIn,
                                  &ulSigLen,
                                  &szSig);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Verify */
    nRet = openSOAPSecVerifyRSASignBin(iType,
                                       szSig,
                                       ulSigLen,
                                       szData,
                                       ulDataSize,
                                       fpPubKey);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
 FuncEnd:
    if (szSig != NULL)
        free(szSig);
    return nRet;
}
/***************************************************************************** 
    Function      : Verify Digital Signature(Using Public Key File)(RSA)
    Return        : int
	************************************************ Yuji Yamawaki 01.07.26 *****/
int
openSOAPSecVerifyRSASignFromFile
(int                  iType,            /* (i)  Hash Type(OPENSOAP_HA_*) */
 const char*          szIn,             /* (i)  Signature(Base64 string) */
 const unsigned char* szData,           /* (i)  Original Data */
 unsigned long        ulDataSize,       /* (i)  Original Data Size */
 const char*          szPubKeyFileName) /* (i)  RSA Public Key File Name */
{
    FILE* fpPublic = NULL;
    int   nRet = OPENSOAP_NO_ERROR;
    /* Check Arguments */
    if (szIn == NULL || szData == NULL || ulDataSize == 0 ||
        szPubKeyFileName == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Public Key File */
    if ((fpPublic = fopen(szPubKeyFileName, "r")) == NULL) {
        return OPENSOAP_SEC_SIGNVERIFY_ERROR;
    }
    /* Verify */
    nRet = openSOAPSecVerifyRSASign(iType, szIn, szData, ulDataSize,
									fpPublic);
    /* Terminate */
    fclose(fpPublic);
    return nRet;
}
