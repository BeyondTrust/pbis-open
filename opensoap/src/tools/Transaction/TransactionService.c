/*-----------------------------------------------------------------------------
 * $RCSfile: TransactionService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>
#include <OpenSOAP/Transport.h>

#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>

#define INT_DIGITS8 ((sizeof(int) * CHAR_BIT + 3 - 1) / 3)

#ifndef SERVICE_LOCALSTATEDIR
# define SERVICE_LOCALSTATEDIR "/usr/local/opensoap/var/services/Transaction"
#endif
static const char TRANSACTION_LOG_FILE[] = SERVICE_LOCALSTATEDIR "/Transaction.log";

#if defined(CONNECT_TYPE_CGI)
# define CONNECT_TYPE "cgi"
# define LOG_STREAM stderr
#else
#include <time.h>
# define CONNECT_TYPE "stdio"
FILE *fp;
# define LOG_STREAM fp
#endif /* CONNECT_TYPE_CGI */

/* error message macro */
#define ERROR_MSG(error, location, message); \
fprintf(LOG_STREAM, \
		"%s: Cannot %s\n" \
		" ---> OpenSOAP Error Code: 0x%04x\n", \
		(location),\
		(message),\
		(error));

/* error check macro */
#define ERROR_CHECK(error, location, message); \
if (OPENSOAP_FAILED((error))) { \
	ERROR_MSG(error, location, message) \
}

/* error return macro */
#define ERROR_RETURN(error, location, message); \
if (OPENSOAP_FAILED((error))) { \
	ERROR_MSG(error, location, message) \
	return (error); \
}

static const char TRANSACTION_CONTROL_METHOD[] = "TransactionControl";
static const char TRANSACTION_HEADER_BLOCK[] = "TransactionHeaderBlock";
static const char TRANSACTION_BODY_BLOCK[] = "TransactionBodyBlock";
static const char TRANSACTION_ENDPOINT[] = "endpoint";
static const char TRANSACTION_REQUEST_ID_ATTRIBUTE[] = "transactionRequestID";

static const char TRANSACTION_NAMESPACE[] = "http://services.opensoap.jp/Transaction/";
static const char TRANSACTION_PREFIX[] = "t";

static const char TRANSACTION_RESULT[] = "TransactionResult";
static const char TRANSACTION_ID_ATTRIBUTE[] = "transactionID";
/* added 1line 2002/07/31 */
static const char TRANSACTION_ACTION_NS_ATTRIBUTE[] = "transactionActionNs";
static const char TRANSACTION_SUCCESS[] = "SUCCESS";
static const char TRANSACTION_ACTION_METHOD[] = "TransactionAction";
static const char TRANSACTION_ACTION_RESPONSE_BODY_BLOCK[] = "TransactionActionResponseBodyBlock";
static const char TRANSACTION_COMMIT[] = "COMMIT";
static const char TRANSACTION_ROLLBACK[] = "ROLLBACK";

static const char TRANSACTION_RESPONSE_METHOD[] = "TransactionResponse";
static const char TRANSACTION_RESPONSE_RESULT[] = "Result";

typedef struct {
    OpenSOAPStringPtr       endpoint;
    OpenSOAPXMLAttrPtr      trRequestIDAttr;

    OpenSOAPXMLElmPtr       requestHeaderBlock;
    OpenSOAPXMLElmPtr       requestBodyBlock;

    OpenSOAPEnvelopePtr     requestEnvelope;
    OpenSOAPEnvelopePtr     responseEnvelope;
    OpenSOAPXMLAttrPtr      trResponseIDAttr;
	/* added 1line 2002/07/31 */
    OpenSOAPXMLAttrPtr      trResponseNsAttr;

    OpenSOAPEnvelopePtr     actionRequestEnvelope;
    OpenSOAPEnvelopePtr     actionResponseEnvelope;
} TransactionParameters;


static
void
OutputEnvelope(OpenSOAPEnvelopePtr env,
			   const char *char_enc,
			   const char *msg,
			   FILE *fp) {

    if (!msg) {
        msg = "";
    }
    if (env && fp) {
        OpenSOAPByteArrayPtr env_buf = NULL;
		int error_code = OpenSOAPByteArrayCreate(&env_buf);
		if (OPENSOAP_SUCCEEDED(error_code)) {
			error_code = OpenSOAPEnvelopeGetCharEncodingString(env,
															   char_enc,
															   env_buf);
			if (OPENSOAP_SUCCEEDED(error_code)) {
				const unsigned char *env_beg = NULL;
				size_t env_sz = 0;
				error_code = OpenSOAPByteArrayGetBeginSizeConst(env_buf,
																&env_beg,
																&env_sz);
				if (OPENSOAP_SUCCEEDED(error_code)) {
					fprintf(fp,
							"\n"
							"=== %s soap envelope begin ===\n",
							msg);
					fwrite(env_beg, 1, env_sz, fp);
					fprintf(fp,
							"\n"
							"=== %s soap envelope  end  ===\n",
							msg);
				}
			}
	    
			OpenSOAPByteArrayRelease(env_buf);
		}
    }
}

