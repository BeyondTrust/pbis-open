/*-----------------------------------------------------------------------------
 * $RCSfile: HelloClient.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>

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
	fprintf(stderr, "\n=== %s envelope  end  ===\n", label);
	
	OpenSOAPByteArrayRelease(envBuf);
}

int
main(int argc,
	 char **argv) {
	
	OpenSOAPEnvelopePtr request = NULL;
	OpenSOAPEnvelopePtr response = NULL;
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPTransportPtr transport = NULL;
	
	OpenSOAPStringPtr myName = NULL;
	
	OpenSOAPStringPtr reply = NULL;
	size_t replyLen = 0;
	char *replyString = NULL;
	
	/* initialize client */
	
	OpenSOAPInitialize(NULL);
	
	/* create request message */
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, &request);
	OpenSOAPEnvelopeAddBodyBlockMB(request, "Hello", &body); 
	OpenSOAPBlockSetNamespaceMB(body,
								"http://services.opensoap.jp/samples/Hello/",
								"m");
	OpenSOAPStringCreateWithMB(argv[1], &myName);
	OpenSOAPBlockSetChildValueMB(body, "MyName", "string", &myName);
	
	PrintEnvelope(request, "request");
	
	/* invoke service */
	
	OpenSOAPTransportCreate(&transport);
#if 1 /* direct client-service connect (using HelloService.cgi) */
	OpenSOAPTransportSetURL(transport,
							"http://localhost/cgi-bin/HelloService.cgi");
#else /* via OpenSOAP server (using HelloService)*/
	OpenSOAPTransportSetURL(transport,
							"http://localhost/cgi-bin/soapInterface.cgi");
#endif
	OpenSOAPTransportInvoke(transport, request, &response);
	OpenSOAPTransportRelease(transport);
	
	PrintEnvelope(response, "response");
	
	OpenSOAPStringRelease(myName);
	
	/* parse response message */
	
	OpenSOAPStringCreate(&reply);
	OpenSOAPEnvelopeGetBodyBlockMB(response, "HelloResponse", &body);
	OpenSOAPBlockGetChildValueMB(body, "Reply", "string", &reply);
	OpenSOAPStringGetLengthMB(reply, &replyLen);
	replyString = malloc(replyLen + 1);
	OpenSOAPStringGetStringMB(reply, &replyLen, replyString);
	
	fprintf(stderr, "\nreply: %s\n\n", replyString);
	free(replyString);

	OpenSOAPStringRelease(reply);
	/* finalize client */
	
	OpenSOAPEnvelopeRelease(response);
	OpenSOAPEnvelopeRelease(request);
	OpenSOAPUltimate();
	
	return 0;
}
