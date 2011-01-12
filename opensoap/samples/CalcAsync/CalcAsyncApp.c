/*-----------------------------------------------------------------------------
 * $RCSfile: CalcAsyncApp.c,v $
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

#include <stdio.h>
#include <string.h>

#define READ_BUF_SZ 256

static const char CALC_CONF_FILE[] = "/var/tmp/CalcAsync.conf";

static const char DEFAULT_ENDPOINT[]
= "http://localhost/cgi-bin/soapInterface.cgi";

static const char DEFAULT_TMP_FILE[] = "/var/tmp/tmp_soap_msg.xml";

static const char TMP_MESSAGE_FILE[] = "/var/tmp/CalcAsyncApp_msg.log";

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
/*	int i; */
	int bufLen;
	char *buf;
	char *contentLength;
	char delim[] = "&";
/*	char *token; */
	char *key;
	char *value;
/*	int ret; */
	
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
	/* ? */
	return 0;
}

/* send html text read from file with given file name */
int
SendMessageText()
{
	if (debug_fp != stderr) {
		fclose(debug_fp);
		printf("<table border=\"1\"><td><xmp>\n");
		SendHtmlText(TMP_MESSAGE_FILE);
		printf("</xmp></td></table>\n");
	}
	return 0;
}

/* make request page */
static
void
MakeRequestPage(char *endpoint,
				char *tmpFile) {
	
	SendHtmlText("./CalcAsyncHtml/CommonHeader.html");
	SendHtmlText("./CalcAsyncHtml/RequestHeader.html");
	
	printf("Endpoint URL: <br>\n");
	printf("<input type=\"text\" size=50 name=\"endpoint\" value = \"%s\">\n",
		   endpoint);
	printf("<br><br>\n");
	
	printf("Temporary file name for acceptance message: <br>\n");
	printf("<input type=\"text\" size=50 name=\"tmp_file\" value = \"%s\">\n",
		   tmpFile);
	printf("<br><br>\n");
	
	SendHtmlText("./CalcAsyncHtml/RequestFooter.html");
	SendHtmlText("./CalcAsyncHtml/CommonFooter.html");
}

/* make accepted page */
static
void
MakeAcceptedPage(char *messageID,
				 char *endpoint,
				 char *tmpFile,
				 char* operator) {
					
	SendHtmlText("./CalcAsyncHtml/CommonHeader.html");
	SendHtmlText("./CalcAsyncHtml/AcceptedHeader.html");
					
	printf("%s\n", messageID);
	
	printf("<input type=\"hidden\" name=\"endpoint\" value=\"%s\">\n",
		   endpoint);
	printf("<input type=\"hidden\" name=\"tmp_file\" value=\"%s\">\n",
		   tmpFile);
	printf("<input type=\"hidden\" name=\"operator\" value=\"%s\">\n",
		   operator);
	if (debug_fp != stderr) {
		printf("<input type=\"hidden\" name=\"message\" value=\"display\">\n");
	}
	
	SendHtmlText("./CalcAsyncHtml/AcceptedFooter.html");
	SendMessageText();
	SendHtmlText("./CalcAsyncHtml/CommonFooter.html");
}

/* make result page */
static
void
MakeResultPage(double result) {
					
	SendHtmlText("./CalcAsyncHtml/CommonHeader.html");
	SendHtmlText("./CalcAsyncHtml/ResultHeader.html");
	
	printf("%f\n", result);

	SendHtmlText("./CalcAsyncHtml/ResultFooter.html");
	SendMessageText();
	SendHtmlText("./CalcAsyncHtml/CommonFooter.html");
}

