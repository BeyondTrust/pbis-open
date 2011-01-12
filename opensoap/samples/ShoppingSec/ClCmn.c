/*-----------------------------------------------------------------------------
 * $RCSfile: ClCmn.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ClCmn.h"

#include <OpenSOAP/Transport.h>

static const char SERVICE_NS_PREFIX[] = "m";

/* read message from text file as byte array */
int
ReadMessage(FILE *fp,
			const char *charEnc,
			OpenSOAPEnvelopePtr *env) {
	static char FuncName[] = "ReadMessage";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPByteArrayPtr ba;
	
	error = (fp ? error : APPLICATION_ERROR);
	ERROR_RETURN(error, FuncName, "find input file pointer");
	
	error = OpenSOAPByteArrayCreate(&ba);
	if (OPENSOAP_SUCCEEDED(error)) {
		unsigned char readBuf[BUFFER_SIZE];
		size_t readSize = 0;
		int relError = OPENSOAP_NO_ERROR;
		
		while (OPENSOAP_SUCCEEDED(error)
			   && (readSize = fread(readBuf, 1, BUFFER_SIZE, fp))) {
			error = OpenSOAPByteArrayAppend(ba, readBuf, readSize);
		}
		
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = OpenSOAPEnvelopeCreateCharEncoding(charEnc, ba, env);
			ERROR_CHECK(error, FuncName, "create envelope from byte array");
			
		}
		else {
			ERROR_MSG(error, FuncName, "complete reading text as byte array");
		}
		
		relError = OpenSOAPByteArrayRelease(ba);
		ERROR_RETURN(relError, FuncName, "release byte array");
	}
	else {
		ERROR_MSG(error, FuncName, "create byte array");
	}
	
	return error;
}

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

/* soap invoke function */
int
InvokeService(const char *endpoint,
			  const char *soapAction,
			  const char *charEnc,
			  OpenSOAPEnvelopePtr *request,
			  OpenSOAPEnvelopePtr *response) {
	static char FuncName[] = "InvokeService";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPTransportPtr transport = NULL;
	
    error = OpenSOAPTransportCreate(&transport);
	if (OPENSOAP_SUCCEEDED(error)) {
	
		error = OpenSOAPTransportSetURL(transport, endpoint);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = OpenSOAPTransportSetSOAPAction(transport, soapAction);
			if (OPENSOAP_SUCCEEDED(error)) {
				
				error = OpenSOAPTransportSetCharset(transport, charEnc);
				if (OPENSOAP_SUCCEEDED(error)) {
					/*
					error = WriteLabeledMessage(*request,
												charEnc,
												"actual request",
												stderr);
					*/
					if (OPENSOAP_SUCCEEDED(error)) {
						
						error = OpenSOAPTransportInvoke(transport,
														*request,
														response);
						if (OPENSOAP_SUCCEEDED(error)) {
							
							error = WriteLabeledMessage(*response,
														NULL,
														"raw response",
														stderr);
							ERROR_CHECK(error, FuncName,
										"output response message");
						}
						else {
							ERROR_MSG(error, FuncName, "invoke transport");
						}
					}
					else {
						ERROR_MSG(error, FuncName, "output request message");
					}
				}
				else {
					ERROR_MSG(error, FuncName, "set Charset");
				}
			}
			else {
				ERROR_MSG(error, FuncName, "set SOAPAction");
			}
		}
		else {
			ERROR_MSG(error, FuncName, "set URL");
		}
		
		error = OpenSOAPTransportRelease(transport);
		ERROR_RETURN(error, FuncName, "release transport");
	}
	else {
		ERROR_MSG(error, FuncName, "create transport");
	}		
	
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

