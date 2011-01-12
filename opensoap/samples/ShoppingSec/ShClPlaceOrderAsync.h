/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderAsync.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef PlaceOrderAsync_H
#define PlaceOrderAsync_H

#include <stdio.h>

int
PlaceOrderAsync(const char *endpoint,
				const char *soapAction,
				const char *charEnc,
				const char *nameSpace,
				const char *code,
				long qty,
				FILE *fp);

int
ConfirmOrder(FILE *fp,
			 const char *endpoint,
			 const char *soapAction,
			 const char *charEnc,
			 const char *nameSpace,
			 char **confirmation);
	
#endif /* PlaceOrderAsync_H */
