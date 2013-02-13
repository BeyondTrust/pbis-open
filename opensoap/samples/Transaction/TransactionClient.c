/*---------------------------------------------------------------------------
 * $RCSfile: TransactionClient.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *---------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Transport.h>

#include <stdio.h>
#include <string.h>

#define APPLICATION_ERROR (0x10000001L)

#define READ_BUF_SZ 256

/* error message macro */
#define ERROR_MSG(error, location, message); \
fprintf(stderr, \
	        "%s function error: Cannot %s\n" \
		" ---> OpenSOAP Error Code: 0x%08lx\n", \
		(location),\
		(message),\
		(long)(error));

/* error check macro */
#define ERROR_CHECK(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
	ERROR_MSG((error), (location), (message)) \
}

/* error return macro */
#define ERROR_RETURN(error, location, message); \
if (OPENSOAP_FAILED(error)) { \
	ERROR_MSG((error), (location), (message)) \
	return (error); \
}

static const char OPENSOAP_HEADER_NS_URI[] = "http://header.opensoap.jp/1.0/";
static const char OPENSOAP_HEADER_NS_PREFIX[] = "opensoap-header";

static const char TRANSACTION_CONTROL_METHOD[] = "TransactionControl";
static const char TRANSACTION_HEADER_BLOCK[] = "TransactionHeaderBlock";
static const char TRANSACTION_BODY_BLOCK[] = "TransactionBodyBlock";
static const char TRANSACTION_ENDPOINT[] = "endpoint";
static const char TRANSACTION_REQUEST_ID_ATTRIBUTE[] = "transactionRequestID";

static const char TRANSACTION_RESULT[] = "TransactionResult";
static const char TRANSACTION_ID_ATTRIBUTE[] = "transactionID";
static const char TRANSACTION_ACTION_RESPONSE_BODY_BLOCK[] = "TransactionActionResponseBodyBlock";
static const char TRANSACTION_ACTION_RESPONSE_METHOD[] = "TransactionActionResponse";

static const char TRANSACTION_RESPONSE_METHOD[] = "TransactionResponse";
static const char TRANSACTION_RESPONSE_RESULT[] = "Result";

static const char TRANSACTION_NAMESPACE[] = "http://services.opensoap.jp/Transaction/";
static const char TRANSACTION_PREFIX[] = "tp";

static int ttl = 80;

/* for test */
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

static const char USAGE_FORMAT[] = "\
Usage: %s [-s endpoint] [-a soapaction] [-c char_enc] method(amount) [account(from)] [account(to)] [amount]\n\
  -s endpoint    SOAP service endpoint URI\n\
  -a soapaction  value of SOAPAction HTTP-Header\n\
  -c char_enc    character encoding of request message (default: UTF-8)\n\
  method         Sync, Async or GetResult.\n\
  account(from)  payment from (except for GetResult)\n\
  account(to)    deposit to (except for GetResult)\n\
  amount         amount of money to transfer (except for GetResult)\n\
";

typedef  struct {
    char *appName;
    char *soapEndpoint;
    char *soapAction;
    char *charEnc;
    char *method;
    char *account1;
    char *account2;
    char *amount;    
} ClientVariables;

typedef  struct {
    char *result;
    char *paymentResult;
    char *paymentComment;
    char *paymentTransactionID;
    char *paymentActionResponse;
    char *depositResult;
    char *depositComment;
    char *depositTransactionID;
    char *depositActionResponse;
} ResultVariables;

static const char PAYMENT_ID[] = "1";
static const char DEPOSIT_ID[] = "2";

/* Check the contents of OpenSOAPString */
static
void 
CheckStr(char *preStr, OpenSOAPStringPtr strPtr) {
    char *string = NULL;
    OpenSOAPStringGetStringMBWithAllocator(strPtr,
										   NULL,
										   NULL,
										   &string);
	
    fprintf(stderr, "%s: %s\n", preStr, string);
    free(string);
}

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

