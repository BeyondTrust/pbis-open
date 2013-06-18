/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvGetStockQty.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvGetStockQty.h"
#include "ShSvCmn.h"
#include "SvCmn.h"

#include <string.h>

#define RECORD_SZ 128

/* process get stock quantity request */
int
GetStockQty(OpenSOAPEnvelopePtr request,
			OpenSOAPEnvelopePtr *response,
			void *opt) {
	static char FuncName[] = "GetStockQty";
	int error = OPENSOAP_NO_ERROR;
	
	char *code = NULL;
 	int i = 0;
	OpenSOAPBlockPtr body = NULL;
  	const char* nameSpace = ((Options *)opt)->nameSpace;

	/* --- load product stock --- */
	
	error = LoadProductStock();
	ERROR_RETURN(error, FuncName, "load product stock");
	LOG_INT(FuncName, "productStockCount", productStockCount);
	
	fprintf(stderr, "hogehoge2\n");
	/* --- parse request message --- */
	
	error = ParseRequestCommon(request,
							   "GetStockQty",
							   &body,
							   nameSpace);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "cannont parse common part of "
								   "GetStockQty request",
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
	
	/* --- make response contents --- */
	
	while (i < productStockCount
		   && strcmp(code, productStockList[i].code) != 0) {
		i++;
	}
	if (i == productStockCount) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "invalid code parameter",
								   NULL,
								   NULL);
		return error;
	}
	
	free(code);
	
	/* --- create response message --- */
	
	error = CreateResponseCommon(response,
								 "GetStockQtyResponse",
								 &body,
								 nameSpace);
	ERROR_RETURN(error, FuncName, "create common part of response");
	
	error = OpenSOAPBlockSetChildValueMB(body,
										 "qty",
										 "int",
										 &productStockList[i].qty);
	ERROR_RETURN(error, FuncName, "set parameter: qty");
	
	/* --- save product stock --- */
	
	error = SaveProductStock();
	ERROR_RETURN(error, FuncName, "save product stock");
	
	return error;
}

