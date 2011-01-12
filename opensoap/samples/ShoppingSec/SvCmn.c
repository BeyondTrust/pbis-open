/*-----------------------------------------------------------------------------
 * $RCSfile: SvCmn.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "SvCmn.h"

#include <stdio.h>
#include <string.h>

static const char SERVICE_NS_PREFIX[] = "m";

/* write message envelope as text file */
int
WriteMessage(OpenSOAPEnvelopePtr env,
			 const char *charEnc,
			 FILE *fp) {
	static char FuncName[] = "WriteMessage";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPByteArrayPtr envBuf = NULL;
	
	error = (env ? error : APPLICATION_ERROR);
	ERROR_RETURN(error, FuncName, "find envelope");
	
	error = (fp ? error : APPLICATION_ERROR);
	ERROR_RETURN(error, FuncName, "find output file pointer");
	
	error = OpenSOAPByteArrayCreate(&envBuf);
	if (OPENSOAP_SUCCEEDED(error)) {
		
		error = OpenSOAPEnvelopeGetCharEncodingString(env, charEnc, envBuf);
		if (OPENSOAP_SUCCEEDED(error)) {
			const unsigned char *envBeg = NULL;
			size_t envSz = 0;
			
			error = OpenSOAPByteArrayGetBeginSizeConst(envBuf,
													   &envBeg,
													   &envSz);
			if (OPENSOAP_SUCCEEDED(error)) {
				fwrite(envBeg, 1, envSz, fp);
			}
			else {
				ERROR_MSG(error, FuncName, "get begin&size of byte array");
			}
		}
		else {
			ERROR_MSG(error, FuncName, "get character encoding string");
		}
		
		error = OpenSOAPByteArrayRelease(envBuf);
		ERROR_RETURN(error, FuncName, "release byte array");
	}
	else {
		ERROR_MSG(error, FuncName, "create byte array");
	}
	
	return error;
}

/* write message envelope with label string as text file */
int
WriteLabeledMessage(OpenSOAPEnvelopePtr env,
					const char *charEnc,
					const char *label,
					FILE *fp) {
	static char FuncName[] = "WriteLabeledMessage";
    int error = OPENSOAP_NO_ERROR;
	
	if (!label) {
		label = "";
	}
	
	fprintf(fp, "\n===== begin %s soap message =====\n", label);
	
	error = WriteMessage(env, charEnc, fp);
	ERROR_RETURN(error, FuncName, "release byte array");
	
	fprintf(fp, "\n===== end %s soap message =====\n", label);
	
	return error;
}

/* get parameter value of string type from message */
int
GetStringParameter(OpenSOAPBlockPtr body,
				   const char *paramName,
				   char **paramValue) {
	static char FuncName[] = "GetStringParameter";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPStringPtr param = NULL;
	size_t paramLen = 0;
	
	error = OpenSOAPBlockGetChildValueMB(body, paramName, "string", &param);
	ERROR_RETURN(error, FuncName, "get value of parameter");
	
	error = OpenSOAPStringGetLengthMB(param, &paramLen);
	ERROR_RETURN(error, FuncName, "get string length of parameter");
	
	*paramValue = malloc(paramLen + 1);	
	error = (*paramValue ? error : OPENSOAP_MEM_BADALLOC);
	ERROR_RETURN(error, FuncName, "allocate memory");
	
	error = OpenSOAPStringGetStringMB(param, &paramLen, *paramValue);
	ERROR_RETURN(error, FuncName, "get string from parameter");
	
	return error;
}

/* add attribute to element */
int
AddAttribute(OpenSOAPBlockPtr block,
			 const char *elmName,
			 const char *attrName,
			 const char *attrType,
			 void *attrValue,
			 OpenSOAPXMLAttrPtr *attr) {
	static char FuncName[] = "AddAttribute";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPXMLElmPtr elm = NULL;
	
    error = OpenSOAPBlockGetChildMB(block, elmName, &elm);
	ERROR_RETURN(error, FuncName, "get specifed element");
	
    error = OpenSOAPXMLElmAddAttributeMB(elm,
										 attrName,
										 attrType,
										 attrValue,
										 attr);
	ERROR_RETURN(error, FuncName, "add attribute to element");
	
	return error;
}

