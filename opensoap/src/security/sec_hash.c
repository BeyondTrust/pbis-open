/*-----------------------------------------------------------------------------
 * $RCSfile: sec_hash.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/Security.h>
#include "security_defs.h"
#include <stdlib.h>
#include <openssl/md5.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>

/***************************************************************************** 
    Function      : SHA-1 Hash
    Return        : unsigned char*(Hash Data, NULL On Error)
                    <call free() after use>
 ************************************************ Yuji Yamawaki 01.09.10 *****/
static unsigned char* makeHashSHA
(const unsigned char* szIn,      /* (i)  Input string */
 unsigned long        ulLenIn,   /* (i)  Length of Input String */
 unsigned long*       pulLenOut) /* (o)  Length of Hashed Data */
{
    unsigned char* szNew = NULL;
    int            iHasErr = 0;
    /* Create Result Area */
    *pulLenOut = SHA_DIGEST_LENGTH;
    szNew = (unsigned char*) malloc(*pulLenOut);
    if (szNew == NULL) {
        iHasErr = 1;
        goto FuncEnd;
    }
    /* Hash */
    SHA1(szIn, ulLenIn, szNew);
FuncEnd:
    if (iHasErr != 0) {
        if (szNew != NULL) {
            free(szNew);
            szNew = NULL;
        }
    }
    return szNew;
}
#if 0 /* There's no MD2 */
/***************************************************************************** 
    Function      : MD2 Hash
    Return        : unsigned char*(Hash Data, NULL On Error)
                    <call free() after use>
 ************************************************ Yuji Yamawaki 01.09.10 *****/
static unsigned char* makeHashMD2
(const unsigned char* szIn,      /* (i)  Input string */
 unsigned long        ulLenIn,   /* (i)  Length of Input String */
 unsigned long*       pulLenOut) /* (o)  Length of Hashed Data */
{
    unsigned char* szNew = NULL;
    int            iHasErr = 0;
    /* Create Result Area */
    *pulLenOut = MD2_DIGEST_LENGTH;
    szNew = (unsigned char*) malloc(*pulLenOut);
    if (szNew == NULL) {
        iHasErr = 1;
        goto FuncEnd;
    }
    /* Hash */
    MD2(szIn, ulLenIn, szNew);
FuncEnd:
    if (iHasErr != 0) {
        if (szNew != NULL) {
            free(szNew);
            szNew = NULL;
        }
    }
    return szNew;
}
#endif
#if 0 /* There's no MD4 */
/***************************************************************************** 
    Function      : MD4 Hash
    Return        : unsigned char*(Hash Data, NULL On Error)
                    <call free() after use>
 ************************************************ Yuji Yamawaki 01.09.10 *****/
static unsigned char* makeHashMD4
(const unsigned char* szIn,      /* (i)  Input string */
 unsigned long        ulLenIn,   /* (i)  Length of Input String */
 unsigned long*       pulLenOut) /* (o)  Length of Hashed Data */
{
    unsigned char* szNew = NULL;
    int            iHasErr = 0;
    /* Create Result Area */
    *pulLenOut = MD4_DIGEST_LENGTH;
    szNew = (unsigned char*) malloc(*pulLenOut);
    if (szNew == NULL) {
        iHasErr = 1;
        goto FuncEnd;
    }
    /* Hash */
    MD4(szIn, ulLenIn, szNew);
FuncEnd:
    if (iHasErr != 0) {
        if (szNew != NULL) {
            free(szNew);
            szNew = NULL;
        }
    }
    return szNew;
}
#endif
/***************************************************************************** 
    Function      : MD5 Hash
    Return        : unsigned char*(Hash Data, NULL On Error)
                    <call free() after use>
 ************************************************ Yuji Yamawaki 01.09.10 *****/
static unsigned char* makeHashMD5
(const unsigned char* szIn,      /* (i)  Input string */
 unsigned long        ulLenIn,   /* (i)  Length of Input String */
 unsigned long*       pulLenOut) /* (o)  Length of Hashed Data */
{
    unsigned char* szNew = NULL;
    int            iHasErr = 0;
    /* Create Result Area */
    *pulLenOut = MD5_DIGEST_LENGTH;
    szNew = (unsigned char*) malloc(*pulLenOut);
    if (szNew == NULL) {
        iHasErr = 1;
        goto FuncEnd;
    }
    /* Hash */
    MD5(szIn, ulLenIn, szNew);
FuncEnd:
    if (iHasErr != 0) {
        if (szNew != NULL) {
            free(szNew);
            szNew = NULL;
        }
    }
    return szNew;
}
#if 0 /* There's no MDC2 */
/***************************************************************************** 
    Function      : MDC2 Hash
    Return        : unsigned char*(Hash Data, NULL On Error)
                    <call free() after use>
 ************************************************ Yuji Yamawaki 01.09.10 *****/
