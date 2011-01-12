/*-----------------------------------------------------------------------------
 * $RCSfile: ShClGetStockQty.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClGetStockQty.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

/* create get stock quantity request message */
static
int
CreateGetStockQtyRequest(const char *nameSpace,
						 const char *code,
						 OpenSOAPEnvelopePtr *request) {
	static char FuncName[] = "CreateGetStockQtyRequest";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr body = NULL;
	OpenSOAPStringPtr codeStr = NULL;
	
	error = CreateRequestCommon(request,
								"GetStockQty",
								&body,
								nameSpace);
	ERROR_RETURN(error, FuncName, "create common part of request");

	error = OpenSOAPStringCreateWithMB(code, &codeStr);
	ERROR_RETURN(error, FuncName, "create string for code");
	
	error = OpenSOAPBlockSetChildValueMB(body, "code", "string", &codeStr);
	ERROR_RETURN(error, FuncName, "set parameter: code");
	
	return error;
}

/* parse get stock quantity response message */
static
int
ParseGetStockQtyResponse(OpenSOAPEnvelopePtr response,
						 const char *nameSpace,
						 long *qty) {
	static char FuncName[] = "ParseGetStockQtyResponse";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	int isFault = 0;
	
	error = ParseResponseCommon(response,
								"GetStockQtyResponse",
								&body,
								nameSpace,
								&isFault);
	ERROR_RETURN(error, FuncName, "parse common part of response");
	
	if (isFault) {
		return error;
	}
	
	error = OpenSOAPBlockGetChildValueMB(body, "qty", "int", qty);
	ERROR_RETURN(error, FuncName, "get parameter: qty");
	
	return error;
}

/* submit get stock quantity request */
int
GetStockQty(const char *endpoint,
			const char *soapAction,
			const char *charEnc,
			const char *nameSpace,
			const char *code,
			long *qty) {
	static char FuncName[] = "GetStockQty";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	
	*qty = -1;
	
	error = CreateGetStockQtyRequest(nameSpace, code, &request);
	if (OPENSOAP_SUCCEEDED(error)) {
		OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = ParseGetStockQtyResponse(response, nameSpace, qty);
			ERROR_CHECK(error, FuncName, "parse response message");
			
			error = OpenSOAPEnvelopeRelease(response);
  			ERROR_CHECK(error, FuncName, "release response envelope");
		} else {
			ERROR_MSG(error, FuncName, "invoke service");
		}
		error = OpenSOAPEnvelopeRelease(request);
		ERROR_RETURN(error, FuncName, "release request envelope");
	}
	else {
		ERROR_MSG(error, FuncName, "create request message");
	}
	
    return error;
}

