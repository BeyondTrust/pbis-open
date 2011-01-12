/*-----------------------------------------------------------------------------
 * $RCSfile: ShClMainSecWeb.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClMainSecWeb.h"
#include "ShClGetProductCount.h"
#include "ShClGetProductSpec.h"
#include "ShClGetStockQty.h"
#include "ShClPlaceOrderSec.h"
#include "ClCmn.h"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/StringHash.h>

#include <stdio.h>
#include <string.h>

static const char SERVICE_NS_URI[]
= "http://services.opensoap.jp/samples/ShoppingSec/";

#ifndef SERVICE_DIR
# define SERVICE_DIR "/usr/local/opensoap/services/ShoppingSec"
#endif

static const char SHOPPING_CONF_FILE[] = SERVICE_DIR "/Shopping.conf";

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
TokenizeInputParameters(OpenSOAPStringHashPtr str) {
	int bufLen;
	char *buf;
	char *contentLength;
	char delim[] = "&";
	char *key;
	char *value;

	contentLength = getenv("CONTENT_LENGTH");

	if (contentLength != NULL) {
	
		bufLen = atoi(contentLength);
		buf = malloc(bufLen + 1);
		buf[bufLen] = '\0';
		fread(buf, 1, bufLen, stdin);

		/*
  		fprintf(stderr, "buf = %s\n", buf);
		*/
	
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
			
			/*
  			fprintf(stderr, "add: key = %s , value = %s\n", key, value);
			*/
			
			OpenSOAPStringHashSetValueMB(str, key, valStr);
		
			key = strtok(NULL, delim);
		}
	}
}

/* send html text read from file with given file name */
int
SendHtmlText(const char *filename) {
  	char buf[1024];
  
	FILE *fp = fopen(filename, "r");
	
	if (fp != NULL) {
 		while (fgets(buf, 1024, fp) != NULL) {
			printf(buf);
		}
		fclose(fp);
		return 0;
	} else {
		printf("<html>Can't open html file: %s</html>", filename);
		return 1;
	}
}

/* make error page with given message */
static
void
MakeErrorPage(char* message) {
	
	fprintf(stderr, "%s\n", message);
	
	SendHtmlText("./ShoppingHtml/CommonHdr.html");
	SendHtmlText("./ShoppingHtml/ErrorHdr.html");
	
	printf("%s\n", message);
	
	SendHtmlText("./ShoppingHtml/ErrorFtr.html");
	SendHtmlText("./ShoppingHtml/CommonFtr.html");
}

/* put request page for starting */
static
void
MakeGetProductListPage(const char *confFile) {
	static char FuncName[] = "MakeGetProductListPage";

	char endpoint[BUFFER_SIZE];
	char pubKey[BUFFER_SIZE];
	char privKey[BUFFER_SIZE];
	FILE *fp;

	fprintf(stderr, "\n---------- MakeGetProductListPage begin ----------\n");
	
	strcpy(endpoint, DEFAULT_ENDPOINT);
	
	fp = fopen(SHOPPING_CONF_FILE, "r");
	if (!fp) {
		fprintf(stderr, "can't open config file %s for input\n", confFile);
	} else {
		if (fgets(endpoint, BUFFER_SIZE, fp) == NULL) {
			fprintf(stderr, "can't read endpoint\n");
		}
		if (fgets(pubKey, BUFFER_SIZE, fp) == NULL) {
			fprintf(stderr, "can't read service's public key\n");
		}
		if (fgets(privKey, BUFFER_SIZE, fp) == NULL) {
			fprintf(stderr, "can't read service's private key\n");
		}
	}
	
	LOG_STRING(FuncName, "endpoint", endpoint ? endpoint : "(null)");
	LOG_STRING(FuncName, "pubKey", pubKey ? pubKey : "(null)");
	LOG_STRING(FuncName, "privKey", privKey ? privKey : "(null)");
	
	SendHtmlText("./ShoppingHtml/CommonHdr.html");
	SendHtmlText("./ShoppingHtml/GetProductListHdr.html");
	
	printf("<b>Service's URL:</b>\n");
	printf("<input type=\"text\" size=60 name=\"endpoint\" value = \"%s\">\n",
		   endpoint);
	printf("<br>\n");
	
	printf("<b>Service's Public Key:</b>\n");
	printf("<input type=\"text\" size=60 name=\"pubKey\" value=\"%s\">\n",
		   pubKey);
	printf("<br>\n");
	
	printf("<b>User's Private Key:</b>\n");
	printf("<input type=\"text\" size=60 name=\"privKey\" value=\"%s\">\n",
		   privKey);
	printf("<br>\n");
	
	printf("<br>\n");
	
	SendHtmlText("./ShoppingHtml/GetProductListFtr.html");
	SendHtmlText("./ShoppingHtml/CommonFtr.html");
	
	fprintf(stderr, "---------- MakeGetProductListPage end ----------\n\n");
}

