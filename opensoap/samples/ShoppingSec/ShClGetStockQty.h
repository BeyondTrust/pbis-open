/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClGetStockQty.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef GetStockQty_H
#define GetStockQty_H

int
GetStockQty(const char *endpoint,
			const char *soapAction,
			const char *charEnc,
			const char *nameSpace,
			const char *code,
			long *qty);
	
#endif /* GetStockQty_H */
