/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvMainSec.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvMainSec.h"
#include "ShSvCmn.h"
#include "ShSvGetProductCount.h"
#include "ShSvGetProductSpec.h"
#include "ShSvGetStockQty.h"
#include "ShSvPlaceOrderSec.h"
#include "SvSecCmn.h"
#include "SvCmn.h"

#include <string.h>

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

const char SERVICE_NS_URI[]
= "http://services.opensoap.jp/samples/ShoppingSec/";

#ifndef SERVICE_DIR
# define SERVICE_DIR "/usr/local/opensoap/services/ShoppingSec"
#endif

const char *SECURITY_KEY_FILE = SERVICE_DIR "/Shopping.keys";

/* main */
int
main(void) {
	static char FuncName[] = "main";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPServicePtr service = NULL;
	FILE *fpSec;
	Options opt;
	
#if !defined(CONNECT_TYPE_CGI)
	fpLog = fopen(SHOPPING_LOG_FILE, "a");
#else
	fpLog = stderr;
#endif /* CONNECT_TYPE_CGI */
	
	LOG_MSG(FuncName, "[secure version] start logging...");
	
	/* initialize application data */
	
	error = LoadProductSpec();
	ERROR_RETURN(error, FuncName, "load product spec");

	/* initialize namespace */
	
	strcpy(opt.nameSpace, SERVICE_NS_URI);
		
	/* initialize security keys */
	
	fpSec = fopen(SECURITY_KEY_FILE, "r");
	ERROR_RETURN((fpSec == 0), FuncName, "open security key name file");
	
  	error = (fgets(opt.opt1, NAME_SZ, fpSec) == 0);
	ERROR_RETURN(error, FuncName, "read private key of service");
 	opt.opt1[strlen(opt.opt1) - 1] = '\0';
	
	error = (fgets(opt.opt2, NAME_SZ, fpSec) == 0);
	ERROR_RETURN(error, FuncName, "read public key of client");
	opt.opt2[strlen(opt.opt2) - 1] = '\0';
	
	fclose(fpSec);
	
	LOG_STRING(FuncName, "privKeyName", opt.opt1);
	LOG_STRING(FuncName, "pubKeyName ", opt.opt2);
	
	/* initialize service */
	error = OpenSOAPInitialize(NULL);
	ERROR_RETURN(error, FuncName, "initialize service");;
	
	/* create service */
	error = OpenSOAPServiceCreateMB(&service,
									"ShoppingService",
									CONNECT_TYPE,
									0);
	if (OPENSOAP_SUCCEEDED(error)) {
		
		/* register methods to service */
		
		error = OpenSOAPServiceRegisterMB(service,
										  "GetProductCount",
										  GetProductCount,
										  (void *)&opt);
		ERROR_CHECK(error, FuncName, "register GetProductCount method");
		
		error = OpenSOAPServiceRegisterMB(service,
										  "GetProductSpec",
										  GetProductSpec,
										  (void *)&opt);
		ERROR_CHECK(error, FuncName, "register GetProductSpec method");
		
		error = OpenSOAPServiceRegisterMB(service,
										  "GetStockQty",
										  GetStockQty,
										  (void *)&opt);
		ERROR_CHECK(error, FuncName, "register GetStockQty method");
		
		error = OpenSOAPServiceRegisterMB(service,
										  "PlaceOrder",
										  PlaceOrder,
										  (void *)&opt);
		ERROR_CHECK(error, FuncName, "register PlaceOrder(Sec) method");
		
		if (OPENSOAP_SUCCEEDED(error)) {
			/* activate service */
			error = OpenSOAPServiceRun(service);
			ERROR_CHECK(error, FuncName, "run service");
		}
		
		/* release service */
		error = OpenSOAPServiceRelease(service);
		ERROR_CHECK(error, FuncName, "release");
	}
	else {
		ERROR_MSG(error, FuncName, "create");
	}
	
	/* finalize service */
	OpenSOAPUltimate();

#if !defined(CONNECT_TYPE_CGI)
	fclose(fpLog);
#endif /* CONNECT_TYPE_CGI */
	
	return error;
}
