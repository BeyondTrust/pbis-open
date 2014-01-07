/*-----------------------------------------------------------------------------
 * $RCSfile: GetCertService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>
#include <OpenSOAP/Security.h>

#include <stdio.h>
#include <string.h>

#if !defined(CONNECT_TYPE)
# define CONNECT_TYPE "stdio"
#endif /* CONNECT_TYPE */

#ifndef SERVICE_DIR
#  define SERVICE_DIR "/usr/local/opensoap/services/GetCert"
#endif

/* DB_refer Directory */
#define REFER_PROGRAM_DIR SERVICE_DIR

#ifdef _MSC_VER
# include <io.h>
# define mkstemp _mktemp
#else
# include <unistd.h>
#endif /* _MSC_VER */


static
unsigned char*
loadFile
(const char* szName,
 size_t*     pSizFile)
{
    FILE*  fp;
    unsigned char* pucNew = NULL;
    fp = fopen(szName, "rb");
    if (fp == NULL) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    *pSizFile = (size_t)ftell(fp);
    rewind(fp);
    if (*pSizFile == 0) {
        return NULL;
    }
    pucNew = (unsigned char*)malloc(*pSizFile);
    if (fread(pucNew, *pSizFile, 1, fp) != 1) {
        free(pucNew);
        pucNew = NULL;
    }
    fclose(fp);
    return pucNew;
}

static
int
ServiceFunc(OpenSOAPEnvelopePtr request,
			OpenSOAPEnvelopePtr *response,
			void *opt) {
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPStringPtr ownerName = NULL;
    char* ownerNameString = NULL;
    size_t ownerNameLen;
#ifdef _MSC_VER
	char* mktempResult = NULL;
#else
    int    nFdCert;
#endif
    char   szTmpFileName[256];
    char   szCmd[1024];
    const char* szNameTempl = "/tmp/OpenSOAP_Cert_XXXXXX";
    long           lResult;
    unsigned char* pucCert = NULL;
    size_t         sizCert;
    OpenSOAPByteArrayPtr barray = NULL;

	/* parse request message */
	OpenSOAPEnvelopeGetBodyBlockMB(request, "GetCert", &body);
	OpenSOAPBlockGetChildValueMB(body, "OwnerName", "string", &ownerName);
	OpenSOAPStringGetLengthMB(ownerName, &ownerNameLen);
    ownerNameString = (char*)malloc(ownerNameLen + 1);
    OpenSOAPStringGetStringMB(ownerName, &ownerNameLen, ownerNameString);

    /* make temporary name to save certificate */
    strcpy(szTmpFileName, szNameTempl);
#ifdef _MSC_VER
	mktempResult = mktemp(szTmpFileName);
	strcpy(szTmpFileName, mktempResult);
#else
    nFdCert = mkstemp(szTmpFileName);
    close(nFdCert);
#endif
    /* make command line */
    sprintf(szCmd, "%s/CA_Refer \"%s\" %s",
            REFER_PROGRAM_DIR, ownerNameString, szTmpFileName);
    free(ownerNameString);
    ownerNameString = NULL;
    /* refer databse */
    lResult = (long)system(szCmd);
    if (lResult == 0) {
        /* Load Result From File */
        pucCert = loadFile(szTmpFileName, &sizCert);
        if (pucCert == NULL) {
            lResult = 2;
        }
    }
    remove(szTmpFileName);
    {
        char szBuf[256];
        sprintf(szBuf, "Return Val = %ld\n", lResult);
    }
    /* make responce */
	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response, "GetCertResponse", &body);
	OpenSOAPBlockSetNamespaceMB(body,
                                "http://services.opensoap.jp/samples/GetCert/",
                                "m");
	OpenSOAPBlockSetChildValueMB(body, "Result", "int", &lResult);
    if (lResult == 0) {
        OpenSOAPByteArrayCreateWithData(pucCert, sizCert, &barray);
        OpenSOAPBlockSetChildValueMB(body, "Certificate", "base64Binary", &barray);
        free(pucCert);
    }
    return 0;
}
int
main(void) {
	OpenSOAPServicePtr service = NULL;

	OpenSOAPInitialize(NULL);

	OpenSOAPServiceCreateMB(&service, "GetCertService", CONNECT_TYPE, 0);
	
	OpenSOAPServiceRegisterMB(service, "GetCert", ServiceFunc, NULL);
	
	OpenSOAPServiceRun(service);
	
	OpenSOAPServiceRelease(service);
	
	OpenSOAPUltimate();
	
	return 0;

}
