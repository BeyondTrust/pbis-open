/*-----------------------------------------------------------------------------
 * $RCSfile: ShClMainSec.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClMainSec.h"
#include "ShClGetProductCount.h"
#include "ShClGetProductSpec.h"
#include "ShClGetStockQty.h"
#include "ShClPlaceOrderSec.h"
#include "ClCmn.h"

#include <OpenSOAP/OpenSOAP.h>

#include <stdio.h>
#include <string.h>

const char *DEFAULT_APP_NAME = "ShoppingClientSec";

static const char SERVICE_NS_URI[]
= "http://services.opensoap.jp/samples/ShoppingSec/";

const char *OPTION_SERVICE_PUB_KEY = "b";
const char *OPTION_CLIENT_PRIV_KEY = "v";

const char *USAGE_SEC = "\
  -b service_pub_key file name of service\'s public key for encryption.\
  -v client_priv_key file name of client\'s private key for authentication.\
  method             GetProductList, GetStockQty, PlaceOrder\n\
  code               product code (for GetStockQty and PlaceOrder)\n\
  qty                quantity of products to order (for PlaceOrder)\n\
";

/* initialize variable given as command-line arguments */
int
InitializeVariables(ClientVariables *appVars,
					char **servicePubKey,
					char **clientPrivKey,
					int argc,
					char **argv) {
	int error = 0;
	const size_t OPTION_PREFIX_LEN = strlen(OPTION_PREFIX);
	char **avEnd = argv + argc;
	char **argv0 = argv;
	
	error = InitializeOptionsCommon(appVars, &argv, avEnd);
	
	appVars->nameSpace = strdup(SERVICE_NS_URI);
	
	for (argv = argv0 + 1; argv != avEnd; ++argv) {
		const char *optBody = *argv;
		if (strncmp(optBody, OPTION_PREFIX, OPTION_PREFIX_LEN) == 0) {
			size_t optBodyLen = 0;
			optBody += OPTION_PREFIX_LEN;
			optBodyLen = strlen(optBody);
			if (strncmp(OPTION_SERVICE_PUB_KEY, optBody, optBodyLen) == 0
				&& !*servicePubKey) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				*servicePubKey = strdup(*argv);
			}
			else if (strncmp(OPTION_CLIENT_PRIV_KEY, optBody, optBodyLen) == 0
					 && !*clientPrivKey) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				*clientPrivKey = strdup(*argv);
			}
			else {
				++argv;
			}
		}
		else {
			break;
		}
	}
	
	error = InitializeMethodArgs(appVars, &argv, avEnd, error);
	
	error = InitializeOptionsPostProc(appVars, DEFAULT_APP_NAME, error);
	
	return error;
}

/* main */
int
main(int argc,
	 char **argv) {
	ClientVariables appVars;
	char *servicePubKey = NULL;
	char *clientPrivKey = NULL;
	
	int error = OpenSOAPInitialize(NULL);
	if (OPENSOAP_FAILED(error)) {
		fprintf(stderr,
				"OpenSOAP API Initialize Failed.\n"
				"OpenSOAP Error Code: 0x%04x\n",
				error);
		
		return 1;
	}
	
	error = InitializeVariables(&appVars,
								&servicePubKey,
								&clientPrivKey,
								argc,
								argv);
	if (OPENSOAP_FAILED(error)) {
		fprintf(stderr,
				"Application Initialize Failed.\n"
				"OpenSOAP Error Code: 0x%04x\n",
				error);
		OpenSOAPUltimate();

		return 1;
	}
	
	fprintf(stderr,
			"SOAPEndpoint: %s\n"
			"SOAPAction: %s\n"
			"character encoding: %s\n"
			"namespace: %s\n"
			"service\'s public key file: %s\n"
			"client\'s private key file: %s\n"
			"method: %s\n"
			"code: %s\n"
			"qty: %ld\n",
			appVars.soapEndpoint ? appVars.soapEndpoint : "(null)",
			appVars.soapAction ? appVars.soapAction : "(null)",
			appVars.charEnc ? appVars.charEnc : "(null)",
			appVars.nameSpace ? appVars.nameSpace : "(null)",
			servicePubKey ? servicePubKey : "(null)",
			clientPrivKey ? clientPrivKey : "(null)",
			appVars.method ? appVars.method : "(null)",
			appVars.code ? appVars.code : "(null)" ,
			appVars.qty);
	
	error = CallMethodCommon(&appVars);
	
	if (OPENSOAP_SUCCEEDED(error) &&
		strcmp(appVars.method, "PlaceOrder") == 0) {
		char* confirmation = NULL;
		error = PlaceOrder(appVars.soapEndpoint,
						   appVars.soapAction,
						   appVars.charEnc,
						   appVars.nameSpace,
						   servicePubKey,
						   clientPrivKey,
						   appVars.code,
						   appVars.qty,
						   &confirmation);
		fprintf(stderr, "\nPlaceOrderResponse: %s\n\n", confirmation);
		free(confirmation);
	}
	
	OpenSOAPUltimate();
	
	return 0;
}