static
void 
CheckStr(char *preStr,
		 OpenSOAPStringPtr strPtr) {
    char *string = NULL;

	OpenSOAPStringGetStringMBWithAllocator(strPtr,
										   NULL,
										   NULL,
										   &string);

	fprintf(LOG_STREAM, "%s: %s\n", preStr, string);
	
    free(string);
}

static
int 
CountProcessNo(OpenSOAPBlockPtr process) {
    int trNo = 0;
    OpenSOAPXMLElmPtr elm = NULL;
	
    while (OPENSOAP_SUCCEEDED(OpenSOAPBlockGetNextChild(process, &elm))
		   && elm) {
		OpenSOAPStringPtr elmName = NULL;
		int cmpRslt = 1;

		OpenSOAPStringCreate(&elmName);
        OpenSOAPXMLElmGetNameString(elm, &elmName);
		OpenSOAPStringCompareMB(elmName,
								TRANSACTION_ENDPOINT,
								&cmpRslt);
		OpenSOAPStringRelease(elmName);
		if (cmpRslt == 0) {
			++trNo;
		}
    }

    fprintf(LOG_STREAM, "Number of Transactions = %d\n", trNo);
	
    return trNo;
}

static
int 
InitializeTransactionProcessParameter(TransactionParameters *trPrm,
									  int trNo) {
	int i;
	for (i = 0; i < trNo; ++i) {
		trPrm[i].endpoint               = NULL;
		trPrm[i].trRequestIDAttr        = NULL;

		trPrm[i].requestHeaderBlock     = NULL;
		trPrm[i].requestBodyBlock       = NULL;
    
		trPrm[i].requestEnvelope        = NULL;
		trPrm[i].responseEnvelope       = NULL;
		trPrm[i].trResponseIDAttr       = NULL;
		/* added 1line 2002/07/31 */
		trPrm[i].trResponseNsAttr       = NULL;

		trPrm[i].actionRequestEnvelope  = NULL;
		trPrm[i].actionResponseEnvelope = NULL;
	}
	
	return 0;
}

static
int 
CheckTransactionParameter(TransactionParameters *trPrm,
						  int trNo) {
	int i;
	OpenSOAPStringPtr attrStr = NULL;

	for (i = 0; i < trNo; ++i) {
		fprintf(LOG_STREAM, "Parameters[%d]\n",i);
		CheckStr("endpoint",trPrm[i].endpoint);
		OpenSOAPXMLAttrGetValueMB(trPrm[i].trRequestIDAttr,"string",&attrStr);
		CheckStr("trIDAttr",attrStr);
	}

	return OPENSOAP_NO_ERROR;
}


static
int 
CriateTransactionProcessParameters(TransactionParameters *trPrm,
								   OpenSOAPEnvelopePtr request,
								   int trNo) {
    int i;
    int errCode = 0;
    OpenSOAPBlockPtr bodyBlock = NULL;
    OpenSOAPXMLElmPtr elm      = NULL; 
    OpenSOAPStringPtr elmName  = NULL;
    int cmpRslt = 1;

    if (OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextBodyBlock(request,
															&bodyBlock))
       && bodyBlock) {
    
        /* Transaction Proccess Section */ 
        for(i = 0;
			OPENSOAP_SUCCEEDED(OpenSOAPBlockGetNextChild(bodyBlock,&elm))
				&& elm
				&& i < trNo;) {

			/* Check Endpoint Element */
			OpenSOAPXMLElmGetNameString(elm, &elmName);
			OpenSOAPStringCompareMB(elmName, TRANSACTION_ENDPOINT, &cmpRslt);
			if(cmpRslt == 0){

				/* Getting each endpoint */
				errCode = OpenSOAPXMLElmGetValueMB(
					elm,
					"string",
					&trPrm[i].endpoint);

				/* Getting each request Transaction ID */
				errCode = OpenSOAPXMLElmGetAttributeMB(
					elm,
					TRANSACTION_REQUEST_ID_ATTRIBUTE,
					&trPrm[i].trRequestIDAttr);
				i++;
			}
		}
    }
	
    return errCode;
}


