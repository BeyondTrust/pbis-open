/*-----------------------------------------------------------------------------
 * $RCSfile: CalcAsyncClient.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "CalcAsyncClient.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/StringHash.h>
#include <OpenSOAP/Transport.h>

#include <stdio.h>
#include <string.h>

#define APPLICATION_ERROR (0x10000001L)

#define READ_BUF_SZ 256

/* error message macro */
#define ERROR_MSG(error, location, message); \
fprintf(stderr, \
		"%s function error: Cannot %s\n" \
		" ---> OpenSOAP Error Code: 0x%04x\n", \
		(location),\
		(message),\
		(error));

/* error check macro */
#define ERROR_CHECK(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
	ERROR_MSG(error, location, message) \
}

/* error return macro */
#define ERROR_RETURN(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
	ERROR_MSG(error, location, message) \
	return (error); \
}

static const unsigned char NULL_CHAR = '\0';

static const int FILE_DELM =
#if defined(__GNUC__)
'/';
#else
// 
'\\';
#endif

static const char DEFAULT_ENDPOINT[]
= "http://localhost/cgi-bin/soapInterface.cgi";

static const char DEFAULT_CHARENC[] = "UTF-8";

static const char OPTION_PREFIX[] = "-";

static const char OPTION_SOAPENDPOINT[] = "s";
static const char OPTION_SOAPACTION[] = "a";
static const char OPTION_CHARENC[] = "c";

static const char OPTION_HOPCOUNT[] = "H";

static const char OPTION_FORWARDPATH[] = "f";
static const char OPTION_FORWARDPATH_FILE[] = "F";

static const char USAGE_FORMAT[] = "\
Usage: %s [options] method [op_a] [op_b]\n\
  method         Add[Async], Subtract[Async], Multiply[Async], Divide[Async]\n\
                 or GetResult.\n\
  op_a           first operand (except for GetResult)\n\
  op_b           second operand (except for GetResult)\n\
\
 [options]\
  -s endpoint    SOAP service endpoint URI\n\
  -a soapaction  value of SOAPAction HTTP-Header\n\
  -c char_enc    character encoding of request message (default: UTF-8)\n\
\n\
  (messaging options)\n\
  -H hopcount             set hopcount (>= 0)\n\
  -f path1[,path2[,...]]  set forward path by URI (Commas are not allowed)\n\
                          e.g.http://host/cgi-bin/soapInterface.cgi\n\
  -F forward_path_file    set forward path from input file\n\
";

typedef  struct {
	char *appName;
	char *soapEndpoint;
	char *soapAction;
	char *charEnc;
	char *method;
	double operandA;
	double operandB;
	int hop_count;
	char **forward_path;
	int forward_path_num;
} ClientVariables;

static const char OPENSOAP_HEADER_NS_URI[] = "http://header.opensoap.jp/1.0/";
static const char OPENSOAP_HEADER_NS_PREFIX[] = "opensoap-header";

static const char METHOD_NS_URI[]
= "http://services.opensoap.jp/samples/CalcAsync/";
static const char METHOD_NS_PREFIX[] = "m";

static int ttl = 80;

/* file pointer for debug output */
FILE *debug_fp = NULL;

/* get foraward_path list */
/* return value */
/* NULL: failure */
/* fwdPathList: success */
char**
getForwardPathListFile(char *argv, int* path_num) 
{
	char line[READ_BUF_SZ + 1];
	char** forwardPathList = NULL;
	size_t read;
	FILE* fp = fopen(argv, "r");
  
	*path_num = 0;
	if(fp == NULL) {
		fprintf(stderr, "Can not open %s", argv);
		return NULL;
	}
	while (fgets(line, READ_BUF_SZ, fp) != NULL) {
		(*path_num)++;
	}
	/*
	  fprintf(stderr, "path_num = %d\n", path_num);
	*/
	if(*path_num == 0) {
		return NULL;
	}
  
	forwardPathList = (char**)malloc(sizeof(char*) * (*path_num));
	if(forwardPathList == NULL) {
		fprintf(stderr, "forwardPathList malloc faild\n");
		return NULL;
	}
	*path_num = 0;
  
	rewind(fp);
	while (fgets(line, READ_BUF_SZ, fp) != NULL) {
		read = strlen(line);
		if (line[read - 1] == '\n') {
			line[read - 1] = '\0';
		}
		forwardPathList[*path_num] = (char*)malloc(sizeof(char) * (read + 1));
		if (forwardPathList[*path_num] == NULL) {
			fprintf(stderr, "forwardPathList[%d] malloc faild", *path_num);
			return NULL;
		}
		memset(forwardPathList[*path_num], 0, read + 1);
		memcpy(forwardPathList[*path_num], line, read);
		(*path_num)++;
	}
	
	fclose(fp);
	return forwardPathList;
}