static
void
MakeGetStockQtyPage(const char* confFile,
					OpenSOAPStringHashPtr str) {
	static char FuncName[] = "MakeGetStockQtyPage";
    int error = OPENSOAP_NO_ERROR;
	
	char *endpoint;
	char *pubKey;
	char *privKey;
	FILE *fp;
	
	ProductListPtr productList;
	
	fprintf(stderr, "\n---------- MakeGetStockQtyPage begin ----------\n");
	
	error = OpenSOAPStringHashGetValueMB(str, "endpoint", (void**)&endpoint);
 	DecodeURLEncodedString(&endpoint);
	LOG_STRING(FuncName, "endpoint", endpoint ? endpoint : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "pubKey", (void**)&pubKey);
 	DecodeURLEncodedString(&pubKey);
	LOG_STRING(FuncName, "pubKey", pubKey ? pubKey : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "privKey", (void**)&privKey);
 	DecodeURLEncodedString(&privKey);
	LOG_STRING(FuncName, "privKey", privKey ? privKey : "(null)");
	
	fprintf(stderr, "confFile = %s\n", confFile);
	fp = fopen(confFile, "w");
	if (!fp) {
		MakeErrorPage("can't open config file for output\n");
	}
	fprintf(fp, "%s\n", endpoint);
	fprintf(fp, "%s\n", pubKey);
	fprintf(fp, "%s\n", privKey);
	fclose(fp);
	
	productList = GetProductList(endpoint,
								 NULL,
								 DEFAULT_CHARENC,
								 SERVICE_NS_URI);
	fprintf(stderr, "GetProductList: %s\n",
			(productList ? "Succeeded" : "Failed"));
	
	if (productList && productList->productCount) {
		ProductListItemPtr products = productList->productList;
		int i = 0;
		
		SendHtmlText("./ShoppingHtml/CommonHdr.html");
		SendHtmlText("./ShoppingHtml/GetStockQtyHdr.html");
		
		for (; i < productList->productCount; ++i) {
			printf("<tr>"
				   "<td><input type=\"radio\" name=\"spec\" value=\""
				   "%s_%s_%s_%ld\"></td>"
				   "<td>%s</td>"
				   "<td>%s</td>"
				   "<td>%s</td>"
				   "<td align=right>%ld</td>"
				   "</tr>\n",
				   products[i].manufacturer,
				   products[i].name,
				   products[i].code,
				   products[i].price,
				   products[i].manufacturer,
				   products[i].name,
				   products[i].code,
				   products[i].price);
		}
		
		printf("<input type=\"hidden\" name=\"endpoint\" value=\"%s\">\n",
			   endpoint);
		printf("<input type=\"hidden\" name=\"pubKey\" value=\"%s\">\n",
			   pubKey);
		printf("<input type=\"hidden\" name=\"privKey\" value=\"%s\">\n",
			   privKey);
		
		SendHtmlText("./ShoppingHtml/GetStockQtyFtr.html");
		SendHtmlText("./ShoppingHtml/CommonFtr.html");
    
		ReleaseProductList(productList);
	}
	else {
		/* error */
		SendHtmlText("./ShoppingHtml/CommonHdr.html");
		SendHtmlText("./ShoppingHtml/GetStockQtyErr.html");
		SendHtmlText("./ShoppingHtml/CommonFtr.html");
	}

	fprintf(stderr, "---------- MakeGetStockQtyPage end ----------\n\n");
}

