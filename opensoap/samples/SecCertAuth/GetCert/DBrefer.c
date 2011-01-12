/*-----------------------------------------------------------------------------
 * $RCSfile: DBrefer.c,v $
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

#ifndef SERVICE_LOCALSTATEDIR
#  define SERVICE_LOCALSTATEDIR "/usr/local/opensoap/var/services/GetCert"
#endif
#ifndef OPENSOAP_SYSCONFDIR
#  define OPENSOAP_SYSCONFDIR "/usr/local/opensoap/etc"
#endif

/* Path to Certificate Authority Database file */
#define CA_DATABASE_FILE SERVICE_LOCALSTATEDIR "/CA.db"
/* Definition of environment variable name for CA Database */
#define CA_DATABASE_ENV  "OPENSOAP_CA_DATABASE"
/* Filename of Private Key of CA */
#define CA_PRIVKEY_FILE   OPENSOAP_SYSCONFDIR "/privKey.pem"
/* Name of Certificate Authority */
#define CA_PUBLISHER_NAME "OpenSOAP-SAMPLE-CA"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif /* _MSC_VER */

/***************************************************************************** 
    Function      : Display the Usage
    Return        : void
 ************************************************ Yuji Yamawaki 02.03.14 *****/
static void usage
(const char* szProg)
{
    fprintf(stderr, "Usage: %s OwnerName CertFileName\n", szProg);
}
/***************************************************************************** 
    Function      : main
    Return        : int(0: No Error, 1: Argument Error 2: Execution Error)
 ************************************************ Yuji Yamawaki 02.03.14 *****/
int main(int argc, char* argv[])
{
    int nRet = 0;
    int nLibRet;
    char szBuf[256];
    OpenSOAPCARecPtr pRec = NULL;
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
    /* Setup for CA Database filename */
    snprintf(szBuf, sizeof(szBuf), "%s=%s", CA_DATABASE_ENV, CA_DATABASE_FILE);
    if (putenv(szBuf) != 0) {
        return 2;
    }
    /* Record Search by Name */
    nLibRet = OpenSOAPSecCASearchOneRecord(argv[1], &pRec);
    if (OPENSOAP_FAILED(nLibRet)) {
        nRet = 2;
        goto FuncEnd;
    }
    /* Create Certification from the Record */
    nLibRet = OpenSOAPSecCertCreateWithFile(CA_PUBLISHER_NAME,
                                            CA_PRIVKEY_FILE,
                                            OPENSOAP_HA_SHA,
                                            pRec,
                                            argv[2]);
    if (OPENSOAP_FAILED(nLibRet)) {
        nRet = 2;
        goto FuncEnd;
    }
FuncEnd:
    if (pRec != NULL) {
        OpenSOAPSecCAFreeRecord(pRec);
    }
    return nRet;
}
