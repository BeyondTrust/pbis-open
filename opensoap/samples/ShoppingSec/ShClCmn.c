/*-----------------------------------------------------------------------------
 * $RCSfile: ShClCmn.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClProductList.h"
#include "ShClGetStockQty.h"
#include "ShClCmn.h"
#include "ClCmn.h"

#include <OpenSOAP/OpenSOAP.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const int FILE_DELM =
#if defined(__GNUC__)
'/';
#else
// 
'\\';
#endif

const char *DEFAULT_ENDPOINT = "http://localhost/cgi-bin/soapInterface.cgi";

const char *DEFAULT_CHARENC = "UTF-8";
const char *OPTION_PREFIX = "-";

const char *OPTION_SOAPENDPOINT = "s";
const char *OPTION_SOAPACTION = "a";
const char *OPTION_CHARENC = "c";

const char *USAGE_COMMON = "\
Usage: %s [-s endpoint] [-a soapaction] [-c char_enc] method [code] [qty]\n\
  -s endpoint    SOAP service endpoint URI\n\
  -a soapaction  value of SOAPAction HTTP-Header\n\
  -c char_enc    character encoding of request message (default: UTF-8)\n\
";

/* get name of this program */
char *
GetAppName(const char *argv0) {
	char *ret = NULL;
	size_t  appNameLen = 0;
	const char *appName = strrchr(argv0, FILE_DELM);
	
	if (appName) {
		++appName;
	}
	else {
		appName = argv0;
	}
	appNameLen = strlen(appName);
	ret = malloc(appNameLen + 1);
	
	return ret ? strcpy(ret, appName) : NULL;
}

/* initialize common options */
int
InitializeOptionsCommon(ClientVariables *appVars,
						char ***argv,
						char **avEnd) {
	int error = OPENSOAP_NO_ERROR;
	const size_t OPTION_PREFIX_LEN = strlen(OPTION_PREFIX);
	
	memset((void *)appVars, 0, sizeof(*appVars)); /* clear */
	
	appVars->appName = GetAppName(**argv);
	
	for (++*argv; *argv != avEnd; ++*argv) {
		const char *optBody = **argv;
		if (strncmp(optBody, OPTION_PREFIX, OPTION_PREFIX_LEN) == 0) {
			size_t optBodyLen = 0;
			optBody += OPTION_PREFIX_LEN;
			optBodyLen = strlen(optBody);
			if (strncmp(OPTION_SOAPENDPOINT, optBody, optBodyLen) == 0
				&& !appVars->soapEndpoint) {
				++*argv;
				if (*argv == avEnd) {
					break;
				}
				appVars->soapEndpoint = strdup(**argv);
			}
			else if (strncmp(OPTION_SOAPACTION, optBody, optBodyLen) == 0
					 && !appVars->soapAction) {
				++*argv;
				if (*argv == avEnd) {
					break;
				}
				appVars->soapAction = strdup(**argv);
			}
			else if (strncmp(OPTION_CHARENC, optBody, optBodyLen) == 0
					 && !appVars->charEnc) {
				++*argv;
				if (*argv == avEnd) {
					break;
				}
				appVars->charEnc = strdup(**argv);
				if (!appVars->charEnc) {
					fprintf(stderr, "memory allocate error\n");
					error = APPLICATION_ERROR;
					*argv = avEnd;
					break;
				}
			}
			else {
 				break;
			}
		}
		else {
			break;
		}
	}
	
	return error;
}

/* initialize method arguments */
int
InitializeMethodArgs(ClientVariables *appVars,
					 char ***argv,
					 char **avEnd,
					 int error) {
	
	if (*argv != avEnd) {
		
		appVars->method = strdup(**argv);
		++*argv;
		
		if (*argv != avEnd) {
			
			appVars->code = strdup(**argv);
			++*argv;
			
			if (*argv != avEnd) {
				
				appVars->qty = atol(**argv);
				++*argv;
			}
		}
	}
	
	return error;
}

/* initialize options postprocessing */
int
InitializeOptionsPostProc(ClientVariables *appVars,
						  const char *defaultAppName,
						  int error) {
	if (!appVars->soapEndpoint) {
		appVars->soapEndpoint = strdup(DEFAULT_ENDPOINT);
	}
	
	if (!appVars->charEnc) {
		appVars->charEnc = strdup(DEFAULT_CHARENC);
		if (!appVars->charEnc) {
			fprintf(stderr, "memory allocate error\n");
			error = APPLICATION_ERROR;
		}
	}
	
	if (error) {
		fprintf(stderr, USAGE_COMMON,
			   appVars->appName ? appVars->appName : defaultAppName);
	}
	
	return error;
}

/* call method common part */
int
CallMethodCommon(ClientVariables *appVars) {
	int error = OPENSOAP_NO_ERROR;

	if (!appVars->method)
		return OPENSOAP_PARAMETER_BADVALUE;
	if (strcmp(appVars->method, "GetProductList") == 0) {
		ProductListPtr productList;
		productList = GetProductList(appVars->soapEndpoint,
									 appVars->soapAction,
									 appVars->charEnc,
									 appVars->nameSpace);
		if (productList && productList->productCount) {
			ProductListItemPtr products = productList->productList;
			size_t i = 0;
			fprintf(stderr,
					"GetProductListResponse:\n"
					"(Manufacturer), (Name), (Code), (Price)\n");
			for (; i < productList->productCount; ++i) {
				fprintf(stderr,
						"%s, %s, %s, %ld\n",
						products[i].manufacturer,
						products[i].name,
						products[i].code,
						products[i].price);
			}
			fprintf(stderr, "\n");
			ReleaseProductList(productList);
		}
	}
	else if (strcmp(appVars->method, "GetStockQty") == 0) {
		long qty = -1;
		error = GetStockQty(appVars->soapEndpoint,
							appVars->soapAction,
							appVars->charEnc,
							appVars->nameSpace,
							appVars->code,
							&qty);
		fprintf(stderr, "\nGetStockQtyResponse: qty=%ld\n\n", qty);
	}
	
	return error;
}
