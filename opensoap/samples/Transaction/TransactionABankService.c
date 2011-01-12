/*-----------------------------------------------------------------------------
 * $RCSfile: TransactionABankService.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(CONNECT_TYPE_CGI)
# define CONNECT_TYPE "cgi"
# define LOG_STREAM stderr
#else
# define CONNECT_TYPE "stdio"
FILE *fp;
# define LOG_STREAM fp
#endif /* CONNECT_TYPE_CGI */

static const char PAYMENT_METHOD[] = "PaymentRequest";
static const char DEPOSIT_METHOD[] = "DepositRequest";
static const char PAYMENT_RESPONSE_METHOD[] = "PaymentResponse";
static const char DEPOSIT_RESPONSE_METHOD[] = "DepositResponse";
static const char TRANSACTION_ID_ATTRIBUTE[] = "transactionID";
static const char TRANSACTION_NAMESPACE[] = "http://services.opensoap.jp/Transaction/";
static const char TRANSACTION_PREFIX[] = "t";
static const char SERVICE_NAMESPACE[] = "http://services.opensoap.jp/samples/a_bank_transfer/";
static const char SERVICE_PREFIX[] = "p";
static const char TRANSACTION_RESULT[] = "TransactionResult";
static const char SERVICE_SUCCESS[] = "SUCCESS";
static const char SERVICE_FAILED[] = "FAILED";
static const char SERVICE_COMMENT[] = "Comment";

static const char TRANSACTION_ACTION_METHOD[] = "TransactionAction";
static const char TRANSACTION_ACTION_RESPONSE_METHOD[] = "TransactionActionResponse";
static const char TRANSACTION_COMMIT[] = "COMMIT";
static const char TRANSACTION_ROLLBACK[] = "ROLLBACK";

static int maxMembers = 4;
char members[][20] = {"okuno1234", "namaji2345", "bando3456", "chaya4567"};

void 
CheckStr(char *preStr, OpenSOAPStringPtr strPtr)
{
  char *string;
  size_t len = 0;
  OpenSOAPStringGetLengthMB(strPtr, &len);
  string = malloc(len + 1);
  OpenSOAPStringGetStringMB(strPtr, &len, string);
  fprintf(LOG_STREAM,"%s: %s\n",preStr,string);
  free(string);
}

