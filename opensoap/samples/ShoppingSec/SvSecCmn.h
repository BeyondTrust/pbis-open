/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SvSecCmn.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SvSecCmn_H
#define SvSecCmn_H

#include <OpenSOAP/Envelope.h>

/*  #define FILE_NAME_SZ 128 */

/*
typedef struct {
    char privKeyName[FILE_NAME_SZ];
    char pubKeyName[FILE_NAME_SZ];
} SecKeyNames;
*/
/*
typedef struct {
	char nameSpace[FILE_NAME_SZ];
    char privKeyName[FILE_NAME_SZ];
    char pubKeyName[FILE_NAME_SZ];
} Options;
*/

/* encrypt specified parameter in body block */
int
EncryptParameter(OpenSOAPEnvelopePtr *envelope,
				 const char *method,
				 const char *param);

#endif /* SvSecCmn_H */
