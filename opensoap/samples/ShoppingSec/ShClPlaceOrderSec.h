/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderSec.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef PlaceOrderSec_H
#define PlaceOrderSec_H

int
PlaceOrder(const char *endpoint,
		   const char *soapAction,
		   const char *charEnc,
		   const char *nameSpace,
		   const char *servicePubKey,
		   const char *clientPrivKey,
		   const char *code,
		   long qty,
		   char **confirmation);

#endif /* PlaceOrderSec_H */