static
int
PaymentServiceFunc(OpenSOAPEnvelopePtr request,
		   OpenSOAPEnvelopePtr *response,
		   void *opt) {
	
    int i = 0;
    OpenSOAPBlockPtr requestBody = NULL;
    OpenSOAPBlockPtr responseBody = NULL;
    
    OpenSOAPStringPtr account = NULL;
    OpenSOAPStringPtr amount = NULL;
    OpenSOAPStringPtr toBank = NULL;
    size_t len1 = 0;
    size_t len2 = 0;
    size_t len3 = 0;
    char *accountString = NULL;
    char *amountString = NULL;
    char *toBankString = NULL;
    char *commentString = NULL;
    char *trIDString = NULL;
	char timeString[32];

    OpenSOAPXMLElmPtr resultElm = NULL;
    OpenSOAPStringPtr serviceResult = NULL;
    OpenSOAPStringPtr transactionID = NULL;
    OpenSOAPXMLAttrPtr transactionIDAttr = NULL;
    OpenSOAPXMLElmPtr commentElm = NULL;
    OpenSOAPStringPtr comment = NULL;


    /* parse request message */

    if(OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetBodyBlockMB(request, 
							 PAYMENT_METHOD, 
							 &requestBody))
       && requestBody){

        OpenSOAPBlockGetChildValueMB(requestBody, 
				     "account", 
				     "string", 
				     &account);
	OpenSOAPBlockGetChildValueMB(requestBody, 
				     "amount", 
				     "string", 
				     &amount);
	OpenSOAPBlockGetChildValueMB(requestBody, 
				     "to", 
				     "string", 
				     &toBank);

	OpenSOAPStringGetLengthMB(account, &len1);
	accountString = malloc(len1 + 1);
	OpenSOAPStringGetStringMB(account, &len1, accountString);

	OpenSOAPStringGetLengthMB(amount, &len2);
	amountString = malloc(len2 + 1);
	OpenSOAPStringGetStringMB(amount, &len2, amountString);

	OpenSOAPStringGetLengthMB(toBank, &len3);
	toBankString = malloc(len3 + 1);
	OpenSOAPStringGetStringMB(toBank, &len3, toBankString);
	
	while(strcmp(accountString,members[i])!=0 && i < maxMembers) i++;
	if(i < maxMembers){
	    OpenSOAPStringCreateWithMB(SERVICE_SUCCESS, &serviceResult);

	    commentString = malloc(len1 + len2 + len3 + 20);
	    sprintf(commentString, 
		    "%s sent $%s to %s", 
		    accountString, amountString, toBankString);
	    OpenSOAPStringCreateWithMB(commentString, &comment);
	}
	else if(i == maxMembers){
	    OpenSOAPStringCreateWithMB(SERVICE_FAILED, &serviceResult);

	    commentString = malloc(len1 + 50);
	    sprintf(commentString, 
		    "Service Failed invalid account: %s", 
		    accountString);
	    OpenSOAPStringCreateWithMB(commentString, &comment);
	}
	else{
	    return 1;
	}

	sprintf(timeString, "%d", (int)time(NULL));
	trIDString = malloc(strlen(accountString) + strlen(timeString) + 3);
	sprintf(trIDString, "pt%s%s",accountString,timeString);
	OpenSOAPStringCreateWithMB(trIDString, &transactionID);

	/* make response */
	
	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response,
				       PAYMENT_RESPONSE_METHOD,
				       &responseBody);
	OpenSOAPBlockSetNamespaceMB(responseBody,
				    SERVICE_NAMESPACE,
				    SERVICE_PREFIX);
	
	OpenSOAPBlockAddChildMB(responseBody, 
				TRANSACTION_RESULT,
				&resultElm);
	OpenSOAPXMLElmAddAttributeMB(resultElm,
				     TRANSACTION_ID_ATTRIBUTE,
				     "string",
				     &transactionID,
				     &transactionIDAttr);	
	OpenSOAPXMLElmSetNamespaceMB(resultElm,
				     TRANSACTION_NAMESPACE,
				     TRANSACTION_PREFIX);
	OpenSOAPXMLElmSetValueMB(resultElm,
				 "string",
				 &serviceResult);
	
	OpenSOAPBlockAddChildMB(responseBody,
				SERVICE_COMMENT,
				&commentElm);
	OpenSOAPXMLElmSetValueMB(commentElm,
				 "string",
				 &comment);

	free(accountString);
	free(amountString);
	free(toBankString);
	free(commentString);
    }
    return 0;
}

