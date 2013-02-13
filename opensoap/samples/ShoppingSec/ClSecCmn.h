/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ClSecCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ClSecCmn_H
#define ClSecCmn_H

#include <OpenSOAP/Envelope.h>

/* encrypt specified parameter in body block */
int
EncryptParameter(OpenSOAPEnvelopePtr *envelope,
				 const char *method,
				 const char *param);

#endif /* ClientSecCmn_H */