static
int
GetTransactionBlock(TransactionParameters *trPrm,
					OpenSOAPEnvelopePtr request,
					int trNo) {
    int                  i = 0;
    int                  ret = 0;
    OpenSOAPBlockPtr     bodyBlock = NULL;
    OpenSOAPXMLAttrPtr   attrPtr = NULL;
    OpenSOAPStringPtr    attrStr = NULL;
    OpenSOAPStringPtr    attrPrm = NULL;
    OpenSOAPStringPtr    blockName = NULL;
    int                  attrCmpRslt = 1;
    int                  bodyCmpRslt = 1;
    int                  headerCmpRslt = 1;

    /* Transaction Body Block Section */
    for (i = 0; i < trNo; ++i) {
        attrPtr = NULL;
		OpenSOAPXMLAttrGetValueMB(trPrm[i].trRequestIDAttr,
								  "string",
								  &attrPrm);
		for(bodyBlock=NULL;
			OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextBodyBlock(request,
																&bodyBlock))
				&& bodyBlock;){

			/* Attribute Name Check */
			if(OPENSOAP_SUCCEEDED(
				   OpenSOAPBlockGetAttributeMB(bodyBlock,
											   TRANSACTION_REQUEST_ID_ATTRIBUTE,
											   &attrPtr))
			   && attrPtr) {
	
				/* Attribute Value Check */
				OpenSOAPXMLAttrGetValueMB(attrPtr,"string",&attrStr);
				OpenSOAPStringCompare(attrStr,attrPrm,&attrCmpRslt);
				if (attrCmpRslt == 0) {

					/* Bodyblock Name Check */
					OpenSOAPBlockGetName(bodyBlock, &blockName);
					OpenSOAPStringCompareMB(blockName,
											TRANSACTION_HEADER_BLOCK,
											&headerCmpRslt);
					OpenSOAPStringCompareMB(blockName,
											TRANSACTION_BODY_BLOCK,
											&bodyCmpRslt);
					if (headerCmpRslt == 0) {
						trPrm[i].requestHeaderBlock 
							= (OpenSOAPXMLElmPtr)bodyBlock;
					}
					else if (bodyCmpRslt == 0) {
						trPrm[i].requestBodyBlock 
							= (OpenSOAPXMLElmPtr)bodyBlock;
					}
					else {
						ret = 1;
					}
				}
			}
		}
    }
	
    return ret;
}



static
int 
XMLElmDupricate(OpenSOAPXMLElmPtr elm,
				OpenSOAPXMLElmPtr dupElm) {
    static char FuncName[] = "XMLElmDupricate";
    int error = OPENSOAP_NO_ERROR;

    size_t                  len = 0;
    int                     i = 0;

    OpenSOAPXMLNamespacePtr ns = NULL;
    OpenSOAPStringPtr       nsURL = NULL;
    char *                  nsURLString = NULL;
    OpenSOAPStringPtr       nsPrefix = NULL;
    char *                  nsPrefixString = NULL;

    int                     attrNo = 0;
    OpenSOAPXMLAttrPtr      attr = NULL;
    OpenSOAPStringPtr       attrName = NULL;
    char *                  attrNameString = NULL;
    OpenSOAPStringPtr       attrValue = NULL;
    OpenSOAPXMLAttrPtr *    addedAttr = NULL;
    OpenSOAPXMLNamespacePtr attrNs = NULL;    
    OpenSOAPStringPtr       attrNsURL = NULL;
    char *                  attrNsURLString = NULL;
    OpenSOAPStringPtr       attrNsPrefix = NULL;
    char *                  attrNsPrefixString = NULL;

    OpenSOAPStringPtr       value = NULL;

    int                     elmNo = 0;
    OpenSOAPXMLElmPtr       child = NULL;
    OpenSOAPStringPtr       childName = NULL;
    char *                  childNameString = NULL;
    OpenSOAPXMLElmPtr *     elmChild = NULL;


    /* Add Namespaces */
    if (OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetNamespace(elm, &ns)) && ns) {

		OpenSOAPXMLNamespaceGetURI(ns,&nsURL);
		OpenSOAPStringGetLengthMB(nsURL, &len);
		nsURLString = malloc(len + 1);
		OpenSOAPStringGetStringMB(nsURL, &len, nsURLString);
	
		OpenSOAPXMLNamespaceGetPrefix(ns,&nsPrefix);
		OpenSOAPStringGetLengthMB(nsPrefix, &len);
		nsPrefixString = malloc(len + 1);
		OpenSOAPStringGetStringMB(nsPrefix, &len, nsPrefixString);
	
		OpenSOAPXMLElmSetNamespaceMB(dupElm,
									 nsURLString,
									 nsPrefixString);
		OpenSOAPStringClear(nsURL);
		OpenSOAPStringClear(nsPrefix);
		free(nsURLString);
		free(nsPrefixString);
    }
    

    /* Add Attributes */
    attrNo = 0;
    for(attr = NULL;
		OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetNextAttr(elm, &attr))
			&& attr;) {
		++attrNo;
    }
    addedAttr = (OpenSOAPXMLAttrPtr*)malloc(attrNo *
											sizeof(OpenSOAPXMLAttrPtr));

    for (attr = NULL;
		 OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetNextAttr(elm, &attr))
			 && attr
			 && i<attrNo;
		 ++i) {
        
        addedAttr[i] = NULL;

        OpenSOAPXMLAttrGetName(attr,&attrName);
		OpenSOAPStringGetLengthMB(attrName, &len);
		attrNameString = malloc(len + 1);
		OpenSOAPStringGetStringMB(attrName, &len, attrNameString);

		OpenSOAPXMLAttrGetValueMB(attr,"string",&attrValue);
	
		OpenSOAPXMLElmAddAttributeMB(dupElm,
									 attrNameString,
									 "string",
									 &attrValue,
									 &addedAttr[i]);

		/* Add Namespace of Attribute */
		if(OPENSOAP_SUCCEEDED(OpenSOAPXMLAttrGetNamespace(attr,
														  &attrNs)) 
		   && attrNs){

			OpenSOAPXMLNamespaceGetURI(attrNs,&attrNsURL);
			OpenSOAPStringGetLengthMB(attrNsURL, &len);
			attrNsURLString = malloc(len + 1);
			OpenSOAPStringGetStringMB(attrNsURL, &len, attrNsURLString);
	
			OpenSOAPXMLNamespaceGetPrefix(attrNs,&attrNsPrefix);
			OpenSOAPStringGetLengthMB(attrNsPrefix, &len);
			attrNsPrefixString = malloc(len + 1);
			OpenSOAPStringGetStringMB(attrNsPrefix, &len, attrNsPrefixString);
	
			OpenSOAPXMLAttrSetNamespaceMB(addedAttr[i],
										  attrNsURLString,
										  attrNsPrefixString);
			OpenSOAPStringClear(attrNsURL);
			OpenSOAPStringClear(attrNsPrefix);
			free(attrNsURLString);
			free(attrNsPrefixString);
		}
	
		OpenSOAPStringClear(attrName);
		OpenSOAPStringClear(attrValue);
		free(attrNameString);
    }
    free(addedAttr);

    /* Add Value */
    if(OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetValueMB(elm,
												   "string",
												   &value))
       && value){
        OpenSOAPXMLElmSetValueMB(dupElm,"string",&value);
        OpenSOAPStringClear(value);
    }

    /* Add Children */
    elmNo = 0;
    for(child = NULL;
		OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetNextChild(elm,&child))
			&& child;){
		elmNo++;
    }
    elmChild = (OpenSOAPXMLElmPtr*)malloc(elmNo * sizeof(OpenSOAPXMLElmPtr));
    i = 0;
    for (child = NULL;
		 OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetNextChild(elm, &child))
			 && child
			 && i<elmNo;
		 i++) {
        
        elmChild[i] = NULL;

        OpenSOAPXMLElmGetNameString(child,&childName);
		OpenSOAPStringGetLengthMB(childName, &len);
		childNameString = malloc(len + 1);
		OpenSOAPStringGetStringMB(childName, &len, childNameString);
	
		OpenSOAPXMLElmAddChildMB(dupElm,
								 childNameString,
								 &elmChild[i]);
	
		XMLElmDupricate(child,elmChild[i]);

    }
    free(elmChild);

    return error;
}

