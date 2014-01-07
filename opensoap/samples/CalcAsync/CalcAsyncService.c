/*-----------------------------------------------------------------------------
 * $RCSfile: CalcAsyncService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "OpenSOAP/OpenSOAP.h"
#include "OpenSOAP/Service.h"

#ifndef SERVICE_LOCALSTATEDIR
# define SERVICE_LOCALSTATEDIR "/usr/local/opensoap/var/services/CalcAsync"
#endif

static const char CALC_LOG_FILE[] = SERVICE_LOCALSTATEDIR "/CalcAsync.log";

#if defined(CONNECT_TYPE_CGI)
# define CONNECT_TYPE "cgi"
# define LOG_STREAM stderr
#else
# define CONNECT_TYPE "stdio"
FILE *fp;
# define LOG_STREAM fp
#endif /* CONNECT_TYPE */

/* error message macro */
#define ERROR_MSG(error, location, message); \
fprintf(LOG_STREAM, \
		"%s: Cannot %s\n" \
		" ---> OpenSOAP Error Code: 0x%04x\n", \
		(location),\
		(message),\
		(error));

/* error check macro */
#define ERROR_CHECK(error, location, message); \
if (OPENSOAP_FAILED((error))) { \
	ERROR_MSG(error, location, message) \
}

/* error return macro */
#define ERROR_RETURN(error, location, message); \
if (OPENSOAP_FAILED((error))) { \
	ERROR_MSG(error, location, message) \
	return (error); \
}

static const char METHOD_NAMESPACE_URI[]
= "http://services.opensoap.jp/samples/CalcAsync/";

static const char METHOD_NAMESPACE_PREFIX[] = "m";

typedef 
int
(*FunctionType)(OpenSOAPEnvelopePtr request,
				OpenSOAPEnvelopePtr *response);

typedef struct {
    const char *requestName;
    const char *responseName;
    FunctionType methodFunc;
} MethodMapItem;

/* parse common part of request message */
static
int
ParseRequestCommon(OpenSOAPEnvelopePtr request,
				   const char *requestName,
				   OpenSOAPBlockPtr *body) {
	static char FuncName[] = "ParseRequestCommon";
	int error = OPENSOAP_NO_ERROR;
	int isSameNs = 0;
	
	error = OpenSOAPEnvelopeGetBodyBlockMB(request, requestName, body);
	ERROR_RETURN(error, FuncName, "get body block");
	
	error = OpenSOAPBlockIsSameNamespaceMB(*body,
										   METHOD_NAMESPACE_URI,
										   &isSameNs);
	ERROR_RETURN(error, FuncName, "complete namespace matching");
	
	error = (isSameNs ? error : OPENSOAP_NOT_CATEGORIZE_ERROR);
	ERROR_RETURN(error, FuncName, "match namespace");
	
	return error;
}