static
int
DepositServiceFunc(OpenSOAPEnvelopePtr request,
		   OpenSOAPEnvelopePtr *response,
		   void *opt) {

    int i=0;
    OpenSOAPBlockPtr requestBody = NULL;
    OpenSOAPBlockPtr responseBody = NULL;
    
    OpenSOAPStringPtr account = NULL;
    OpenSOAPStringPtr amount = NULL;
    OpenSOAPStringPtr fromBank = NULL;
    size_t len1 = 0;
    size_t len2 = 0;
    size_t len3 = 0;
    char *accountString = NULL;
    char *amountString = NULL;
    char *fromBankString = NULL;
    char *commentString = NULL;
    char *trIDString = NULL;
	char timeString[32];

    OpenSOAPXMLElmPtr resultElm = NULL;
    OpenSOAPStringPtr serviceResult = NULL;
    OpenSOAPStringPtr transactionID = NULL;
    OpenSOAPXMLAttrPtr transactionIDAttr = NULL;
    OpenSOAPXMLElmPtr commentElm = NULL;
    OpenSOAPStringPtr comment = NULL;


    /* parse request message */

    if(OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetBodyBlockMB(request, 
							 DEPOSIT_METHOD, 
							 &requestBody))
       && requestBody){

        OpenSOAPBlockGetChildValueMB(requestBody, 
				     "account", 
				     "string", 
				     &account);
	OpenSOAPBlockGetChildValueMB(requestBody, 
				     "amount", 
				     "string", 
				     &amount);
	OpenSOAPBlockGetChildValueMB(requestBody, 
				     "from",
				     "string", 
				     &fromBank);

	OpenSOAPStringGetLengthMB(account, &len1);
	accountString = malloc(len1 + 1);
	OpenSOAPStringGetStringMB(account, &len1, accountString);

	OpenSOAPStringGetLengthMB(amount, &len2);
	amountString = malloc(len2 + 1);
	OpenSOAPStringGetStringMB(amount, &len2, amountString);

	OpenSOAPStringGetLengthMB(fromBank, &len3);
	fromBankString = malloc(len3 + 1);
	OpenSOAPStringGetStringMB(fromBank, &len3, fromBankString);

	while(strcmp(accountString,members[i])!=0 && i < maxMembers) i++;
	if(i < maxMembers){
	    OpenSOAPStringCreateWithMB(SERVICE_SUCCESS, &serviceResult);

	    commentString = malloc(len1 + len2 + len3 + 20);
	    sprintf(commentString, 
		    "%s received $%s from %s", 
		    accountString, amountString, fromBankString);
	    OpenSOAPStringCreateWithMB(commentString, &comment);
	}
	else if(i == maxMembers){
	    OpenSOAPStringCreateWithMB(SERVICE_FAILED, &serviceResult);

	    commentString = malloc(len1 + 50);
	    sprintf(commentString, 
		    "Service Failed invalid account: %s", 
		    accountString);
	    OpenSOAPStringCreateWithMB(commentString, &comment);
	}
	else{
	    return 1;
	}

	sprintf(timeString, "%d", (int)time(NULL));
	trIDString = malloc(strlen(accountString) + strlen(timeString) + 3);
	sprintf(trIDString, "dt%s%s",accountString,timeString);
	OpenSOAPStringCreateWithMB(trIDString, &transactionID);

	/* make response */

	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response,
				       DEPOSIT_RESPONSE_METHOD,
				       &responseBody);
	OpenSOAPBlockSetNamespaceMB(responseBody,
				    SERVICE_NAMESPACE,
				    SERVICE_PREFIX);
	
	OpenSOAPBlockAddChildMB(responseBody, 
				TRANSACTION_RESULT,
				&resultElm);
	OpenSOAPXMLElmAddAttributeMB(resultElm,
				     TRANSACTION_ID_ATTRIBUTE,
				     "string",
				     &transactionID,
				     &transactionIDAttr);	
	OpenSOAPXMLElmSetNamespaceMB(resultElm,
				     TRANSACTION_NAMESPACE,
				     TRANSACTION_PREFIX);
	OpenSOAPXMLElmSetValueMB(resultElm,
				 "string",
				 &serviceResult);
	
	OpenSOAPBlockAddChildMB(responseBody,
				SERVICE_COMMENT,
				&commentElm);
	OpenSOAPXMLElmSetValueMB(commentElm,
				 "string",
				 &comment);

	free(accountString);
	free(amountString);
	free(fromBankString);
	free(commentString);
    }
    return 0;
}

