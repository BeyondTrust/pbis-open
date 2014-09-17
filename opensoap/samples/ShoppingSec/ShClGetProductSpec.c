/*-----------------------------------------------------------------------------
 * $RCSfile: ShClGetProductSpec.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClGetProductSpec.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

/* create get product specification request message */
static
int
CreateGetProductSpecRequest(const char *nameSpace,
							long index,
							OpenSOAPEnvelopePtr *request) {
	static char FuncName[] = "CreateGetProductSpecRequest";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr body = NULL;
	
	error = CreateRequestCommon(request,
								"GetProductSpec",
								&body,
								nameSpace);
	ERROR_RETURN(error, FuncName, "create common part of request");
	
	error = OpenSOAPBlockSetChildValueMB(body, "index", "int", &index);
	ERROR_RETURN(error, FuncName, "set parameter: index");
	
	return error;
}

/* parse get product specification response message */
static
int
ParseGetProductSpecResponse(OpenSOAPEnvelopePtr response,
							const char *nameSpace,
							ProductListItemPtr productList) {
	static char FuncName[] = "ParseGetProductSpecResponse";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	int isFault = 0;
	char *manufacturer = NULL;
	char *name = NULL;
	char *code = NULL;
	long price = -1;
	
	error = ParseResponseCommon(response,
								"GetProductSpecResponse",
								&body,
								nameSpace,
								&isFault);
	ERROR_RETURN(error, FuncName, "parse common part of response");
	
	if (isFault) {
		return error;
	}
	
	error = GetStringParameter(body, "manufacturer", &manufacturer);
	ERROR_RETURN(error, FuncName, "get parameter: manufacturer");
	
	error = GetStringParameter(body, "name", &name);
	ERROR_RETURN(error, FuncName, "get parameter: name");
	
	error = GetStringParameter(body, "code", &code);
	ERROR_RETURN(error, FuncName, "get parameter: code");
	
	error = OpenSOAPBlockGetChildValueMB(body, "price", "int", &price);
	ERROR_RETURN(error, FuncName, "get parameter: price");
	
	fprintf(stderr,
			"\nGetProductSpecResponse:\n"
			"    manufacturer=%s\n"
			"    name=%s\n"
			"    code=%s\n"
			"    price=%ld\n\n",
			manufacturer ? manufacturer : "(null)",
			name ? name : "(null)",
			code ? code : "(null)",
			price);
	
	if (!manufacturer || !name || !code || price == -1) {
		error = APPLICATION_ERROR;
		ERROR_MSG(error, FuncName, "get all of parameters");
	}
	
	if (!ProductListItemSetValues(productList,
								  manufacturer,
								  name,
								  code,
								  price)) {
		error = APPLICATION_ERROR;
		ERROR_MSG(error, FuncName, "alloc memory");
	}
	
	free(code);
	free(name);
	free(manufacturer);
	
	return error;
}

/* submit get product specification request */
int
GetProductSpec(const char *endpoint,
			   const char *soapAction,
			   const char *charEnc,
			   const char *nameSpace,
			   long index,
			   ProductListItemPtr productList) {
	static char FuncName[] = "GetProductSpec";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	
	error = CreateGetProductSpecRequest(nameSpace, index, &request);
	if (OPENSOAP_SUCCEEDED(error)) {
		OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = ParseGetProductSpecResponse(response,
												nameSpace,
												productList);
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
