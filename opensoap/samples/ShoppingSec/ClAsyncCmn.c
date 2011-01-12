/*-----------------------------------------------------------------------------
 * $RCSfile: ClAsyncCmn.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ClAsyncCmn.h"
#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

static const char OPENSOAP_HEADER_NS_URI[] = "http://header.opensoap.jp/1.0/";
static const char OPENSOAP_HEADER_NS_PREFIX[] = "opensoap-header";

static int ttl = 80;

/* add header elements for async mode */
int
AddAsyncHeader(OpenSOAPEnvelopePtr *request) {
	static char FuncName[] = "AddAsyncHeader";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr header;
	OpenSOAPXMLElmPtr ttlElm = NULL;
	OpenSOAPStringPtr typeStr = NULL;
	OpenSOAPStringPtr asyncStr = NULL;
	OpenSOAPXMLElmPtr asyncElm = NULL;
		
	/* add opensoap-header block */
	error = OpenSOAPEnvelopeAddHeaderBlockMB(*request,
											 "opensoap-header-block",
											 &header); 
	ERROR_RETURN(error, FuncName, "add header part");
	
	/* set namespace to opensoap-header block */
	error = OpenSOAPBlockSetNamespaceMB(header,
										OPENSOAP_HEADER_NS_URI,
										OPENSOAP_HEADER_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace to header part");
	
	/* set ttl element to opensoap-header */
	error = OpenSOAPBlockSetChildValueMB(header, "ttl", "int", &ttl);
	ERROR_RETURN(error, FuncName, "set ttl element");
	
	/* set namespace to ttl element */
	error = OpenSOAPBlockGetChildMB(header, "ttl", &ttlElm);
	ERROR_RETURN(error, FuncName, "get ttl element");
	
	error = OpenSOAPXMLElmSetNamespaceMB(ttlElm,
										 OPENSOAP_HEADER_NS_URI,
										 OPENSOAP_HEADER_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace to ttl element");
	
	/* add type attribute with namespace to ttl element */
	
	error = OpenSOAPStringCreateWithMB("second", &typeStr);
	ERROR_RETURN(error, FuncName, "create string for type attr");
	
	error = AddAttributeWithNamespace(header,
									  "ttl",
									  "type",
									  "string",
									  &typeStr,
									  OPENSOAP_HEADER_NS_URI,
									  OPENSOAP_HEADER_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "add type attr to ttl element");
	
	/* add async element */
	
	error = OpenSOAPStringCreateWithMB("true", &asyncStr);
	ERROR_RETURN(error, FuncName, "create string for async");
	
	error = OpenSOAPBlockSetChildValueMB(header, "async", "string", &asyncStr);
	ERROR_RETURN(error, FuncName, "set async element");
	
	/* set namespace to async element */
	
	error = OpenSOAPBlockGetChildMB(header, "async", &asyncElm);
	ERROR_RETURN(error, FuncName, "get async element");
	
	error = OpenSOAPXMLElmSetNamespaceMB(asyncElm,
										 OPENSOAP_HEADER_NS_URI,
										 OPENSOAP_HEADER_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace to async element");
	
	return error;
}
