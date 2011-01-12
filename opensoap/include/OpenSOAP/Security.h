/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Security.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef OpenSOAP_Security_H
#define OpenSOAP_Security_H

#include <stdio.h>
#include <OpenSOAP/Envelope.h>

/**
 * @file OpenSOAP/Security.h
 * @brief OpenSOAP API Security Processing
 * @author
 *    OpenSOAP Development Team
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @def OPENSOAP_HA_MD2    (1)
 * @brief Hash Algorithm Definition For MD2
 */
#define OPENSOAP_HA_MD2    (1)  /* MD2 */

/**
 * @def OPENSOAP_HA_MD4    (2)
 * @brief Hash Algorithm Definition For MD4
 */
#define OPENSOAP_HA_MD4    (2)  /* MD4 */

/**
 * @def OPENSOAP_HA_MD5    (3)
 * @brief Hash Algorithm Definition For MD5
 */
#define OPENSOAP_HA_MD5    (3)  /* MD5 */

/**
 * @def OPENSOAP_HA_MDC2   (4)
 * @brief Hash Algorithm Definition For MDC2
 */
#define OPENSOAP_HA_MDC2   (4)  /* MDC2 */

/**
 * @def OPENSOAP_HA_RIPEMD (5)
 * @brief Hash Algorithm Definition For RIPEMD
 */
#define OPENSOAP_HA_RIPEMD (5)  /* RIPEMD */

/**
 * @def OPENSOAP_HA_SHA    (6)
 * @brief Hash Algorithm Definition For SHA
 */
#define OPENSOAP_HA_SHA    (6)  /* SHA */

/**
 * @def OPENSOAP_CA_OWNER_LEN  (1024)
 * @brief Max Owner Length in CA Database
 */
#define OPENSOAP_CA_OWNER_LEN  (1024)
/**
 * @def OPENSOAP_CERT_DATE_LEN   (14)
 * @brief Data Length in Certificate
 */
