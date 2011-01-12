/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClGetProductCount.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef GetProductCount_H
#define GetProductCount_H

int
GetProductCount(const char *endpoint,
				const char *soapAction,
				const char *charEnc,
				const char *nameSpace,
				long *count);
	
#endif /* GetProductCount_H */
