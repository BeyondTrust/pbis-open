/*-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderSec.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClPlaceOrderSec.h"
#include "ShClPlaceOrderCmn.h"
#include "ClSecCmn.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>
#include <OpenSOAP/Security.h>

/* submit place order request */
/* return value should be free()'d after using */
int
PlaceOrder(const char *endpoint,
		   const char *soapAction,
		   const char *charEnc,
		   const char *nameSpace,
		   const char *pubKeyName,
		   const char *privKeyName,
		   const char *code,
		   long qty,
		   char **confirmation) {
	static char FuncName[] = "PlaceOrder";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	
	*confirmation = NULL;
	
	error = CreatePlaceOrderRequest(nameSpace,
									code,
									qty,
									&request);
	ERROR_RETURN(error, FuncName, "create request message");
	
	error = EncryptParameter(&request, "PlaceOrder", "code");
	ERROR_RETURN(error, FuncName, "encrypt parameter: code");
	
	error = EncryptParameter(&request, "PlaceOrder", "qty");
	ERROR_RETURN(error, FuncName, "encrypt parameter: qty");
	
	error = WriteLabeledMessage(request,
								charEnc,
								"raw request (client)",
								stderr);
	ERROR_RETURN(error, FuncName, "write labeled message");
	
	error = OpenSOAPSecEncWithFile(request, pubKeyName);
	WriteLabeledMessage(request,
						charEnc,
						"encrypted request (client)",
						stderr);
	
	if (OPENSOAP_SUCCEEDED(error)) {
		error = OpenSOAPSecAddSignWithFile(request,
										   OPENSOAP_HA_SHA,
										   privKeyName,
										   NULL);
		WriteLabeledMessage(request,
							charEnc,
							"signed request (client)",
							stderr);
		if (OPENSOAP_SUCCEEDED(error)) {
			OpenSOAPEnvelopePtr response = NULL;
			
			error = InvokeService(endpoint,
								  soapAction,
								  charEnc,
								  &request,
								  &response);
			if (OPENSOAP_SUCCEEDED(error)) {
				
				error = WriteLabeledMessage(response,
											charEnc,
											"raw response (client)",
											stderr);
				ERROR_RETURN(error, FuncName, "write labeled message");
				
				error = OpenSOAPSecVerifySignWithFile(response, pubKeyName);
				ERROR_CHECK(error, FuncName, "verify signature");
				LOG_MSG(FuncName, "Service's signature successfully verified");
				
				error = OpenSOAPSecDecWithFile(response, privKeyName);
				ERROR_CHECK(error, FuncName, "decrypt request message");
				LOG_MSG(FuncName, "Encrypted message successfully decrypted");
				
				error = WriteLabeledMessage(response,
											charEnc,
											"decrypted response (client)",
											stderr);
				ERROR_RETURN(error, FuncName, "write labeled message");
				
				error = ParsePlaceOrderResponse(response,
												nameSpace,
												confirmation);
				ERROR_CHECK(error, FuncName, "parse resopnse message");
				
				error = OpenSOAPEnvelopeRelease(response);
				
				ERROR_CHECK(error, FuncName, "release response envelope");
				
			} else {
				ERROR_MSG(error, FuncName, "invoke service");
			}
		}
		else {
			ERROR_MSG(error, FuncName, "add signature");
		}
	}
	else {
		ERROR_MSG(error, FuncName, "encrypt envelope");
	}
	error = OpenSOAPEnvelopeRelease(request);
	ERROR_RETURN(error, FuncName, "release request envelope");
	
    return error;
}
