/*-----------------------------------------------------------------------------
 * $RCSfile: TransactionApp.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "TransactionClient.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/StringHash.h>

#include <stdio.h>
#include <string.h>

#define READ_BUF_SZ 256

static const char TRANSFER_CONF_FILE[] = "/var/tmp/Transfer.conf";

static const char DEFAULT_ENDPOINT[]
= "http://localhost/cgi-bin/soapInterface.cgi";

static const char DEFAULT_TMP_FILE[] = "/var/tmp/tmp_soap_msg.xml";

/* decode string with url-encoding */
void
DecodeURLEncodedString(char **str) {
	int i;
	int len = strlen(*str);
	char *str2 = malloc(len + 1);
	
	strcpy(str2, "");

	for (i = 0; i < len; i++) {
		char *buf = malloc(len + 1);
		
		strcpy(buf, *str + i);
		
		if (strncmp(buf, "%3A", 3) == 0) {
			strcat(str2, ":");
			i = i + 2;
		}
		else if (strncmp(buf, "%2F", 3) == 0) {
			strcat(str2, "/");
			i = i + 2;
		}
		else if (strncmp(buf, "%7E", 3) == 0) {
			strcat(str2, "~");
			i = i + 2;
		}
		else {
			strncat(str2, buf, 1);
		}
		
		free(buf);
	}
	
	strcpy(*str, str2);
	free(str2);
}

/* tokenize input prameters of html document */
void
TokenizeInputParameters(OpenSOAPStringHashPtr str)
{
	int bufLen;
	char *buf;
	char *contentLength;
	char delim[] = "&";
	char *key;
	char *value;
	
	contentLength = getenv("CONTENT_LENGTH");
	
	if (contentLength != NULL) {
		
		bufLen = atoi(contentLength) + 1; /* added "+ 1" to fix problem */
		buf = malloc(bufLen);
		fread(buf, 1, bufLen, stdin);
		
		fprintf(stderr, "buf = %s\n", buf);
		
		key = strtok(buf, delim);
		
		while (key != NULL) {
			char *valStr;
			size_t valLen;
			
			value = strchr(key, '=');
			*value = '\0';
			value++;
			valLen = strlen(value);
			valStr = malloc(valLen + 1);
			strcpy(valStr, value);
			
			fprintf(stderr, "add: key = %s , value = %s\n", key, value);
			
			OpenSOAPStringHashSetValueMB(str, key, valStr);
			
			key = strtok(NULL, delim);
		}
	}
}

/* send html text read from file with given file name */
int
SendHtmlText(const char *filename)
{
  	char buf[1024];
  
	FILE *fp = fopen(filename, "r");
	
	if (fp != NULL) {
 		while (fgets(buf, 1024, fp) != NULL) {
			printf(buf);
		}
		fclose(fp);
	} else {
		printf("<html>Can't open html file: %s</html>", filename);
	}
	return 0;
}

/* make request page */
static
void
MakeRequestPage(char *endpoint,
		char *tmpFile) {
	
	SendHtmlText("./TransactionHtml/CommonHeader.html");
	SendHtmlText("./TransactionHtml/RequestHeader.html");
	
	printf("Endpoint URL: <br>\n");
	printf("<input type=\"text\" size=50 name=\"endpoint\" value = \"%s\">\n",
		   endpoint);
	printf("<br><br>\n");
	
	printf("Temporary file name for acceptance message: <br>\n");
	printf("<input type=\"text\" size=50 name=\"tmp_file\" value = \"%s\">\n",
		   tmpFile);
	printf("<br><br>\n");
	
	SendHtmlText("./TransactionHtml/RequestFooter.html");
	SendHtmlText("./TransactionHtml/CommonFooter.html");
}

/* make accepted page */
static
void
MakeAcceptedPage(char *messageID,
		 char *endpoint,
		 char *tmpFile) {
					
	SendHtmlText("./TransactionHtml/CommonHeader.html");
	SendHtmlText("./TransactionHtml/AcceptedHeader.html");
					
	printf("%s\n", messageID);
	
	printf("<input type=\"hidden\" name=\"endpoint\" value=\"%s\">\n",
		   endpoint);
	printf("<input type=\"hidden\" name=\"tmp_file\" value=\"%s\">\n",
		   tmpFile);
					
	SendHtmlText("./TransactionHtml/AcceptedFooter.html");
	SendHtmlText("./TransactionHtml/CommonFooter.html");
}

