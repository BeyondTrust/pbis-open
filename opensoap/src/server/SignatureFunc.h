/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SignatureFunc.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef SIGNATURE_FUNC_H
#define SIGNATURE_FUNC_H

#include <OpenSOAP/Defines.h>


extern
bool
OPENSOAP_API
addSignatureToString(std::string& soapMsg, const std::string& keyName);

#if 0
extern
bool
getPrivKeyName(string& prviKeyName);
extern
bool
addSignatureToFile(std::string fileID);
extern
bool
needSignature();
extern
bool
getAddSignature(string& addSigValue);
#endif

#endif /* SIGNATURE_FUNC_H */