/* create common part of request message */
int
CreateRequestCommon(OpenSOAPEnvelopePtr *request,
					const char *requestName,
					OpenSOAPBlockPtr *body,
					const char *nameSpace) {
	static char FuncName[] = "CreateRequestCommon";
    int error = OPENSOAP_NO_ERROR;
/*	OpenSOAPBlockPtr header; */
	
    error = OpenSOAPEnvelopeCreateMB("1.1", NULL, request);
	ERROR_RETURN(error, FuncName, "create envelope");
	
    /* add method body block */
    error = OpenSOAPEnvelopeAddBodyBlockMB(*request, requestName, body); 
	ERROR_RETURN(error, FuncName, "add body part");
	
    /* set namespace to method body block */
    error = OpenSOAPBlockSetNamespaceMB(*body,
										nameSpace,
										SERVICE_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace to body part");
	
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
	error = (*paramValue ? error : APPLICATION_ERROR);
	ERROR_RETURN(error, FuncName, "allocate memory");
	
	error = OpenSOAPStringGetStringMB(param, &paramLen, *paramValue);
	ERROR_RETURN(error, FuncName, "get string from parameter");
	
	return error;
}

/* parse message in case of fault*/
int
ParseFaultMessage(OpenSOAPBlockPtr body) {
	static char FuncName[] = "ParseFaultMessage";
	int error = OPENSOAP_NO_ERROR;
	char *paramValue = NULL;
	
	fprintf(stderr, "Fault message:\n");
	
	/* parse faultcode */
	
	error = GetStringParameter(body, "faultcode", &paramValue);
	ERROR_RETURN(error, FuncName, "get faultcode");
	
	fprintf(stderr, "    faultcode: %s\n", paramValue);
	
	free(paramValue);
	paramValue = NULL;
	
	/* parse faultstring */
	
	error = GetStringParameter(body, "faultstring", &paramValue);
	ERROR_RETURN(error, FuncName, "get faultstring");
	
	fprintf(stderr, "    faultstring: %s\n", paramValue);
	
	free(paramValue);
	paramValue = NULL;
	
	/* parse faultactor */
	
	error = GetStringParameter(body, "faultactor", &paramValue);
	ERROR_RETURN(error, FuncName, "get faultactor");
	
	fprintf(stderr,
			"    faultactor: %s\n",
			paramValue ? paramValue : "(null)");
	
	free(paramValue);
	paramValue = NULL;
	
	/* parse detail (as plane string) */
	
	error = GetStringParameter(body, "detail", &paramValue);
	ERROR_RETURN(error, FuncName, "get detail");
	
	fprintf(stderr, "    detail: %s\n", paramValue ? paramValue : "(null)");
	
	free(paramValue);
	paramValue = NULL;
	
	return error;
}

/* parse common part of response message */
int
ParseResponseCommon(OpenSOAPEnvelopePtr response,
					const char *responseName,
					OpenSOAPBlockPtr *body,
					const char *nameSpace,
					int *isFault) {
	static char FuncName[] = "ParseResponseCommon";
    int error = OPENSOAP_NO_ERROR;
	int isSameNs = 0;
	
	*isFault = 0;
	
	/* for fault message */
	
	error = OpenSOAPEnvelopeGetBodyBlockMB(response, "Fault", body);
	ERROR_RETURN(error, FuncName, "complete getting body block");
	
	if (*body != NULL) {
		
		error = ParseFaultMessage(*body);
		ERROR_RETURN(error, FuncName, "parse fault message");
		
		*isFault = 1;
		
		return error;
	}
	
	/* process response body */
	
	error = OpenSOAPEnvelopeGetBodyBlockMB(response, responseName, body);
	ERROR_RETURN(error, FuncName, "complete getting body block");
	
	if (*body != NULL) {
		
		error = OpenSOAPBlockIsSameNamespaceMB(*body,
											   nameSpace,
											   &isSameNs);
		ERROR_RETURN(error, FuncName, "complete namespace matching");
		
		error = (isSameNs ? error : APPLICATION_ERROR);
		ERROR_RETURN(error, FuncName, "match namespace");
		
		return error;
	}
	
	return APPLICATION_ERROR;
}