/* make result page */
static
void
MakeResultPage(ResultVariables *rsltVars) {
					
	SendHtmlText("./TransactionHtml/CommonHeader.html");
	SendHtmlText("./TransactionHtml/ResultHeader.html");
	
	printf("%s\n</td></tr></table><br><br>", rsltVars->result);
    	printf("<table border=\"0\">");
	printf("<tr><td colspan=\"2\"><hr></td></tr>");
	printf("<th colspan=\"2\" align=\"left\" bgcolor=\"#00ccee\">");
	printf("Result of Bank Service for Payment</th>");
	printf("<tr><td>Transaction ID:</td>");
	printf("<td><b>%s</b></td></tr>",rsltVars->paymentTransactionID);
	printf("<tr><td>Operation Result:</td>");
	printf("<td><b>%s</b></td></tr>",rsltVars->paymentResult);
	printf("<tr><td>Comment:</td>");
	printf("<td><b>%s</b></td></tr>",rsltVars->paymentComment);
	printf("<tr><td>Transaction Complete:</td>");
	printf("<td><b>%s</b><br></td></tr>",rsltVars->paymentActionResponse);
	printf("<tr><td colspan=\"2\"><hr></td></tr>");
	printf("<th colspan=\"2\" align=\"left\" bgcolor=\"#ffcc00\">");
	printf("Result of Bank Service for Deposit</th>");
	printf("<tr><td>Transaction ID:</td>");
	printf("<td><b>%s</b></td></tr>",rsltVars->depositTransactionID);
	printf("<tr><td>Operation Result:</td>");
	printf("<td><b>%s</b></td></tr>",rsltVars->depositResult);
	printf("<tr><td>Comment:</td>");
	printf("<td><b>%s</b></td></tr>",rsltVars->depositComment);
	printf("<tr><td>Transaction Complete:</td>");
	printf("<td><b>%s</b></td></tr>",rsltVars->depositActionResponse);
	printf("<tr><td colspan=\"2\"><hr></td></tr>");
	printf("</table>");
	
	SendHtmlText("./TransactionHtml/ResultFooter.html");
	SendHtmlText("./TransactionHtml/CommonFooter.html");
}

/* make error page with given message */
static
void
MakeErrorPage(char* message) {
					
	fprintf(stderr, "%s\n", message);
	
	SendHtmlText("./TransactionHtml/CommonHeader.html");
	SendHtmlText("./TransactionHtml/ErrorHeader.html");
	
	printf("%s\n", message);
	
	SendHtmlText("./TransactionHtml/ErrorFooter.html");
	SendHtmlText("./TransactionHtml/CommonFooter.html");
}

/* put request page for starting */
static
void
PutRequestPage(const char *confFile) {
	char endpoint[READ_BUF_SZ];
	char tmpFile[READ_BUF_SZ];
	
	FILE *fp;
	
	fprintf(stderr, "\n---------- PutRequestPage begin ----------\n");
	
	strcpy(endpoint, DEFAULT_ENDPOINT);
	strcpy(tmpFile, DEFAULT_TMP_FILE);

	fp = fopen(TRANSFER_CONF_FILE, "r");
	if (!fp) {
		fprintf(stderr, "can't open config file %s for input\n", confFile);
	} else {
		if (fgets(endpoint, READ_BUF_SZ, fp) == NULL) {
			fprintf(stderr, "can't read endpoint\n");
		}
		if (fgets(tmpFile, READ_BUF_SZ, fp) == NULL) {
			fprintf(stderr, "can't read tmp_file\n");
		}
	}
	
	MakeRequestPage(endpoint, tmpFile);
	
    fprintf(stderr, "---------- PutRequestPage end ----------\n\n");
}

/* process request page and make accepted page or result page */
static
void

