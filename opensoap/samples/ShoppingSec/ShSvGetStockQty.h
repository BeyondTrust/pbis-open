/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShSvGetStockQty.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SvGetStockQty_H
#define SvGetStockQty_H

#include <OpenSOAP/Envelope.h>

int
GetStockQty(OpenSOAPEnvelopePtr request,
			OpenSOAPEnvelopePtr *response,
			void *opt);
	
#endif /* SvGetStockQty_H */