static
int 
CriateEachRequestEnvelope(TransactionParameters *trPrm,
						  int trNo) {
    int i;
    int errCode = OPENSOAP_NO_ERROR;
    size_t len = 0;
    OpenSOAPXMLElmPtr headerBlock = NULL;
    OpenSOAPXMLElmPtr bodyBlock = NULL;
    OpenSOAPStringPtr blockName  = NULL;
    char *blockNameString = NULL;
    OpenSOAPBlockPtr requestHeaderBlock = NULL;
    OpenSOAPBlockPtr requestBodyBlock = NULL;
	static const char envelopeNameFormat[] = "request[%d]";
    char envelopeName[sizeof(envelopeNameFormat) + INT_DIGITS8 - 2];

    for (i = 0; i < trNo; ++i) {
        errCode = OpenSOAPEnvelopeCreateMB("1.1", 
										   NULL,
										   &trPrm[i].requestEnvelope);
		if (OPENSOAP_FAILED(errCode)) {
			return errCode;
		}

		for(headerBlock = NULL;
			OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetNextChild(
								   trPrm[i].requestHeaderBlock,
								   &headerBlock))
				&& headerBlock
				&& trPrm[i].requestHeaderBlock;){

			/* Create Header Block */
			OpenSOAPXMLElmGetNameString(headerBlock,&blockName);

			OpenSOAPStringGetLengthMB(blockName, &len);
			blockNameString = malloc(len + 1);
			OpenSOAPStringGetStringMB(blockName, &len, blockNameString);

			OpenSOAPEnvelopeAddHeaderBlockMB(trPrm[i].requestEnvelope,
											 blockNameString,
											 &requestHeaderBlock);
			XMLElmDupricate(headerBlock,
							(OpenSOAPXMLElmPtr)requestHeaderBlock);
		}

		for (bodyBlock = NULL;
			 OPENSOAP_SUCCEEDED(OpenSOAPXMLElmGetNextChild(
									trPrm[i].requestBodyBlock,
									&bodyBlock))
				 && bodyBlock
				 && trPrm[i].requestBodyBlock;) {

			/* Create Body Block */
			OpenSOAPXMLElmGetNameString(bodyBlock,&blockName);

			OpenSOAPStringGetLengthMB(blockName, &len);
			blockNameString = malloc(len + 1);
			OpenSOAPStringGetStringMB(blockName, &len, blockNameString);

			OpenSOAPEnvelopeAddBodyBlockMB(trPrm[i].requestEnvelope,
										   blockNameString,
										   &requestBodyBlock);
			XMLElmDupricate(bodyBlock,
							(OpenSOAPXMLElmPtr)requestBodyBlock);      
		}
		free(blockNameString);

		sprintf(envelopeName, envelopeNameFormat, i);
		OutputEnvelope(trPrm[i].requestEnvelope,
					   "UTF-8",
					   envelopeName,
					   LOG_STREAM);
    }
	
    return errCode;
}

