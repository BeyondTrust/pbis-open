/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SignatureFunc.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include <string>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

/* libxml2 */
#if defined(HAVE_LIBXML2_XMLVERSION_H) 
#include <libxml2/tree.h>
#include <libxml2/parser.h>
#else
#include <libxml/tree.h>
#include <libxml/parser.h>
#endif /* !HAVE_LIBXML2_XMLVERSION_H */

#include "SignatureFunc.h"
#include "fio.h"
#include "ServerCommon.h"
//#include "SrvConfAttrHandler.h"
#include "OpenSOAP/OpenSOAP.h"
#include "OpenSOAP/Envelope.h"
#include "OpenSOAP/Security.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

#if 0
bool
getPrivKeyName(string& privKeyName)
{
  SrvConfAttrHandler srvConfAttrHndl;
  vector<std::string> values;
  string query = SrvConfAttrHandler::SERVER_CONF_TAG + "/keys/path=?";
  if (0 > srvConfAttrHndl.queryXml(query, values)) {
    AppLogger::Write(APLOG_ERROR,"*** ERROR ***: keys/path get failed.");
    return false;
  }
  privKeyName = values[0];
  if (privKeyName[privKeyName.length()] != '/') {
    privKeyName += "/";
  }
  privKeyName += "privKey.pem";
  return true;
}
#endif

#if 0
bool
getAddSignature(std::string& addSigValue)
{
  SrvConfAttrHandler ssmlAttrHndl;
  std::vector<std::string> values;
  std::string query;
  std::string addSignatureValue;
  
  query = SrvConfAttrHandler::SERVER_CONF_TAG + "/add_signature=?";
  if (0 > ssmlAttrHndl.queryXml(query, values)) {
    AppLogger::Write(APLOG_ERROR,"*** ERROR ***");
    return false;
  }
  addSigValue = values[0];
  return true;
}
#endif

#if 0
// check "conf/server/signature.conf" to sign or not
bool
needSignature()
{
  string addSignatureValue;
  if (!getAddSignature(addSignatureValue)) {
    AppLogger::Write(APLOG_ERROR,"*** ERROR ***: getAddSignature() failed.");
    return false;
  }

  AppLogger::Write(APLOG_DEBUG,"%s%s=[%s]"
                  ,"SignatureFunc::needSignature:"
                  ,"addSignatureValue",addSignatureValue.c_str());
  
  if(addSignatureValue == "true") {
    return true;
  }
  else {
    return false;
  }
}
#endif

#if 0
bool
addSignatureToFile(std::string fileID)
{
  // add signature
  string filename;
  if (!getMessageSpoolPath(filename)) {
    AppLogger::Write(APLOG_ERROR
                    ,"*** ERROR ***: getMessageSpoolPath() failed.");
    return false;
  }
  filename += fileID;

  string szPrivKName;
  if (!getPrivKeyName(szPrivKName)) {
    AppLogger::Write(APLOG_ERROR,"*** ERROR ***: getPrivKeyName() failed.");
    return false;
  }

  int nRet;
  OpenSOAPEnvelopePtr env = NULL;

  /* load Envelope from file */
  nRet = LoadEnvelope(filename.c_str(), &env);
  if (!OPENSOAP_SUCCEEDED(nRet)) {
    return false;
  }
  /* add signature */
  nRet = OpenSOAPSecAddSignWithFile(env, OPENSOAP_HA_SHA,
				    szPrivKName.c_str(), NULL);
  if (!OPENSOAP_SUCCEEDED(nRet)) {
    return false;
  }
  
  /* save signatured data */
  nRet = SaveEnvelope(env, filename.c_str());
  if (!OPENSOAP_SUCCEEDED(nRet)) {
    return false;
  }
  
  return true;
}
#endif

bool
OPENSOAP_API
addSignatureToString(std::string& soapMsg, const std::string& keyName)
{
  int nRet = OPENSOAP_NO_ERROR;
  OpenSOAPEnvelopePtr env = NULL;

  /* load Envelope from file */
  nRet = LoadEnvelope(soapMsg, &env);
  if (OPENSOAP_FAILED(nRet)) {
    AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                    ,"addSignatureToString: LoadEnvelope failed."
                    ,"rc",nRet);
    return false;
  }
#ifdef DEBUG
  else {
    AppLogger::Write(APLOG_DEBUG,"addSignatureToString: LoadEnvelope done.");
  }
  string orgSoapMsg(soapMsg);
#endif //DEBUG
  
  /* add signature */
  nRet = OpenSOAPSecAddSignWithFile(env, OPENSOAP_HA_SHA,
				    keyName.c_str(), NULL);
  if (OPENSOAP_FAILED(nRet)) {
    AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                    ,"addSignatureToString: OpenSOAPSecAddSignWithFile failed."
                    ,"rc",nRet);
    return false;
  }
#ifdef DEBUG
  else {
    AppLogger::Write(APLOG_DEBUG
                    ,"addSignatureToString: OpenSOAPSecAddSignWithFile done.");
  }
#endif //DEBUG
  
  /* save signatured data */
  nRet = SaveEnvelope(env, soapMsg);
  if (OPENSOAP_FAILED(nRet)) {
    AppLogger::Write(APLOG_ERROR,"%s %s=(%d)"
                    ,"addSignatureToString: SaveEnvelope failed."
                    ,"rc",nRet);
    return false;
  }
#ifdef DEBUG
  else {
    AppLogger::Write(APLOG_DEBUG5,"addSignatureToString: SaveEnvelope done.");
  }
  AppLogger::Write(APLOG_DEBUG5,"%s=[%s] %s=[%s]"
                    ,"orgSoapMsg",orgSoapMsg.c_str()
                    ,"signaturedSoapMsg",soapMsg.c_str());
#endif //DEBUG

  return true;
}
