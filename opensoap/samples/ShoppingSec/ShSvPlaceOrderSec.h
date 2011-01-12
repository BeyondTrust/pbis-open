/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShSvPlaceOrderSec.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SvPlaceOrder_H
#define SvPlaceOrder_H

#include <OpenSOAP/Envelope.h>

int
PlaceOrder(OpenSOAPEnvelopePtr request,
		   OpenSOAPEnvelopePtr *response,
		   void *opt);
	
#endif /* SvPlaceOrder_H */