ProcessRequestPage(OpenSOAPStringHashPtr str,
  				   const char* confFile) {
	char *endpoint = NULL;
	char *tmpFile = NULL;
 	char *mode = NULL;
        char *endpoint1 = NULL;
        char *endpoint2 = NULL;
	char *account1 = NULL;
	char *account2 = NULL;
	char *money = NULL;
	char *messageID;
	ResultVariables rsltVars;
	FILE *fp = NULL;
	int error = OPENSOAP_NO_ERROR;
	
	fprintf(stderr, "\n---------- ProcessRequestPage begin ---------\n");
	
	OpenSOAPStringHashGetValueMB(str, "endpoint", (void**)&endpoint);
 	DecodeURLEncodedString(&endpoint);
	fprintf(stderr, "endpoint=%s\n", endpoint ? endpoint : "(null)");

	OpenSOAPStringHashGetValueMB(str, "tmp_file", (void**)&tmpFile);
	DecodeURLEncodedString(&tmpFile);
	fprintf(stderr, "tmpFile=%s\n", tmpFile ? tmpFile : "(null)");

	OpenSOAPStringHashGetValueMB(str, "mode", (void**)&mode);
	fprintf(stderr, "mode=%s\n", mode ? mode : "(null)");
	
	OpenSOAPStringHashGetValueMB(str, "endpoint1", (void**)&endpoint1);
 	DecodeURLEncodedString(&endpoint1);
	fprintf(stderr, "endpoint1=%s\n", endpoint1 ? endpoint1 : "(null)");

	OpenSOAPStringHashGetValueMB(str, "endpoint2", (void**)&endpoint2);
 	DecodeURLEncodedString(&endpoint2);
	fprintf(stderr, "endpoint2=%s\n", endpoint2 ? endpoint2 : "(null)");

	OpenSOAPStringHashGetValueMB(str, "account1", (void**)&account1);
	fprintf(stderr, "account1=%s\n", account1 ? account1 : "(null)");

	OpenSOAPStringHashGetValueMB(str, "account2", (void**)&account2);
	fprintf(stderr, "account2=%s\n", account2 ? account2 : "(null)");

	OpenSOAPStringHashGetValueMB(str, "money", (void**)&money);
	fprintf(stderr, "money=%s\n", money ? money : "(null)");

	fprintf(stderr, "confFile=%s\n", confFile);
	fp = fopen(confFile, "w");
	if (!fp) {
		MakeErrorPage("can't open config file for output\n");
	}
	fprintf(fp, "%s\n", endpoint);
	fprintf(fp, "%s\n", tmpFile);
	fclose(fp);

	if (strcmp(mode, "Async") == 0) {

	    FILE *fp = fopen(tmpFile, "w");
	    if (!fp) {
	        MakeErrorPage("Can't open acceptance messge file");
	    }
			
	    /* transaction client function */
	    error = TransferAsync(endpoint,
				  NULL,
				  "EUC-JP",
				  endpoint1,
				  endpoint2,
				  account1,
				  account2,
				  money,
				  &messageID,
				  fp);
			
	    if (fp) {
	        fclose(fp);
	    }
			
	    if (OPENSOAP_SUCCEEDED(error)) {
	        MakeAcceptedPage(messageID, endpoint, tmpFile);
	    }
	    else {
	        MakeErrorPage("SOAP client error");
	    }
			
	    if (messageID) {
	        free(messageID);
	    }
			
	}
	else {
				
	    /* transaction client function */
	    error = Transfer(endpoint,
			     NULL,
			     "EUC-JP",
			     endpoint1,
			     endpoint2,
			     account1,
			     account2,
			     money,
			     &rsltVars);
				
	    if (OPENSOAP_SUCCEEDED(error)) {
	        MakeResultPage(&rsltVars);
	    }
	    else {
	        MakeErrorPage("SOAP client error");
	    }
	}
	fprintf(stderr, "---------- ProcessRequestPage end ----------\n\n");
}

/* process accepted page and make result page */
static
void
ProcessAcceptedPage(OpenSOAPStringHashPtr str) {
	char *tmpFile;
	char *endpoint;
	FILE *fp;
	long error = OPENSOAP_NO_ERROR;
	ResultVariables rsltVars;

	fprintf(stderr, "\n---------- ProcessAcceptedPage begin ----------\n");
	
	OpenSOAPStringHashGetValueMB(str, "tmp_file", (void**)&tmpFile);
	DecodeURLEncodedString(&tmpFile);
	fprintf(stderr, "tmpFile=%s\n", tmpFile ? tmpFile : "(null)");
	
	OpenSOAPStringHashGetValueMB(str, "endpoint", (void**)&endpoint);
 	DecodeURLEncodedString(&endpoint);
	fprintf(stderr, "endpoint=%s\n", endpoint ? endpoint : "(null)");
	
	fp = fopen(tmpFile, "r");
	if (!fp) {
		MakeErrorPage("Can't open acceptance messge file");
	}
	
	/* transaction client function */
	error = GetResult(fp, endpoint, NULL, "EUC-JP", &rsltVars);
	
	fclose(fp);
		
	if (OPENSOAP_SUCCEEDED(error)) {
		MakeResultPage(&rsltVars);
	}
	else {
		MakeErrorPage("SOAP client error");
	}
	
	fprintf(stderr, "---------- ProcessAcceptedPage end ----------\n\n");
}

/* free hash value */
static
int
HashValueFree(void *val,
              void *opt) {
    free(val);

    return OPENSOAP_NO_ERROR;
}

/* main */
int
main(int argc,
	 char* argv[])
{
	OpenSOAPStringHashPtr str = NULL;
	char *screenID = NULL;
	
	long error = OpenSOAPInitialize(NULL);
	
	if (OPENSOAP_FAILED(error)) {
		fprintf(stderr,
				"OpenSOAP API Initialize Failed.\n"
				"OpenSOAP Error Code: 0x%08lx\n",
				error);
		
		return 1;
	}
	
	OpenSOAPStringHashCreate(&str);
	/* error handling must be here */
	
	TokenizeInputParameters(str);
	/* error handling must be here */
	
	OpenSOAPStringHashGetValueMB(str, "screenID", (void**)&screenID);
	
	fprintf(stderr, "screenID = %s\n", screenID ? screenID : "(null)");
	
	printf("Content-Type: text/html; charset=\"EUC_JP\"\n\n");
	
	if (screenID == NULL) {
		PutRequestPage(TRANSFER_CONF_FILE);
	}
	else if (strcmp(screenID, "Request") == 0) {
  		ProcessRequestPage(str, TRANSFER_CONF_FILE);
/*    		ProcessRequestPage(str); */
	}
	else if (strcmp(screenID, "Accepted") == 0) {
		ProcessAcceptedPage(str);
	}
	
	/* hash free */
	OpenSOAPStringHashApplyToValues(str, HashValueFree, NULL);
	OpenSOAPStringHashRelease(str);
	
	return 0;
}

