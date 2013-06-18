/*-----------------------------------------------------------------------------
 * $RCSfile: HelloSecService.c,v $
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

#if !defined(SERVICE_DIR)
# define SERVICE_DIR "/usr/local/opensoap/services/HelloSec"
#endif

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

static
int
ServiceFunc(OpenSOAPEnvelopePtr request,
			OpenSOAPEnvelopePtr *response,
			void *opt) {
	
	OpenSOAPBlockPtr body = NULL;
	
	OpenSOAPStringPtr myName = NULL;
	size_t myNameLen = 0;
	char *myNameString = NULL;
	static char noNameString[] = "who are you?";
	
	OpenSOAPStringPtr reply = NULL;
	size_t replyLen = 0;
	char *replyString = NULL;

	OpenSOAPXMLElmPtr elem = NULL;
	OpenSOAPXMLAttrPtr attr = NULL;
	int encryption = 1;
	int result = OPENSOAP_NO_ERROR;
	
	PrintEnvelope(request, "request (raw)");
	
	/* verify signature on request message */
	
	result 
		= OpenSOAPSecVerifySignWithFile(
			request,
			SERVICE_DIR "/pubKey_HelloClient.pem");
	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecService: verify signature: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecService: verify signature: failed\n");
	}
	
	/* decrypt request message */
	
	result
		= OpenSOAPSecDecWithFile(
			request,
			SERVICE_DIR "/privKey_HelloService.pem");

	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecService: decrypt: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecService: decrypt: failed\n");
	}
	
	PrintEnvelope(request, "request (decrypted)");
	
	/* parse request message */
	
	OpenSOAPEnvelopeGetBodyBlockMB(request, "Hello", &body);
	OpenSOAPBlockGetChildValueMB(body, "MyName", "string", &myName);
	OpenSOAPStringGetLengthMB(myName, &myNameLen);
	if (myNameLen == 0) {
		myNameString = malloc(strlen(noNameString) + 1);
		strcpy(myNameString, noNameString);
	}
	else {
		myNameString = malloc(myNameLen + 1);
		OpenSOAPStringGetStringMB(myName, &myNameLen, myNameString);
	}
	fprintf(stderr, "\nHelloSecService: my name: %s\n", myNameString);
	
	/* make response message */
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response, "HelloResponse", &body);
	OpenSOAPBlockSetNamespaceMB(body,
								"http://services.opensoap.jp/samples/Hello/",
								"m");
	
	replyLen = strlen(myNameString);
	replyString = malloc(replyLen + 9);
	sprintf(replyString, "Hello, %s!", myNameString);
	free(myNameString);
	fprintf(stderr, "\nHelloSecService: reply: %s\n", replyString);
	
	OpenSOAPStringCreateWithMB(replyString, &reply);
	free(replyString);
	OpenSOAPBlockSetChildValueMB(body, "Reply", "string", &reply);
	
	PrintEnvelope(*response, "response (raw)");
	
	/* encrypt response message */
	
    OpenSOAPBlockGetChildMB(body, "Reply", &elem);
	OpenSOAPXMLElmAddAttributeMB(elem, 
								 "encrypt", 
								 "boolean", 
								 &encryption,
								 &attr);
    OpenSOAPXMLAttrSetNamespaceMB(attr,
								  "http://security.opensoap.jp/1.0/", 
								  "s");
	result 
		= OpenSOAPSecEncWithFile(*response, 
								 SERVICE_DIR "/pubKey_HelloClient.pem");
	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecService: encrypt: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecService: encrypt: failed\n");
	}
#if 0           /* to avoid the misbehaviour on apache2 */
	PrintEnvelope(*response, "response (encrypted)");
#endif
	/* add signature to response message */
	
	result 
		= OpenSOAPSecAddSignWithFile( 
			*response,
			OPENSOAP_HA_SHA, 
			SERVICE_DIR "/privKey_HelloService.pem",
			NULL);
	if (OPENSOAP_SUCCEEDED(result)) {
		fprintf(stderr, "\nHelloSecService: add signature: succeeded\n");
	}
	else {
		fprintf(stderr, "\nHelloSecService: add signature failed\n");
	}
#if 0           /* to avoid the misbehaviour on apache2 */
	PrintEnvelope(*response, "response (encrypted and signed)");
#endif
	return 0;
}

int
main(void) {
	
	OpenSOAPServicePtr service = NULL;
	
	fprintf(stderr, "\n***** start HelloSecService *****\n");
	
	OpenSOAPInitialize(NULL);
	
	OpenSOAPServiceCreateMB(&service, "HelloSecService", CONNECT_TYPE, 0);
	
	OpenSOAPServiceRegisterMB(service, "Hello", ServiceFunc, NULL);
	
	OpenSOAPServiceRun(service);
	
	OpenSOAPServiceRelease(service);
	
	OpenSOAPUltimate();
	
	fprintf(stderr, "\n***** end HelloSecService *****\n\n");
	
	return 0;
}
