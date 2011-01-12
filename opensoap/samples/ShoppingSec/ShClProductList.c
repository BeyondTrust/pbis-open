/*-----------------------------------------------------------------------------
 * $RCSfile: ShClProductList.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include "ShClProductList.h"
#include "ShClGetProductCount.h"
#include "ShClGetProductSpec.h"

#include <OpenSOAP/StringHash.h>

#include <stdio.h>
#include <string.h>

/* release members of product list struct */
void
ProductListReleaseMembers(ProductListItemPtr productListItem) {
    if (productListItem) {
        free(productListItem->manufacturer);
	    productListItem->manufacturer = NULL;
		
        free(productListItem->name);
		productListItem->name = NULL;
		
        free(productListItem->code);
        productListItem->code = NULL;
    }
}

/* set values to product list struct */
ProductListItemPtr
ProductListItemSetValues(ProductListItemPtr productListItem,
                         const char *manufacturer,
                         const char *name,
                         const char *code,
						 long price) {
    ProductListItemPtr ret = productListItem;
	
    if (ret) {
        free(ret->manufacturer);
        free(ret->name);
        free(ret->code);
        ret->manufacturer = manufacturer ? strdup(manufacturer) : NULL;
        ret->name = name ? strdup(name) : NULL;
        ret->code = code ? strdup(code) : NULL;
		ret->price = price;
        if (!ret->manufacturer || !ret->name || !ret->code) {
            ProductListReleaseMembers(ret);
            ret = NULL;
        }
    }

    return ret;
}

/* release product list struct */
void
ReleaseProductList(ProductListPtr productList) {
    if (productList) {
		
        ProductListItemPtr i = productList->productList;
        ProductListItemPtr e = i + productList->productCount;
		
        for (; i != e; ++i) {
            ProductListReleaseMembers(i);
        }
		
        free(productList->productList);
        free(productList);
    }
}

/* get product list by getting product count and specifications */
ProductListPtr
GetProductList(const char *endpoint,
			   const char *soapAction,
			   const char *charEnc,
			   const char *nameSpace) {
    ProductListPtr ret = NULL;

	long productCount;
    int error = GetProductCount(endpoint,
								soapAction,
								charEnc,
								nameSpace,
								&productCount);
    if (OPENSOAP_SUCCEEDED(error) && productCount >= 0) {
		fprintf(stderr, "GetProductList: count = %ld\n", productCount);
		
        ret = malloc(sizeof(ProductList));
        if (ret) {
            ret->productCount = productCount;
            ret->productList  = malloc(sizeof(ProductListItem)
                                       * productCount);
            if (ret->productList) {
                ProductListItemPtr productList = ret->productList;
                long index = 0;
				
                memset(productList, 0, sizeof(ProductListItem) * productCount);
				
				while (index < productCount
					   && OPENSOAP_SUCCEEDED(
						   GetProductSpec(endpoint,
										  soapAction,
										  charEnc,
										  nameSpace,
										  index,
										  &productList[index]))) {
					index++;
				}
				
                if (index != productCount) {
                    ReleaseProductList(ret);
                    ret = NULL;
                }
            }
        }
    }
	
    return ret;
}