static
int
SequentialInvokeEachRequestEnvelope(TransactionParameters *trPrm,
									int trNo) {
	int ret = 0;
    int                    i = 0;
    char                   *endpointString = NULL;
    OpenSOAPXMLElmPtr      transactionResultElm = NULL;
    OpenSOAPStringPtr      transactionResultStr = NULL;
    int                    successCmpRslt = 1;
	TransactionParameters *trPrmItr = trPrm;

	for (i = 0; i != trNo; ++i, ++trPrmItr) {
		static const char envelopeNameFormat[] = "response[%d]";
		static const char transactionNoFormat[] = "transaction[%d]";
		char envelopeName[sizeof(envelopeNameFormat) + INT_DIGITS8 - 2];
		char transactionNo[sizeof(transactionNoFormat) + INT_DIGITS8 - 2];
		OpenSOAPTransportPtr   transport = NULL;
		OpenSOAPBlockPtr       firstBodyBlock = NULL;
		int error = OPENSOAP_NO_ERROR;

		free(endpointString);
		OpenSOAPStringGetStringMBWithAllocator(trPrmItr->endpoint,
											   NULL,
											   NULL,
											   &endpointString);
		fprintf(LOG_STREAM, "\nendpoint[%d]:%s\n", i, endpointString);

		OpenSOAPTransportCreate(&transport);
		OpenSOAPTransportSetURL(transport,
								endpointString);
		OpenSOAPTransportInvoke(transport,
								trPrmItr->requestEnvelope, 
								&trPrmItr->responseEnvelope);
		OpenSOAPTransportRelease(transport);
	
		sprintf(envelopeName, envelopeNameFormat, i);
		OutputEnvelope(trPrmItr->responseEnvelope,
					   "UTF-8",
					   envelopeName,
					   LOG_STREAM);

		/*Judge SUCCESS */
		error = OpenSOAPEnvelopeGetNextBodyBlock(trPrmItr->responseEnvelope, 
												 &firstBodyBlock);
		if (OPENSOAP_FAILED(error) || !firstBodyBlock) {
			fprintf(LOG_STREAM, "transaction[%d] envelope failed\n", i);
			ret = i + 1;
			break;
		}
		
		/* */
		error = OpenSOAPBlockGetChildMB(firstBodyBlock,
										TRANSACTION_RESULT,
										&transactionResultElm);
		if (OPENSOAP_FAILED(error) || !transactionResultElm) {
			fprintf(LOG_STREAM, "transaction[%d] body failed\n", i);
			ret = i + 1;
			break;
		}

		/* */
		OpenSOAPXMLElmGetAttributeMB(transactionResultElm,
									 TRANSACTION_ID_ATTRIBUTE,
									 &trPrmItr->trResponseIDAttr);
		/* added 3lines 2002/07/31 */
		OpenSOAPXMLElmGetAttributeMB(transactionResultElm,
									 TRANSACTION_ACTION_NS_ATTRIBUTE,
									 &trPrmItr->trResponseNsAttr);

		OpenSOAPStringCreate(&transactionResultStr);
		OpenSOAPXMLElmGetValueMB(transactionResultElm,
								 "string",
								 &transactionResultStr);
		sprintf(transactionNo, transactionNoFormat, i);
		CheckStr(transactionNo,transactionResultStr);
		OpenSOAPStringCompareMB(transactionResultStr,
								TRANSACTION_SUCCESS,
								&successCmpRslt);
		OpenSOAPStringRelease(transactionResultStr);
		if (successCmpRslt != 0) {
			fprintf(LOG_STREAM, "transaction[%d] element failed\n", i);
			ret = i + 1;
			break;
		}
	}

	free(endpointString);
	
    return ret;
}


