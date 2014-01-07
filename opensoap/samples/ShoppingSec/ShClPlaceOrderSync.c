/*-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderSync.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClPlaceOrderSync.h"
#include "ShClPlaceOrderCmn.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

/* submit place order request */
/* return value should be free()'d after using */
int
PlaceOrderSync(const char *endpoint,
			   const char *soapAction,
			   const char *charEnc,
			   const char *nameSpace,
			   const char *code,
			   long qty,
			   char **confirmation) {
	static char FuncName[] = "PlaceOrderSync";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	
	*confirmation = NULL;
	
	error = CreatePlaceOrderRequest(nameSpace,
									code,
									qty,
									&request);
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