static
int
BankTransactionActionFunc(OpenSOAPEnvelopePtr request,
			  OpenSOAPEnvelopePtr *response,
			  void *opt) {

    OpenSOAPBlockPtr requestBody = NULL;
    OpenSOAPBlockPtr responseBody = NULL;

    OpenSOAPStringPtr transactionID = NULL;
    OpenSOAPXMLAttrPtr trRequestIDAttr = NULL;
    OpenSOAPXMLAttrPtr trResponseIDAttr = NULL;

    OpenSOAPStringPtr transactionAction = NULL;
    int actionCmpRslt = 1;
    OpenSOAPStringPtr actionResponseComment = NULL;

    if(OPENSOAP_SUCCEEDED(OpenSOAPEnvelopeGetBodyBlockMB(
			      request, 
			      TRANSACTION_ACTION_METHOD, 
			      &requestBody))
       && requestBody){

	OpenSOAPEnvelopeCreateMB("1.1", NULL, response);
	OpenSOAPEnvelopeAddBodyBlockMB(*response,
				       TRANSACTION_ACTION_RESPONSE_METHOD,
				       &responseBody);
	OpenSOAPBlockSetNamespaceMB(responseBody,
				    TRANSACTION_NAMESPACE,
				    TRANSACTION_PREFIX);

	OpenSOAPBlockGetAttributeMB(requestBody,
				    TRANSACTION_ID_ATTRIBUTE,
				    &trRequestIDAttr);
	OpenSOAPXMLAttrGetValueMB(trRequestIDAttr,
				  "string",
				  &transactionID);
	CheckStr("transactionID",transactionID);
	OpenSOAPBlockAddAttributeMB(responseBody,
				    TRANSACTION_ID_ATTRIBUTE,
				    "string",
				    &transactionID,
				    &trResponseIDAttr);	

        if(OPENSOAP_SUCCEEDED(OpenSOAPBlockGetValueMB(requestBody,
						      "string",
						      &transactionAction))
	   && transactionAction){
	    if(OPENSOAP_SUCCEEDED(OpenSOAPStringCompareMB(
				      transactionAction,
				      TRANSACTION_COMMIT,
				      &actionCmpRslt))
	       && actionCmpRslt == 0){
	        OpenSOAPStringCreateWithMB("COMMITED",
					   &actionResponseComment);
	        OpenSOAPBlockSetValueMB(responseBody,
					"string",
					&actionResponseComment);
	    }
	    else if(OPENSOAP_SUCCEEDED(OpenSOAPStringCompareMB(
					   transactionAction,
					   TRANSACTION_ROLLBACK,
					   &actionCmpRslt))
		    && actionCmpRslt == 0){
	        OpenSOAPStringCreateWithMB("ROLLBACKED", 
					   &actionResponseComment);
	        OpenSOAPBlockSetValueMB(responseBody,
					"string",
					&actionResponseComment);
	    }
	}
    }
    return 0;
}

int
main(void) {
	
    OpenSOAPServicePtr a_bank_service = NULL;
	
    int error_code = OpenSOAPInitialize(NULL);
	
    if (OPENSOAP_SUCCEEDED(error_code)) {
        error_code = OpenSOAPServiceCreateMB(&a_bank_service,
					     "A_Bank_Service_for_Transaction",
					     CONNECT_TYPE,
					     0);
	
	if (OPENSOAP_SUCCEEDED(error_code)) {
	  error_code = OpenSOAPServiceRegisterMB(a_bank_service,
						   PAYMENT_METHOD,
						   PaymentServiceFunc,
						   NULL);
	  error_code = OpenSOAPServiceRegisterMB(a_bank_service,
						   DEPOSIT_METHOD,
						   DepositServiceFunc,
						   NULL);
	  error_code = OpenSOAPServiceRegisterMB(a_bank_service,
						 TRANSACTION_ACTION_METHOD,
						 BankTransactionActionFunc,
						 NULL);
	}

	if (OPENSOAP_SUCCEEDED(error_code)) {
	    error_code = OpenSOAPServiceRun(a_bank_service);
	}
	
	OpenSOAPServiceRelease(a_bank_service);
	
	OpenSOAPUltimate();
    }

    return error_code;
	
}
