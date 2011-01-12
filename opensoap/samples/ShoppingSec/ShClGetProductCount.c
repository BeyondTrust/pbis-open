/*-----------------------------------------------------------------------------
 * $RCSfile: ShClGetProductCount.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClGetProductCount.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

/* create get product count request message */
static
int
CreateGetProductCountRequest(const char *nameSpace,
							 OpenSOAPEnvelopePtr *request) {
	static char FuncName[] = "CreateGetProductCountRequest";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	
	error = CreateRequestCommon(request,
								"GetProductCount",
								&body,
								nameSpace);
	ERROR_RETURN(error, FuncName, "create request");
	
	return error;
}

/* parse get product count response message */
static
int
ParseGetProductCountResponse(OpenSOAPEnvelopePtr response,
							 const char *nameSpace,
							 long *count) {
	static char FuncName[] = "ParseGetProductCountResponse";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	int isFault = 0;
	
	error = ParseResponseCommon(response,
								"GetProductCountResponse",
								&body,
								nameSpace,
								&isFault);
	ERROR_RETURN(error, FuncName, "parse common part of response");
	
	if (isFault) {
		return error;
	}
	
	error = OpenSOAPBlockGetChildValueMB(body, "count", "int", count);
	ERROR_RETURN(error, FuncName, "get parameter: count");
	
	return error;
}

/* submit get product count request */
int
GetProductCount(const char *endpoint,
				const char *soapAction,
				const char *charEnc,
				const char *nameSpace,
				long *count) {
	static char FuncName[] = "GetProductCount";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;

	*count = -1;
	
	error = CreateGetProductCountRequest(nameSpace, &request);
	if (OPENSOAP_SUCCEEDED(error)) {
		OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = ParseGetProductCountResponse(response, nameSpace, count);
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

