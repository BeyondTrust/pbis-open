/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvPlaceOrderSec.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvPlaceOrder.h"
#include "ShSvCmn.h"
#include "SvSecCmn.h"
#include "SvCmn.h"

#include <string.h>

#include <OpenSOAP/Security.h>

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
	Options *options = (Options *)opt;
	int i = 0;
	
	/* --- load product stock --- */
	
	error = LoadProductStock();
	ERROR_RETURN(error, FuncName, "load product stock");
	LOG_INT(FuncName, "productStockCount", productStockCount);
	
	LOG_ENVELOPE(FuncName, "raw request", request);
	
	/* --- verify signature on request message with client's public key --- */
	
	error = OpenSOAPSecVerifySignWithFile(request, options->opt2);
	ERROR_RETURN(error, FuncName, "verify signature on request message");
	
	/* --- decrypt request message with service's private key --- */
	
    error = OpenSOAPSecDecWithFile(request, options->opt1);
	ERROR_RETURN(error, FuncName, "decrypt request message");
	
	LOG_ENVELOPE(FuncName, "decrypted request", request);
	
	/* --- parse request message --- */
	
	error = ParseRequestCommon(request,
							   "PlaceOrder",
							   &body,
							   options->nameSpace);
	if (OPENSOAP_FAILED(error)) {
		LOG_STRING(FuncName, "nameSpace", options->nameSpace);
		error = CreateFaultMessage(response,
								   options->nameSpace,
								   "SOAP-ENV:service",
								   "cannont parse common part of "
								   "PlaceOrder request",
								   NULL,
								   NULL);
		LOG_ENVELOPE(FuncName, "fault response", *response);
		return error;
	}
	
	error = GetStringParameter(body, "code", &code);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   options->nameSpace,
								   "SOAP-ENV:service",
								   "cannont get code parameter",
								   NULL,
								   NULL);
  		LOG_ENVELOPE(FuncName, "fault response", *response);
		return error;
	}
	
	error = OpenSOAPBlockGetChildValueMB(body, "qty", "int", &qty);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   options->nameSpace,
								   "SOAP-ENV:service",
								   "cannont get qty parameter",
								   NULL,
								   NULL);
		LOG_ENVELOPE(FuncName, "fault response", *response);
		return error;
	}
	
	/* --- make response contents --- */
	
	while (i < productStockCount
		   && strcmp(code, productStockList[i].code) != 0) {
		i++;
	}
	free(code);
	
	if (i == productStockCount) {
		error = CreateFaultMessage(response,
								   options->nameSpace,
								   "SOAP-ENV:service",
								   "invalid code parameter",
								   NULL,
								   NULL);
		LOG_ENVELOPE(FuncName, "fault response", *response);
	}
	else if (qty < 1 || qty > productStockList[i].qty) {
		error = CreateFaultMessage(response,
								   options->nameSpace,
								   "SOAP-ENV:service",
								   "qty parameter out of range",
								   NULL,
								   NULL);
		LOG_ENVELOPE(FuncName, "fault response", *response);
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
									 options->nameSpace);
		ERROR_RETURN(error, FuncName, "create common part of response");
		
		error = OpenSOAPBlockSetChildValueMB(body,
											 "confirmation",
											 "string",
											 &confirmation);
		ERROR_RETURN(error, FuncName, "set parameter: confirmation");
		
		error = EncryptParameter(response,
								 "PlaceOrderResponse",
								 "confirmation");
		ERROR_RETURN(error, FuncName, "encrypt parameter: confirmation");
		
		LOG_ENVELOPE(FuncName, "raw response", *response);
	}
	
	/* --- encrypt response message with client's public key --- */
	
	error = OpenSOAPSecEncWithFile(*response, options->opt2);
	ERROR_RETURN(error, FuncName, "encrypt envelope with public key");
	
	LOG_ENVELOPE(FuncName, "encrypted response", *response);
	
	/* --- add signature on response message with service's private key --- */
	
	error = OpenSOAPSecAddSignWithFile(*response,
									   OPENSOAP_HA_SHA,
									   options->opt1,
									   NULL);
	ERROR_RETURN(error, FuncName, "add signature on response message");

	LOG_ENVELOPE(FuncName, "signed response", *response);
	
	/* --- save product stock --- */
	
	error = SaveProductStock();
	ERROR_RETURN(error, FuncName, "save product stock");
	
	return error;
}
