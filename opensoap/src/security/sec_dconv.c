/*-----------------------------------------------------------------------------
 * $RCSfile: sec_dconv.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/ErrorCode.h>
#include "security_defs.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define BASE64_EMPTY_CHAR '='

/* Translation Table(Encode) */
static char s_acEnc[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P', /*  0-15 */
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f', /* 16-31 */
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v', /* 32-47 */
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'  /* 48-63 */
};
/* Translation Table(Decode) */
static unsigned char s_acDec[] = {
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*   0-  7 */
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*   8- 15 */
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  16- 23 */
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  24- 31 */
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, /*  32- 39 */
    0xff,0xff,0xff,0x3e,0xff,0xff,0xff,0x3f, /*  40- 47 */
    0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b, /*  48- 55 */
    0x3c,0x3d,0xff,0xff,0xff,0xff,0xff,0xff, /*  56- 63 */
    0xff,0x00,0x01,0x02,0x03,0x04,0x05,0x06, /*  64- 71 */
    0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e, /*  72- 79 */
    0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16, /*  80- 87 */
    0x17,0x18,0x19,0xff,0xff,0xff,0xff,0xff, /*  88- 95 */
    0xff,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20, /*  96-103 */
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28, /* 104-111 */
    0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30, /* 112-119 */
    0x31,0x32,0x33,0xff,0xff,0xff,0xff,0xff  /* 120-127 */
};

/***************************************************************************** 
    Function      : Encode
    Return        : int(Processed Data Count)
 ************************************************ Yuji Yamawaki 01.10.11 *****/
static int encodeValue
(unsigned char* szIn,           /* (i)  Input binary string */
 unsigned long  ulLenIn,        /* (i)  Length of input binary string */
 char           szOut[4])       /* (o)  Output Area */
{
    unsigned long ulVal = 0;
    unsigned long i;
    int           nPackCnt = 0;
    int           nIdx;
    /* Extend */
    for (i = 0; i < ulLenIn && i < 3; i++) {
        ulVal |= ((unsigned long)szIn[i] << ((2 - i) * 8));
        nPackCnt++;
    }
    /* Replace */
    for (i = 0; i < 4; i++) {
        nIdx = (int)((ulVal >> ((3 - i) * 6)) & 0x0000003f);
        szOut[i] = s_acEnc[nIdx];
    }
    /* Fill Empty Area */
    if (nPackCnt == 2) {
        szOut[3] = BASE64_EMPTY_CHAR;
    } else if (nPackCnt == 1) {
        szOut[3] = BASE64_EMPTY_CHAR;
        szOut[2] = BASE64_EMPTY_CHAR;
    }
    return nPackCnt;
}
/***************************************************************************** 
    Function      : Decode
    Return        : int(Processed Input Data Count)
 ************************************************ Yuji Yamawaki 01.10.11 *****/
static int decodeValue
(const char*   szIn,            /* (i)  Input String(base64) */
 unsigned long ulLenIn,         /* (i)  Input Data Length */
 unsigned char szOut[3],        /* (o)  Output Data Area */
 int*          pnOut)           /* (o)  Output Data Count */
{
    int           nEffIn;
    int           i;
    unsigned char ucVal;
    const char*   szCurIn;
    unsigned long ulVal;
    int           nDone = 0;
    nEffIn   = 0;    /* Effective Input Data Count */
    ulVal    = 0;
    /* Decode */
    for (szCurIn = szIn; (unsigned long)nDone < ulLenIn && nEffIn < 4;
         szCurIn++, nDone++) {
        ucVal = s_acDec[(int)(*szCurIn)];
        if (ucVal == 0xff)
            continue;       /* Skip Undefined Character */
        ulVal |= (unsigned long)ucVal << ((3 - nEffIn) * 6);
        nEffIn++;
    }
    /* Set to Output Area */
    for (i = 0; i < 3; i++) {
        szOut[i] = (unsigned char)((ulVal >> ((2 - i) * 8)) & 0x000000ff);
    }
    /* Set Output Data Count */
    if (nEffIn <= 1) {
        *pnOut = 0;
    } else if (nEffIn == 2) {
        *pnOut = 1;
    } else if (nEffIn == 3) {
        *pnOut = 2;
    } else {
        *pnOut = 3;
    }
    return nDone;
}
/***************************************************************************** 
    Function      : Encode to base64 ascii string
    Return        : int
 ************************************************ Yuji Yamawaki 01.10.11 *****/
