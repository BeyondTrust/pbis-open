/*-----------------------------------------------------------------------------
 * $RCSfile: SimpleCalcService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

#include <stdio.h>
#include <string.h>

#if !defined(CONNECT_TYPE)
# define CONNECT_TYPE "stdio"
#endif /* CONNECT_TYPE */

static
void
ParseRequestMessage(OpenSOAPEnvelopePtr request,
					double *operandA,
					double *operandB) {
	
	OpenSOAPBlockPtr body = NULL;
	
	OpenSOAPEnvelopeGetNextBodyBlock(request, &body);
	OpenSOAPBlockGetChildValueMB(body, "A", "double", operandA);
	OpenSOAPBlockGetChildValueMB(body, "B", "double", operandB);
}

static
void
CreateResponseMessage(OpenSOAPEnvelopePtr *response,
					  char *responseName,
					  double result) {
	
	const char* namespace = "http://services.opensoap.jp/samples/SimpleCalc/";
	
	OpenSOAPBlockPtr body = NULL;
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response, responseName, &body);
	OpenSOAPBlockSetNamespaceMB(body, namespace, "m");
	
 	OpenSOAPBlockSetChildValueMB(body, "Result", "double", &result);
}

static
int
AddFunc(OpenSOAPEnvelopePtr request,
		OpenSOAPEnvelopePtr *response,
		void *opt) {
	
	double operandA = 0.0;
	double operandB = 0.0;
	double result = 0.0;
	
	ParseRequestMessage(request, &operandA, &operandB);
	
	result = operandA + operandB;
	
	CreateResponseMessage(response, "AddResponse", result);
	
	return 0;
}

static
int
SubtractFunc(OpenSOAPEnvelopePtr request,
			 OpenSOAPEnvelopePtr *response,
			 void *opt) {
	
	double operandA = 0.0;
	double operandB = 0.0;
	double result = 0.0;
	
	ParseRequestMessage(request, &operandA, &operandB);
	
	result = operandA - operandB;
	
	CreateResponseMessage(response, "SubtractResponse", result);
	
	return 0;
}

static
int
MultiplyFunc(OpenSOAPEnvelopePtr request,
			 OpenSOAPEnvelopePtr *response,
			 void *opt) {
	
	double operandA = 0.0;
	double operandB = 0.0;
	double result = 0.0;
	
	ParseRequestMessage(request, &operandA, &operandB);
	
	result = operandA * operandB;
	
	CreateResponseMessage(response, "MultiplyResponse", result);
	
	return 0;
}

static
int
DivideFunc(OpenSOAPEnvelopePtr request,
		   OpenSOAPEnvelopePtr *response,
		   void *opt) {
	
	double operandA = 0.0;
	double operandB = 0.0;
	double result = 0.0;
	
	ParseRequestMessage(request, &operandA, &operandB);

	/* temporary error avoidance */
	/* this must be substituted by fault message generation */
	if (operandB != 0.0) {
		result = operandA / operandB;
	}
	
	CreateResponseMessage(response, "DivideResponse", result);
	
	return 0;
}

int
main(void) {
	
	OpenSOAPServicePtr service = NULL;
	
	OpenSOAPInitialize(NULL);
	
	OpenSOAPServiceCreateMB(&service, "SimpleCalcService", CONNECT_TYPE, 0);
	
	OpenSOAPServiceRegisterMB(service, "Add", AddFunc, NULL);
	OpenSOAPServiceRegisterMB(service, "Subtract", SubtractFunc, NULL);
 	OpenSOAPServiceRegisterMB(service, "Multiply", MultiplyFunc, NULL);
 	OpenSOAPServiceRegisterMB(service, "Divide", DivideFunc, NULL);
	
	OpenSOAPServiceRun(service);
	
	OpenSOAPServiceRelease(service);
	
	OpenSOAPUltimate();
	
	return 0;
}