static
int
TransactionActionInvoke(TransactionParameters *trPrm,
						int trNo,
						int successCode)
{
  
    int                    i = 0;
    size_t                 len = 0;
    int                    errCode = 0;
    int                    transactionServices = 0;
    OpenSOAPStringPtr      transactionAction = NULL;
    char                   envelopeName[50];
    char *                 endpointString;
    OpenSOAPBlockPtr       actionBlock = NULL;
    OpenSOAPStringPtr      transactionID = NULL;
    OpenSOAPXMLAttrPtr     trIDAttr = NULL;
    /* added 3lines 2002/07/31 */
    OpenSOAPStringPtr      transactionActionNs = NULL;
    OpenSOAPXMLAttrPtr     trNsAttr = NULL;
    char*                  transactionActionNsStr = NULL;


    if (successCode == 0) {
        transactionServices = trNo;
		OpenSOAPStringCreateWithMB(TRANSACTION_COMMIT, &transactionAction);
    }
    else {
        transactionServices = successCode - 1;
		OpenSOAPStringCreateWithMB(TRANSACTION_ROLLBACK, &transactionAction);
    }
	
    for (i = 0; i < transactionServices; ++i) {
		OpenSOAPTransportPtr   transport = NULL;

        /* Make Envelope of Transaction Action (COMMIT or ROLLBACK) */
        errCode = OpenSOAPEnvelopeCreateMB("1.1", 
										   NULL,
										   &trPrm[i].actionRequestEnvelope);
		if(OPENSOAP_FAILED(errCode)){
			return errCode;
		}
		OpenSOAPEnvelopeAddBodyBlockMB(trPrm[i].actionRequestEnvelope,
									   TRANSACTION_ACTION_METHOD,
									   &actionBlock);

		/* Add Transaction ID Attribute */
		trIDAttr = NULL;
		errCode = OpenSOAPXMLAttrGetValueMB(trPrm[i].trResponseIDAttr,
											"string",
											&transactionID);
		if(OPENSOAP_SUCCEEDED(errCode)){
			OpenSOAPBlockAddAttributeMB(actionBlock,
										TRANSACTION_ID_ATTRIBUTE,
										"string",
										&transactionID,
										&trIDAttr);
		}
		else return errCode;

		/* added 15lines 2002/07/31 */
		/* Add TransactionAction Namespace Attribute */
		errCode = OpenSOAPXMLAttrGetValueMB(trPrm[i].trResponseNsAttr,
											"string",
											&transactionActionNs);
		if(OPENSOAP_SUCCEEDED(errCode)){
			errCode = OpenSOAPStringGetStringMBWithAllocator(transactionActionNs,
															 NULL,
															 &len,
															 &transactionActionNsStr);
			/* debug */
#ifdef DEBUG
			if(OPENSOAP_SUCCEEDED(errCode)){
				fprintf(stderr, "#### TRANS_ACT_NS=[%s]:(%d) ####\n",
						transactionActionNsStr, len);
			}
#endif
		}
	
		/* Add Namespace */
		/* added 5lines 2002/07/31 */
		if (transactionActionNs) {
#ifdef DEBUG
			fprintf(stderr, "######### transActionNs OK!! #######\n");
#endif
			OpenSOAPBlockSetNamespaceMB(actionBlock,
										transactionActionNsStr,
										TRANSACTION_PREFIX);
		}
		else {
#ifdef DEBUG
			fprintf(stderr, "######### transActionNs NG!! #######\n");
#endif
			OpenSOAPBlockSetNamespaceMB(actionBlock,
										TRANSACTION_NAMESPACE,
										TRANSACTION_PREFIX);
		}

		/* Add Action Value */	
		OpenSOAPBlockSetValueMB(actionBlock,
								"string",
								&transactionAction);

		sprintf(envelopeName, "action request[%d]",i);
		OutputEnvelope(trPrm[i].actionRequestEnvelope,
					   "UTF-8",
					   envelopeName,
					   LOG_STREAM);

		/* Invoke Envelope */
		OpenSOAPStringGetLengthMB(trPrm[i].endpoint, &len);
		endpointString = malloc(len + 1);
		OpenSOAPStringGetStringMB(trPrm[i].endpoint, &len, endpointString);

		fprintf(LOG_STREAM, "\nendpoint[%d]:%s\n",i,endpointString);

        OpenSOAPTransportCreate(&transport);
		OpenSOAPTransportSetURL(transport,
								endpointString);
		OpenSOAPTransportInvoke(transport,
								trPrm[i].actionRequestEnvelope, 
								&trPrm[i].actionResponseEnvelope);
		OpenSOAPTransportRelease(transport);
	
		sprintf(envelopeName, "action response[%d]",i);
		OutputEnvelope(trPrm[i].actionResponseEnvelope,
					   "UTF-8",
					   envelopeName,
					   LOG_STREAM);

    }
	
    return 0;
}