/* add attribute with namespace to element */
int
AddAttributeWithNamespace(OpenSOAPBlockPtr block,
						  const char *elmName,
						  const char *attrName,
						  const char *attrType,
						  void *attrValue,
						  const char *nsUri,
						  const char *nsPrefix) {
	static char FuncName[] = "AddAttributeWithNamespace";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPXMLAttrPtr attr = NULL;
	
	error = AddAttribute(block, elmName, attrName, attrType, attrValue, &attr);
	ERROR_RETURN(error, FuncName, "add attribute to element");
	
    error = OpenSOAPXMLAttrSetNamespaceMB(attr, nsUri, nsPrefix);
	ERROR_RETURN(error, FuncName, "set namespace to attribute");
	
	return error;
}

/* parse common part of request message */
int
ParseRequestCommon(OpenSOAPEnvelopePtr request,
				   const char *requestName,
				   OpenSOAPBlockPtr *body,
				   const char *nameSpace) {
	static char FuncName[] = "ParseRequestCommon";
	int error = OPENSOAP_NO_ERROR;
	int isSameNs = 0;
	
	error = OpenSOAPEnvelopeGetBodyBlockMB(request, requestName, body);
	ERROR_RETURN(error, FuncName, "get body block");
	
	error = OpenSOAPBlockIsSameNamespaceMB(*body, nameSpace, &isSameNs);
	ERROR_RETURN(error, FuncName, "complete namespace matching");
	
	error = (isSameNs ? error : OPENSOAP_NOT_CATEGORIZE_ERROR);
	ERROR_RETURN(error, FuncName, "match namespace");
	
	return error;
}

/* create common part of response message */
int
CreateResponseCommon(OpenSOAPEnvelopePtr *response,
					 const char *responseName,
					 OpenSOAPBlockPtr *body,
					 const char *nameSpace) {
	static char FuncName[] = "CreateResponseCommon";
	int error = OPENSOAP_NO_ERROR;
	
	error = OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	ERROR_RETURN(error, FuncName, "create envelope");
	
	/* add body response block */
	error = OpenSOAPEnvelopeAddBodyBlockMB(*response, responseName, body);
	ERROR_RETURN(error, FuncName, "add body part");
	
	/* set namespace to response block */
	error = OpenSOAPBlockSetNamespaceMB(*body,
										nameSpace,
										SERVICE_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace of body part");
	
	return error;
}

/* create fault response message */
int
CreateFaultMessage(OpenSOAPEnvelopePtr *response,
				   const char *nameSpace,
				   const char *faultcode,
				   const char *faultstring,
				   const char *faultactor,
				   const char *detail) {
	static char FuncName[] = "CreateFaultMessage";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	OpenSOAPStringPtr faultcodeOssp = NULL;
	OpenSOAPStringPtr faultstringOssp = NULL;
	OpenSOAPStringPtr faultactorOssp = NULL;
	OpenSOAPStringPtr detailOssp = NULL;
	
	error = CreateResponseCommon(response,
								 "Fault",
								 &body,
								 nameSpace);
	ERROR_RETURN(error, FuncName, "create common part of response");
	
	error = OpenSOAPStringCreateWithMB(faultcode, &faultcodeOssp);
	ERROR_RETURN(error, FuncName, "create string: faultcode");
	
	error = OpenSOAPBlockSetChildValueMB(body,
										 "faultcode",
										 "string",
										 &faultcodeOssp);
	ERROR_RETURN(error, FuncName, "set parameter: faultcode");
	
	error = OpenSOAPStringCreateWithMB(faultstring, &faultstringOssp);
	ERROR_RETURN(error, FuncName, "create string: faultstring");
	
	error = OpenSOAPBlockSetChildValueMB(body,
										 "faultstring",
										 "string",
										 &faultstringOssp);
	ERROR_RETURN(error, FuncName, "set parameter: faultstring");
	
	if (faultactor != NULL) {
		error = OpenSOAPStringCreateWithMB("faultactor", &faultactorOssp);
		ERROR_RETURN(error, FuncName, "create string: faultactor");
		
		error = OpenSOAPBlockSetChildValueMB(body,
											 "faultactor",
											 "string",
											 &faultactorOssp);
		ERROR_RETURN(error, FuncName, "set parameter: faultactor");
	}
	   
	if (detail != NULL) {
		
		error = OpenSOAPStringCreateWithMB(detail, &detailOssp);
		ERROR_RETURN(error, FuncName, "create string: detail");
		
		error = OpenSOAPBlockSetChildValueMB(body,
											 "detail",
											 "string",
											 &detailOssp);
		ERROR_RETURN(error, FuncName, "set parameter: detail");
	}
	
	return error;
}