static
void
MakePlaceOrderPage(OpenSOAPStringHashPtr str) {
	static char FuncName[] = "MakePlaceOrderPage";
    int error = OPENSOAP_NO_ERROR;
	
	char *endpoint;
	char *pubKey;
	char *privKey;
	char *spec = NULL;
	const char *manifucture = NULL;
	const char *name = NULL;
	const char *code = NULL;
	const char *price = NULL;
	
	fprintf(stderr, "\n---------- MakePlaceOrderPage begin ----------\n");
	
	error = OpenSOAPStringHashGetValueMB(str, "endpoint", (void**)&endpoint);
 	DecodeURLEncodedString(&endpoint);
	LOG_STRING(FuncName, "endpoint", endpoint ? endpoint : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "pubKey", (void**)&pubKey);
 	DecodeURLEncodedString(&pubKey);
	LOG_STRING(FuncName, "pubKey", pubKey ? pubKey : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "privKey", (void**)&privKey);
 	DecodeURLEncodedString(&privKey);
	LOG_STRING(FuncName, "privKey", privKey ? privKey : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "spec", (void**)&spec);
	
	if (OPENSOAP_SUCCEEDED(error)) {
		char *spec_tmp = strdup(spec);
		long qty;
		
		manifucture = strtok(spec_tmp, "_");
		name = strtok(NULL, "_");
		code = strtok(NULL, "_");
		price = strtok(NULL, "_");
		
		error = GetStockQty(endpoint,
							NULL,
							DEFAULT_CHARENC,
							SERVICE_NS_URI,
							code,
							&qty);
		
		fprintf(stderr,
				"GetStockQty: %s, %s, %s, %s, %ld\n",
				manifucture ? manifucture : "(null)",
				name ? name : "(null)",
				code ? code : "(null)",
				price,
				qty);

		if (qty >= 0) {
			SendHtmlText("./ShoppingHtml/CommonHdr.html");
			SendHtmlText("./ShoppingHtml/PlaceOrderHdr.html");

			printf("<input type=\"hidden\" name=\"spec\" value=\"%s\">", spec);

			printf("<tr>"
				   "<td>%s</td>"
				   "<td>%s</td>"
				   "<td>%s</td>"
				   "<td align=right>%s</td>"
				   "<td align=right>%ld</td>"
				   "</tr>\n",
				   manifucture,
				   name,
				   code,
				   price,
				   qty);

			printf("<input type=\"hidden\" name=\"endpoint\" value=\"%s\">\n",
				   endpoint);
			printf("<input type=\"hidden\" name=\"pubKey\" value=\"%s\">\n",
				   pubKey);
			printf("<input type=\"hidden\" name=\"privKey\" value=\"%s\">\n",
				   privKey);
			
			SendHtmlText("./ShoppingHtml/PlaceOrderFtr.html");
			SendHtmlText("./ShoppingHtml/CommonFtr.html");
		}
		else {
			/* error */
			SendHtmlText("./ShoppingHtml/CommonHdr.html");
			SendHtmlText("./ShoppingHtml/PlaceOrderErr.html");
			SendHtmlText("./ShoppingHtml/CommonFtr.html");
		}
	}
	else {
		/* error */
		SendHtmlText("./ShoppingHtml/CommonHdr.html");
		SendHtmlText("./ShoppingHtml/PlaceOrderErr.html");
		SendHtmlText("./ShoppingHtml/CommonFtr.html");
	}
	
	fprintf(stderr, "\n---------- MakePlaceOrderPage end ----------\n");
}