/* create common part of response message */
static
int
CreateResponseCommon(OpenSOAPEnvelopePtr *response,
					 const char *responseName,
					 OpenSOAPBlockPtr *body) {
	static char FuncName[] = "CreateRequestCommon";
	int error = OPENSOAP_NO_ERROR;
	
	error = OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	ERROR_RETURN(error, FuncName, "create envelope");
	
	/* add body response block */
	error = OpenSOAPEnvelopeAddBodyBlockMB(*response, responseName, body);
	ERROR_RETURN(error, FuncName, "add body part");
	
	/* set namespace to response block */
	error = OpenSOAPBlockSetNamespaceMB(*body,
										METHOD_NAMESPACE_URI,
										METHOD_NAMESPACE_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace of body part");
	
	return error;
}

/* create fault response message */
static
int
CreateFaultMessage(OpenSOAPEnvelopePtr *response,
				   const char *faultcode,
				   const char *faultstring,
				   const char *faultactor,
				   const char *detail) {
	static char FuncName[] = "CreateFaultMessage";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPStringPtr faultcodeOssp = NULL;
	OpenSOAPStringPtr faultstringOssp = NULL;
	OpenSOAPStringPtr faultactorOssp = NULL;
	OpenSOAPStringPtr detailOssp = NULL;
	
	error = CreateResponseCommon(response, "Fault", &body);
	ERROR_RETURN(error, FuncName, "create common part of response");
	
	error = OpenSOAPStringCreateWithMB(faultcode, &faultcodeOssp);
	ERROR_RETURN(error, FuncName, "create string: faultcode");
	
	error = OpenSOAPBlockSetChildValueMB(body,
										 "faultcode",
										 "string",
										 &faultcodeOssp);
	ERROR_RETURN(error, FuncName, "set parameter: faultcode");
	
	error = OpenSOAPStringCreateWithMB(faultstring, &faultstringOssp);
	ERROR_RETURN(error, FuncName, "create string: faultstring");
	
	error = OpenSOAPBlockSetChildValueMB(body,
										 "faultstring",
										 "string",
										 &faultstringOssp);
	ERROR_RETURN(error, FuncName, "set parameter: faultstring");
	
	if (faultactor != NULL) {
		error = OpenSOAPStringCreateWithMB("faultactor", &faultactorOssp);
		ERROR_RETURN(error, FuncName, "create string: faultactor");
		
		error = OpenSOAPBlockSetChildValueMB(body,
											 "faultactor",
											 "string",
											 &faultactorOssp);
		ERROR_RETURN(error, FuncName, "set parameter: faultactor");
	}
	   
	if (detail != NULL) {
		
		error = OpenSOAPStringCreateWithMB(detail, &detailOssp);
		ERROR_RETURN(error, FuncName, "create string: detail");
		
		error = OpenSOAPBlockSetChildValueMB(body,
											 "detail",
											 "string",
											 &detailOssp);
		ERROR_RETURN(error, FuncName, "set parameter: detail");
	}
	
	return error;
}

/* process calc request */
static
int
Calc(char *operator,
	 OpenSOAPEnvelopePtr request,
	 OpenSOAPEnvelopePtr *response) {
	static char FuncName[] = "Calc";
	int error = OPENSOAP_NO_ERROR;
	
	double operandA = 0.0;
	double operandB = 0.0;
	double result = 0.0;
	char rspOperator[64];
	
	OpenSOAPBlockPtr body = NULL;
	
	/* --- parse request message --- */
	
	error = ParseRequestCommon(request, operator, &body);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   "SOAP-ENV:service",
								   "cannont parse common part of request",
								   NULL,
								   NULL);
		return error;
	}
	
	error = OpenSOAPBlockGetChildValueMB(body, "A", "double", &operandA);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   "SOAP-ENV:service",
								   "cannont get parameter: operand A",
								   NULL,
								   NULL);
		return error;
	}

	error = OpenSOAPBlockGetChildValueMB(body, "B", "double", &operandB);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   "SOAP-ENV:service",
								   "cannont get parameter: operand B",
								   NULL,
								   NULL);
		return error;
	}
	
	/* --- make response contents --- */
	
	if (strcmp(operator, "Add") == 0) {
		
		result = operandA + operandB;
	}
	else if (strcmp(operator, "Subtract") == 0) {
		
		result = operandA - operandB;
	}
	else if (strcmp(operator, "Multiply") == 0) {
		
		result = operandA * operandB;
	}
	else if (strcmp(operator, "Divide") == 0) {

		if (operandB == 0.0) {
			error = CreateFaultMessage(response,
									   "SOAP-ENV:service",
									   "division by zero",
									   NULL,
									   NULL);
			return error;
			
		}
		result = operandA / operandB;
	}
	
	/* --- create response message --- */
	
	sprintf(rspOperator, "%sResponse", operator);
	
	error = CreateResponseCommon(response, rspOperator, &body);
	ERROR_RETURN(error, FuncName, "create common part of response");
	
	error = OpenSOAPBlockSetChildValueMB(body, "Result", "double", &result);
	ERROR_RETURN(error, FuncName, "set result");
	
	return error;
}

/* process add request */
static
int
Add(OpenSOAPEnvelopePtr request,
	OpenSOAPEnvelopePtr *response) {
	static char FuncName[] = "Add";
	int error = OPENSOAP_NO_ERROR;

	error = Calc(FuncName, request, response);
	ERROR_RETURN(error, FuncName, "complete calc function");

	return error;
}

