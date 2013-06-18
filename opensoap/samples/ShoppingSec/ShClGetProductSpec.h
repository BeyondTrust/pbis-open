/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClGetProductSpec.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef GetProductSpec_H
#define GetProductSpec_H

#include "ShClProductList.h"

int
GetProductSpec(const char *endpoint,
			   const char *soapAction,
			   const char *charEnc,
			   const char *nameSpace,
			   long index,
			   ProductListItemPtr product);
	
#endif /* GetProductSpec_H */