static
int
CreateResponseEnvelope(TransactionParameters *trPrm,
					   int trNo,
					   int successCode,
					   OpenSOAPEnvelopePtr *response) {
    int                    i;
    int                    errCode;
    size_t                 len = 0;
    OpenSOAPStringPtr      transactionAction = NULL;
    OpenSOAPBlockPtr       responseBlock = NULL;
    OpenSOAPXMLElmPtr      resultElm = NULL;

    OpenSOAPStringPtr      trIDAttrValue  = NULL;
    OpenSOAPXMLAttrPtr     trIDAttr = NULL;

    OpenSOAPBlockPtr       headerBlocks = NULL;
    OpenSOAPBlockPtr       bodyBlocks = NULL;
    OpenSOAPBlockPtr       actionResponseBodyBlocks = NULL;
    OpenSOAPXMLElmPtr      addedHeaderBlocks = NULL;
    OpenSOAPXMLElmPtr      addedBodyBlocks = NULL;
    OpenSOAPXMLElmPtr      addedActionResponseBodyBlocks = NULL;

    OpenSOAPStringPtr      blockName  = NULL;
    char *                 blockNameString = NULL;


    if(successCode == 0){
		OpenSOAPStringCreateWithMB(TRANSACTION_COMMIT, &transactionAction);
    }
    else{
		OpenSOAPStringCreateWithMB(TRANSACTION_ROLLBACK, &transactionAction);
    }

    errCode = OpenSOAPEnvelopeCreateMB("1.1",  NULL, response);
    if(OPENSOAP_FAILED(errCode)){
		return errCode;
    }
    
    /* Add Final Result Response Block */
    OpenSOAPEnvelopeAddBodyBlockMB(*response,
								   TRANSACTION_RESPONSE_METHOD,
								   &responseBlock);
    OpenSOAPBlockSetNamespaceMB(responseBlock,
								TRANSACTION_NAMESPACE,
								TRANSACTION_PREFIX);
    OpenSOAPBlockAddChildMB(responseBlock,
							TRANSACTION_RESPONSE_RESULT,
							&resultElm);
    OpenSOAPXMLElmSetValueMB(resultElm,
							 "string",
							 &transactionAction);

    for (i = 0; i < trNo; ++i) {
		OpenSOAPBlockPtr responseHeaderBlock     = NULL;
		OpenSOAPBlockPtr responseBodyBlock       = NULL;
		OpenSOAPBlockPtr actionResponseBodyBlock = NULL;
		
		OpenSOAPXMLAttrGetValueMB(trPrm[i].trRequestIDAttr,
								  "string",
								  &trIDAttrValue);

        /* Create Header Block */
		headerBlocks = NULL;
		if(OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextHeaderBlock(
								  trPrm[i].responseEnvelope,
								  &headerBlocks))
		   && headerBlocks
		   && trPrm[i].responseEnvelope){
	  
			OpenSOAPEnvelopeAddBodyBlockMB(*response,
										   TRANSACTION_HEADER_BLOCK,
										   &responseHeaderBlock);
			OpenSOAPBlockSetNamespaceMB(responseHeaderBlock,
										TRANSACTION_NAMESPACE,
										TRANSACTION_PREFIX);
			OpenSOAPBlockAddAttributeMB(responseHeaderBlock,
										TRANSACTION_REQUEST_ID_ATTRIBUTE,
										"string",
										&trIDAttrValue,
										&trIDAttr);
			for(headerBlocks = NULL;
				OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextHeaderBlock(
									   trPrm[i].responseEnvelope,
									   &headerBlocks))
					&& headerBlocks;){

				addedHeaderBlocks = NULL;
				OpenSOAPBlockGetName(headerBlocks,&blockName);

				OpenSOAPStringGetLengthMB(blockName, &len);
				blockNameString = malloc(len + 1);
				OpenSOAPStringGetStringMB(blockName, &len, blockNameString);

				addedHeaderBlocks = NULL;
				OpenSOAPBlockAddChildMB(responseHeaderBlock,
										blockNameString,
										&addedHeaderBlocks);

				XMLElmDupricate((OpenSOAPXMLElmPtr)headerBlocks,
								addedHeaderBlocks);
			}
		}

        /* Create Body Block */
		bodyBlocks = NULL;
		if(OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextBodyBlock(
								  trPrm[i].responseEnvelope,
								  &bodyBlocks))
		   && bodyBlocks
		   && trPrm[i].responseEnvelope){

			OpenSOAPEnvelopeAddBodyBlockMB(*response,
										   TRANSACTION_BODY_BLOCK,
										   &responseBodyBlock);
			OpenSOAPBlockSetNamespaceMB(responseBodyBlock,
										TRANSACTION_NAMESPACE,
										TRANSACTION_PREFIX);
			OpenSOAPBlockAddAttributeMB(responseBodyBlock,
										TRANSACTION_REQUEST_ID_ATTRIBUTE,
										"string",
										&trIDAttrValue,
										&trIDAttr);
			for(bodyBlocks = NULL;
				OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextBodyBlock(
									   trPrm[i].responseEnvelope,
									   &bodyBlocks))
					&& bodyBlocks;){

				addedBodyBlocks = NULL;
				OpenSOAPBlockGetName(bodyBlocks,&blockName);

				OpenSOAPStringGetLengthMB(blockName, &len);
				blockNameString = malloc(len + 1);
				OpenSOAPStringGetStringMB(blockName, &len, blockNameString);

				addedBodyBlocks = NULL;
				OpenSOAPBlockAddChildMB(responseBodyBlock,
										blockNameString,
										&addedBodyBlocks);

				XMLElmDupricate((OpenSOAPXMLElmPtr)bodyBlocks,
								addedBodyBlocks);
			}
		}

        /* Create Action Response Block */
		actionResponseBodyBlocks = NULL;
		if(OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextBodyBlock(
								  trPrm[i].actionResponseEnvelope,
								  &actionResponseBodyBlocks))
		   && actionResponseBodyBlocks
		   && trPrm[i].actionResponseEnvelope){

			OpenSOAPEnvelopeAddBodyBlockMB(*response,
										   TRANSACTION_ACTION_RESPONSE_BODY_BLOCK,
										   &actionResponseBodyBlock);
			OpenSOAPBlockSetNamespaceMB(actionResponseBodyBlock,
										TRANSACTION_NAMESPACE,
										TRANSACTION_PREFIX);
			OpenSOAPBlockAddAttributeMB(actionResponseBodyBlock,
										TRANSACTION_REQUEST_ID_ATTRIBUTE,
										"string",
										&trIDAttrValue,
										&trIDAttr);
			for(actionResponseBodyBlocks = NULL;
				OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetNextBodyBlock(
									   trPrm[i].actionResponseEnvelope,
									   &actionResponseBodyBlocks))
					&& actionResponseBodyBlocks;){

				addedActionResponseBodyBlocks = NULL;
				OpenSOAPBlockGetName(actionResponseBodyBlocks,&blockName);

				OpenSOAPStringGetLengthMB(blockName, &len);
				blockNameString = malloc(len + 1);
				OpenSOAPStringGetStringMB(blockName, &len, blockNameString);

				addedActionResponseBodyBlocks = NULL;
				OpenSOAPBlockAddChildMB(actionResponseBodyBlock,
										blockNameString,
										&addedActionResponseBodyBlocks);

				XMLElmDupricate((OpenSOAPXMLElmPtr)actionResponseBodyBlocks,
								addedActionResponseBodyBlocks);
			}
		}
		free(blockNameString);

    }
    /* OutputEnvelope(*response,"UTF-8", "response", LOG_STREAM); */

    return errCode;
}


