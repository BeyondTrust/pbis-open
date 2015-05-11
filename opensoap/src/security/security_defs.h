/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: security_defs.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(_SECURITY_DEFS_H_)
#define _SECURITY_DEFS_H_

#include <stdio.h>
#include <OpenSOAP/Security.h>

#define OPENSOAP_CA_DATABASE_ENV     "OPENSOAP_CA_DATABASE"
#define OPENSOAP_CA_DATABASE_BLKSIZE (1320)

#define OPENSOAP_PUBKEY_HEAD_STRING "-----BEGIN RSA PUBLIC KEY-----\n"
#define OPENSOAP_PUBKEY_TAIL_STRING "-----END RSA PUBLIC KEY-----\n"

#if defined  __cplusplus
extern "C" {
#endif

/* Certicicate Authority Database Record */
struct tagOpenSOAPCARec {
    unsigned long  ulSerial;      /* Serial Number */
    unsigned char  ucEff;         /* Effective Record?(0:No, Others:Yes) */
    char           szOwner[OPENSOAP_CA_OWNER_LEN];  /* Owner's Name */
    char           szStart[OPENSOAP_CERT_DATE_LEN]; /* Start Date */
    char           szEnd[OPENSOAP_CERT_DATE_LEN];   /* End Date */
    unsigned short usKeyLen;      /* Length of Public Key */
    unsigned char  ucKey[256];    /* Public Key */
};

/* Certificate */
    struct tagOpenSOAPSecCert {
        unsigned char  aucVersion[4]; /* Version ID */ 
        char*          szPublisher;   /* Name of Publisher */
        int            iHashType;     /* Hash Type (OPENSOAP_HA_*) */
        int            nLenSign;      /* Sign Length */
        unsigned char* pucSign;       /* Sign */
        unsigned long  ulSerial;      /* Serial Number */
        char*          szOwner;       /* Name Of Owner */
        char           szBeginDate[OPENSOAP_CERT_DATE_LEN]; /* Begin Date */
        char           szEndDate[OPENSOAP_CERT_DATE_LEN];   /* End Date */
        int            nLenPubKey;    /* Length of public key */
        unsigned char* pucPubKey;     /* Public Key(Binary) */
    };

/* Utility */
#if defined(DIM)
# undef DIM
#endif
#define DIM(arr)   ((int) (sizeof(arr) / sizeof((arr)[0])))

/* Encode to base64 ascii string */
int
openSOAPSecEncByBase64
(unsigned char* szIn,           /* (i)  Input data */
 unsigned long  ulLenIn,        /* (i)  Length of input binary string */
 char**         pszEncStr);     /* (o)  Encoded String (call free() after use) */

/* Decode base64 ascii string */
int
openSOAPSecDecByBase64
(const char*     szIn,          /* (i)  Input string (base64) */
 unsigned long*  pulLenOut,     /* (o)  Length of output data */
 unsigned char** pszDecode);    /* (o)  Decoded string(call free() after use) */

/* Make Hash */
int
openSOAPSecMakeHash
(int                  iType,      /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szIn,       /* (i)  Input string */
 unsigned long        ulLenIn,    /* (i)  Length of Input String */
 unsigned long*       pulLenOut,  /* (o)  Length of Hashed Data */
 unsigned char**      pszHash);   /* (o)  Hash String */

/* Encrypt Using Public Key */
int
openSOAPSecEncData
(const unsigned char* szData,     /* (i)  Data to be Encrypted */
 unsigned long        ulDataLen,  /* (i)  Length of Data */
 FILE*                fpPubKey,   /* (i)  Public Key File stream */
 char**               pszRet);    /* (o)  Encrypted data<base64> */
                                  /*      (call free() after use)  */

/* Encrypt Using Public Key(from Public Key File) */
int
openSOAPSecEncDataFromFile
(const unsigned char* szData,     /* (i)  Data to be Encrypted */
 unsigned long        ulDataLen,  /* (i)  Length of Data */
 const char*          szPubKeyFileName,  /* (i)  Public Key File Name */
 char**               pszRet);   /* (o)  Encrypted data<base64> */
                                 /*      (call free() after use)  */

/* Decrypt Using Private Key */
int 
openSOAPSecDecData
(char*           szIn,       /* (i)  Data to be Decrypted(base64 string) */
 FILE*           fpPrivKey,  /* (i)  Private Key File stream */
 unsigned long*  pulResLen,  /* (o)  Crypted Data Length */
 unsigned char** pszRet);    /* (o)  Decrypted data */
                             /*      (call free() after use)  */

/* Decrypt Using Private Key(from Private Key File) */
int
openSOAPSecDecDataFromFile
(char*           szIn,       /* (i)  Data to be Crypted */
 const char*     szPrivKeyFileName, /* (i)  Private Key File Name */
 unsigned long*  pulResLen,  /* (o)  Crypted Data Length */
 unsigned char** pszRet);    /* (o)  Decrypted data */
                             /*      (call free() after use)  */

/* Prepare To Write Public Key */
int
openSOAPSecWriteSupPubKeyFile
(const char* szName,            /* (i)  Name Identifier */
 FILE**      pFp);              /* (o)  File */

/* Prepare To Read Public Key */
int
openSOAPSecReadSupPubKeyFile
(const char* szName,            /* (i)  Name Identifier */
 FILE**      pFp);              /* (o)  File */

/* Generate Digital Signature(RSA) <Binary> */
int
openSOAPSecMakeRSABinSign
(int                  iType,     /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szData,    /* (i)  Data to be signed */
 unsigned long        ulDataLen, /* (i)  Length of Data */
 FILE*                fpPrivKey, /* (i)  RSA Private Key File stream */
 unsigned char**      pszDSig,   /* (o)  Digital Signature */
 int*                 pnSigLen,  /* (o)  Signature Length */
 unsigned char**      pszDigest, /* (o)  Digest Value(NULL Ok) */
 int*                 pnDigestLen); /* (o)  Digest Length(NULL Ok) */

/* Generate Digital Signature(RSA) */
int
openSOAPSecMakeRSASign
(int                  iType,      /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szData,     /* (i)  Data to be signed */
 unsigned long        ulDataLen,  /* (i)  Length of Data */
 FILE*                fpPrivKey,  /* (i)  RSA Private Key File stream */
 char**               pszDigest,  /* (o)  Digest Value<base64> */
 char**               pszDSig);   /* (o)  Digital Signature<base64> */

/* Generate Digital Signature(from Private Key File)(RSA) */
int
openSOAPSecMakeRSASignFromFile
(int                  iType,       /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szData,      /* (i)  Data to be signed */
 unsigned long        ulDataLen,   /* (i)  Length of Data */
 const char*          szPrivKeyFileName, /* (i)  RSA Private Key File Name */
 char**               pszDigest,   /* (o)  Digest Value<base64> */
 char**               pszDSig);    /* (o)  Digital Signature<base64> */

/* Verify Digital Signature(RSA) <Binary> */
int
openSOAPSecVerifyRSASignBin
(int                  iType,          /* (i)  Hash Type(OPENSOAP_HA_*) */
 unsigned char*       szIn,           /* (i)  Signature */
 unsigned long        ulInSize,       /* (i)  Signature Size */
 const unsigned char* szData,         /* (i)  Original Data */
 unsigned long        ulDataSize,     /* (i)  Original Data Size */
 FILE*                fpPubKey);      /* (i)  RSA Public Key File Stream */

/* Verify Digital Signature(RSA) */
int
openSOAPSecVerifyRSASign
(int                  iType,          /* (i)  Hash Type(OPENSOAP_HA_*) */
 const char*          szIn,           /* (i)  Signature(Base64 string) */
 const unsigned char* szData,         /* (i)  Original Data */
 unsigned long        ulDataSize,     /* (i)  Original Data Size */
 FILE*                fpPubKey);      /* (i)  Public Key File Stream */

/* Verify Digital Signature(Using Public Key File )(RSA) */
int
openSOAPSecVerifyRSASignFromFile
(int                  iType,             /* (i)  Hash Type(OPENSOAP_HA_*) */
 const char*          szIn,              /* (i)  Signature(Base64 string) */
 const unsigned char* szData,            /* (i)  Original Data */
 unsigned long        ulDataSize ,       /* (i)  Original Data Size */
 const char*          szPubKeyFileName); /* (i)  RSA Public Key File Name */

/* Check if Base64 Character */
int
openSOAPIsBase64Char
(char cVal,                     /* (i)  Value */
 int* pnYes);                   /* (o)  0: No, Others: Yes */

/* Open CA Datanase File */
int openSOAPSecCAOpenDatabase
(FILE** pFpDb);                 /* (o)  CA Database Handle */

#if defined  __cplusplus
}
#endif
#endif