static unsigned char* makeHashMDC2
(const unsigned char* szIn,      /* (i)  Input string */
 unsigned long        ulLenIn,   /* (i)  Length of Input String */
 unsigned long*       pulLenOut) /* (o)  Length of Hashed Data */
{
    unsigned char* szNew = NULL;
    int            iHasErr = 0;
    /* Create Result Area */
    *pulLenOut = MDC2_DIGEST_LENGTH;
    szNew = (unsigned char*) malloc(*pulLenOut);
    if (szNew == NULL) {
        iHasErr = 1;
        goto FuncEnd;
    }
    /* Hash */
    MDC2(szIn, ulLenIn, szNew);
FuncEnd:
    if (iHasErr != 0) {
        if (szNew != NULL) {
            free(szNew);
            szNew = NULL;
        }
    }
    return szNew;
}
#endif
/***************************************************************************** 
    Function      : RIPEMD160 Hash
    Return        : unsigned char*(Hash Data, NULL On Error)
                    <call free() after use>
 ************************************************ Yuji Yamawaki 01.09.10 *****/
static unsigned char* makeHashRIPEMD160
(const unsigned char* szIn,      /* (i)  Input string */
 unsigned long        ulLenIn,   /* (i)  Length of Input String */
 unsigned long*       pulLenOut) /* (o)  Length of Hashed Data */
{
    unsigned char* szNew = NULL;
    int            iHasErr = 0;
    /* Create Result Area */
    *pulLenOut = RIPEMD160_DIGEST_LENGTH;
    szNew = (unsigned char*) malloc(*pulLenOut);
    if (szNew == NULL) {
        iHasErr = 1;
        goto FuncEnd;
    }
    /* Hash */
    RIPEMD160(szIn, ulLenIn, szNew);
FuncEnd:
    if (iHasErr != 0) {
        if (szNew != NULL) {
            free(szNew);
            szNew = NULL;
        }
    }
    return szNew;
}
/***************************************************************************** 
    Function      : Make Hash
    Return        : int
 ************************************************ Yuji Yamawaki 01.09.10 *****/
int openSOAPSecMakeHash
(int                  iType,     /* (i)  Hash Type(OPENSOAP_HA_*) */
 const unsigned char* szIn,      /* (i)  Input string */
 unsigned long        ulLenIn,   /* (i)  Length of Input String */
 unsigned long*       pulLenOut, /* (o)  Length of Hashed Data */
 unsigned char**      pszHash)   /* (o)  Hash String */
{
    static struct {
        int              iType;
        unsigned char* (*pFn)(const unsigned char* szIn,
                              unsigned long        ulLenIn,
                              unsigned long*       plLenOut);
    } aFnTbl[] = {
/*         {OPENSOAP_HA_MD2,    makeHashMD2}, */
/*         {OPENSOAP_HA_MD4,    makeHashMD4}, */
        {OPENSOAP_HA_MD5,    makeHashMD5},
/*         {OPENSOAP_HA_MDC2,   makeHashMDC2}, */
        {OPENSOAP_HA_RIPEMD, makeHashRIPEMD160},
        {OPENSOAP_HA_SHA,    makeHashSHA}
    };
    int   i;
    if (szIn == NULL || ulLenIn == 0 || pulLenOut == NULL || pszHash == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    for (i = 0; i < DIM(aFnTbl); i++) {
        if (aFnTbl[i].iType == iType) {
            *pszHash = (aFnTbl[i].pFn)(szIn, ulLenIn, pulLenOut);
            if (*pszHash == NULL) {
                return OPENSOAP_MEM_BADALLOC;
            }
            return OPENSOAP_NO_ERROR; /* Normal End */
        }
    }
    return OPENSOAP_PARAMETER_BADVALUE; /* Digest Method Not Found */
}
