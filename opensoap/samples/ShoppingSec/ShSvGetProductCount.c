/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvGetProductCount.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvGetProductCount.h"
#include "ShSvCmn.h"
#include "SvCmn.h"

/* process get product count request */
int
GetProductCount(OpenSOAPEnvelopePtr request,
				OpenSOAPEnvelopePtr *response,
				void *opt) {
	static char FuncName[] = "GetProductCount";
	int error = OPENSOAP_NO_ERROR;
	
	OpenSOAPBlockPtr body = NULL;
	const char* nameSpace = opt;
	
	/* --- parse request message --- */
	
	error = ParseRequestCommon(request,
							   "GetProductCount",
							   &body,
							   nameSpace);
	if (OPENSOAP_FAILED(error)) {
		error = CreateFaultMessage(response,
								   nameSpace,
								   "SOAP-ENV:service",
								   "cannont parse common part of "
								   "GetProductCount request",
								   NULL,
								   NULL);
		return error;
	}
	
	/* --- create request message --- */
	
	error = CreateResponseCommon(response,
								 "GetProductCountResponse",
								 &body,
								 nameSpace);
	ERROR_RETURN(error, FuncName, "create common part of response");
	
	error = OpenSOAPBlockSetChildValueMB(body,
										 "count",
										 "int",
										 &productSpecCount);
	ERROR_RETURN(error, FuncName, "set parameter: count");
	
	return error;
}
