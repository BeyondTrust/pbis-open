/*-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderAsync.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClPlaceOrderAsync.h"
#include "ShClPlaceOrderCmn.h"
#include "ClAsyncCmn.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

/* submit place order request in asyncronized mode */
int
PlaceOrderAsync(const char *endpoint,
				const char *soapAction,
				const char *charEnc,
				const char *nameSpace,
				const char *code,
				long qty,
				FILE *fp) {
	static char FuncName[] = "PlaceOrderAsync";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	OpenSOAPEnvelopePtr response = NULL;
	
	error = CreatePlaceOrderRequest(nameSpace,
									code,
									qty,
									&request);
	ERROR_RETURN(error, FuncName, "create request message");
	
	error = AddAsyncHeader(&request);
	ERROR_RETURN(error, FuncName, "add async header");
	
	error = InvokeService(endpoint, soapAction, charEnc, &request, &response);
	if (OPENSOAP_SUCCEEDED(error)) {
		
		error = WriteMessage(response, NULL, fp);
		ERROR_CHECK(error, FuncName, "output resopnse message to file");
		
		error = OpenSOAPEnvelopeRelease(response);
		ERROR_CHECK(error, FuncName, "release response envelope");
	} else {
		ERROR_MSG(error, FuncName, "invoke service");
	}
	error = OpenSOAPEnvelopeRelease(request);
	ERROR_RETURN(error, FuncName, "release request envelope");
	
    return error;
}

/* submit confirm order request in asyncronized mode */
int
ConfirmOrder(FILE *fp,
			 const char *endpoint,
			 const char *soapAction,
			 const char *charEnc,
			 const char *nameSpace,
			 char **confirmation) {
	static char FuncName[] = "ConfirmOrder";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	
	*confirmation = NULL;
	
	error = ReadMessage(fp, charEnc, &request);
	ERROR_RETURN(error, FuncName, "input request message from file");
	
	if (OPENSOAP_SUCCEEDED(error)) {
		OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = ParsePlaceOrderResponse(response,
											nameSpace,
											confirmation);
			ERROR_CHECK(error, FuncName, "parse resopnse message");
			
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