/* initialize variable for result parameters */
static
int
InitializeResultVariables(ResultVariables *rsltVars) {

    int error = 0;
    rsltVars->result = NULL;
    rsltVars->paymentResult = NULL;
    rsltVars->paymentComment = NULL;
    rsltVars->paymentTransactionID = NULL;
    rsltVars->paymentActionResponse = NULL;
    rsltVars->depositResult = NULL;
    rsltVars->depositComment = NULL;
    rsltVars->depositTransactionID = NULL;
    rsltVars->depositActionResponse = NULL;
    return error;   
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
			appVars->account1 = strdup(*argv);
			++argv;
			if (argv == avEnd) {
				/* error check needed */
			}
			else {
				/* error check needed */
				appVars->account2 = strdup(*argv);
				++argv;
				if (argv == avEnd) {
					/* error check needed */
				}
				else {
					/* error check needed */
					appVars->amount = strdup(*argv);
					++argv;
					if (argv != avEnd) {
						fprintf(stderr,
								"arguments error: too many parameters\n");
						error = APPLICATION_ERROR;
					}
				}
			}
		}
    }
    else {
        fprintf(stderr, USAGE_FORMAT,
				appVars->appName ? appVars->appName : "TransactionClient");
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
				appVars->appName ? appVars->appName : "TransactionClient");
    }

    return error;
}

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
static
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
	
    fprintf(fp, "\n===== end %s soap message =====\n", label);
	
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
					const char *endpoint1,
					const char *endpoint2,
					const char *async) {

    static char FuncName[] = "CreateRequestCommon";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr header;
    OpenSOAPBlockPtr body = NULL;
    OpenSOAPXMLElmPtr endpointElm1 = NULL;
    OpenSOAPXMLElmPtr endpointElm2 = NULL;
    OpenSOAPXMLAttrPtr transactionIDAttr1 = NULL;
    OpenSOAPXMLAttrPtr transactionIDAttr2 = NULL;
    OpenSOAPStringPtr transactionIDAttrValue1 = NULL;
    OpenSOAPStringPtr transactionIDAttrValue2 = NULL;
    OpenSOAPStringPtr endpointValue1 = NULL;
    OpenSOAPStringPtr endpointValue2 = NULL;

    error = OpenSOAPEnvelopeCreateMB("1.1", NULL, request);
    ERROR_RETURN(error, FuncName, "create envelope");
	
    if (strcmp(async, "true") == 0) {
		
        OpenSOAPXMLElmPtr ttlElm = NULL;
		OpenSOAPStringPtr typeStr = NULL;
		OpenSOAPStringPtr asyncStr = NULL;
		OpenSOAPXMLElmPtr asyncElm = NULL;

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
	
    /* add method body block */
    error = OpenSOAPEnvelopeAddBodyBlockMB(*request,
										   TRANSACTION_CONTROL_METHOD,
										   &body);
    ERROR_RETURN(error, FuncName, "add body part");

    /* set namespace to method body block */
    error = OpenSOAPBlockSetNamespaceMB(body,
										TRANSACTION_NAMESPACE,
										TRANSACTION_PREFIX);
    ERROR_RETURN(error, FuncName, "set namespace to body part");

    /* set child:endpoint1 to method body block */
    error = OpenSOAPBlockAddChildMB(body,
									TRANSACTION_ENDPOINT,
									&endpointElm1);
    ERROR_RETURN(error, FuncName, "set children:endpoint1 to body part");

    
    OpenSOAPStringCreateWithMB(PAYMENT_ID, &transactionIDAttrValue1);
    error = OpenSOAPXMLElmAddAttributeMB(endpointElm1,
										 TRANSACTION_REQUEST_ID_ATTRIBUTE,
										 "string",
										 &transactionIDAttrValue1,
										 &transactionIDAttr1);
    ERROR_RETURN(error, FuncName, "set attribute to endpoint1");	           

    OpenSOAPStringCreateWithMB(endpoint1, &endpointValue1);
    error = OpenSOAPXMLElmSetValueMB(endpointElm1,
									 "string",
									 &endpointValue1);
    ERROR_RETURN(error, FuncName, "set value to endpoint1");

    /* set child:endpoint2 to method body block */
    error = OpenSOAPBlockAddChildMB(body,
									TRANSACTION_ENDPOINT,
									&endpointElm2);
    ERROR_RETURN(error, FuncName, "set children:endpoint2 to body part");

    
    OpenSOAPStringCreateWithMB(DEPOSIT_ID, &transactionIDAttrValue2);
    error = OpenSOAPXMLElmAddAttributeMB(endpointElm2,
										 TRANSACTION_REQUEST_ID_ATTRIBUTE,
										 "string",
										 &transactionIDAttrValue2,
										 &transactionIDAttr2);
    ERROR_RETURN(error, FuncName, "set attribute to endpoint2");	           

    OpenSOAPStringCreateWithMB(endpoint2, &endpointValue2);
    error = OpenSOAPXMLElmSetValueMB(endpointElm2,
									 "string",
									 &endpointValue2);
    ERROR_RETURN(error, FuncName, "set value to endpoint2");
    
    return error;
}

/* create payment request part of request message */
static
int
CreateTransactionPaymentBlock(OpenSOAPEnvelopePtr *request,
                              const char *account,
                              const char *amount,
                              const char *to) {

    static char FuncName[] = "CreateTransactionPaymentBlock";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr body = NULL;

    OpenSOAPXMLElmPtr requestElm = NULL;
    OpenSOAPXMLAttrPtr transactionIDAttr = NULL;
    OpenSOAPStringPtr transactionIDAttrValue = NULL;
    OpenSOAPXMLElmPtr accountElm = NULL;
    OpenSOAPStringPtr accountValue = NULL;
    OpenSOAPXMLElmPtr amountElm = NULL;
    OpenSOAPStringPtr amountValue = NULL;
    OpenSOAPXMLElmPtr toElm = NULL;
    OpenSOAPStringPtr toValue = NULL;

    /* add transaction body block */
    error = OpenSOAPEnvelopeAddBodyBlockMB(*request,
										   TRANSACTION_BODY_BLOCK,
										   &body);
    ERROR_RETURN(error, FuncName, "add body part");

    /* set namespace to transaction body block */
    error = OpenSOAPBlockSetNamespaceMB(body,
										TRANSACTION_NAMESPACE,
										TRANSACTION_PREFIX);
    ERROR_RETURN(error, FuncName, "set namespace to body part");

    /* set ID attribute to transaction body block */
    OpenSOAPStringCreateWithMB(PAYMENT_ID, &transactionIDAttrValue);
    error = OpenSOAPBlockAddAttributeMB(body,
										TRANSACTION_REQUEST_ID_ATTRIBUTE,
										"string",
										&transactionIDAttrValue,
										&transactionIDAttr);
    ERROR_RETURN(error, FuncName, "set attribute to body");

     
    /* set child:PaymentRequest to method body block */
    error = OpenSOAPBlockAddChildMB(body,
									"PaymentRequest",
									&requestElm);
    ERROR_RETURN(error, FuncName, "set child:PaymentRequest to body part");

    /* set namespace to request element */
    error = OpenSOAPXMLElmSetNamespaceMB(requestElm,
										 "http://services.opensoap.jp/a_bank_transfer/",
										 "p");
    ERROR_RETURN(error, FuncName, "set namespace to request element");
    
    /* set request contents */
    /* account */
    OpenSOAPStringCreateWithMB(account, &accountValue);
    error = OpenSOAPXMLElmAddChildMB(requestElm,
									 "account",
									 &accountElm);
    ERROR_RETURN(error, FuncName, "set child:account to request element");

    error = OpenSOAPXMLElmSetValueMB(accountElm,
									 "string",
									 &accountValue);
    ERROR_RETURN(error, FuncName, "set value to account");

    /* amount */
    OpenSOAPStringCreateWithMB(amount, &amountValue);
    error = OpenSOAPXMLElmAddChildMB(requestElm,
									 "amount",
									 &amountElm);
    ERROR_RETURN(error, FuncName, "set child:amount to request element");

    error = OpenSOAPXMLElmSetValueMB(amountElm,
									 "string",
									 &amountValue);
    ERROR_RETURN(error, FuncName, "set value to amount");
    
    /* to */
    OpenSOAPStringCreateWithMB(to, &toValue);
    error = OpenSOAPXMLElmAddChildMB(requestElm,
									 "to",
									 &toElm);
    ERROR_RETURN(error, FuncName, "set child:to to request element");

    error = OpenSOAPXMLElmSetValueMB(toElm,
									 "string",
									 &toValue);
    ERROR_RETURN(error, FuncName, "set value to to");

    return error;
}

/* create deposit request part of request message */
static
int
CreateTransactionDepositBlock(OpenSOAPEnvelopePtr *request,
                              const char *account,
                              const char *amount,
                              const char *from) {

    static char FuncName[] = "CreateTransactionDepositBlock";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr body = NULL;

    OpenSOAPXMLElmPtr requestElm = NULL;
    OpenSOAPXMLAttrPtr transactionIDAttr = NULL;
    OpenSOAPStringPtr transactionIDAttrValue = NULL;
    OpenSOAPXMLElmPtr accountElm = NULL;
    OpenSOAPStringPtr accountValue = NULL;
    OpenSOAPXMLElmPtr amountElm = NULL;
    OpenSOAPStringPtr amountValue = NULL;
    OpenSOAPXMLElmPtr fromElm = NULL;
    OpenSOAPStringPtr fromValue = NULL;

    /* add transaction body block */
    error = OpenSOAPEnvelopeAddBodyBlockMB(*request,
										   TRANSACTION_BODY_BLOCK,
										   &body);
    ERROR_RETURN(error, FuncName, "add body part");

    /* set namespace to transaction body block */
    error = OpenSOAPBlockSetNamespaceMB(body,
										TRANSACTION_NAMESPACE,
										TRANSACTION_PREFIX);
    ERROR_RETURN(error, FuncName, "set namespace to body part");

    /* set ID attribute to transaction body block */
    OpenSOAPStringCreateWithMB(DEPOSIT_ID, &transactionIDAttrValue);
    error = OpenSOAPBlockAddAttributeMB(body,
										TRANSACTION_REQUEST_ID_ATTRIBUTE,
										"string",
										&transactionIDAttrValue,
										&transactionIDAttr);
    ERROR_RETURN(error, FuncName, "set attribute to body");

     
    /* set child:PaymentRequest to method body block */
    error = OpenSOAPBlockAddChildMB(body,
									"DepositRequest",
									&requestElm);
    ERROR_RETURN(error, FuncName, "set child:DepositRequest to body part");

    /* set namespace to request element */
    error = OpenSOAPXMLElmSetNamespaceMB(requestElm,
										 "http://services.opensoap.jp/b_bank_transfer/",
										 "p");
    ERROR_RETURN(error, FuncName, "set namespace to request element");
    
    /* set request contents */
    /* account */
    OpenSOAPStringCreateWithMB(account, &accountValue);
    error = OpenSOAPXMLElmAddChildMB(requestElm,
									 "account",
									 &accountElm);
    ERROR_RETURN(error, FuncName, "set child:account to request element");

    error = OpenSOAPXMLElmSetValueMB(accountElm,
									 "string",
									 &accountValue);
    ERROR_RETURN(error, FuncName, "set value to account");

    /* amount */
    OpenSOAPStringCreateWithMB(amount, &amountValue);
    error = OpenSOAPXMLElmAddChildMB(requestElm,
									 "amount",
									 &amountElm);
    ERROR_RETURN(error, FuncName, "set child:amount to request element");

    error = OpenSOAPXMLElmSetValueMB(amountElm,
									 "string",
									 &amountValue);
    ERROR_RETURN(error, FuncName, "set value to amount");
    
    /* to */
    OpenSOAPStringCreateWithMB(from, &fromValue);
    error = OpenSOAPXMLElmAddChildMB(requestElm,
									 "from",
									 &fromElm);
    ERROR_RETURN(error, FuncName, "set child:from to request element");

    error = OpenSOAPXMLElmSetValueMB(fromElm,
									 "string",
									 &fromValue);
    ERROR_RETURN(error, FuncName, "set value to from");

    return error;
}

/* create transfer request message */
static
int
CreateTransferRequest(const char *endpoint1,
					  const char *endpoint2,
					  const char *account1,
					  const char *account2,
					  const char *money,
					  const char *async,
					  OpenSOAPEnvelopePtr *request) {

    static char FuncName[] = "CreateTransferRequest";
    int error = OPENSOAP_NO_ERROR;

    error = CreateRequestCommon(request, endpoint1, endpoint2, async);
    ERROR_RETURN(error, FuncName, "create common part of request");
    
    error = CreateTransactionPaymentBlock(request, account1, money, account2)
		ERROR_RETURN(error, FuncName, "set paymentBlock");

    error = CreateTransactionDepositBlock(request, account2, money, account1)
		ERROR_RETURN(error, FuncName, "set depositBlock");

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
												stderr);
					if (OPENSOAP_SUCCEEDED(error)) {
		  
						error = OpenSOAPTransportInvoke(transport,
														*request,
														response);
						if (OPENSOAP_SUCCEEDED(error)) {
		      
							error = WriteLabeledMessage(*response,
														NULL,
														"response",
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

/* get parameter value of string type from message */
static
int
GetStringParameter(OpenSOAPBlockPtr body,
				   const char *paramName,
				   char **paramValue) {
    static const char FuncName[] = "GetStringParameter";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPXMLElmPtr childElm = NULL;
	
    *paramValue = NULL;
	
    error = OpenSOAPBlockGetChildMB(body, paramName, &childElm);
    if (childElm != NULL) {
		OpenSOAPStringPtr param = NULL;
		error = OpenSOAPStringCreate(&param);
		ERROR_RETURN(error, FuncName, "create OpenSOAPString");
		
        error = OpenSOAPBlockGetChildValueMB(body,
											 paramName,
											 "string",
											 &param);
		if (OPENSOAP_FAILED(error)) {
			ERROR_MSG(error, FuncName, "get value of parameter");
			OpenSOAPStringRelease(param);
			return error;
		}

		error = OpenSOAPStringGetStringMBWithAllocator(param,
													   NULL,
													   NULL,
													   paramValue);
		OpenSOAPStringRelease(param);
		ERROR_RETURN(error, FuncName, "get string from parameter");
    }
	
    return error;
}

/* parse element of fault message */
static
int
ParseFaultElement(OpenSOAPBlockPtr body,
				  const char *elementName,
				  char **elementValue) {
    static const char FuncName[] = "ParseFaultElement";
    static const char errorMessagePrefix[] = "get ";
	static const char badAllocErrorMessage[] = "get unknow element";
	const char *errorMessage = badAllocErrorMessage;
    char *errorMessageImpl = NULL;
    char *paramValue = NULL;
    int error = OPENSOAP_NO_ERROR;

    if (!elementName) {
        elementName = "";
    }
    /* create error message */
    errorMessageImpl
		= malloc(sizeof(errorMessagePrefix) + strlen(elementName) + 1);
	if (errorMessageImpl) {
        strcpy(errorMessageImpl, errorMessagePrefix);
		strcat(errorMessageImpl, elementName);
		errorMessage = errorMessageImpl;
	}

    /* */
    error = GetStringParameter(body, elementName, &paramValue);
	if (OPENSOAP_FAILED(error)) {
		ERROR_MSG(error, FuncName, errorMessage);
		free(errorMessageImpl);
		return error;
	}
	free(errorMessageImpl);

    if (!paramValue) {
        paramValue = strdup("(null)");
    }

    fprintf(stderr, "    %s: %s\n", elementName, paramValue);
    
    /* */
    if (elementValue) {
        free(*elementValue);
		*elementValue = paramValue;
    }
    else {
        free(paramValue);
    }

    return error;
}

/* parse message in case of fault*/
static
int
ParseFaultMessage(OpenSOAPBlockPtr body,
				  char **faultCode,
				  char **faultString,
				  char **faultActor,
				  char **faultDetail) {

    int error = OPENSOAP_NO_ERROR;
	
    fprintf(stderr, "Fault message:\n");
	
    /* parse faultcode */
	
    error = ParseFaultElement(body, "faultcode", faultCode);
    if (OPENSOAP_FAILED(error)) {
        return error;
    }
	
    /* parse faultstring */
	
    error = ParseFaultElement(body, "faultstring", faultString);
    if (OPENSOAP_FAILED(error)) {
        return error;
    }
    
    /* parse faultactor */

    error = ParseFaultElement(body, "faultactor", faultActor);
    if (OPENSOAP_FAILED(error)) {
        return error;
    }
	
    /* parse detail (as plane string) */
	
    error = ParseFaultElement(body, "detail", faultDetail);
	
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
		
        error = ParseFaultMessage(*body,NULL,NULL,NULL,NULL);
		ERROR_RETURN(error, FuncName, "parse fault message");
		
		*isFault = 1;
		
		return error;
    }
	
    /* process response body */
	
    error = OpenSOAPEnvelopeGetBodyBlockMB(response, responseName, body);
    ERROR_RETURN(error, FuncName, "complete getting body block");
	
    if (*body != NULL) {
		
        error = OpenSOAPBlockIsSameNamespaceMB(*body,
											   TRANSACTION_NAMESPACE,
											   &isSameNS);
		ERROR_RETURN(error, FuncName, "complete namespace matching");
		
		error = (isSameNS ? error : APPLICATION_ERROR);
		ERROR_RETURN(error, FuncName, "match namespace");
		
		return error;
    }
    
    return APPLICATION_ERROR;
}

/* parse trnsaction response message */
static
int
ParseTransactionResponse(OpenSOAPEnvelopePtr response,
						 ResultVariables *rsltVars) {

    static char FuncName[] = "ParseTransactionResponse";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPBlockPtr responseBody = NULL;
    int isFault = 0;
    size_t len = 0;

    OpenSOAPBlockPtr bodyBlock = NULL;
    int requestIDCmpRslt = 1;
    OpenSOAPStringPtr blockName = NULL;
    int blockNameCmpRslt = 1;

    /* Parse TransactionResponse Block */
    error = ParseResponseCommon(response,
								TRANSACTION_RESPONSE_METHOD,
								&responseBody,
								&isFault);
    ERROR_RETURN(error, FuncName, "parse common part of response");
    
    if (isFault) {
        free(rsltVars->result);
		rsltVars->result = strdup("Fault");
        return error;
    }
	
    error = GetStringParameter(responseBody,
							   TRANSACTION_RESPONSE_RESULT,
							   &rsltVars->result);
    ERROR_RETURN(error, FuncName, "get resultstring");

    fprintf(stderr, "\nResponse: result = %s\n", rsltVars->result);

    /* Parse TransactionBodyBlock and TransactionActionResponseBodyBlock */
    for(bodyBlock=NULL;
		OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextBodyBlock(response,
															&bodyBlock))
			&& bodyBlock;){

        /* Attribute Name Check */
        OpenSOAPXMLAttrPtr requestIDAttr = NULL;

        if(OPENSOAP_SUCCEEDED(
			   OpenSOAPBlockGetAttributeMB(bodyBlock,
										   TRANSACTION_REQUEST_ID_ATTRIBUTE,
										   &requestIDAttr))
		   && requestIDAttr!=NULL){
	
			/* Attribute Value Check */
			OpenSOAPStringPtr requestIDStr = NULL;

			error = OpenSOAPXMLAttrGetValueMB(requestIDAttr,
											  "string",
											  &requestIDStr);
			ERROR_RETURN(error, FuncName, "get request ID attribute");

			/* Payment Part */
			if(OPENSOAP_SUCCEEDED(
				   OpenSOAPStringCompareMB(requestIDStr,
										   PAYMENT_ID,
										   &requestIDCmpRslt))
			   && requestIDCmpRslt == 0){

				/* Bodyblock Name Check */
				error = OpenSOAPBlockGetName(bodyBlock, &blockName);
				ERROR_RETURN(error, FuncName, "get bodyblock name");

				/* Parse TransactionBodyBlock */
				if(OPENSOAP_SUCCEEDED(
					   OpenSOAPStringCompareMB(blockName,
											   TRANSACTION_BODY_BLOCK,
											   &blockNameCmpRslt))
				   && blockNameCmpRslt == 0){

					OpenSOAPXMLElmPtr serviceResponseElm;
					if(OPENSOAP_SUCCEEDED(
						   OpenSOAPBlockGetChildMB(bodyBlock,
												   "PaymentResponse",
												   &serviceResponseElm))
					   && serviceResponseElm !=0){

						OpenSOAPXMLElmPtr resultElm = NULL;
						OpenSOAPXMLAttrPtr serviceIDAttr = NULL;
						OpenSOAPStringPtr serviceID = NULL;

						error = GetStringParameter((OpenSOAPBlockPtr)serviceResponseElm,
												   TRANSACTION_RESULT,
												   &rsltVars->paymentResult);
						ERROR_CHECK(error,
									FuncName,
									"get payment resultstring");

						error = GetStringParameter((OpenSOAPBlockPtr)serviceResponseElm,
												   "Comment",
												   &rsltVars->paymentComment);
						ERROR_CHECK(error,
									FuncName,
									"get payment commentstring");
			
						error = OpenSOAPXMLElmGetChildMB(serviceResponseElm,
														 TRANSACTION_RESULT,
														 &resultElm);
						ERROR_CHECK(error,
									FuncName,
									"get payment result element");
						error = OpenSOAPXMLElmGetAttributeMB(resultElm,
															 TRANSACTION_ID_ATTRIBUTE,
															 &serviceIDAttr);
						ERROR_CHECK(error,
									FuncName,
									"get transaction ID attribute");
						error = OpenSOAPXMLAttrGetValueMB(serviceIDAttr,
														  "string",
														  &serviceID);
						ERROR_CHECK(error,
									FuncName,
									"get transaction ID value");
						OpenSOAPStringGetLengthMB(serviceID, &len);
						rsltVars->paymentTransactionID = malloc(len + 1);
						OpenSOAPStringGetStringMB(serviceID,
												  &len,
												  rsltVars->paymentTransactionID);

					}
					else if(OPENSOAP_SUCCEEDED(
                                OpenSOAPBlockGetChildMB(bodyBlock,
														"Fault",
														&serviceResponseElm))
							&& serviceResponseElm !=0){
						error = ParseFaultMessage(
							(OpenSOAPBlockPtr)serviceResponseElm,
							&rsltVars->paymentTransactionID,
							&rsltVars->paymentResult,
							&rsltVars->paymentActionResponse,
							&rsltVars->paymentComment);
						ERROR_RETURN(error,
									 FuncName,
									 "parse fault message");
					}
					else{
						ERROR_MSG(APPLICATION_ERROR,
								  FuncName,
								  "get Response Block from Bank Service A");
					}

				}

				else if(OPENSOAP_SUCCEEDED(
							OpenSOAPStringCompareMB(blockName,
													TRANSACTION_ACTION_RESPONSE_BODY_BLOCK,
													&blockNameCmpRslt))
						&& blockNameCmpRslt == 0){
					error = GetStringParameter(bodyBlock,
											   TRANSACTION_ACTION_RESPONSE_METHOD,
											   &rsltVars->paymentActionResponse);
					ERROR_CHECK(error,
								FuncName,
								"get payment action result");

				}

				else{
					CheckStr("Invalid block name", blockName);
					error = APPLICATION_ERROR;
				}
			}
			/* Deposit Part */
			else if(OPENSOAP_SUCCEEDED(
						OpenSOAPStringCompareMB(requestIDStr,
												DEPOSIT_ID,
												&requestIDCmpRslt))
					&& requestIDCmpRslt == 0){

				/* Bodyblock Name Check */
				error = OpenSOAPBlockGetName(bodyBlock, &blockName);
				ERROR_RETURN(error, FuncName, "get bodyblock name");

				/* Parse TransactionBodyBlock */
				if(OPENSOAP_SUCCEEDED(
					   OpenSOAPStringCompareMB(blockName,
											   TRANSACTION_BODY_BLOCK,
											   &blockNameCmpRslt))
				   && blockNameCmpRslt == 0){

					OpenSOAPXMLElmPtr serviceResponseElm;
					if(OPENSOAP_SUCCEEDED(
						   OpenSOAPBlockGetChildMB(bodyBlock,
												   "DepositResponse",
												   &serviceResponseElm))
					   && serviceResponseElm !=0){

						OpenSOAPXMLElmPtr resultElm = NULL;
						OpenSOAPXMLAttrPtr serviceIDAttr = NULL;
						OpenSOAPStringPtr serviceID = NULL;

						error = GetStringParameter((OpenSOAPBlockPtr)serviceResponseElm,
												   TRANSACTION_RESULT,
												   &rsltVars->depositResult);
						ERROR_CHECK(error,
									FuncName,
									"get payment resultstring");

						error = GetStringParameter((OpenSOAPBlockPtr)serviceResponseElm,
												   "Comment",
												   &rsltVars->depositComment);
						ERROR_CHECK(error,
									FuncName,
									"get payment commentstring");
			
						error = OpenSOAPXMLElmGetChildMB(serviceResponseElm,
														 TRANSACTION_RESULT,
														 &resultElm);
						ERROR_CHECK(error,
									FuncName,
									"get payment result element");
						error = OpenSOAPXMLElmGetAttributeMB(resultElm,
															 TRANSACTION_ID_ATTRIBUTE,
															 &serviceIDAttr);
						ERROR_CHECK(error,
									FuncName,
									"get transaction ID attribute");
						error = OpenSOAPXMLAttrGetValueMB(serviceIDAttr,
														  "string",
														  &serviceID);
						ERROR_CHECK(error,
									FuncName,
									"get transaction ID value");
						OpenSOAPStringGetLengthMB(serviceID, &len);
						rsltVars->depositTransactionID = malloc(len + 1);
						OpenSOAPStringGetStringMB(serviceID,
												  &len,
												  rsltVars->depositTransactionID);

					}
					else if(OPENSOAP_SUCCEEDED(
                                OpenSOAPBlockGetChildMB(bodyBlock,
														"Fault",
														&serviceResponseElm))
							&& serviceResponseElm !=0){
						error = ParseFaultMessage(
							(OpenSOAPBlockPtr)serviceResponseElm,
							&rsltVars->depositTransactionID,
							&rsltVars->depositResult,
							&rsltVars->depositActionResponse,
							&rsltVars->depositComment);
						ERROR_RETURN(error,
									 FuncName,
									 "parse fault message");
					}

					else{
						ERROR_MSG(APPLICATION_ERROR,
								  FuncName,
								  "get Response Block from Bank Service B");
					}

				}

				else if(OPENSOAP_SUCCEEDED(
							OpenSOAPStringCompareMB(blockName,
													TRANSACTION_ACTION_RESPONSE_BODY_BLOCK,
													&blockNameCmpRslt))
						&& blockNameCmpRslt == 0){
					error = GetStringParameter(bodyBlock,
											   TRANSACTION_ACTION_RESPONSE_METHOD,
											   &rsltVars->depositActionResponse);
					ERROR_CHECK(error,
								FuncName,
								"get deposit action result");

				}

				else{
					CheckStr("Invalid block name", blockName);
					error = APPLICATION_ERROR;
				}
			}
		}
    }

    fprintf(stderr,
			"\nResponse: paymentResult = %s\n"
			"Response: paymentComment = %s\n"
			"Response: paymentTransactionID = %s\n"
			"Response: paymentActionResponse = %s\n"
			"\nResponse: depositResult = %s\n"
			"Response: depositComment = %s\n"
			"Response: depositTransactionID = %s\n"
			"Response: depositActionResponse = %s\n",
			rsltVars->paymentResult,
			rsltVars->paymentComment,
			rsltVars->paymentTransactionID,
			rsltVars->paymentActionResponse,
			rsltVars->depositResult,
			rsltVars->depositComment,
			rsltVars->depositTransactionID,
			rsltVars->depositActionResponse);

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
		
        error = ParseFaultMessage(block,NULL,NULL,NULL,NULL);
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
Transfer(const char *endpoint,
		 const char *soapAction,
		 const char *charEnc,
		 const char *endpoint1,
		 const char *endpoint2,
		 const char *account1,
		 const char *account2,
		 const char *money,
		 ResultVariables *rsltVars) {
    static char FuncName[] = "Transfer";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPEnvelopePtr request = NULL;
	
    error = InitializeResultVariables(rsltVars);
    ERROR_RETURN(error, FuncName, "initialize Variables for result");

    error = CreateTransferRequest(endpoint1, endpoint2,
								  account1, account2,
								  money,
								  "false",
								  &request);   

    if (OPENSOAP_SUCCEEDED(error)) {
        OpenSOAPEnvelopePtr response = NULL;
		
		error = InvokeService(endpoint,
							  soapAction,
							  charEnc,
							  &request,
							  &response);
		if (OPENSOAP_SUCCEEDED(error)) {
			
			error = ParseTransactionResponse(response, rsltVars);
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
TransferAsync(const char *endpoint,
			  const char *soapAction,
			  const char *charEnc,
			  const char *endpoint1,
			  const char *endpoint2,
			  const char *account1,
			  const char *account2,
			  const char *money,
			  char **messageID,
			  FILE *fp) {
    static char FuncName[] = "TransferAsync";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPEnvelopePtr request = NULL;

    error = CreateTransferRequest(endpoint1, endpoint2,
								  account1, account2,
								  money,
								  "true",
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
		  ResultVariables *rsltVars) {
    static char FuncName[] = "GetResult";
    int error = OPENSOAP_NO_ERROR;
    OpenSOAPEnvelopePtr request = NULL;

    error = InitializeResultVariables(rsltVars);
    ERROR_RETURN(error, FuncName, "initialize Variables for result");
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
			
			error = ParseTransactionResponse(response, rsltVars);
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
  
    const char endpoint1[] = "http://localhost/cgi-bin/TransactionABankService.cgi";
    const char endpoint2[] = "http://localhost/cgi-bin/TransactionABankService.cgi";

    ClientVariables appVars;
    long error = OPENSOAP_NO_ERROR;
	
    ResultVariables rsltVars;

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
			"method   : %s\n"
			"account 1: %s\n"
			"account 2: %s\n"
			"amount   : %s\n",
			appVars.soapEndpoint ? appVars.soapEndpoint : "",
			appVars.soapAction ? appVars.soapAction : "",
			appVars.charEnc ? appVars.charEnc : "",
			appVars.method,
			appVars.account1,
			appVars.account2,
			appVars.amount);
	
    if (strcmp(appVars.method, "Sync") == 0) {
		error = Transfer(appVars.soapEndpoint,
						 appVars.soapAction,
						 appVars.charEnc,
						 endpoint1,
						 endpoint2,
						 appVars.account1,
						 appVars.account2,
						 appVars.amount,
						 &rsltVars);
    }
    else if (strcmp(appVars.method, "Async") == 0){
        char *messageID;
	
		error = TransferAsync(appVars.soapEndpoint,
							  appVars.soapAction,
							  appVars.charEnc,
							  endpoint1,
							  endpoint2,
							  appVars.account1,
							  appVars.account2,
							  appVars.amount,
							  &messageID,
							  stdout);
		
		fprintf(stderr, "MessageID: %s\n", messageID);
    }
    else if (strcmp(appVars.method, "GetResult") == 0) {
	
		error = GetResult(stdin,
						  appVars.soapEndpoint,
						  appVars.soapAction,
						  appVars.charEnc,
						  &rsltVars);

    }
    else{
		fprintf(stderr, "Transaction Sample: Invalid Method Name\n");
        fprintf(stderr, USAGE_FORMAT,
				appVars.appName ? appVars.appName : "TransactionClient");
		exit(0);
    }
    OpenSOAPUltimate();

    return 0;
}
#endif /* CLIENT_CGI */