#define OPENSOAP_CERT_DATE_LEN   (14)



    /* Certificate */
    /**
     * @typedef struct tagOpenSOAPSecCert OpenSOAPSecCert
     * @brief OpenSOAPSecCert Structure Type Definition
     * 
     */
    typedef struct tagOpenSOAPSecCert OpenSOAPSecCert;

    /**
     * @typedef OpenSOAPSecCert* OpenSOAPSecCertPtr
     * @brief OpenSOAPSecCert Pointer Type Definition
     * 
     */
    typedef OpenSOAPSecCert* OpenSOAPSecCertPtr;


    /* CA */
    /**
     * @typedef struct tagOpenSOAPCARec OpenSOAPCARec
     * @brief OpenSOAPCARec Structure Type Definition
     * 
     */
    typedef struct tagOpenSOAPCARec OpenSOAPCARec;

    /**
     * @typedef OpenSOAPCARec* OpenSOAPCARecPtr
     * @brief OpenSOAPCARec Pointer Type Definition
     * 
     */
    typedef OpenSOAPCARec* OpenSOAPCARecPtr;

    /**
      * @fn int OpenSOAPSecGenerateRSAKeys(const unsigned char* szSeedPhrase, FILE* fpPrivKey, FILE* fpPubKey)
      * @brief Generate RSA Keys To Stream
      * @param
      *    szSeedPhrase const unsigned char * [in] ((|szSeedPhrase|)) Seed Phrase
      * @param
      *    fpPrivKey FILE * [in] ((|fpPrivKey|)) RSA Private Key File Stream
      * @param
      *    fpPubKey FILE * [in] ((|fpPubKey|)) RSA Public Key File Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecGenerateRSAKeys
    (const unsigned char* szSeedPhrase, /* (i)  Seed phrase */
     FILE*          fpPrivKey,    /* (i)  RSA Private Key File stream */
     FILE*          fpPubKey);    /* (i)  RSA Public Key File stream */

    /**
      * @fn int OpenSOAPSecGenerateRSAKeysToFile(const unsigned char* szSeedPhrase, const char* szPrivKeyFileName, const char* szPubKeyFileName)
      * @brief Generate RSA Keys To File
      * @param
      *    szSeedPhrase const unsigned char * [in] ((|szSeedPhrase|)) Seed Phrase
      * @param
      *    szPrivKeyFileName const char * [in] ((|szPrivKeyFileName\)) Private Key File Name
      * @param
      *    szPubKeyFileName const char * [in] ((|szPrivKeyFileName\)) Public Key File Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecGenerateRSAKeysToFile
    (const unsigned char* szSeedPhrase,      /* (i)  Seed phrase */
     const char*          szPrivKeyFileName, /* (i)  RSA Private Key File Name */
     const char*          szPubKeyFileName); /* (i)  RSA Public Key File Name */

    /**
      * @fn int OpenSOAPSecEncWithStream(OpenSOAPEnvelopePtr env, FILE* fpPubKey)
      * @brief Encrypt Envelope Using Public Key File Stream
      * @param
      *    env OpenSOAPEnvelopePtr [in, out] ((|env|)) Envelope
      * @param
      *    fpPubKey FILE * [in] ((|fpPubKey|)) Public Key File Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecEncWithStream
    (OpenSOAPEnvelopePtr env,       /* (i)  Envelope */
     FILE*               fpPubKey); /* (i)  Public Key File stream */

    /**
      * @fn int OpenSOAPSecEncWithFile(OpenSOAPEnvelopePtr env, const char* szPubKName)
      * @brief Encrypt Envelope Using Public Key File
      * @param
      *    env OpenSOAPEnvelopePtr [in, out] ((|env|)) Envelope
      * @param
      *    szPubKName const char * [in] ((|szPubKName|)) Public Key File Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecEncWithFile
    (OpenSOAPEnvelopePtr env,         /* (i)  Envelope */
     const char*         szPubKName); /* (i)  Public Key File Name */

    /**
      * @fn int OpenSOAPSecDecWithStream(OpenSOAPEnvelopePtr env, FILE* fpPrivKey)
      * @brief Decrypt Envelope Using Private Key File Stream
      * @param
      *    env OpenSOAPEnvelopePtr [in] ((|env|)) Envelope
      * @param
      *    fpPrivKey FILE * [in] ((|fpPrivKey|)) Private Key File Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecDecWithStream
    (OpenSOAPEnvelopePtr env,        /* (i)  Envelope */
     FILE*               fpPrivKey); /* (i)  Private Key File stream */

    /**
      * @fn int OpenSOAPSecDecWithFile(OpenSOAPEnvelopePtr env, const char* szPrivKName)
      * @brief Decrypt Envelope Using Private Key File
      * @param
      *    env OpenSOAPEnvelopePtr [in] ((|env|)) Envelope
      * @param
      *    szPrivKName const char * [in] ((|szPrivKName|)) Private Key File Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecDecWithFile
    (OpenSOAPEnvelopePtr env,          /* (i)  Envelope */
     const char*         szPrivKName); /* (i)  Private Key File Name */

    /**
      * @fn int OpenSOAPSecAddSignWithStream(OpenSOAPEnvelopePtr env, int iType, FILE* fpPrivKey, OpenSOAPStringPtr name)
      * @brief Add Signature to SOAP Message using PrivateKey File Stream
      * @param
      *    env OpenSOAPEnvelopePtr [in, out] ((|env|)) Envelope
      * @param
      *    iType int [in] ((|iType|)) Hash Type(OPENSOAP_HA_*)
      * @param
      *    fpPrivKey FILE * [in] ((|fpPrivKey|)) Private Key File Stream
      * @param
      *    name OpenSOAPStringPtr [in] ((|name|)) Signature Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecAddSignWithStream
    (OpenSOAPEnvelopePtr env,        /* (io) Envelope */
     int                 iType,      /* (i)  Hash Type(OPENSOAP_HA_*) */
     FILE*               fpPrivKey,  /* (i)  Private Key File stream */
     OpenSOAPStringPtr   name);      /* (i)  Name of Sign */

    /**
      * @fn int OpenSOAPSecAddSignWithFile(OpenSOAPEnvelopePtr env, int iType, const char* szPrivKName, OpenSOAPStringPtr name)
      * @brief Add Signature to SOAP Message using PrivateKey File
      * @param
      *    env OpenSOAPEnvelopePtr [in, out] ((|env|)) Envelope
      * @param
      *    iType int [in] ((|iType|)) Hash Type(OPENSOAP_HA_*)
      * @param
      *    szPrivKName const char * [in] ((|szPrivKName|)) Private Key File Name
      * @param
      *    name OpenSOAPStringPtr [in] ((|name|)) Signature Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecAddSignWithFile
    (OpenSOAPEnvelopePtr env,          /* (io) Envelope */
     int                 iType,        /* (i)  Hash Type(OPENSOAP_HA_*) */
     const char*         szPrivKName,  /* (i)  Private Key File Name */
     OpenSOAPStringPtr   name);        /* (i)  Name of Sign */

    /**
      * @fn int OpenSOAPSecCntSign(const OpenSOAPEnvelopePtr env, int* pnSig)
      * @brief Count Signatures
      * @param
      *    env const OpenSOAPEnvelopePtr [in] ((|env|)) OpenSOAP Envelope
      * @param
      *    pnSig int * [out] ((|pnSig|)) Signature Count
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCntSign
    (const OpenSOAPEnvelopePtr env,    /* (i)  Envelope */
     int*                      pnSig); /* (o)  Signature Count */

    /**
      * @fn int OpenSOAPSecGetSignedByList(OpenSOAPEnvelopePtr env, int nCntMax, OpenSOAPStringPtr list[], int* pnCntPacked)
      * @brief Get "SignedBy" List of Signatures
      * @param
      *    env OpenSOAPEnvelopePtr [in] ((|env|)) OpenSOAP Envelope
      * @param
      *    nCntMax int [in] ((|nCntMax|)) Maximum Count
      * @param
      *    list[] OpenSOAPStringPtr [in, out] ((|list[]|)) SignedBy List
      * @param
      *    pnCntPacked int * [out] ((|pnCntPacked|)) Packed Count
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecGetSignedByList
    (OpenSOAPEnvelopePtr env,          /* (i)  Envelope */
     int                 nCntMax,      /* (i)  Maximum Count */
     OpenSOAPStringPtr   list[],       /* (io) SignedBy List  */
     int*                pnCntPacked); /* (o)  Packed Count */

    /**
      * @fn int OpenSOAPSecVerifySignWithStream(OpenSOAPEnvelopePtr env, FILE* fpPubKey)
      * @brief Verify Signature using PublicKey File Stream
      * @param
      *    env OpenSOAPEnvelopePtr [in, out] ((|env|)) OpenSOAP Envelope
      * @param
      *    fpPubKey FILE * [in] ((|fpPubKey|)) Public Key File Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecVerifySignWithStream
    (OpenSOAPEnvelopePtr env,       /* (io) Envelope */
     FILE*               fpPubKey); /* (i)  Public Key File stream */

    /**
      * @fn int OpenSOAPSecVerifySignWithFile(OpenSOAPEnvelopePtr env, const char* szPubKName)
      * @brief Verify Signature using PublicKey File
      * @param
      *    env OpenSOAPEnvelopePtr [in, out] ((|env|)) OpenSOAP Envelope
      * @param
      *    szPubKName const char * [in] ((|szPubKName||) Public Key File Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecVerifySignWithFile
    (OpenSOAPEnvelopePtr env,         /* (io) Envelope */
     const char*         szPubKName); /* (i)  Public Key File Name */

    /**
      * @fn int OpenSOAPSecCertCreateWithStream(const char* szPublish, FILE* fpPrivKey, int iHashType, const OpenSOAPCARecPtr pRec, FILE* fpCert)
      * @brief Create Certificate from CA Record(stream)
      * @param
      *    szPublish const char * [in] ((|szPublish|)) Publisher's Name
      * @param
      *    fpPrivKey FILE * [in] ((|fpPrivKey|)) Private Key(Publisher
      * @param
      *    iHashType int [in] ((|iHashType|)) Hash type(OPENSOAP_HA_*)
      * @param
      *    pRec const OpenSOAPRecPtr [in] ((|pRec|)) CA Record
      * @param
      *    fpCert FILE * [out] ((|fpCert|)) Certificate File
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertCreateWithStream
    (const char*            szPublish, /* (i)  Publisher's Name */
     FILE*                  fpPrivKey, /* (i)  Private Key(Publisher) */
     int                    iHashType, /* (i)  Hash Type(OPENSOAP_HA_*) */
     const OpenSOAPCARecPtr pRec,      /* (i)  CA Record */
     FILE*                  fpCert);   /* (o)  Certificate File */

    /**
      * @fn int OpenSOAPSecCertCreateWithFile(const char* szPublish, const char* szPrivKeyFile, int iHashType, const OpenSOAPCARecPtr pRec, const char* szCertName)
      * @brief Create Certificate from CA Record(file)
      * @param
      *    szPublish const char * [in] ((|szPublish|)) Publisher's Name
      * @param
      *    szPrivKeyFile const char * [in] ((|szPrivKeyFile|)) Private Key File Name(Publisher)
      * @param
      *    iHashType int [in] ((|iHashType|)) Hash Type(OPENSOAP_HA_*)
      * @param
      *    pRec const OpenSOAPCARecPtr [in] ((|pRec|)) CA Record
      * @param
      *    szCertName const char * [in] ((|szCertName|)) Certificate File Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertCreateWithFile
    (const char*         szPublish,     /* (i)  Publisher's Name */
     const char*         szPrivKeyFile, /* (i)  PrivKey File Name (Publisher) */
     int                 iHashType,     /* (i)  Hash Type(OPENSOAP_HA_*) */
     const OpenSOAPCARecPtr pRec,          /* (i)  CA Record */
     const char*         szCertName);   /* (i)  Certificate File Name */

    /**
      * @fn int OpenSOAPSecCertLoadFromMem(size_t sizArea, const unsigned char* pucArea, OpenSOAPSecCertPtr* ppCert)
      * @brief Load Certificate From Memory
      * @param
      *    sizArea size_t [in] ((|sizArea|)) Size Of Input Area
      * @param
      *    pucArea const unsigned char * [in] ((|pucArea|)) Input Area
      * @param
      *    ppCert OpenSOAPSecCertPtr * [out] ((|ppCert|)) Certificate
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertLoadFromMem
    (size_t               sizArea,  /* (i)  Size of Input Area */
     const unsigned char* pucArea,  /* (i)  Input Area */
     OpenSOAPSecCertPtr*  ppCert);  /* (o)  Certificate */

    /**
      * @fn int OpenSOAPSecCertLoad(const char* szName, OpenSOAPSecCertPtr* ppCert)
      * @brief Load Certificate From File
      * @param
      *    szName const char * [in] ((|szName|)) File Name
      * @param
      *    ppCert OpenSOAPSecCertPtr * [out] ((|ppCert|)) Certificate
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertLoad
    (const char*          szName,   /* (i)  File Name */
     OpenSOAPSecCertPtr*  ppCert);  /* (o)  Certificate */

    /**
      * @fn int OpenSOAPSecCertFree(OpenSOAPSecCertPtr ppCert)
      * @brief Free Certificate
      * @param
      *    ppCert OpenSOAPSecCertPtr * [in] ((|ppCert|)) Certificate
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertFree
    (OpenSOAPSecCertPtr ppCert);

    /**
      * @fn int OpenSOAPSecCertVerifyWithStream(FILE* fpCert, FILE* fpPubKey)
      * @brief Verify Certificate (Stream)
      * @param
      *    fpCert FILE * [in] ((|fpCert|)) Certificate File Stream
      * @param
      *    fpPubKey FILE * [in] ((|fpPubKey|)) Public Key File Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertVerifyWithStream
    (FILE* fpCert,                  /* (i)  Certificate File Stream */
     FILE* fpPubKey);               /* (i)  Public Key File Stream */

    /**
      * @fn int OpenSOAPSecCertVerifyWithFile(const char* szCertName, const char* szPubKeyName)
      * @brief Verify Certificate (File)
      * @param
      *    szCertName const char * [in] ((|szCertName|)) Certificate File Name
      * @param
      *    szPubKeyName const char * [in] ((|szPubKeyName|)) Public Key File Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertVerifyWithFile
    (const char* szCertName,    /* (i)  Certificate File Name */
     const char* szPubKeyName); /* (i)  Public Key File Name */

    /**
      * @fn int OpenSOAPSecCertGetPublisherName(OpenSOAPSecCertPtr pCert, char** pszName)
      * @brief Get Name Of Publisher
      * @param
      *    pCert OpenSOAPSecCertPtr [in] ((|pCert|)) Certificate
      * @param
      *    pszName char ** [out] ((|pszName|)) Name (Internal Area)
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertGetPublisherName
    (OpenSOAPSecCertPtr pCert,      /* (i)   */
     char**             pszName);   /* (o)  Name (Internal Area) */

    /**
      * @fn int OpenSOAPSecCertGetSerialNo(OpenSOAPSecCertPtr pCert, unsigned long* pulSerial)
      * @brief Get Serial Number
      * @param
      *    pCert OpenSOAPSecCertPtr [in] ((|pCert|)) Certificate
      * @param
      *    pulSerial unsigned long * [out] ((|pulSerial|)) Serial Number
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertGetSerialNo
    (OpenSOAPSecCertPtr pCert,      /* (i)   */
     unsigned long*     pulSerial); /* (o)  Serial Number */

    /**
      * @fn int OpenSOAPSecCertGetOwnerName(OpenSOAPSecCertPtr pCert, char** pszName)
      * @brief Get Name of Owner
      * @param
      *    pCert OpenSOAPSecCertPtr [in] ((|pCert|)) Certificate
      * @param
      *    pszName char ** [out] ((|pszName|)) Name (Internal Area)
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertGetOwnerName
    (OpenSOAPSecCertPtr pCert,      /* (i)   */
     char**             pszName);   /* (o)  Name (Internal Area) */

    /**
      * @fn int OpenSOAPSecCertGetEndDate(OpenSOAPSecCertPtr pCert, char** pszDate)
      * @brief Get End Date
      * @param
      *    pCert OpenSOAPSecCertPtr [in] ((|pCert|)) Certificate
      * @param
      *    pszDate char ** [out] ((|pszDate|)) Date (Internal Area)
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertGetEndDate
    (OpenSOAPSecCertPtr pCert,      /* (i)   */
     char**             pszDate);   /* (o)  Date(Internal Area) */

    /**
      * @fn int OpenSOAPSecCertGetPubKey(OpenSOAPSecCertPtr pCert, const char* szSaveName)
      * @brief Get Public Key
      * @param
      *    pCert OpenSOAPSecCertPtr [in] ((|pCert|))
      * @param
      *    szSaveName const char * [in] ((|szSaveName|)) Public Key File Name
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCertGetPubKey
    (OpenSOAPSecCertPtr pCert,       /* (i)   */
     const char*        szSaveName); /* (i)  Public Key File Name */

    /**
      * @fn int OpenSOAPSecDecodeKeyFile(FILE* fp, unsigned long* pulLenOut, unsigned char** ppucDecode)
      * @brief Decode Key File
      * @param
      *    fp FILE * [in] ((|fp|)) File Stream
      * @param
      *    pulLenOut unsigned long * [out] ((|pulLenOut|)) Length of output data
      * @param
      *    ppucDecode unsigned char ** [out] ((|ppucDecode|)) Decoded String
      * @note
      *    After calling this function, call free() to free resources used by ppucDecode.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecDecodeKeyFile
    (FILE*           fp,          /* (i)  File Stream */
     unsigned long*  pulLenOut,   /* (o)  Length of output data */
     unsigned char** ppucDecode); /* (o)  Decoded String(call free() after use) */

    /**
      * @fn int OpenSOAPSecCABrowseRec(const OpenSOAPCARecPtr pRec, FILE* fpOut)
      * @brief Browse CA Record
      * @param
      *    pRec const OpenSOAPCARecPtr [in] ((|pRec|)) Record
      * @param
      *    fpOut FILE * [in] ((|fpOut|)) Output Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCABrowseRec
    (const OpenSOAPCARecPtr pRec,   /* (i)  Record */
     FILE*                  fpOut); /* (i)  Output Stream */

    /**
      * @fn int OpenSOAPSecCABrowse(FILE* fpOut)
      * @brief Browse CA Database
      * @param
      *    fpOut FILE * [in] ((|fpOut|)) Output Stream
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCABrowse
    (FILE* fpOut);                  /* (i)  Output Stream */

    /**
      * @fn int OpenSOAPSecCARegist(const char* szNameOwner, const char* szTermDate, size_t sizPubkey, const unsigned char* szPubKey, unsigned long* pulSerialNo)
      * @brief Regist Key To CA Databse
      * @param
      *    szNameOwner const char * [in] ((|szNameOwner|)) Owner's Name
      * @param
      *    szTermDate const char * [in] ((|szTermDate|)) Terminate Date ("YYYYMMDDHHMMSS")
      * @param
      *    sizPubkey size_t [in] ((|sizPubkey|)) Size of Public Key
      * @param
      *    szPubKey const unsigned char * [in] ((|szPubKey|)) Public Key
      * @param
      *    pulSerialNo unsigned long * [out] ((pulSerialNo|)) Serial Number
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCARegist
    (const char*    szNameOwner,    /* (i)  Owner's Name */
     const char*    szTermDate,     /* (i)  Terminate Date */
                                    /*      "YYYYMMDDHHMMSS" */
     size_t         sizPubkey,      /* (i)  Size of public key */
     const unsigned char* szPubKey, /* (i)  Public Key */
     unsigned long* pulSerialNo);   /* (o)  Serial Number */

    /**
      * @fn int OpenSOAPSecCAInvalidate(const char* szNameOwner, unsigned long ulSerial)
      * @brief Invalidate CA Record
      * @param
      *    szNameOwner const char * [in] ((|szNameOwner|)) Owner's Name
      * @param
      *    ulSerial unsigned long [in] ((|ulSerial|)) Serial Number
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCAInvalidate
    (const char*     szNameOwner,   /* (i)  Owner's Name */
     unsigned long   ulSerial);     /* (i)  Serial Number */

    /**
      * @fn int OpenSOAPSecCASearchRecords(const char* szNameOwner, int* pnRec, long** pplIdxs)
      * @brief Search Records
      * @param
      *    szNameOwner const char * [in] ((|szNameOwner|)) Owner's Name
      * @param
      *    pnRec int * [out] ((|pnRec|)) Record Count
      * @param
      *    pplIdxs lomg ** [out] ((|pplIdxs|)) Index Numbers
      * @note
      *    After calling this function, call free() to free resources used by pplIdxs.
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCASearchRecords
    (const char* szNameOwner, /* (i)  Owner's Name */
     int*        pnRec,       /* (o)  Record Count */
     long**      pplIdxs);    /* (o)  Index Numbers (call free() after use) */

    /**
      * @fn int OpenSOAPSecCASearchOneRecord(const char* szNameOwner, OpenSOAPCARecPtr* ppRec)
      * @brief Search Record(Longest Period)
      * @param
      *    szNameOwner const char * [in] ((|szNameOwner|)) Owner's Name
      * @param
      *    ppRec OpenSOAPCARecPtr * [out] ((|ppRec|)) Record
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCASearchOneRecord
    (const char*         szNameOwner, /* (i)  Owner's Name */
     OpenSOAPCARecPtr*   ppRec);      /* (o)  Record */

    /**
      * @fn int OpenSOAPSecCAGetRecord(long lIdx, OpenSOAPCARecPtr* ppRec)
      * @brief Get Record
      * @param
      *    lIdx long [in] ((|lIdx|)) Index
      * @param
      *    ppRec OpenSOAPCARecPtr * [out] ((|ppRec|)) Record
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCAGetRecord
    (long              lIdx,    /* (i)  Index */
     OpenSOAPCARecPtr* ppRec);  /* (o)  Record */

    /**
      * @fn int OpenSOAPSecCAFreeRecord(OpenSOAPCARecPtr pRec)
      * @brief Free Record
      * @param
      *    pRec OpenSOAPCARecPtr [in] ((|pRec|)) Record
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCAFreeRecord
    (OpenSOAPCARecPtr pRec);  /* (i)  Record */

    /**
      * @fn int OpenSOAPSecCARemoveRecord(unsigned long ulSerial)
      * @brief Remove CA Record
      * @param
      *    ulSerial unsigned long [in] ((|ulSerial|)) Serial Number
      * @return
      *    Error Code
      */
    int
    OPENSOAP_API
    OpenSOAPSecCARemoveRecord
    (unsigned long ulSerial);   /* (i)  Serial Number */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OpenSOAP_Security_H */
