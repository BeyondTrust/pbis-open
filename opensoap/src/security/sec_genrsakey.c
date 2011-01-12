/*-----------------------------------------------------------------------------
 * $RCSfile: sec_genrsakey.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/Security.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <string.h>
#include <openssl/pem.h>

/***************************************************************************** 
    Function      : Generate RSA Keys To Stream
    Return        : int
 ************************************************ Yuji Yamawaki 01.07.26 *****/
int
OPENSOAP_API
OpenSOAPSecGenerateRSAKeys
(const unsigned char* szSeedPhrase, /* (i)  Seed phrase */
 FILE*                fpPrivKey,    /* (i)  RSA Private Key File stream */
 FILE*                fpPubKey)     /* (i)  RSA Public Key File stream */
{
    int   iRet = OPENSOAP_NO_ERROR;
    RSA*  pRsa = NULL;
    /* Check Arguments */
    if (!szSeedPhrase || !fpPubKey || !fpPrivKey) {
        iRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Set Seed */
    RAND_seed(szSeedPhrase, strlen(szSeedPhrase));
    /* Generate Key */
    pRsa = RSA_generate_key(1024, 3, NULL, NULL);
    if (pRsa == NULL) {
        iRet = OPENSOAP_SEC_KEYGEN_ERROR;
        goto FuncEnd;
    }

    /* Write Public Key */
    if (PEM_write_RSAPublicKey(fpPubKey, pRsa) == 0) {
        iRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }
    /* Write Private Key */
    if (PEM_write_RSAPrivateKey(fpPrivKey, pRsa, NULL,
                                NULL, 0, NULL, NULL) == 0) {
        iRet = OPENSOAP_IO_WRITE_ERROR;
        goto FuncEnd;
    }

FuncEnd:
    if (pRsa != NULL)
        RSA_free(pRsa);
    return iRet;
}
/***************************************************************************** 
    Function      : Generate RSA Keys To File
    Return        : int
 ************************************************ Yuji Yamawaki 01.07.26 *****/
int
OPENSOAP_API
OpenSOAPSecGenerateRSAKeysToFile
(const unsigned char* szSeedPhrase,      /* (i)  Seed phrase */
 const char*          szPrivKeyFileName, /* (i)  Private Key File Name */
 const char*          szPubKeyFileName)  /* (i)  Public Key File Name */
{
    int   iRet = OPENSOAP_PARAMETER_BADVALUE;
    FILE* fpPrivate = NULL;
    FILE* fpPublic = NULL;
    /* Check arguments */
    if (szSeedPhrase == NULL ||
        szPubKeyFileName == NULL ||
        szPubKeyFileName == NULL) {
        return iRet;
    }
    /* Open Key File */
    if ((fpPrivate = fopen(szPrivKeyFileName, "w")) == NULL) {
        iRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    if ((fpPublic = fopen(szPubKeyFileName, "w")) == NULL) {
        iRet = OPENSOAP_FILEOPEN_ERROR;
        goto FuncEnd;
    }
    /* Output */
    iRet = OpenSOAPSecGenerateRSAKeys(szSeedPhrase,
                                      fpPrivate,
                                      fpPublic);

FuncEnd:
    if (fpPrivate != NULL)
        fclose(fpPrivate);
    if (fpPublic != NULL)
        fclose(fpPublic);
    return iRet;
}
