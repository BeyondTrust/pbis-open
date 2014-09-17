/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShSvCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ShSvCmn_H
#define ShSvCmn_H

#include "SvCmn.h"

#include <OpenSOAP/StringHash.h>

typedef struct {
	char *manufacturer;
	char *name;
	char *code;
	long price;
} ProductSpec;

typedef struct {
	char *code;
	long qty;
} ProductStock;

extern const char *SHOPPING_LOG_FILE;
extern const char *METHOD_NS_URI;
extern const char *METHOD_NS_PREFIX;

extern ProductSpec *productSpecList;
int productSpecCount;

extern ProductStock *productStockList;
int productStockCount;

int
LoadProductSpec();

int
LoadProductStock();

int
SaveProductStock();

char *
getAllocToken(const char **first,
			  const char *sepChars);

#endif /* ShSvCmn_H */
