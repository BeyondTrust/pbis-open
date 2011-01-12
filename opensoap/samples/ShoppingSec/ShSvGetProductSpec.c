/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvGetProductSpec.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvGetProductSpec.h"
#include "ShSvCmn.h"
#include "SvCmn.h"

/* process get product specification request */
int
GetProductSpec(OpenSOAPEnvelopePtr request,
			   OpenSOAPEnvelopePtr *response,
			   void *opt) {
	static char FuncName[] = "GetProductSpec";
	int error = OPENSOAP_NO_ERROR;
	
	long index = -1;
	OpenSOAPStringPtr manufacturer = NULL;
	OpenSOAPStringPtr name = NULL;
	OpenSOAPStringPtr code = NULL;
	long price = -1;
	OpenSOAPBlockPtr body = NULL;
	const char* nameSpace = opt;
	
	/* --- parse request message --- */
	
	error = ParseRequestCommon(request,
							   "GetProductSpec",
							   &body,
							   nameSpace);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "cannont parse common part of "
								   "GetProductSpec request",
								   NULL,
								   NULL);
		return error;
	}
	
  	error = OpenSOAPBlockGetChildValueMB(body, "index", "int", &index);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "cannont get index parameter",
								   NULL,
								   NULL);
		return error;
	}
	
	/* --- make response contents --- */
	
	if (index < 0 || index >= productSpecCount) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "index parameter out of range",
								   NULL,
								   NULL);
		return error;
	}
	
	error = OpenSOAPStringCreateWithMB(productSpecList[index].manufacturer,
									   &manufacturer);
	ERROR_RETURN(error, FuncName, "create string: manufacturer");
	
	error = OpenSOAPStringCreateWithMB(productSpecList[index].name, &name);
	ERROR_RETURN(error, FuncName, "create string: name");
	
	error = OpenSOAPStringCreateWithMB(productSpecList[index].code, &code);
	ERROR_RETURN(error, FuncName, "create string: code");
	
	price = productSpecList[index].price;
	
	/* --- create response message --- */
	
	error = CreateResponseCommon(response,
								 "GetProductSpecResponse",
								 &body,
								 nameSpace);
	ERROR_RETURN(error, FuncName, "create common part of response");
	
	error = OpenSOAPBlockSetChildValueMB(body,
										 "manufacturer",
										 "string",	
										 &manufacturer);
	ERROR_RETURN(error, FuncName, "set parameter: manufacturer");
	
	error = OpenSOAPBlockSetChildValueMB(body, "name", "string", &name);
	ERROR_RETURN(error, FuncName, "set parameter: name");
	
	error = OpenSOAPBlockSetChildValueMB(body, "code", "string", &code);
	ERROR_RETURN(error, FuncName, "set parameter: code");
	
	error = OpenSOAPBlockSetChildValueMB(body, "price", "int", &price);
	ERROR_RETURN(error, FuncName, "set parameter: price");
	
	return error;
}
