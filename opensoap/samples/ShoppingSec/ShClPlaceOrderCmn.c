/*-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderCmn.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClPlaceOrderCmn.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

/* create place order request message */
int
CreatePlaceOrderRequest(const char *nameSpace,
						const char *code,
						long qty,
						OpenSOAPEnvelopePtr *request) {
	static char FuncName[] = "CreatePlaceOrderRequest";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr body = NULL;
	OpenSOAPStringPtr codeStr = NULL;
	
	error = CreateRequestCommon(request,
								"PlaceOrder",
								&body,
								nameSpace);
	ERROR_RETURN(error, FuncName, "create common part of request");
	
	error = OpenSOAPStringCreateWithMB(code, &codeStr);
	ERROR_RETURN(error, FuncName, "create string for code");
	
	error = OpenSOAPBlockSetChildValueMB(body, "code", "string", &codeStr);
	ERROR_RETURN(error, FuncName, "set parameter: code");
	
	error = OpenSOAPBlockSetChildValueMB(body, "qty", "int", &qty);
	ERROR_RETURN(error, FuncName, "set parameter: qty");
	
	return error;
}

/* parse place order response message */
int
ParsePlaceOrderResponse(OpenSOAPEnvelopePtr response,
						const char *nameSpace,
						char **confirmation) {
	static char FuncName[] = "ParsePlaceOrderResponse";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	int isFault = 0;
	
	error = ParseResponseCommon(response,
								"PlaceOrderResponse",
								&body,
								nameSpace,
								&isFault);
	ERROR_RETURN(error, FuncName, "parse common part of response");

	if (isFault) {
		
		error = GetStringParameter(body, "faultconfstring", confirmation);
		ERROR_RETURN(error, FuncName, "get parameter: faultstring");
		
		return error;
	}
	
	error = GetStringParameter(body, "confirmation", confirmation);
	ERROR_RETURN(error, FuncName, "get parameter: confirmation");
	
	return error;
}