/* get foraward_path list */
/* return value */
/* NULL: failure */
/* fwdPathList: success */
char**
getForwardPathListString(char *argv, int* path_num) 
{
	char** forwardPathList = NULL;
	
	char *str = strdup(argv);
	char *cp = str;
	if (*cp == '\0') {
		*path_num = 0;
		return NULL;
	}
	*path_num = 1;
	while(*cp) {
		if (*cp == ',') {
			(*path_num)++;
		}
		cp++;
	}
	forwardPathList = (char**)malloc(sizeof(char*) * (*path_num));
	if(forwardPathList == NULL) {
		fprintf(stderr, "forwardPathList malloc faild\n");
		*path_num = 0;
		return NULL;
	}
	*path_num = 0;
	cp = strtok(str, ",");
	while(cp) {
		forwardPathList[*path_num] = cp;
		cp = strtok(NULL, ",");
		if (strcmp(forwardPathList[*path_num], "")) {
			(*path_num)++;
		}
	}
	return forwardPathList;
}

/* release forward_path list */
/* return value */
/* EXIT_SUCCESS: success   */
/* EXIT_FAILURE: false     */
int
releaseForwardPathList(char** forwardPathList, int path_num){
	int i;
	for(i = 0; i < path_num; i++) {
		free(forwardPathList[i]);
	}
	free(forwardPathList);
	return EXIT_SUCCESS;
}

#if !defined(CLIENT_CGI)
/* get name of this program */
static
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

/* initialize variable given as command-line arguments */
static
long
InitializeVariables(ClientVariables *appVars,
					int argc,
					char **argv) {
	long error = 0;
	const size_t OPTION_PREFIX_LEN = strlen(OPTION_PREFIX);
	char **avEnd = argv + argc;
	
	memset((void *)appVars, 0, sizeof(*appVars)); /* clear */
	
	appVars->appName = GetAppName(*argv);
	appVars->hop_count = -1;
	appVars->forward_path = NULL;
	appVars->forward_path_num = 0;

	for (++argv; argv != avEnd; ++argv) {
		const char *optBody = *argv;
		if (strncmp(optBody, OPTION_PREFIX, OPTION_PREFIX_LEN) == 0) {
			size_t optBodyLen = 0;
			optBody += OPTION_PREFIX_LEN;
			optBodyLen = strlen(optBody);
			if (strncmp(OPTION_SOAPENDPOINT, optBody, optBodyLen) == 0
				&& !appVars->soapEndpoint) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				appVars->soapEndpoint = strdup(*argv);
			}
			else if (strncmp(OPTION_SOAPACTION, optBody, optBodyLen) == 0
					 && !appVars->soapAction) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				appVars->soapAction = strdup(*argv);
			}
			else if (strncmp(OPTION_CHARENC, optBody, optBodyLen) == 0
					 && !appVars->charEnc) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				appVars->charEnc = strdup(*argv);
				if (!appVars->charEnc) {
					fprintf(stderr, "memory allocate error\n");
					error = APPLICATION_ERROR;
					argv = avEnd;
					break;
				}
			}
			else if (strncmp(OPTION_HOPCOUNT, optBody, optBodyLen) == 0) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				appVars->hop_count = atoi(*argv);
				if (!appVars->hop_count < 0) {
					appVars->hop_count = 0;
					fprintf(stderr, "hopcount should be more than equal 0\n");
					error = APPLICATION_ERROR;
					argv = avEnd;
					break;
				}
			}
			else if (appVars->forward_path == NULL &&
					 strncmp(OPTION_FORWARDPATH_FILE, optBody,
							 optBodyLen) == 0) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				appVars->forward_path
					= getForwardPathListFile(*argv,
											 &(appVars->forward_path_num));
			}
			else if (appVars->forward_path == NULL &&
					 strncmp(OPTION_FORWARDPATH, optBody, optBodyLen) == 0) {
				++argv;
				if (argv == avEnd) {
					break;
				}
				appVars->forward_path
					= getForwardPathListString(*argv,
											   &(appVars->forward_path_num));
			}
			else {
				argv = avEnd;
				break;
			}
		}
		else {
			break;
		}
	}
	
	if (argv != avEnd) {
		
		appVars->method = strdup(*argv);
		++argv;
		if (argv == avEnd) {
			/* error check needed */
		}
		else {
			/* error check needed */
			appVars->operandA = (double)atof(*argv);
			++argv;
			if (argv == avEnd) {
				/* error check needed */
			}
			else {
				/* error check needed */
				appVars->operandB = (double)atof(*argv);
				++argv;
				if (argv != avEnd) {
					fprintf(stderr, "arguments error: too many parameters\n");
					error = APPLICATION_ERROR;
				}
			}
		}
	}
	else {
		fprintf(stderr, USAGE_FORMAT,
				appVars->appName ? appVars->appName : "CalcAsyncClient");
		exit(0);
	}
	
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
		fprintf(stderr, USAGE_FORMAT,
				appVars->appName ? appVars->appName : "CalcAsyncClient");
	}
	
	return error;
}			

