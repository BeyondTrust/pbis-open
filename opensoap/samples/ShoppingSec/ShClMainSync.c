/*-----------------------------------------------------------------------------
 * $RCSfile: ShClMainSync.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClMainSync.h"
#include "ShClGetProductCount.h"
#include "ShClGetProductSpec.h"
#include "ShClGetStockQty.h"
#include "ShClPlaceOrderSync.h"
#include "ClCmn.h"

#include <OpenSOAP/OpenSOAP.h>

#include <stdio.h>
#include <string.h>

const char *DEFAULT_APP_NAME = "ShoppingClient";

static const char SERVICE_NS_URI[]
= "http://services.opensoap.jp/samples/Shopping/";

/* initialize variable given as command-line arguments */
int
InitializeVariables(ClientVariables *appVars,
					int argc,
					char **argv) {
	int error = 0;
	char **avEnd = argv + argc;
	
	error = InitializeOptionsCommon(appVars, &argv, avEnd);
	
	appVars->nameSpace = strdup(SERVICE_NS_URI);
	
	error = InitializeMethodArgs(appVars, &argv, avEnd, error);
	
	error = InitializeOptionsPostProc(appVars, DEFAULT_APP_NAME, error);
	
	return error;
}

/* main */
int
main(int argc,
	 char **argv) {
	ClientVariables appVars;
	
	int error = OpenSOAPInitialize(NULL);
	if (OPENSOAP_FAILED(error)) {
		fprintf(stderr,
				"OpenSOAP API Initialize Failed.\n"
				"OpenSOAP Error Code: 0x%04x\n",
				error);
		
		return 1;
	}

	error = InitializeVariables(&appVars, argc, argv);
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
			"method: %s\n"
			"code: %s\n"
			"qty: %ld\n",
			appVars.soapEndpoint ? appVars.soapEndpoint : "(null)",
			appVars.soapAction ? appVars.soapAction : "(null)",
			appVars.charEnc ? appVars.charEnc : "(null)",
			appVars.nameSpace ? appVars.nameSpace : "(null)",
			appVars.method ? appVars.method : "(null)",
			appVars.code ? appVars.code : "(null)" ,
			appVars.qty);
	
	error = CallMethodCommon(&appVars);
	
	if (appVars.method && strcmp(appVars.method, "PlaceOrder") == 0) {
		char* confirmation = NULL;
		error = PlaceOrderSync(appVars.soapEndpoint,
							   appVars.soapAction,
							   appVars.charEnc,
							   appVars.nameSpace,
							   appVars.code,
							   appVars.qty,
							   &confirmation);
		fprintf(stderr, "\nPlaceOrderResponse: %s\n\n", confirmation);
		free(confirmation);
	}
	
	OpenSOAPUltimate();
	
	return 0;
}
