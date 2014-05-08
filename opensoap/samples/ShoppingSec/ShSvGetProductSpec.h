/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShSvGetProductSpec.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SvGetProductSpec_H
#define SvGetProductSpec_H

#include <OpenSOAP/Envelope.h>

int
GetProductSpec(OpenSOAPEnvelopePtr request,
			   OpenSOAPEnvelopePtr *response,
			   void *opt);

#endif /* SvGetProductSpec_H */
