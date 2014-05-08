/*-----------------------------------------------------------------------------
 * $RCSfile: regist.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include <OpenSOAP/Security.h>

/***************************************************************************** 
    Function      : Display the Usage
    Return        : void
 ************************************************ Yuji Yamawaki 02.03.12 *****/
static void usage
(const char* szProg)
{
    fprintf(stderr, "Usage: %s RegistFileName\n", szProg);
}
/***************************************************************************** 
    Function      : Process Main
    Return        : int(0: No Error, Others: Error)
 ************************************************ Yuji Yamawaki 02.03.12 *****/
static int procMain
(const char* szFile) /* (i)  Filename for Registration */
{
    int   nRet = OPENSOAP_NO_ERROR;
    FILE* fp   = NULL;
    char  szName[OPENSOAP_CA_OWNER_LEN + 1];
    char  szDate[OPENSOAP_CERT_DATE_LEN + 2];
    int   nLen;
    unsigned long  ulLenKey;
    unsigned char* pucKey = NULL;
    unsigned long  ulSereal;

    /* Open Registration File */
    /* 1st line: Owner name, 2l: Expiry date, 3l-: Public Key (one, or
       more than one keys)  */
    fp = fopen(szFile, "rt");
    if (fp == NULL) {
        fprintf(stderr, "Cannot Open File[%s].\n", szFile);
        nRet = 1;
        goto FuncEnd;
    }
    /* Orderly Registration */
    for  (;;) {
        /* Owner name */
        if (fgets(szName, sizeof(szName), fp) == NULL) {
            break;
        }
        nLen = strlen(szName);
        if (nLen <= 1) {
            continue;  // Skip a empty line
        }
        szName[nLen - 1] = '\0';
        /* Date */
        if (fgets(szDate, sizeof(szDate), fp) == NULL) {
            break;
        }
        nLen = strlen(szDate);
        if (nLen != OPENSOAP_CERT_DATE_LEN + 1) {
            /* Invalid string length (including the line feed) */
            break;
        }
        szDate[nLen - 1] = '\0';
        /* Public Key to Decode */
        nRet = OpenSOAPSecDecodeKeyFile(fp, &ulLenKey, &pucKey);
        if (OPENSOAP_FAILED(nRet)) {
            fprintf(stderr, "OpenSOAPSecDecodeKeyFile() Error[0x%08x]. \n",
                    (unsigned int)nRet);
            goto FuncEnd;
        }
        /* Register */
        nRet = OpenSOAPSecCARegist(szName, szDate, ulLenKey, pucKey,
                                   &ulSereal);
        if (OPENSOAP_FAILED(nRet)) {
            fprintf(stderr, "OpenSOAPSecCARegist() Error[0x%08x]. \n",
                    (unsigned int)nRet);
            goto FuncEnd;
        }
        fprintf(stderr, "Regist OK. Serial:%lu\n", ulSereal);
        /* Clean-up */
        if (pucKey != NULL) {
            free(pucKey);
            pucKey = NULL;
        }
    }
FuncEnd:
    if (fp != NULL) {
        fclose(fp);
    }
    if (pucKey != NULL) {
        free(pucKey);
    }
    return nRet;
}
/***************************************************************************** 
    Function      : Main
    Return        : int
 ************************************************ Yuji Yamawaki 02.03.12 *****/
int main(int argc, char* argv[])
{
    int nRet = 0;
    /* Check the Argument number */
    if (argc < 2) {
        usage(argv[0]);
        nRet = 1;
        goto FuncEnd;
    }
    /* Process start */
    nRet = procMain(argv[1]);
    /* Display the results */
    nRet = OpenSOAPSecCABrowse(stdout);
    if (OPENSOAP_FAILED(nRet)) {
        fprintf(stderr, "OpenSOAPSecCABrowse() Error[0x%08x]. \n",
                (unsigned int)nRet);
        goto FuncEnd;
    }
FuncEnd:
    return nRet;
}
