/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvPlaceOrder.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvPlaceOrder.h"
#include "ShSvCmn.h"
#include "SvCmn.h"

#include <string.h>

/* process place order request */
int
PlaceOrder(OpenSOAPEnvelopePtr request,
		   OpenSOAPEnvelopePtr *response,
		   void *opt) {
	static char FuncName[] = "PlaceOrder";
	int error = OPENSOAP_NO_ERROR;
	
	char *code = NULL;
	long qty = -1;
	OpenSOAPStringPtr confirmation = NULL;
	OpenSOAPBlockPtr body = NULL;
	int i = 0;
	const char* nameSpace = opt;
	
	/* --- load product stock --- */
	
	error = LoadProductStock();
	ERROR_RETURN(error, FuncName, "load product stock");
	LOG_INT(FuncName, "productStockCount", productStockCount);
	
	/* --- parse request message --- */
	
	error = ParseRequestCommon(request,
							   "PlaceOrder",
							   &body,
							   nameSpace);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "cannont parse common part of "
								   "PlaceOrder request",
								   NULL,
								   NULL);
		return error;
	}
	
	error = GetStringParameter(body, "code", &code);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "cannont get code parameter",
								   NULL,
								   NULL);
		return error;
	}
	
	error = OpenSOAPBlockGetChildValueMB(body, "qty", "int", &qty);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "cannont get qty parameter",
								   NULL,
								   NULL);
		return error;
	}
	
	/* --- make response contents --- */
	
	while (i < productStockCount
		   && strcmp(code, productStockList[i].code) != 0) {
		LOG_STRING(FuncName, "code", productStockList[i].code);
		i++;
	}
	free(code);
	
	if (i == productStockCount) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "invalid code parameter",
								   NULL,
								   NULL);
		return error;
	}
	else if (qty < 1 || qty > productStockList[i].qty) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "qty parameter out of range",
								   NULL,
								   NULL);
		return error;
	}
	else {
		productStockList[i].qty -= qty;
		
		error = OpenSOAPStringCreateWithMB("order successfully placed",
										   &confirmation);
		ERROR_RETURN(error, FuncName, "create string: confirmation");
		
	
		/* --- create response message --- */
		
		error = CreateResponseCommon(response,
									 "PlaceOrderResponse",
									 &body,
									 nameSpace);
		ERROR_RETURN(error, FuncName, "create common part of response");
		
		error = OpenSOAPBlockSetChildValueMB(body,
											 "confirmation",
											 "string",
											 &confirmation);
		ERROR_RETURN(error, FuncName, "set parameter: confirmation");
	}
	
	/* --- save product stock --- */
	
	error = SaveProductStock();
	ERROR_RETURN(error, FuncName, "save product stock");
	
	return error;
}