/* make error page with given message */
static
void
MakeErrorPage(char* message) {
					
	fprintf(stderr, "%s\n", message);
	
	SendHtmlText("./CalcAsyncHtml/CommonHeader.html");
	SendHtmlText("./CalcAsyncHtml/ErrorHeader.html");
	
	printf("%s\n", message);
	
	SendHtmlText("./CalcAsyncHtml/ErrorFooter.html");
	SendHtmlText("./CalcAsyncHtml/CommonFooter.html");
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

	fp = fopen(CALC_CONF_FILE, "r");
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
	char *operator = NULL;
	char *operandA = NULL;
	char *operandB = NULL;

	char *tmp;
	char *cp;
	int hopcount;
	char **forward_path;
	int forward_path_num;
	char *path_string;
	
	char *messageID;
	double result = 0.0;
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
	
	OpenSOAPStringHashGetValueMB(str, "operator", (void**)&operator);
	fprintf(stderr, "operator=%s\n", operator ? operator : "(null)");
	
	OpenSOAPStringHashGetValueMB(str, "operand_a", (void**)&operandA);
	fprintf(stderr, "operand_a=%s\n", operandA ? operandA : "(null)");
	
	OpenSOAPStringHashGetValueMB(str, "operand_b", (void**)&operandB);
	fprintf(stderr, "operand_b=%s\n", operandB ? operandB : "(null)");

	if (OPENSOAP_SUCCEEDED(OpenSOAPStringHashGetValueMB(
							   str, "onhop", (void**)&tmp))) {
		fprintf(stderr, "onhop=%s\n", tmp);
		OpenSOAPStringHashGetValueMB(str, "hopcount", (void**)&tmp);
		fprintf(stderr, "hopcount=%s\n", tmp);
		hopcount = atoi(tmp);
	} else {
		hopcount = -1;
	}
	if (OPENSOAP_SUCCEEDED(OpenSOAPStringHashGetValueMB(
							   str, "onpath", (void**)&tmp))) {
		int i;
		fprintf(stderr, "onpath=%s\n", tmp);
		OpenSOAPStringHashGetValueMB(str, "path", (void**)&tmp);
		fprintf(stderr, "path=%s\n", tmp);
		path_string = strdup(tmp);
		cp = path_string;
		while(*tmp) {
			if (! strncmp(tmp, "%0D%0A", 6)) {
				*cp = ',';
				cp++;
				tmp += 6;
			}
			else if (! strncmp(tmp, "%3A", 3)) {
				*cp = ':';
				cp++;
				tmp += 3;
			}
			else if (! strncmp(tmp, "%2F", 3)) {
				*cp = '/';
				cp++;
				tmp += 3;
			}
			else {
				*cp = *tmp;
				cp++;
				tmp ++;
			}
			fprintf(stderr, "[%c]", *cp);
		}
		*cp = '\0';
		fprintf(stderr, "\n");
		fprintf(stderr, "forward_path_string=%s\n", path_string);
		forward_path = getForwardPathListString(path_string, &forward_path_num) ;
		fprintf(stderr, "forward_path_num=%d\n", forward_path_num);
		for(i = 0; i < forward_path_num; i++) {
			fprintf(stderr, "forward_path=%s\n", forward_path[i]);
		}
	} else {
		forward_path = NULL;
		forward_path_num = 0;
	}
	if (OPENSOAP_SUCCEEDED(OpenSOAPStringHashGetValueMB(
							   str, "message", (void**)&tmp))) {
		if (! (debug_fp = fopen(TMP_MESSAGE_FILE, "w"))) {
			debug_fp = stderr;
		}
	} else {
		debug_fp = stderr;
	}

	fprintf(stderr, "confFile=%s\n", confFile);
	fp = fopen(confFile, "w");
	if (!fp) {
		MakeErrorPage("can't open config file for output\n");
	}
	fprintf(fp, "%s\n", endpoint);
	fprintf(fp, "%s\n", tmpFile);
	fclose(fp);

	if (strcmp(operator, "Add") == 0
		|| strcmp(operator, "Subtract") == 0
		|| strcmp(operator, "Multiply") == 0
		|| strcmp(operator, "Divide") == 0) {
		
		if (strcmp(mode, "Async") == 0) {

			FILE *fp = fopen(tmpFile, "w");
			if (!fp) {
				MakeErrorPage("Can't open acceptance messge file");
			}
			
			/* calc client function */
			error = CalcAsync(endpoint,
							  NULL,
							  "EUC-JP",
							  operator,
							  atof(operandA),
							  atof(operandB),
							  hopcount,
							  (const char **)forward_path,
							  forward_path_num,
							  &messageID,
							  fp);
			
			if (fp) {
				fclose(fp);
			}
			
			if (OPENSOAP_SUCCEEDED(error)) {
				MakeAcceptedPage(messageID, endpoint, tmpFile, operator);
			}
			else {
				MakeErrorPage("SOAP client error");
			}
			
			if (messageID) {
				free(messageID);
			}
			
		}
		else {
				
			/* calc client function */
			error = CalcSync(endpoint,
							 NULL,
							 "EUC-JP",
							 operator,
							 atof(operandA),
							 atof(operandB),
							 hopcount,
							 (const char **)forward_path,
							 forward_path_num,
							 &result);
			
			if (OPENSOAP_SUCCEEDED(error)) {
				MakeResultPage(result);
			}
			else {
				MakeErrorPage("SOAP client error");
			}
		}
	} else {
		MakeErrorPage("CalcAsync operator mismatch");
	}
	
	fprintf(stderr, "---------- ProcessRequestPage end ----------\n\n");
}

/* process accepted page and make result page */
static
void
ProcessAcceptedPage(OpenSOAPStringHashPtr str) {
	char *tmpFile;
	char *endpoint;
	char *operator;
	double result;
	FILE *fp;
	long error = OPENSOAP_NO_ERROR;
	
	fprintf(stderr, "\n---------- ProcessAcceptedPage begin ----------\n");
	
	if (OPENSOAP_SUCCEEDED(OpenSOAPStringHashGetValueMB(
							   str, "message", (void**)&tmpFile))) {
		if (! (debug_fp = fopen(TMP_MESSAGE_FILE, "w"))) {
			debug_fp = stderr;
		}
	} else {
		debug_fp = stderr;
	}
	
	OpenSOAPStringHashGetValueMB(str, "tmp_file", (void**)&tmpFile);
	DecodeURLEncodedString(&tmpFile);
	fprintf(stderr, "tmpFile=%s\n", tmpFile ? tmpFile : "(null)");
	
	OpenSOAPStringHashGetValueMB(str, "endpoint", (void**)&endpoint);
 	DecodeURLEncodedString(&endpoint);
	fprintf(stderr, "endpoint=%s\n", endpoint ? endpoint : "(null)");
	
	OpenSOAPStringHashGetValueMB(str, "operator", (void**)&operator);
	fprintf(stderr, "operator=%s\n", operator ? operator : "(null)");

	fp = fopen(tmpFile, "r");
	if (!fp) {
		MakeErrorPage("Can't open acceptance messge file");
	}
	
	/* calc client function */
	error = GetResult(fp, endpoint, NULL, "EUC-JP", &result);
	
	fclose(fp);
		
	fprintf(stderr, "%s: result = %f\n", operator, result);
	
	if (OPENSOAP_SUCCEEDED(error)) {
		MakeResultPage(result);
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
		PutRequestPage(CALC_CONF_FILE);
	}
	else if (strcmp(screenID, "Request") == 0) {
  		ProcessRequestPage(str, CALC_CONF_FILE);
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