static
void
MakeOrderResultPage(OpenSOAPStringHashPtr str) {
	static char FuncName[] = "MakeOrderResultPage";
    int error = OPENSOAP_NO_ERROR;
	
	char *endpoint;
	char *pubKey;
	char *privKey;
	char *spec = NULL;
	const char *manufacture = NULL;
	const char *name = NULL;
	const char *code = NULL;
	const char *price = NULL;
	
	error = OpenSOAPStringHashGetValueMB(str, "endpoint", (void**)&endpoint);
 	DecodeURLEncodedString(&endpoint);
	LOG_STRING(FuncName, "endpoint", endpoint ? endpoint : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "pubKey", (void**)&pubKey);
 	DecodeURLEncodedString(&pubKey);
	LOG_STRING(FuncName, "pubKey", pubKey ? pubKey : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "privKey", (void**)&privKey);
 	DecodeURLEncodedString(&privKey);
	LOG_STRING(FuncName, "privKey", privKey ? privKey : "(null)");
	
	error = OpenSOAPStringHashGetValueMB(str, "spec", (void**)&spec);
	if (OPENSOAP_SUCCEEDED(error)) {
		char *confirmation = NULL;
		char *qty;
		manufacture = strtok(spec, "_");
		name = strtok(NULL, "_");
		code = strtok(NULL, "_");
		price = strtok(NULL, "_");
		
		error = OpenSOAPStringHashGetValueMB(str, "qty", (void**)&qty);
		
		fprintf(stderr,
				"PlaceOrder: %s, %s\n",
				code ? code : "(null)",
				qty);
		
		error = PlaceOrder(endpoint,
						   NULL,
						   DEFAULT_CHARENC,
						   SERVICE_NS_URI,
						   pubKey,
						   privKey,
						   code,
						   atol(qty),
						   &confirmation);
		
		fprintf(stderr,
				"OrderResult: %s\n",
				confirmation ? confirmation : "(null)");
		
		if (confirmation) {
			SendHtmlText("./ShoppingHtml/CommonHdr.html");
			SendHtmlText("./ShoppingHtml/OrderResultHdr.html");
			
			printf("<tr>"
				   "<td>%s</td>"
				   "<td>%s</td>"
				   "<td>%s</td>"
				   "<td align=right>%s</td>"
				   "<td align=right>%s</td>"
				   "<td>%s</td>"
				   "</tr>\n",
				   manufacture,
				   name,
				   code,
				   price,
				   qty,
				   confirmation);
			
			SendHtmlText("./ShoppingHtml/OrderResultFtr.html");
			SendHtmlText("./ShoppingHtml/CommonFtr.html");
			
			free(confirmation);
		}
		else {
			/* error */
			SendHtmlText("./ShoppingHtml/CommonHdr.html");
			SendHtmlText("./ShoppingHtml/OrderResultErr.html");
			SendHtmlText("./ShoppingHtml/CommonFtr.html");
		}
	}
	else {
		/* error */
		SendHtmlText("./ShoppingHtml/CommonHdr.html");
		SendHtmlText("./ShoppingHtml/OrderResultErr.html");
		SendHtmlText("./ShoppingHtml/CommonFtr.html");
	}
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
	 char* argv[]) {
	static char FuncName[] = "main";
    int error = OPENSOAP_NO_ERROR;

	OpenSOAPStringHashPtr str = NULL;
	char *pageID = NULL;
	
	error = OpenSOAPInitialize(NULL);
	
	if (OPENSOAP_FAILED(error)) {
		fprintf(stderr,
				"OpenSOAP API Initialize Failed.\n"
				"OpenSOAP Error Code: 0x%04x\n",
				error);
		
		return 1;
	}
	
	OpenSOAPStringHashCreate(&str);
	/* error handling must be here */
	
	TokenizeInputParameters(str);
	/* error handling must be here */
	
	OpenSOAPStringHashGetValueMB(str, "pageID", (void**)&pageID);
	
	LOG_STRING(FuncName, "pageID", pageID ? pageID : "(null)");
	
	printf("Content-Type: text/html; charset=\"EUC_JP\"\n\n");
	
	if (pageID == NULL) {
		MakeGetProductListPage(SHOPPING_CONF_FILE);
	}
	else if (strcmp(pageID, "GetProductList") == 0) {
		MakeGetStockQtyPage(SHOPPING_CONF_FILE, str);
	}
	else if (strcmp(pageID, "GetStockQty") == 0) {
		MakePlaceOrderPage(
						   str);
	}
	else if (strcmp(pageID, "PlaceOrder") == 0) {
		MakeOrderResultPage(str);
	}
	
	/* hash free */
	OpenSOAPStringHashApplyToValues(str, HashValueFree, NULL);
	OpenSOAPStringHashRelease(str);
	
	return 0;
}

