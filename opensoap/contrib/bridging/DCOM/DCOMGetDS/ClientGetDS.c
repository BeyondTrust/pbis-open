/*-----------------------------------------------------------------------------
 * $RCSfile: DCOMDllGetDSClient.c,v $
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
	OpenSOAPEnvelopeGetCharEncodingString(env, "UTF-8", envBuf);
	OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
	
	fprintf(stderr, "\n=== %s envelope begin ===\n", label);
	fwrite(envBeg, 1, envSz, stderr);
	fprintf(stderr, "\n=== %s envelope  end  ===\n", label);
	
	OpenSOAPByteArrayRelease(envBuf);
}

int
main(int argc,
	 char **argv) {

	int error_code = OPENSOAP_PARAMETER_BADVALUE;
	
	OpenSOAPEnvelopePtr request = NULL;
	OpenSOAPEnvelopePtr response = NULL;
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPTransportPtr transport = NULL;
	
	OpenSOAPStringPtr serverName = NULL;
	
	int diskSize = 0;

	/* initialize client */
	
	error_code = OpenSOAPInitialize(NULL);
	
	/* create request message */
	
	error_code = OpenSOAPEnvelopeCreateMB("1.1", NULL, &request);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPEnvelopeAddBodyBlockMB(request, "GetServerDiskSize", &body); 
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPBlockSetNamespaceMB(body, "http://namespaces.opensoap.jp", "m");
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPStringCreateWithMB(argv[1], &serverName);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPBlockSetChildValueMB(body, "ServerName", "string", &serverName);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	
	PrintEnvelope(request, "request");
	
	/* invoke service */
	
	error_code = OpenSOAPTransportCreate(&transport);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
#if 1 /* direct client-service connect (using HelloService.cgi) */
	error_code = OpenSOAPTransportSetURL(transport,
							"http://localhost/cgi-bin/DCOMDllGetDS_CGI.exe");
#else /* via OpenSOAP server (using HelloService)*/
	error_code = OpenSOAPTransportSetURL(transport,
							"http://localhost/cgi-bin/soapInterface.cgi");
#endif
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPTransportInvoke(transport, request, &response);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPTransportRelease(transport);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	
	PrintEnvelope(response, "response");
	
	/* parse response message */
	
	error_code = OpenSOAPEnvelopeGetBodyBlockMB(response, "GetServerDiskSizeResponse", &body);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPBlockGetChildValueMB(body, "ServerDiskSize", "int", &diskSize);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	fprintf(stderr, "\nDisk Size: %d\n\n", diskSize);
	
	/* finalize client */
	
	error_code = OpenSOAPEnvelopeRelease(response);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPEnvelopeRelease(request);
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	error_code = OpenSOAPUltimate();
	if( OPENSOAP_FAILED(error_code) ) {
		return 1;
	}
	
	return 0;
}
