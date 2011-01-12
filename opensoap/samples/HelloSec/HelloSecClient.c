/*-----------------------------------------------------------------------------
 * $RCSfile: HelloSecClient.c,v $
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

static
void
PrintEnvelope(OpenSOAPEnvelopePtr env,
			  const char *label) {
	
	OpenSOAPByteArrayPtr envBuf = NULL;
	const unsigned char *envBeg = NULL;
	size_t envSz = 0;
	
	OpenSOAPByteArrayCreate(&envBuf);
	OpenSOAPEnvelopeGetCharEncodingString(env, NULL, envBuf);
	OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
	
	fprintf(stderr, "\n=== %s envelope begin ===\n", label);
	fwrite(envBeg, 1, envSz, stderr);
	fprintf(stderr, "=== %s envelope  end  ===\n", label);
	
	OpenSOAPByteArrayRelease(envBuf);
}

int
main(int argc,
	 char **argv) {
	
#if 1 /* direct client-service connect (using HelloSecService.cgi) */
 	char* endpointURL = "http://localhost/cgi-bin/HelloSecService.cgi";
#else /* via OpenSOAP server (using HelloService)*/
	char* endpointURL = "http://localhost/cgi-bin/soapInterface.cgi";
#endif
	
	OpenSOAPEnvelopePtr request = NULL;
	OpenSOAPEnvelopePtr response = NULL;
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPTransportPtr transport = NULL;
	
	OpenSOAPStringPtr myName = NULL;
	
	OpenSOAPStringPtr reply = NULL;
	size_t replyLen = 0;
	char *replyString = NULL;
	
	OpenSOAPXMLElmPtr elem = NULL;
	OpenSOAPXMLAttrPtr attr = NULL;
	int encryption = 1;
	int result = OPENSOAP_NO_ERROR;
	
	fprintf(stderr, "\n***** start HelloSecClient *****\n");
	
	/* initialize input arameter */
	
	if (argc > 2) {
		endpointURL = argv[2];
		fprintf(stderr, "\nEndpoint URL: %s\n", endpointURL);
	}
	
	/* initialize client */
	
	OpenSOAPInitialize(NULL);
	
	/* create request message */
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, &request);
	OpenSOAPEnvelopeAddBodyBlockMB(request, "Hello", &body); 
	OpenSOAPBlockSetNamespaceMB(
		body,
		"http://services.opensoap.jp/samples/HelloSec/",
		"m");
	OpenSOAPStringCreateWithMB(argv[1], &myName);
	OpenSOAPBlockSetChildValueMB(body, "MyName", "string", &myName);
	
	PrintEnvelope(request, "request (raw)");
	
	/* encrypt request message */
	
    OpenSOAPBlockGetChildMB(body, "MyName", &elem);
	OpenSOAPXMLElmAddAttributeMB(elem, 
								 "encrypt", 
								 "boolean", 
								 &encryption,
								 &attr);
    OpenSOAPXMLAttrSetNamespaceMB(attr,
								  "http://security.opensoap.jp/1.0/", 
								  "s");
	result = OpenSOAPSecEncWithFile(request, "pubKey_HelloService.pem");
	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecClient: encrypt: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecClient: encrypt: failed\n");
	}
	
	PrintEnvelope(request, "request (encrypted)");
	
	/* add signature to request message */
	
	result = OpenSOAPSecAddSignWithFile(request,
										OPENSOAP_HA_SHA, 
										"privKey_HelloClient.pem", 
										NULL);
	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecClient: add signature: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecClient: add signature: failed\n");
	}
	
	PrintEnvelope(request, "request (encrypted and signed)");
	
	/* invoke service */
	
	OpenSOAPTransportCreate(&transport);
	OpenSOAPTransportSetURL(transport, endpointURL);
	OpenSOAPTransportInvoke(transport, request, &response);
	OpenSOAPTransportRelease(transport);
	
	PrintEnvelope(response, "response (raw)");
	
	OpenSOAPStringRelease(myName);
	
	/* verify signature on response message */
	
	result = OpenSOAPSecVerifySignWithFile(response, 
										   "pubKey_HelloService.pem");
	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecClient: verify signature: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecClient: verify signature: failed\n");
	}
	
	/* decrypt response message */
	
	result = OpenSOAPSecDecWithFile(response, "privKey_HelloClient.pem");
	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecClient: decrypt: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecClient: decrypt: failed\n");
	}
	
	PrintEnvelope(response, "response (decrypted)");
	
	/* parse response message */
	
	OpenSOAPStringCreate(&reply);
	OpenSOAPEnvelopeGetBodyBlockMB(response, "HelloResponse", &body);
	OpenSOAPBlockGetChildValueMB(body, "Reply", "string", &reply);
	OpenSOAPStringGetLengthMB(reply, &replyLen);
	replyString = malloc(replyLen + 1);
	OpenSOAPStringGetStringMB(reply, &replyLen, replyString);
	OpenSOAPStringRelease(reply);
	
	fprintf(stderr, "\nHelloSecClient: reply: %s\n", replyString);
	free(replyString);
	
	/* finalize client */
	
	OpenSOAPEnvelopeRelease(response);
	OpenSOAPEnvelopeRelease(request);
	OpenSOAPUltimate();
	
	fprintf(stderr, "\n***** end HelloSecClient *****\n\n");
	
	return 0;
}
