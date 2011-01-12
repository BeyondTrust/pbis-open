/*-----------------------------------------------------------------------------
 * $RCSfile: sec_encrypt.c,v $
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

/***************************************************************************** 
    Function      : Encrypt Using Public Key
    Return        : int
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int 
openSOAPSecEncData
(const unsigned char* szData,    /* (i)  Data to be Encrypted */
 unsigned long        ulDataLen, /* (i)  Length of Data */
 FILE*                fpPubKey,  /* (i)  Public Key File stream */
 char**               pszRet)    /* (o)  Encrypted data<base64> */
                                 /*      (call free() after use)  */
{
    RSA*           pRsa = NULL;
    char*          szResult = NULL;
    int            nCnt;
    int            nSizeSingle;    /* Max Input Size of Single Operation */
    int            nSizeOutSingle; /* Output Size of Single Operation */
    unsigned long  ulRest;
    unsigned long  ulSizeOut;
    int            i;
    const unsigned char* szCurIn;
    char*          szCurOut;
    int            nRet = OPENSOAP_NO_ERROR;
    fpos_t         fposKey;

    /* Check Arguments */
    if (szData == NULL || ulDataLen == 0 || fpPubKey == NULL) {
        nRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Backup Stream's Position */
    if (fgetpos(fpPubKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Generate RSA from Public Key */
    if (PEM_read_RSAPublicKey(fpPubKey, &pRsa, NULL, NULL) == NULL) {
        nRet = OPENSOAP_SEC_ENCRYPT_ERROR;
        goto FuncEnd;
    }
    /* Recover Position */
    if (fsetpos(fpPubKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Decide Data Size For One Operation and Total Operation Count */
    nSizeOutSingle = RSA_size(pRsa);
    nSizeSingle = nSizeOutSingle - 12; /* size for PKCS #1 v1.5 */
    if (nSizeSingle <= 0) {
        nRet = OPENSOAP_SEC_ENCRYPT_ERROR;
        goto FuncEnd;
    }
    nCnt = ulDataLen / (unsigned long)nSizeSingle;
    if (ulDataLen % (unsigned long)nSizeSingle != 0)
        nCnt++;
    /* Allocate Output Area */
    ulSizeOut = (unsigned long)nSizeOutSingle * (unsigned long)nCnt;
    if ((szResult = (char*)malloc(ulSizeOut)) == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Encrypt And Save */
    szCurIn  = szData;
    szCurOut = szResult;
    ulRest   = ulDataLen;
    for (i = 0; i < nCnt; i++) {
        int nSizeProc;
        int nSizeRet;
        /* Decide Current Proc Length */
        if (ulRest > (unsigned long)nSizeSingle)
            nSizeProc = nSizeSingle;
        else
            nSizeProc = (int)ulRest;
        /* Encrypt */
        nSizeRet = RSA_public_encrypt(nSizeProc,
                                      (char*)szCurIn,
                                      szCurOut,
                                      pRsa,
                                      RSA_PKCS1_PADDING);
        if (nSizeRet != nSizeOutSingle) {
            free(szResult);
            szResult = NULL;
            nRet = OPENSOAP_SEC_ENCRYPT_ERROR;
            goto FuncEnd;       /* Encrypt Error */
        }
        /* Update Parameters */
        ulRest -= (unsigned long)nSizeProc;
        szCurOut += nSizeRet;
        szCurIn  += nSizeProc;
    }
    /* Convert To Base64 */
    nRet = openSOAPSecEncByBase64(szResult,
                                  ulSizeOut,
                                  pszRet);
    free(szResult);

FuncEnd:
    if (pRsa != NULL)
        RSA_free(pRsa);
    return nRet;
}
/***************************************************************************** 
    Function      : Encrypt Using Public Key(from Public Key File)
    Return        : int
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int 
openSOAPSecEncDataFromFile
(const unsigned char* szData,    /* (i)  Data to be Encrypted */
 unsigned long        ulDataLen, /* (i)  Length of Data */
 const char*          szPubKeyFileName, /* (i)  Public Key File Name */
 char**               pszRet)    /* (o)  Encrypted data<base64> */
                                 /*      (call free() after use)  */
{
    FILE* fpPublic = NULL;
    int   nRet;
    /* Check Argumrnts */
    if (szData == NULL || ulDataLen == 0 ||
        szPubKeyFileName == NULL || pszRet == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Public Key File */
    if ((fpPublic = fopen(szPubKeyFileName, "r")) == NULL) {
        return OPENSOAP_FILEOPEN_ERROR;
    }
    /* Encrypt */
    nRet = openSOAPSecEncData(szData,
                              ulDataLen,
                              fpPublic,
                              pszRet);
    /* Terminate */
    fclose(fpPublic);
    return nRet;
}

/***************************************************************************** 
    Function      : Decrypt Using Private Key
    Return        : int
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int
openSOAPSecDecData
(char*           szIn,      /* (i)  Data to be Decrypted(base64 string) */
 FILE*           fpPrivKey, /* (i)  Private Key File stream */
 unsigned long*  pulResLen, /* (o)  Crypted Data Length */
 unsigned char** pszRet)    /* (o)  Decrypted data */
                           /*      (call free() after use)  */
{
    RSA*           pRsa = NULL;
    unsigned char* szResult = NULL;
    int            nSizeSingle;    /* Max Input Size of Single Operation */
    int            nCnt;
    int            i;
    unsigned char* szCurIn;
    unsigned char* szCurOut;
    int            nSizeRet;
    unsigned long  ulSizeOutAlloc;
    unsigned char* szData = NULL;      /* Data to be Decrypted(binary) */
    unsigned long  ulDataLen;          /* Length of Binary Data */
    int            nRet = OPENSOAP_NO_ERROR;
    fpos_t         fposKey;

    /* Check Arguments */
    if (szIn == NULL || fpPrivKey == NULL ||
        pulResLen == NULL || pszRet == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    *pulResLen = 0;
    /* Decode Base64 */
    nRet = openSOAPSecDecByBase64(szIn, &ulDataLen, &szData);
    if (OPENSOAP_FAILED(nRet)) {
        goto FuncEnd;
    }
    /* Backup Stream's Position */
    if (fgetpos(fpPrivKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Generate RSA from Private Key */
    if (PEM_read_RSAPrivateKey(fpPrivKey, &pRsa, NULL, NULL) == NULL) {
        nRet = OPENSOAP_SEC_DECRYPT_ERROR;
        goto FuncEnd;
    }
    /* Restore Stream's Position */
    if (fsetpos(fpPrivKey, &fposKey) != 0) {
        nRet = OPENSOAP_IO_READ_ERROR;
        goto FuncEnd;
    }
    /* Decide Data Size For One Operation and Total Operation Count */
    nSizeSingle = RSA_size(pRsa);
    if (ulDataLen % nSizeSingle != 0) {
        nRet = OPENSOAP_SEC_DECRYPT_ERROR;
        goto FuncEnd; /* Invalid Data Size!! */
    }
    nCnt = ulDataLen / nSizeSingle;
    /* Allocate Output Area(maximum size) */
    ulSizeOutAlloc = (unsigned long)(nSizeSingle - 12) *
        (unsigned long)nCnt;
    if ((szResult = (unsigned char*)malloc(ulSizeOutAlloc)) == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Crypt And Save */
    szCurIn  = szData;
    szCurOut = szResult;
    for (i = 0; i < nCnt; i++) {
        /* Decrypt */
        nSizeRet = RSA_private_decrypt(nSizeSingle,
                                       szCurIn,
                                       szCurOut,
                                       pRsa,
                                       RSA_PKCS1_PADDING);
        if (nSizeRet < 0) {
            /* Decrypt Error */
            free (szResult);
            szResult = NULL;
            nRet = OPENSOAP_SEC_DECRYPT_ERROR;
            goto FuncEnd;
        }
        /* Update Parameters */
        szCurOut += nSizeRet;
        *pulResLen += (unsigned long)nSizeRet;
        szCurIn += nSizeSingle;
    }

FuncEnd:
    if (szData != NULL)
        free(szData);
    if (pRsa != NULL)
        RSA_free(pRsa);
    *pszRet = szResult;
    return nRet;
}
/***************************************************************************** 
    Function      : Decrypt Using Private Key(from Private Key File)
    Return        : unsigned char*(decrypted data, NULL on error)
 ************************************************ Yuji Yamawaki 01.07.27 *****/
int
OpenSOAPSecDecDataFromFile
(char*           szIn,      /* (i)  Data to be Decrypted(base64 string) */
 const char*     szPrivKeyFileName, /* (i)  Private Key File Name */
 unsigned long*  pulResLen, /* (o)  Crypted Data Length */
 unsigned char** pszRet)    /* (o)  Decrypted data */
                            /*      (call free() after use)  */
{
    FILE*          fpPrivate = NULL;
    int            nRet = OPENSOAP_NO_ERROR;
    /* Check Arguments */
    if (szIn == NULL || szPrivKeyFileName == NULL ||
        pulResLen == NULL || pszRet == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open private key file */
    if ((fpPrivate = fopen(szPrivKeyFileName, "r")) == NULL) {
        return OPENSOAP_FILEOPEN_ERROR;
    }
    /* Decrypt */
    nRet = openSOAPSecDecData(szIn,
                              fpPrivate,
                              pulResLen,
                              pszRet);
    /* Terminate */
    fclose(fpPrivate);
    return nRet;
}
