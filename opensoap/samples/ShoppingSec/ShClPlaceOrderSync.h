/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderSync.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef PlaceOrderSync_H
#define PlaceOrderSync_H

int
PlaceOrderSync(const char *endpoint,
			   const char *soapAction,
			   const char *charEnc,
			   const char *nameSpace,
			   const char *code,
			   long qty,
			   char **confirmation);
	
#endif /* PlaceOrderSync_H */