/* process subtract request */
static
int
Subtract(OpenSOAPEnvelopePtr request,
		 OpenSOAPEnvelopePtr *response) {
	static char FuncName[] = "Subtract";
	int error = OPENSOAP_NO_ERROR;
	
	error = Calc(FuncName, request, response);
	ERROR_RETURN(error, FuncName, "complete calc function");
	
	return error;
}

/* process multiply request */
static
int
Multiply(OpenSOAPEnvelopePtr request,
		 OpenSOAPEnvelopePtr *response) {
	static char FuncName[] = "Multiply";
	int error = OPENSOAP_NO_ERROR;
	
	error = Calc(FuncName, request, response);
	ERROR_RETURN(error, FuncName, "complete calc function");
	
	return error;
}

/* process divide request */
static
int
Divide(OpenSOAPEnvelopePtr request,
	   OpenSOAPEnvelopePtr *response) {
	static char FuncName[] = "Divide";
	int error = OPENSOAP_NO_ERROR;

	error = Calc(FuncName, request, response);
	ERROR_RETURN(error, FuncName, "complete calc function");

	return error;
}

static
const MethodMapItem
MethodMap[] = {
	{ "Add",      "AddResponse",      Add      },
	{ "Subtract", "SubtractResponse", Subtract },
	{ "Multiply", "MultiplyResponse", Multiply },
	{ "Divide",   "DivideResponse",   Divide   },
	{ NULL,       NULL,               NULL     }
};

/* service function to be registered */
static
int
ServiceFunc(OpenSOAPEnvelopePtr request,
			OpenSOAPEnvelopePtr *response,
			void *opt) {
	static char FuncName[] = "ServiceFunc";
	int error = OPENSOAP_NO_ERROR;
	
	MethodMapItem *methodMapItem = (MethodMapItem *)opt;
	
	if (methodMapItem) {
		error = (methodMapItem->methodFunc)(request, response);
		ERROR_RETURN(error, FuncName, "complete method");
		/*
		fprintf(stderr, "method name: %s\n", methodMapItem->requestName);
		*/
	}
	
	return error;
}

/* main */
int
main(void) {
	static char FuncName[] = "main";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPServicePtr service = NULL;
	
#if !defined(CONNECT_TYPE_CGI)
	if ((fp = fopen(CALC_LOG_FILE, "a")) != NULL) {
		fprintf(fp, "start logging...\n");
	} else {
		fp = stderr;
	}
#endif /* CONNECT_TYPE_CGI */
	
	/* initialize service */
	error = OpenSOAPInitialize(NULL);
	ERROR_RETURN(error, FuncName, "initialize service");;
	
	/* create service */
	error = OpenSOAPServiceCreateMB(&service,
									"CalcAsyncService",
									CONNECT_TYPE,
									0);
	if (OPENSOAP_SUCCEEDED(error)) {
		const MethodMapItem *methodMapItem = MethodMap;
		
		/* register methods to service */
		for (; OPENSOAP_SUCCEEDED(error) && methodMapItem->requestName;
			 ++methodMapItem) {
			error = OpenSOAPServiceRegisterMB(service,
											  methodMapItem->requestName,
											  ServiceFunc,
											  (void *)methodMapItem);
			ERROR_CHECK(error, FuncName, "register method");
			/*
			fprintf(stderr, "method name: %s\n", methodMapItem->requestName);
			*/
		}
		
		if (OPENSOAP_SUCCEEDED(error)) {
			/* activate service */
			error = OpenSOAPServiceRun(service);
			ERROR_CHECK(error, FuncName, "run service");
		}
		
		/* release service */
		error = OpenSOAPServiceRelease(service);
		ERROR_CHECK(error, FuncName, "release");
	}
	else {
		ERROR_MSG(error, FuncName, "create");
	}
	
	/* finalize service */
	OpenSOAPUltimate();

#if !defined(CONNECT_TYPE_CGI)
	fclose(fp);
#endif /* CONNECT_TYPE_CGI */
	
	return error;
}
