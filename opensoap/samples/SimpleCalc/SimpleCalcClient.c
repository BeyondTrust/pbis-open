/*-----------------------------------------------------------------------------
 * $RCSfile: SimpleCalcClient.c,v $
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
#include <string.h>

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
	
	OpenSOAPEnvelopePtr request = NULL;
	OpenSOAPEnvelopePtr response = NULL;
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPTransportPtr transport = NULL;
	
  	const char* namespace = "http://services.opensoap.jp/samples/SimpleCalc/";
 	char* endpointURL = "http://localhost/cgi-bin/SimpleCalcService.cgi";
	
	char* responseName = NULL; /* "*Response" string */
	char* responseNamePostfix = "Response";
	
	const char* operator = NULL;  /* Add, Subtract, Multiply, or Divide */
	double operandA = 0.0;        /* first operand */
	double operandB = 0.0;        /* second operand */
	double result = 0.0;          /* result of calculation */
	
	/* initialize input arameters */

	if (argc < 4) {
		fprintf(stderr, "Usage: SimpleCalcClient {Add|Subtract|Multiply|Divide} param1 param2 [endpoint]\n\n");
		exit(1);
	}
	
	operator = argv[1];
	printf("\nOperand: %s\n", operator);
	
	operandA = atof(argv[2]);
	printf("A = %f\n", operandA);
	
	operandB = atof(argv[3]);
	printf("B = %f\n", operandB);
	
	if (argc > 4) {
		endpointURL = argv[4];
	}
	printf("\nEndpoint URL: %s\n", endpointURL);
	
	/* make "*Response" string */
	
	responseName = malloc(strlen(operator) + strlen(responseNamePostfix) + 1);
 	strcpy(responseName, operator);
  	strcat(responseName, responseNamePostfix);
	
	/*--- initialize client ---*/
	
	OpenSOAPInitialize(NULL);
	
	/*--- create request message ---*/
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, &request);
	OpenSOAPEnvelopeAddBodyBlockMB(request, operator, &body); 
	
	OpenSOAPBlockSetNamespaceMB(body, namespace, "m");
	OpenSOAPBlockSetChildValueMB(body, "A", "double", &operandA);
	OpenSOAPBlockSetChildValueMB(body, "B", "double", &operandB);
	
	PrintEnvelope(request, "request");
	
	/*--- invoke service ---*/
	
	OpenSOAPTransportCreate(&transport);
	OpenSOAPTransportSetURL(transport, endpointURL);
	OpenSOAPTransportInvoke(transport, request, &response);
	OpenSOAPTransportRelease(transport);
	
	PrintEnvelope(response, "response");
	
	/*--- parse response message ---*/
	
	OpenSOAPEnvelopeGetBodyBlockMB(response, responseName, &body);
	OpenSOAPBlockGetChildValueMB(body, "Result", "double", &result);
	
	free(responseName);
	
	fprintf(stderr, "\nresult = %f\n\n", result);
	
	/*--- finalize client ---*/
	
	OpenSOAPEnvelopeRelease(response);
	OpenSOAPEnvelopeRelease(request);
	OpenSOAPUltimate();
	
	return 0;
}
