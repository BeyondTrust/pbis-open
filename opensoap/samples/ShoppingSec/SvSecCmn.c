/*-----------------------------------------------------------------------------
 * $RCSfile: SvSecCmn.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "SvSecCmn.h"
#include "SvCmn.h"

#include <OpenSOAP/Transport.h>
#include <OpenSOAP/Security.h>

#include <string.h>

static const char OPENSOAP_ENCRYPT_NS_URI[]
= "http://security.opensoap.jp/1.0/";

static const char OPENSOAP_ENCRYPT_NS_PREFIX[] = "s";

/* encrypt specified parameter in body block */
int
EncryptParameter(OpenSOAPEnvelopePtr *env,
				 const char *method,
				 const char *param) {
	static char FuncName[] = "EncryptParameter";
    int error = OPENSOAP_NO_ERROR;
	int encryption = 1;
	OpenSOAPBlockPtr body = NULL;
	
 	error = OpenSOAPEnvelopeGetBodyBlockMB(*env, method, &body);
	ERROR_RETURN(error, FuncName, "get body block");
	
	error = AddAttributeWithNamespace(body,
									  param,
									  "encrypt",
									  "boolean",
									  &encryption,
									  OPENSOAP_ENCRYPT_NS_URI,
									  OPENSOAP_ENCRYPT_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "add encrypt attr to parameter");
	
	return error;
}
