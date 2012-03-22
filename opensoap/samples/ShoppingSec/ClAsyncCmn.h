/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ClAsyncCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ClAsyncCmn_H
#define ClAsyncCmn_H

#include <OpenSOAP/Envelope.h>

/* add header elements for async mode */
int
AddAsyncHeader(OpenSOAPEnvelopePtr *request);

#endif /* ClAsyncCmn_H */
