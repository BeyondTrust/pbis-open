/*-----------------------------------------------------------------------------
 * $RCSfile: sec_keyfile.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/Security.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************
  File Name is specified by environment variable "OPENSOAP_PUBKDEF_NAME"

  <<EXAMPLE>>
====<begin>====
:Yuji Yamawaki
-----BEGIN RSA PUBLIC KEY-----
MIGHAoGBANUW2uodaSQQ3BO7R9NOxh49ymcok/uyNZxRL2C5lCPeyapzN1xcGN7S
OPKv3vguOU6macq9qYWEEjFmB9CH0PNJu+axoX3KT6tGbrAhwqrC8gdsVYIm78gb
ePj9S1JnfaRUmyDcISX+lSlOFKor91oFHOmPN41Fmkg+fUTJnxONAgED
-----END RSA PUBLIC KEY-----
:http://www.opensoap.jp/server1
-----BEGIN RSA PUBLIC KEY-----
MIGHAoGBAL14yqOKM0E5S8/HHFIs3YCMtKBF0Rl+tOgj/fDwjWLZ1zw/BfceVdK9
gmpRtg0USg3RcXZsl/tvOtYxl3Qab+IyiNxX84j09j6QsFbkaZt5if7UZaGwM3xC
KYC8FF41OY+z8AfK1w0NbzC572phThHWvf0yymct0772HaX7bXIHAgED
-----END RSA PUBLIC KEY-----
====<end>====

Two Names are defined("Yuji Yamawaki", "http://www.opensoap.jp/server1").
 *****************************************************************************/

#define PUBK_MAXLLEN   (1024)                  /* Maximum Line Length */
#define PUBK_ENVFILE   "OPENSOAP_PUBKDEF_NAME" /* environment variable */
#define PUBK_NAME_HEAD "Name::"                /* Name Line Identifier String */

/***************************************************************************** 
    Function      : Open Key Database File
    Return        : FILE* (NULL on error)
 ************************************************ Yuji Yamawaki 01.09.13 *****/
static FILE* openKeyDatabaseFile
(const char* szMode)
{
    char* szFile;
    /* Get File Name from Environment Variable */
    if ((szFile = getenv(PUBK_ENVFILE)) == NULL) {
        /* Not Defined */
        return NULL;
    }
    /* Open */
    return fopen(szFile, szMode);
}
/***************************************************************************** 
    Function      : Get String(Removes Tail Newline)
    Return        : char* (String area, NULL on error or eof)
 ************************************************ Yuji Yamawaki 01.09.13 *****/
static char* getString
(char* szBuf,
 int   nLenMax,
 FILE* fp)
{
    char* szTailNL;
    if (fgets(szBuf, nLenMax, fp) == NULL)
        return NULL;
    /* Remove Tail Newline */
    if ((szTailNL = strchr(szBuf, '\n')) != NULL) {
        *szTailNL = '\0';
    }
    return szBuf;
}
/***************************************************************************** 
    Function      : Find Specifiled Name String
                    (If Found, FP is located top of next line)
    Return        : int (0 : Not Found, 1: Found)
 ************************************************ Yuji Yamawaki 01.09.13 *****/
static int findName
(FILE*       fp,                /* (i)  Input File Stream */
 const char* szName)            /* (i)  Name String */
{
    char szBuf[PUBK_MAXLLEN];
    while (getString(szBuf, PUBK_MAXLLEN, fp) != NULL) {
        if (szBuf[0] == ':') {
            /* Name Line Found */
            if (strcmp(szName, szBuf + 1) == 0) {
                /* matched!! */
                return 1;
            }
        }
    }
    /* Not Found */
    return 0;
}
/***************************************************************************** 
    Function      : Prepare To Write Public Key
    Return        : int
 ************************************************ Yuji Yamawaki 01.09.13 *****/
int
openSOAPSecWriteSupPubKeyFile
(const char* szName,            /* (i)  Name Identifier */
 FILE**      pFp)               /* (o)  File */
{
    /* Check Arguments */
    if (szName == NULL || pFp == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    if (strlen(szName) <= 0) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Key Database File */
    if ((*pFp = openKeyDatabaseFile("a+")) == NULL) {
        return OPENSOAP_FILEOPEN_ERROR;
    }
    /* Check Name is Duplicated */
    rewind(*pFp);
    if (findName(*pFp, szName)) {
        fclose(*pFp);           /* The Name is Already Defined */
        *pFp = NULL;
        return OPENSOAP_IO_WRITE_ERROR;
    }
    /* Write Name */
    if (fprintf(*pFp, ":%s\n", szName) < 0) {
        fclose(*pFp);
        *pFp = NULL;
        return OPENSOAP_IO_WRITE_ERROR;
    }
    /* Ready to write! */
    return OPENSOAP_NO_ERROR;
}
/***************************************************************************** 
    Function      : Prepare to Read Public Key
    Return        : int
 ************************************************ Yuji Yamawaki 01.09.13 *****/
int
OpenSOAPSecReadSupPubKeyFile
(const char* szName,            /* (i)  Name Identifier */
 FILE**      pFp)               /* (o)  File */
{
    /* Check Arguments */
    if (szName == NULL || pFp == NULL) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    if (strlen(szName) <= 0) {
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    /* Open Key Database File */
    if ((*pFp = openKeyDatabaseFile("r")) == NULL) {
        return OPENSOAP_FILEOPEN_ERROR; /* File Not Found */
    }
    /* Find Name */
    if (!findName(*pFp, szName)) {
        fclose(*pFp);
        *pFp = NULL;
        return OPENSOAP_IO_READ_ERROR;
    }
    /* Ready To Read! */
    return OPENSOAP_NO_ERROR;
}
