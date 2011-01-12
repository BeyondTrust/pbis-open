/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ShClMainSec.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ShClMainSec_H
#define ShClMainSec_H

#include "ShClCmn.h"

int
InitializeVariables(ClientVariables *appVars,
					char **servicePubKey,
					char **clientPrivKey,
					int argc,
					char **argv);

#endif /* ShClMainSec_H */