int openSOAPSecEncByBase64
(unsigned char* szIn,           /* (i)  Input data */
 unsigned long  ulLenIn,        /* (i)  Length of input binary string */
 char**         pszEncStr)      /* (o)  Encoded String (call free() after use) */
{
    char*          szRet = NULL;
    unsigned long  ulRest;
    int            nDone;
    unsigned char* szCurIn;
    char*          szCurOut;
    unsigned long  ulLenOut;
    int            iRet = OPENSOAP_NO_ERROR;

    /* Check Arguments */
    if (szIn == NULL || ulLenIn > INT_MAX || ulLenIn == 0 || pszEncStr == NULL) {
        iRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Allocate Output Area */
    if (ulLenIn % 3 != 0) {
        ulLenOut = (ulLenIn / 3 + 1) * 4 + 1;
    } else {
        ulLenOut = (ulLenIn / 3) * 4 + 1;
    }
    if ((szRet = (char*)malloc(ulLenOut)) == NULL) {
        iRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Encode */
    ulRest = ulLenIn;
    szCurIn  = szIn;
    szCurOut = szRet;
    while (ulRest > 0) {
        nDone = encodeValue(szCurIn, ulRest, szCurOut);
        ulRest -= (unsigned long)nDone;
        szCurIn += nDone;
        szCurOut += 4;
    }

FuncEnd:
    if (OPENSOAP_FAILED(iRet)) {
        if (szRet != NULL) {
            free(szRet);
            szRet = NULL;
        }
    }
    szRet[ulLenOut - 1] = '\0';
    *pszEncStr = szRet;
    return iRet;
}

/***************************************************************************** 
    Function      : Decode base64 ascii string
    Return        : int
 ************************************************ Yuji Yamawaki 01.08.07 *****/
int openSOAPSecDecByBase64
(const char*     szIn,          /* (i)  Input string (base64) */
 unsigned long*  pulLenOut,     /* (o)  Length of output data */
 unsigned char** pszDecode)     /* (o)  Decoded string(call free() after use) */
{
    unsigned char* szRet = NULL;
    int            nProcIn, nProcOut;
    unsigned long  ulLenRest;
    const char*    szCurIn;
    unsigned char* szCurOut;
    unsigned long  ulLenIn;
    int            nRet = OPENSOAP_NO_ERROR;

    /* Check Arguments */
    if (szIn == NULL || pulLenOut == NULL || pszDecode == NULL) {
        nRet = OPENSOAP_PARAMETER_BADVALUE;
        goto FuncEnd;
    }
    /* Allocate Output Area */
    ulLenIn = strlen(szIn);
    szRet = (unsigned char*) malloc(ulLenIn);
    if (szRet == NULL) {
        nRet = OPENSOAP_MEM_BADALLOC;
        goto FuncEnd;
    }
    /* Decode */
    ulLenRest = ulLenIn;
    szCurIn   = szIn;
    szCurOut  = szRet;
    *pulLenOut = 0;
    while (ulLenRest > 0) {
        nProcIn = decodeValue(szCurIn, ulLenRest, szCurOut, &nProcOut);
        szCurIn   += nProcIn;
        ulLenRest -= (unsigned long)nProcIn;
        *pulLenOut += (unsigned long)nProcOut;
        szCurOut += nProcOut;
    }
FuncEnd:
    if (OPENSOAP_FAILED(nRet)) {
        if (szRet != NULL) {
            free(szRet);
            szRet = NULL;
        }
    }
    *pszDecode = szRet;
    return nRet;
}

/***************************************************************************** 
    Function      : Check if Base64 Character
    Return        : int
 ************************************************ Yuji Yamawaki 02.02.06 *****/
int openSOAPIsBase64Char
(char cVal,                     /* (i)  Value */
 int* pnYes)                    /* (o)  0: No, Others: Yes */
{
    *pnYes = 1;
    if (cVal < 0) {
        *pnYes = 0;
    } else if (s_acDec[(int)cVal] == 0xff) {
        *pnYes = 0;
    }
    return OPENSOAP_NO_ERROR;
}
