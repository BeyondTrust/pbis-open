/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClProductList.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ProductList_H
#define ProductList_H

#include <stdlib.h>

typedef struct tagProductListItem ProductListItem;
typedef ProductListItem *ProductListItemPtr;
	
struct tagProductListItem {
	char *manufacturer;
	char *name;
	char *code;
	long price;
};
	
typedef struct {
	size_t productCount;
	ProductListItemPtr productList;
} ProductList;
typedef ProductList *ProductListPtr;

ProductListItemPtr
ProductListItemSetValues(ProductListItemPtr productListItem,
						 const char *manufacturer,
						 const char *name,
						 const char *code,
						 long price);

void
ReleaseProductList(ProductListPtr productList);

ProductListPtr
GetProductList(const char *endpoint,
			   const char *soapAction,
			   const char *charEnc,
			   const char *nameSpace);
	
#endif /* ProductList_H */
