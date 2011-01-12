/*-----------------------------------------------------------------------------
 * $RCSfile: ShSvCmn.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShSvCmn.h"

#include <string.h>

#ifndef SERVICE_LOCALSTATEDIR
# define SERVICE_LOCALSTATEDIR "/usr/local/opensoap/var/services/ShoppingSec"
#endif

const char *SHOPPING_LOG_FILE = SERVICE_LOCALSTATEDIR "/Shopping.log";

const char *PRODUCT_SPEC_FILE = SERVICE_LOCALSTATEDIR "/ProductSpec.data";
const char *PRODUCT_STOCK_FILE = SERVICE_LOCALSTATEDIR "/ProductStock.data";

const char *SERVICE_NS_PREFIX = "m";

ProductSpec* productSpecList;
ProductStock* productStockList;

/* load product specification information */
int
LoadProductSpec() {
	static char FuncName[] = "LoadProductSpec";
	int error = OPENSOAP_NO_ERROR;
	FILE *fp;
	char buf[BUFFER_SIZE];
	int i;
	
	fp = fopen(PRODUCT_SPEC_FILE, "r");
	ERROR_RETURN(!fp, FuncName, "open product spec file");
	
	productSpecCount = 0;
	while (fgets(buf, 1024, fp) != NULL) {
		productSpecCount++;
	}
	
	/*
  	LOG_INT(FuncName, "productSpecCount", productSpecCount);
	*/
	
	productSpecList = malloc(sizeof(ProductSpec) * productSpecCount);
	
	rewind(fp);
	
	for (i = 0; i < productSpecCount; i++) {
		const char *first = &buf[0];
		char *priceString = NULL;
		
		error = !fgets(buf, 1024, fp);
		ERROR_RETURN(error, FuncName, "read product spec");
		
		buf[strlen(buf) - 1] = '\0';

		/*
  		LOG_STRING(FuncName, "spec", buf);
		*/
		
		productSpecList[i].manufacturer = getAllocToken(&first,  ",");
		productSpecList[i].name = getAllocToken(&first, ",");
		productSpecList[i].code = getAllocToken(&first, ",");
		priceString = getAllocToken(&first, ",");
		productSpecList[i].price = atol(priceString);
		free(priceString);
    }
	
	fclose(fp);

	return error;
}

/* load product stock information */
int
LoadProductStock() {
	static char FuncName[] = "LoadProductStock";
	int error = OPENSOAP_NO_ERROR;
	FILE *fp;
	char buf[BUFFER_SIZE];
	int i;
	
	fp = fopen(PRODUCT_STOCK_FILE, "r");
	ERROR_RETURN(!fp, FuncName, "open product stock file");
	
	productStockCount = 0;
	while (fgets(buf, 1024, fp) != NULL) {
		productStockCount++;
	}
	
	/*
	LOG_INT(FuncName, "productStockCount", productStockCount);
	*/
	
	productStockList = malloc(sizeof(ProductStock) * productStockCount);
	
	rewind(fp);
	
	for (i = 0; i < productStockCount; i++) {
		const char *first = &buf[0];
		char *qtyString = NULL;
		
		error = !fgets(buf, 1024, fp);
		ERROR_RETURN(error, FuncName, "read product stock");
		
		buf[strlen(buf) - 1] = '\0';
		
		/*
  		LOG_STRING(FuncName, "stock", buf);
		*/
		
		productStockList[i].code = getAllocToken(&first, ",");
		qtyString = getAllocToken(&first, ",");
		productStockList[i].qty = atol(qtyString);
		free(qtyString);
    }
	
	fclose(fp);
	
	return error;
}

/* save product stock information */
int
SaveProductStock() {
	static char FuncName[] = "SaveProductStock";
	int error = OPENSOAP_NO_ERROR;
	FILE *fp;
	char buf[BUFFER_SIZE];
	int i;
	
	fp = fopen(PRODUCT_STOCK_FILE, "w");
	ERROR_RETURN(!fp, FuncName, "open product stock file");
	
	for (i = 0; i < productStockCount; i++) {
		
		sprintf(buf,
				"%s,%ld",
				productStockList[i].code,
				productStockList[i].qty);
		
		/*
  		LOG_STRING(FuncName, "stock", buf);
		*/
		
		fputs(buf, fp);
		fputs("\n", fp);
    }
	
	fclose(fp);
	
	/* all allocated memory should be freed */
	
	return error;
}

/*
=begin
--- function#getAllocToken(first, sepChars)
    get token.

    :Parameters
      :[in, out] const char ** ((|first|))
        token first string pointer's pointer. If first is NULL, 
		*first is NULL, or **first is '\0', then return NULL.
	  :[in] const char * ((|sepChars|))
	    separete character's array pointer. If sepChars is NULL
		or *sepChars is '\0', then return NULL.
    :Return Value
      :char *
	    Return memory allocated token.
=end
 */

char *
getAllocToken(/* [in, out] */ const char **first,
			  /* [in] */      const char *sepChars) {
	char *ret = NULL;
	
	if (sepChars && *sepChars && first && *first && **first) {
		const char *beg = *first;
		const char *i = beg;
		size_t len = 0;
		/* find sepChars or '\0' */
		for (; *i && !strchr(sepChars, *i); ++i) {
		}
		/* get token length */
		len = i - beg;
		/* memory allocate */
		ret = malloc(len + 1);
		if (ret) {
			/* copy token */
			memcpy(ret, beg, len);
			ret[len] = '\0';
		}

		if (*i) {
			/* sepChars next pointer */
			++i;
		}
		/* return value */
		*first = i;
	}

	return ret;
}

