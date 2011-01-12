/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShSvGetProductCount.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SvGetProductCount_H
#define SvGetProductCount_H

#include <OpenSOAP/Envelope.h>

int
GetProductCount(OpenSOAPEnvelopePtr request,
				OpenSOAPEnvelopePtr *response,
				void *opt);

#endif /* SvGetProductCount_H */