static
void
ReleaseVariables(ClientVariables *appVars) {
	if (appVars) {
		free(appVars->appName);
		free(appVars->soapEndpoint);
		free(appVars->soapAction);
		free(appVars->charEnc);
		free(appVars->method);
	}
}

#endif /* CLIENT_CGI */

/* read message from text file as byte array */
static
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
		unsigned char readBuf[READ_BUF_SZ];
		size_t readSize = 0;
		int relError = OPENSOAP_NO_ERROR;
		
		while (OPENSOAP_SUCCEEDED(error)
			   && (readSize = fread(readBuf, 1, READ_BUF_SZ, fp))) {
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
			const unsigned char *env_beg = NULL;
			size_t env_sz = 0;
			
			error = OpenSOAPByteArrayGetBeginSizeConst(envBuf, &env_beg,
													   &env_sz);
			if (OPENSOAP_SUCCEEDED(error)) {
				fwrite(env_beg, 1, env_sz, fp);
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
static
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
	
	fprintf(fp, "===== end %s soap message =====\n", label);
	
	return error;
}

/* soap invoke function */
static
int
InvokeService(const char *end_point,
			  const char *soapAction,
			  const char *charEnc,
			  OpenSOAPEnvelopePtr *request,
			  OpenSOAPEnvelopePtr *response) {
	static char FuncName[] = "InvokeService";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPTransportPtr transport = NULL;
	
    error = OpenSOAPTransportCreate(&transport);
	if (OPENSOAP_SUCCEEDED(error)) {
	
		error = OpenSOAPTransportSetURL(transport, end_point);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = OpenSOAPTransportSetSOAPAction(transport, soapAction);
			if (OPENSOAP_SUCCEEDED(error)) {
				
				fprintf(stderr, "char=%s\n", charEnc);
				error = OpenSOAPTransportSetCharset(transport, charEnc);
				if (OPENSOAP_SUCCEEDED(error)) {
					
					error = WriteLabeledMessage(*request,
												charEnc,
												"request",
												debug_fp);
					if (OPENSOAP_SUCCEEDED(error)) {
						
						error = OpenSOAPTransportInvoke(transport,
														*request,
														response);
						if (OPENSOAP_SUCCEEDED(error)) {
							
							error = WriteLabeledMessage(*response,
														NULL,
														"response",
														debug_fp);
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
static
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
static
int
AddAttributeWithNamespace(OpenSOAPBlockPtr block,
						  const char *elmName,
						  const char *attrName,
						  const char *attrType,
						  void *attrValue,
						  const char *NsUri,
						  const char *NsPrefix) {
	static char FuncName[] = "AddAttributeWithNamespace";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPXMLAttrPtr attr = NULL;
	
	error = AddAttribute(block, elmName, attrName, attrType, attrValue, &attr);
	ERROR_RETURN(error, FuncName, "add attribute to element");
	
    error = OpenSOAPXMLAttrSetNamespaceMB(attr, NsUri, NsPrefix);
	ERROR_RETURN(error, FuncName, "set namespace to attribute");
	
	return error;
}

/* create common part of request message */
static
int
CreateRequestCommon(OpenSOAPEnvelopePtr *request,
					const char *requestName,
					const char *async,
					int hop_count,
					const char **forward_path,
					int forward_path_num,
					OpenSOAPBlockPtr *body) {
	static char FuncName[] = "CreateRequestCommon";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr header;
	OpenSOAPXMLElmPtr forwarderElm = NULL;
	OpenSOAPXMLElmPtr ttlElm = NULL;
	OpenSOAPStringPtr typeStr = NULL;
	OpenSOAPStringPtr asyncStr = NULL;
	OpenSOAPXMLElmPtr asyncElm = NULL;


	error = OpenSOAPEnvelopeCreateMB("1.1", NULL, request);
	ERROR_RETURN(error, FuncName, "create envelope");
	
	/* add opensoap header block */
	error = OpenSOAPEnvelopeAddHeaderBlockMB(*request,
											 "opensoap-header-block",
											 &header); 
	ERROR_RETURN(error, FuncName, "add header part");
		
	/* set namespace to opensoap-header block */
	error = OpenSOAPBlockSetNamespaceMB(header,
										OPENSOAP_HEADER_NS_URI,
										OPENSOAP_HEADER_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace to header part");
		
	if (strcmp(async, "true") == 0) {
		/* set ttl element to opensoap-header header */
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
		
		error = OpenSOAPStringCreateWithMB(async, &asyncStr);
		ERROR_RETURN(error, FuncName, "create string for async");
		
		error = OpenSOAPBlockSetChildValueMB(header,
											 "async",
											 "string",
											 &asyncStr);
		ERROR_RETURN(error, FuncName, "set async element");
		
		/* set namespace to async element */
		
		error = OpenSOAPBlockGetChildMB(header, "async", &asyncElm);
		ERROR_RETURN(error, FuncName, "get async element");
		
		error = OpenSOAPXMLElmSetNamespaceMB(asyncElm,
											 OPENSOAP_HEADER_NS_URI,
											 OPENSOAP_HEADER_NS_PREFIX);
		ERROR_RETURN(error, FuncName, "set namespace to async element");
	}

	if (hop_count >= 0 || forward_path_num > 0) {
		/* add forwarder element */
		error = OpenSOAPBlockAddChildMB(header, 
										"forwarder", &forwarderElm);
		ERROR_RETURN(error, FuncName, "add forwarder element");
	}

	if (hop_count >= 0) {
		OpenSOAPXMLElmPtr hopCountElm = NULL;

		/* set namespace to forwarder element */
		error = OpenSOAPXMLElmSetNamespaceMB(forwarderElm,
											 OPENSOAP_HEADER_NS_URI,
											 OPENSOAP_HEADER_NS_PREFIX);
		ERROR_RETURN(error, FuncName, 
					 "set namespace to forwarder element");
	
		/* add forwarder element */
		error = OpenSOAPXMLElmAddChildMB(forwarderElm, 
										 "hopcount", &hopCountElm);
		ERROR_RETURN(error, FuncName, "add hopcount element");
	
		/* set namespace to forwarder element */
		error = OpenSOAPXMLElmSetNamespaceMB(hopCountElm,
											 OPENSOAP_HEADER_NS_URI,
											 OPENSOAP_HEADER_NS_PREFIX);
		ERROR_RETURN(error, FuncName, 
					 "set namespace to hopcount element");
		
		/* add hopcount element */
		error = OpenSOAPXMLElmSetValueMB(hopCountElm, 
										 "int", &hop_count);
		ERROR_RETURN(error, FuncName, "set hopcount element");
	}
	
	if (forward_path_num > 0) {
		int i;
		OpenSOAPXMLElmPtr fwdPathElm = NULL;
		OpenSOAPStringPtr fwdPathStr = NULL;

		/* forward path element */
		fprintf(stderr, "path_num = %d\n", forward_path_num); 
		for (i = 0; i < forward_path_num; i++) {
			fprintf(stderr, "forwardPathList[%d] = %s\n", i, forward_path[i]);
		}
	
		/* add forward_path element */
		for (i = 0; i < forward_path_num; i++) {
			fwdPathElm = NULL;
			error = OpenSOAPXMLElmAddChildMB(forwarderElm, 
											 "forward_path", 
											 &(fwdPathElm));
			ERROR_RETURN(error, FuncName, "add forward_path element");
		
			/* set namespace to forwarder element */
			error = OpenSOAPXMLElmSetNamespaceMB(fwdPathElm,
												 OPENSOAP_HEADER_NS_URI,
												 OPENSOAP_HEADER_NS_PREFIX);
			ERROR_RETURN(error, FuncName, 
						 "set namespace to forward_path element");
		
			/* create forward_path string */
			fwdPathStr = NULL;
			error = OpenSOAPStringCreateWithMB(forward_path[i], 
											   &(fwdPathStr));
			ERROR_RETURN(error, FuncName, 
						 "create string for forward_path");
		
			/* add forward_path element */
			error = OpenSOAPXMLElmSetValueMB(fwdPathElm, 
											 "string", 
											 &(fwdPathStr));
			ERROR_RETURN(error, FuncName, "set forward_path element");
		
			/* release forward_path element */
			error = OpenSOAPStringRelease(fwdPathStr);
			ERROR_RETURN(error, FuncName, 
						 "release string for forward_path");
		
		}/* loop of for(i) */
		
		/* releaseForwardPathList(forward_path, forward_path_num); */
	}
	
    /* add method body block */
    error = OpenSOAPEnvelopeAddBodyBlockMB(*request, requestName, body); 
	ERROR_RETURN(error, FuncName, "add body part");
	
    /* set namespace to method body block */
    error = OpenSOAPBlockSetNamespaceMB(*body,
										METHOD_NS_URI,
										METHOD_NS_PREFIX);
	ERROR_RETURN(error, FuncName, "set namespace to body part");
	
	return error;
}

/* set shopping service method parameter: qty */
static
int
SetParameterOperand(const char *paramName,
					double paramValue,
					OpenSOAPBlockPtr *body) {
	static char FuncName[] = "SetParameterOperand";
    int error = OPENSOAP_NO_ERROR;
	
	error = OpenSOAPBlockSetChildValueMB(*body,
										 paramName,
										 "double",
										 &paramValue);
	ERROR_RETURN(error, FuncName, "set operand parameter");
	
	return error;
}

/* get parameter value of string type from message */
static
int
GetStringParameter(OpenSOAPBlockPtr body,
				   const char *paramName,
				   char **paramValue) {
	static char FuncName[] = "GetStringParameter";
	int error = OPENSOAP_NO_ERROR;
	OpenSOAPStringPtr param = NULL;
    size_t paramLen = 0;
	OpenSOAPXMLElmPtr childElm = NULL;
	
	*paramValue = NULL;
	
	error = OpenSOAPBlockGetChildMB(body, paramName, &childElm);
	if (childElm != NULL) {
		error = OpenSOAPStringCreate(&param);
		ERROR_RETURN(error, FuncName, "create value of parameter buffer");
		
		error = OpenSOAPBlockGetChildValueMB(body,
											 paramName,
											 "string",
											 &param);
		ERROR_RETURN(error, FuncName, "get value of parameter");
		
		error = OpenSOAPStringGetLengthMB(param, &paramLen);
		ERROR_RETURN(error, FuncName, "get string length of parameter");
		
		*paramValue = malloc(paramLen + 1);
		error = (*paramValue ? error : APPLICATION_ERROR);
		ERROR_RETURN(error, FuncName, "allocate memory");
		
		error = OpenSOAPStringGetStringMB(param, &paramLen, *paramValue);
		ERROR_RETURN(error, FuncName, "get string from parameter");

		OpenSOAPStringRelease(param);
	}
	
	return error;
}

/* parse message in case of fault*/
static
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
static
int
ParseResponseCommon(OpenSOAPEnvelopePtr response,
					const char *responseName,
					OpenSOAPBlockPtr *body,
					int *isFault) {
	static char FuncName[] = "ParseResponseCommon";
    int error = OPENSOAP_NO_ERROR;
	int isSameNS = 0;
	
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
											   METHOD_NS_URI,
											   &isSameNS);
		ERROR_RETURN(error, FuncName, "complete namespace matching");
		
 		error = (isSameNS ? error : APPLICATION_ERROR);
		ERROR_RETURN(error, FuncName, "match namespace");
		
		return error;
	}
	
	return APPLICATION_ERROR;
}

/* create calc request message */
static
int
CreateCalcRequest(const char *operator,
				  double operandA,
				  double operandB,
				  const char *async,
				  int hop_count,
				  const char **forward_path,
				  int forward_path_num,
				  OpenSOAPEnvelopePtr *request) {
	static char FuncName[] = "CreateCalcRequest";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr body = NULL;
	
	error = CreateRequestCommon(request, operator, async, hop_count,
								forward_path, forward_path_num, &body);
	ERROR_RETURN(error, FuncName, "create common part of request");
	
	error = SetParameterOperand("A", operandA, &body);
	ERROR_RETURN(error, FuncName, "set parameter: operandA");
	
	error = SetParameterOperand("B", operandB, &body);
	ERROR_RETURN(error, FuncName, "set parameter: operandB");
	
	return error;
}

/* parse calc response message */
static
int
ParseCalcResponse(const char *operator,
				  OpenSOAPEnvelopePtr response,
				  double *result) {
	static char FuncName[] = "ParseCalcResponse";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	int isFault = 0;
	char rspOperator[64];
	
	sprintf(rspOperator, "%sResponse", operator);
	
	error = ParseResponseCommon(response, rspOperator, &body, &isFault);
	ERROR_RETURN(error, FuncName, "parse common part of response");
	
	if (isFault) {
		return error;
	}
	
	error = OpenSOAPBlockGetChildValueMB(body, "Result", "double", result);
	ERROR_RETURN(error, FuncName, "get parameter: result");
	
	return error;
}

/* parse calc response message in asyncronized mode */
static
int
ParseCalcResponseAsync(OpenSOAPEnvelopePtr response,
					   double *result) {
	static char FuncName[] = "ParseCalcResponseAsync";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr body = NULL;
	int isFault = 0;
	
	if (OPENSOAP_SUCCEEDED(ParseResponseCommon(response,
											   "AddResponse",
											   &body,
											   &isFault))) {
	}
	else if (OPENSOAP_SUCCEEDED(ParseResponseCommon(response,
													"SubtractResponse",
													&body,
													&isFault))) {
	}
	else if (OPENSOAP_SUCCEEDED(ParseResponseCommon(response,
													"MultiplyResponse",
													&body,
													&isFault))) {
	}
	else if (OPENSOAP_SUCCEEDED(ParseResponseCommon(response,
													"DivideResponse",
													&body,
													&isFault))) {
	}
	else {
		return APPLICATION_ERROR;
	}
	
	if (isFault) {
		return error;
	}
	
	error = OpenSOAPBlockGetChildValueMB(body, "Result", "double", result);
	ERROR_RETURN(error, FuncName, "get parameter: result");
	
	return error;
}

/* parse response message from server in async mode */
static
int
ParseServerResponse(OpenSOAPEnvelopePtr response,
					char **messageID) {
	static char FuncName[] = "ParseServerResponse";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPBlockPtr block = NULL;
	int isSameNS = 0;
	
	/* for fault message */
	
	error = OpenSOAPEnvelopeGetBodyBlockMB(response, "Fault", &block);
	ERROR_RETURN(error, FuncName, "complete getting body block");

	if (block != NULL) {
		
		error = ParseFaultMessage(block);
		ERROR_RETURN(error, FuncName, "parse fault message");
		
		return error;
	}
	
	/* process response header */
	
	error = OpenSOAPEnvelopeGetHeaderBlockMB(response,
											 "opensoap-header-block",
											 &block);
	ERROR_RETURN(error, FuncName, "complete getting header block");
	
	if (block != NULL) {
		
		error = OpenSOAPBlockIsSameNamespaceMB(block,
											   OPENSOAP_HEADER_NS_URI,
											   &isSameNS);
		ERROR_RETURN(error, FuncName, "complete namespace matching");
		
 		error = (isSameNS ? error : APPLICATION_ERROR);
		ERROR_RETURN(error, FuncName, "match namespace");
	}
	
	error = GetStringParameter(block, "message_id", messageID);
	ERROR_RETURN(error, FuncName, "get message_id");
	
	return error;
}

/* submit calc request in syncronized mode */
extern
int
CalcSync(const char *endpoint,
		 const char *soapAction,
		 const char *charEnc,
		 const char *operator,
		 double operandA,
		 double operandB,
		 int hop_count,
		 const char **forward_path,
		 int forward_path_num,
		 double *result) {
	static char FuncName[] = "CalcSync";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	
	error = CreateCalcRequest(operator,
							  operandA,
							  operandB,
							  "false",
							  hop_count,
							  forward_path,
							  forward_path_num,
							  &request);
	if (OPENSOAP_SUCCEEDED(error)) {
		OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = ParseCalcResponse(operator, response, result);
			ERROR_CHECK(error, FuncName, "parse response message");
			
			error = OpenSOAPEnvelopeRelease(response);
  			ERROR_CHECK(error, FuncName, "release response envelope");
		} else {
			ERROR_MSG(error, FuncName, "invoke service");
		}
		error = OpenSOAPEnvelopeRelease(request);
		ERROR_RETURN(error, FuncName, "release request envelope");
	}
	else {
		ERROR_MSG(error, FuncName, "create request message");
	}
	
    return error;
}

/* submit calc request in asyncronized mode */
extern
int
CalcAsync(const char *endpoint,
		  const char *soapAction,
		  const char *charEnc,
		  const char *operator,
		  double operandA,
		  double operandB,
		  int hop_count,
		  const char **forward_path,
		  int forward_path_num,
		  char **messageID,
		  FILE *fp) {
	static char FuncName[] = "CalcAsync";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;

	error = CreateCalcRequest(operator,
							  operandA,
							  operandB,
							  "true",
							  hop_count,
							  forward_path,
							  forward_path_num,
							  &request);
	if (OPENSOAP_SUCCEEDED(error)) {
		OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = WriteMessage(response, NULL, fp);
			ERROR_CHECK(error, FuncName, "output resopnse message to file");
			
			error = ParseServerResponse(response, messageID);
			ERROR_CHECK(error, FuncName, "parse server response message");
			
			error = OpenSOAPEnvelopeRelease(response);
			ERROR_CHECK(error, FuncName, "release response envelope");
		} else {
			ERROR_MSG(error, FuncName, "invoke service");
		}
		error = OpenSOAPEnvelopeRelease(request);
 		ERROR_RETURN(error, FuncName, "release request envelope");
	}
	else {
		ERROR_MSG(error, FuncName, "create request message");
	}
	
    return error;
}

/* submit get result request in asyncronized mode */
extern
int
GetResult(FILE *fp,
		  const char *endpoint,
		  const char *soapAction,
		  const char *charEnc,
		  double *result) {
	static char FuncName[] = "GetResult";
    int error = OPENSOAP_NO_ERROR;
	OpenSOAPEnvelopePtr request = NULL;
	
	error = ReadMessage(fp, charEnc, &request);
	ERROR_RETURN(error, FuncName, "input request message from file");
	
	if (OPENSOAP_SUCCEEDED(error)) {
		OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		
		if (OPENSOAP_SUCCEEDED(error)) {
			error = ParseCalcResponseAsync(response, result);
			ERROR_CHECK(error, FuncName, "parse resopnse message");
			
  			error = OpenSOAPEnvelopeRelease(response);
  			ERROR_CHECK(error, FuncName, "release response envelope");
		} else {
			ERROR_MSG(error, FuncName, "invoke service");
		}
  		error = OpenSOAPEnvelopeRelease(request);
  		ERROR_RETURN(error, FuncName, "release request envelope");
	}
	else {
		ERROR_MSG(error, FuncName, "create request message");
	}
	
    return error;
}

/* main for command-line program */
#if !defined(CLIENT_CGI)
int
main(int argc,
	 char **argv) {
	ClientVariables appVars;
	long error = OPENSOAP_NO_ERROR;

	debug_fp = stderr;
	
	error = OpenSOAPInitialize(NULL);
	
	if (OPENSOAP_FAILED(error)) {
		fprintf(stderr,
				"OpenSOAP API Initialize Failed.\n"
				"OpenSOAP Error Code: 0x%08lx\n",
				error);
		
		return error;
	}

	error = InitializeVariables(&appVars, argc, argv);
	if (OPENSOAP_FAILED(error)) {
		fprintf(stderr,
				"Application Initialize Failed.\n"
				"OpenSOAP Error Code: 0x%08lx\n",
				error);
		OpenSOAPUltimate();

		return 1;
	}
	
	fprintf(stderr,
			"SOAPEndpoint: %s\n"
			"SOAPAction: %s\n"
			"character encoding: %s\n"
			"method: %s\n"
			"operand A: %f\n"
			"operand B: %f\n",
			appVars.soapEndpoint ? appVars.soapEndpoint : "",
			appVars.soapAction ? appVars.soapAction : "",
			appVars.charEnc ? appVars.charEnc : "",
			appVars.method,
			appVars.operandA,
			appVars.operandB);

	if (appVars.hop_count >= 0) {
		fprintf(stderr, "hopcount = %d\n", appVars.hop_count);
	}
	if (appVars.forward_path != NULL) {
		fprintf(stderr, "forward_path = %s\n", "*" /* appVars.forward_path */);
	}
	
	if (strcmp(appVars.method, "Add") == 0
		|| strcmp(appVars.method, "Subtract") == 0
		|| strcmp(appVars.method, "Multiply") == 0
		|| strcmp(appVars.method, "Divide") == 0) {
		double result = 0.0;
		
		error = CalcSync(appVars.soapEndpoint,
						 appVars.soapAction,
						 appVars.charEnc,
						 appVars.method,
						 appVars.operandA,
						 appVars.operandB,
						 appVars.hop_count,
						 (const char**)appVars.forward_path,
						 appVars.forward_path_num,
						 &result);
		
		fprintf(stderr, "\n%sResponse: result=%f\n\n", appVars.method, result);
	}
	else if (strcmp(appVars.method, "AddAsync") == 0
			 || strcmp(appVars.method, "SubtractAsync") == 0
			 || strcmp(appVars.method, "MultiplyAsync") == 0
			 || strcmp(appVars.method, "DivideAsync") == 0) {
 		char bareOperator[64];
		char *messageID;
		int bareOpLen = strlen(appVars.method) - strlen("Async");
		
 		strncpy(bareOperator, appVars.method, bareOpLen);
		bareOperator[bareOpLen] = '\0';
		fprintf(stderr, "Async: %s\n", bareOperator);
		
		error = CalcAsync(appVars.soapEndpoint,
						  appVars.soapAction,
						  appVars.charEnc,
						  bareOperator,
						  appVars.operandA,
						  appVars.operandB,
						  appVars.hop_count,
						  (const char**)appVars.forward_path,
						  appVars.forward_path_num,
						  &messageID,
						  stdout);
		
		fprintf(stderr, "MessageID: %s\n", messageID);
	}
	else if (strcmp(appVars.method, "GetResult") == 0) {
		double result = 0.0;
		
		error = GetResult(stdin,
						  appVars.soapEndpoint,
						  appVars.soapAction,
						  appVars.charEnc,
						  &result);
		
		fprintf(stderr, "Result of %s: %f\n", appVars.method, result);
	}
	else {
		fprintf(stderr, "Error: Operation[%s] is not supported.\n", appVars.method);
		fprintf(stderr, USAGE_FORMAT,
				appVars.appName ? appVars.appName : "CalcAsyncClient");
		exit(1);
	}
	
	ReleaseVariables(&appVars);
	
	OpenSOAPUltimate();
	
	return 0;
}
#endif /* CLIENT_CGI */
