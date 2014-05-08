/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClPlaceOrderCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef PlaceOrderCmn_H
#define PlaceOrderCmn_H

#include <OpenSOAP/Envelope.h>

/* create place order request message */
int
CreatePlaceOrderRequest(const char *nameSpace,
						const char *code,
						long qty,
						OpenSOAPEnvelopePtr *request);
/* parse place order response message */
int
ParsePlaceOrderResponse(OpenSOAPEnvelopePtr response,
						const char *nameSpace,
						char **confirmation);

#endif /* PlaceOrderCmn_H */
