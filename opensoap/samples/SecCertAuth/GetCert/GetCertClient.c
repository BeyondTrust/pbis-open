/*-----------------------------------------------------------------------------
 * $RCSfile: GetCertClient.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>
#include <OpenSOAP/Security.h>

#include <stdio.h>

#define RESULT_CERT_FILE_NAME   "result.cert"    /* Result Certificate */
#define RESULT_PUBKEY_FILE_NAME "pubKey_res.pem" /* Result Public Key */

#ifndef OPENSOAP_SYSCONFDIR
#  define OPENSOAP_SYSCONFDIR "/usr/local/opensoap/etc"
#endif

#define CA_PUBKEY_FILE OPENSOAP_SYSCONFDIR "/pubKey.pem"

static
void
PrintEnvelope(OpenSOAPEnvelopePtr env,
			  const char *label) {
	
	OpenSOAPByteArrayPtr envBuf = NULL;
	const unsigned char *envBeg = NULL;
	size_t envSz = 0;
	
	OpenSOAPByteArrayCreate(&envBuf);
	OpenSOAPEnvelopeGetCharEncodingString(env, "EUC-JP", envBuf);
	OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
	
	fprintf(stderr, "\n=== %s envelope begin ===\n", label);
	fwrite(envBeg, 1, envSz, stderr);
	fprintf(stderr, "\n=== %s envelope  end  ===\n", label);
	
	OpenSOAPByteArrayRelease(envBuf);
}
static
void
saveBinFile
(const char* szName,
 const void* pData,
 size_t sizLen) {
    FILE* fp = NULL;
    fp = fopen(szName, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Output File[%s] Open Error.\n", szName);
        return;
    }
    if (fwrite(pData, sizLen, 1, fp) != 1) {
        fprintf(stderr, "Output File[%s] Write Error.\n", szName);
        fclose(fp);
        return;
    }
    fclose(fp);
}
int
main(int argc,
	 char **argv) {
	OpenSOAPEnvelopePtr request = NULL;
	OpenSOAPEnvelopePtr response = NULL;
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPTransportPtr transport = NULL;
	OpenSOAPStringPtr ownerName = NULL; 
    long lRes;
    OpenSOAPByteArrayPtr barrayCert = NULL;
    const unsigned char* pucCert = NULL;
    size_t         sizCert;
    OpenSOAPSecCertPtr pCert = NULL;
    int nLibRet;

	/* initialize client */
	OpenSOAPInitialize(NULL);

	/* create request message */
	OpenSOAPEnvelopeCreateMB("1.1", NULL, &request);
	OpenSOAPEnvelopeAddBodyBlockMB(request, "GetCert", &body); 
	OpenSOAPBlockSetNamespaceMB(body,
                                "http://services.opensoap.jp/samples/GetCert/",
                                "m");
	OpenSOAPStringCreateWithMB(argv[1], &ownerName);
	OpenSOAPBlockSetChildValueMB(body, "OwnerName", "string", &ownerName);

	PrintEnvelope(request, "request");

	/* invoke service */
	OpenSOAPTransportCreate(&transport);
#if 1 /* direct client-service connect (using GetCertService.cgi) */
	OpenSOAPTransportSetURL(transport,
							"http://localhost/cgi-bin/GetCertService.cgi");
#else /* via OpenSOAP server (using HelloService)*/
	OpenSOAPTransportSetURL(transport,
							"http://localhost/cgi-bin/soapInterface.cgi");
#endif
	OpenSOAPTransportInvoke(transport, request, &response);
	OpenSOAPTransportRelease(transport);

	PrintEnvelope(response, "response");

	/* parse response message */
	OpenSOAPEnvelopeGetBodyBlockMB(response, "GetCertResponse", &body);
	OpenSOAPBlockGetChildValueMB(body, "Result", "int", &lRes);
    if (lRes != 0) {
        fprintf(stderr, "Certificate Get Failed.\n");
        return 0;
    }
    OpenSOAPByteArrayCreate(&barrayCert);
    OpenSOAPBlockGetChildValueMB(body, "Certificate", "base64Binary", &barrayCert);
    OpenSOAPByteArrayGetBeginSizeConst(barrayCert, &pucCert, &sizCert);

    /* Save Certicficate */
    saveBinFile(RESULT_CERT_FILE_NAME, pucCert, sizCert);
    /* Check Signatuire of Certicficate */
    nLibRet = OpenSOAPSecCertVerifyWithFile(RESULT_CERT_FILE_NAME,
                                            CA_PUBKEY_FILE);
    if (OPENSOAP_FAILED(nLibRet)) {
        fprintf(stderr, "Certificate's Signature Verify Failed.\n");
        return 0;
    } else {
        fprintf(stderr, "Certificate's Signature Verify OK.\n");
    }
    
    /* Save Public Key File */
    OpenSOAPSecCertLoadFromMem(sizCert, pucCert, &pCert);
    OpenSOAPSecCertGetPubKey(pCert, RESULT_PUBKEY_FILE_NAME);
    OpenSOAPSecCertFree(pCert);
    pCert = NULL;

	/* finalize client */
	OpenSOAPEnvelopeRelease(response);
	OpenSOAPEnvelopeRelease(request);
	OpenSOAPUltimate();
	
	return 0;
}
