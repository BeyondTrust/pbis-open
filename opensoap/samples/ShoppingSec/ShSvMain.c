/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvMain.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvMain.h"
#include "ShSvCmn.h"
#include "ShSvGetProductCount.h"
#include "ShSvGetProductSpec.h"
#include "ShSvGetStockQty.h"
#include "ShSvPlaceOrder.h"
#include "SvCmn.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

#include <string.h>

static const char SERVICE_NS_URI[]
= "http://services.opensoap.jp/samples/Shopping/";

/* main */
int
main(void) {
	static char FuncName[] = "main";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPServicePtr service = NULL;
	Options opt;
	
#if !defined(CONNECT_TYPE_CGI)
	fpLog = fopen(SHOPPING_LOG_FILE, "a");
#else
	fpLog = stderr;
#endif /* CONNECT_TYPE_CGI */

	LOG_MSG(FuncName, "start logging...\n");
	fprintf(stderr, "ppp\n");
	
	/* initialize application data */
	
	error = LoadProductSpec();
	ERROR_RETURN(error, FuncName, "load product spec");
	
	/* initialize namespace */
	
	strcpy(opt.nameSpace, SERVICE_NS_URI);
	
	/* initialize service */
	
	error = OpenSOAPInitialize(NULL);
	ERROR_RETURN(error, FuncName, "initialize service");
	
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
		ERROR_CHECK(error, FuncName, "register PlaceOrder method");
		
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