static
int
TransactionServiceFunc(OpenSOAPEnvelopePtr request,
					   OpenSOAPEnvelopePtr *response,
					   void *opt) {
	
    static char FuncName[] = "TransactionServiceFunc";
    int error_code = OPENSOAP_NO_ERROR;

    OpenSOAPBlockPtr transactionProcess = NULL;

    TransactionParameters *trPrm;
    int trNo;
    int trSucceed;

    /* parse request message -process part-*/

    error_code = OpenSOAPEnvelopeGetBodyBlockMB(request,
												TRANSACTION_CONTROL_METHOD,
												&transactionProcess);
    ERROR_CHECK(error_code, FuncName, "get control body block");

    trNo = CountProcessNo(transactionProcess);
    trPrm = (TransactionParameters*)malloc(trNo *
										   sizeof(TransactionParameters));
    error_code = InitializeTransactionProcessParameter(trPrm, trNo);
    ERROR_CHECK(error_code, FuncName, "initialize transaction parameters");

    error_code = CriateTransactionProcessParameters(trPrm, request, trNo);
    ERROR_CHECK(error_code, FuncName, "create transaction parameters");

    error_code = GetTransactionBlock(trPrm, request, trNo);
    ERROR_CHECK(error_code, FuncName, "get transaction blocks");

    error_code = CheckTransactionParameter(trPrm,trNo);
    ERROR_CHECK(error_code, FuncName, "check transaction parameters");

    error_code = CriateEachRequestEnvelope(trPrm, trNo);
    ERROR_CHECK(error_code, FuncName, "create each request envelope");

    trSucceed = SequentialInvokeEachRequestEnvelope(trPrm, trNo);
    fprintf(LOG_STREAM, "Transaction:: Success Code = %d\n", trSucceed);

    error_code = TransactionActionInvoke(trPrm, trNo, trSucceed);
    ERROR_CHECK(error_code, FuncName, "invoke transaction actions");

    error_code = CreateResponseEnvelope(trPrm, trNo, trSucceed, response);
    ERROR_CHECK(error_code, FuncName, "create response envelope");

    free(trPrm);
	
    return error_code;
}


int
main(void) {
	
    static char FuncName[] = "main";
    int error_code = OPENSOAP_NO_ERROR;
    OpenSOAPServicePtr transaction_service = NULL;
	
#if !defined(CONNECT_TYPE_CGI)
    time_t logTime;
	if ((fp = fopen(TRANSACTION_LOG_FILE, "a")) != NULL) {
		time(&logTime);
		fprintf(fp, "\n\nstart logging : %s",ctime(&logTime));
	} else {
		fp = stderr;
	}
#endif /* CONNECT_TYPE_CGI */

    error_code = OpenSOAPInitialize(NULL);
    ERROR_RETURN(error_code, FuncName, "initialize service");;
	
    error_code = OpenSOAPServiceCreateMB(&transaction_service,
										 "TransactionService",
										 CONNECT_TYPE,
										 0);

    if (OPENSOAP_SUCCEEDED(error_code)) {
        error_code = OpenSOAPServiceRegisterMB(transaction_service,
											   TRANSACTION_CONTROL_METHOD,
											   TransactionServiceFunc,
											   NULL);
		ERROR_CHECK(error_code, FuncName, "register method");

		if (OPENSOAP_SUCCEEDED(error_code)) {
			error_code = OpenSOAPServiceRun(transaction_service);
			ERROR_CHECK(error_code, FuncName, "run service");
		}

		OpenSOAPServiceRelease(transaction_service);
		ERROR_CHECK(error_code, FuncName, "release");

    }
    else {
        ERROR_MSG(error_code, FuncName, "create");
    }

    OpenSOAPUltimate();

#if !defined(CONNECT_TYPE_CGI)
    fclose(fp);
#endif /* CONNECT_TYPE_CGI */

    return error_code;
}
