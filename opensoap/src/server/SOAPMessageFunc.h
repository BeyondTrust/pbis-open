/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SOAPMessageFunc.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SOAP_MESSAGEFUNC_H
#define SOAP_MESSAGEFUNC_H

#include <string>
#include <vector>

#include <OpenSOAP/Defines.h>

#include <OpenSOAP/String.h>
#include <OpenSOAP/Envelope.h>

//class MsgAttrHandler;
#include "MsgAttrHandler.h"

static const int OPENSOAP_MESSAGE_LIMIT_SIZE = 1048576;

extern
std::string
OPENSOAP_API
createSOAPFaultMessage (std::string faultcodeStr, std::string faultstringStr,
			std::string faultactorStr, std::string detailStr);

extern
bool
OPENSOAP_API
extractOperation(OpenSOAP::MsgAttrHandler& msgAttrHndl,
                 std::string& operationName,
                 std::string* nsPtr = NULL);

/*
  check Messaage ID
 */
extern
bool
OPENSOAP_API
isItMsgID(OpenSOAP::MsgAttrHandler& msgAttrHndl, 
          bool& itIsMsgID,
          std::string& msgId);

/*
  check sync or async, and ttl info
 */
extern
bool
OPENSOAP_API
getAsyncInfoFromHeader(OpenSOAP::MsgAttrHandler& msgAttrHndl,
		       bool& asyncHeaderExist, bool& isAsync);

/*
  check forward
*/
extern
bool
OPENSOAP_API
getFwdInfoFromHeader(OpenSOAP::MsgAttrHandler& msgAttrHndl,
		     bool& fwdPathExist, std::vector<std::string>& fwdPath,
		     bool& hopCountExist, unsigned int& hopCount);
/*
  Received Path
*/
bool
OPENSOAP_API
getReceivedPathInfo(const std::string& soapMsg, 
		    bool& receivedPathExist, 
		    std::vector<std::string>& receivedPath);

/*
  check TTL
 */
extern
bool
OPENSOAP_API
getTTLInfo(OpenSOAP::MsgAttrHandler& msgAttrHndl,
	   bool& ttlHeaderExist, unsigned int& ttl, std::string& ttlType);

// get message ID from SOAPmessage
extern
std::string
OPENSOAP_API
getMsgIDContent(OpenSOAP::MsgAttrHandler& msgAttrHndl);

//create fault message for timeout
extern
std::string
OPENSOAP_API
makeTimeoutMessage();

//create fault message for operation not exist
extern
std::string
OPENSOAP_API
makeOperationNotFoundMessage();

//create fault message for invalid request message
extern
std::string
OPENSOAP_API
makeInvalidRequestMessage();

//----- create Falut Messgage of No Entry -----//
extern
std::string
OPENSOAP_API
makeNoEntryMessage();

//----- create Falut Messgage in the case of -----//
//----- Response from Serive is Empty        -----//
std::string
OPENSOAP_API
makeResponseIsEmptyMessage();

//----- create Falut Messgage for limit size over -----//
extern
std::string
OPENSOAP_API
makeLimitSizeOverMessage(long limitSize);

extern
std::string
OPENSOAP_API
makeFaultMessage(std::string& faultcode, std::string& faultstring);


#if 0
//----- attach message ID into request SOAP message ----------//
//----- create response SOAPMessage contained message ID -----//
//----- use in MsgDrv ----------------------------------------//
extern
bool
OPENSOAP_API
attachMsgID(OpenSOAP::MsgAttrHandler& msgAttrHndl,
            const std::string& fileIdOfRequestSoapMsg,
            const std::string& operationName,
            std::string& response);
#endif //if 0

//----- attach message ID into SOAP message -----//
//----- return message ID -----//
//----- use in SrvDrv -----//
extern
std::string
OPENSOAP_API
attachOrUpdateMsgID(std::string& soapMsg, std::string msgIDContent);


//----- attach backward path to response message,                 -----//
//----- if this server is the final receiver of async fwd message -----//
//----- This is used by SrvDrv -----//
extern
bool
OPENSOAP_API
attachBackwardPathToResponse(std::string& msgFromService, 
                             OpenSOAP::MsgAttrHandler& );
//const std::string& msgFromMsgDrv);

//----- attach ttl to response message,                 -----//
//----- if this server is the final receiver of async fwd message -----//
//----- This is used by SrvDrv -----//
extern
bool
OPENSOAP_API
attachTTLToResponse(std::string& msgFromService, 
		    const std::string& msgFromMsgDrv);

//attach received_path into SOAP message
extern
bool
OPENSOAP_API
attachReceivedPath(std::string& soapMsg, 
		   const std::string& url);

//attach or update backward_path SOAPmessage
extern
bool
OPENSOAP_API
updateBackwardPath(std::string& soapMsg, 
		   const std::string& backwardURL);

//attach message ID
//use service
extern
bool
OPENSOAP_API
attachMsgID(std::string& soapMsg, const std::string& msgIDContent);


//check response message
// use MsgDrv, Forwarder
extern
bool
OPENSOAP_API
isItResponseMessage(OpenSOAP::MsgAttrHandler& msgAttrHndl);

//----- attach header for Service communication -----//
std::string
OPENSOAP_API
attachHeaderForService(const std::string& soapMsg,
		       const std::string charEnc = "");

//----- remove header for Service communication -----//
std::string
OPENSOAP_API
removeHeaderForService(std::string& soapMsg);

//----- return hostname with FQDN -----//
std::string
OPENSOAP_API
getLocalhostName();

//----- check tag undelete ------//
bool
OPENSOAP_API
hasUndeleteTag(OpenSOAP::MsgAttrHandler& msgAttrHndl);

//----- convert enc -----//
int
OPENSOAP_API
convertEncoding(std::string& soapMsg, 
		const std::string& fromEnc, 
		const std::string& toEnc);

#if 0 //del 2003/06/30
//---- recv message from CGI stdin
int
OPENSOAP_API
recvStdinCGI(std::string& soapMsg);
#endif

//----- convert enc -----//
int
OPENSOAP_API
convertEncoding(OpenSOAPByteArrayPtr fromStr,
		const std::string& fromEnc, 
		const std::string& toEnc,
		std::string& soapMsg);

//----- get charset for convert enc ------//
bool
OPENSOAP_API
getEncodingCharset(const std::string& soapMsg, 
		   std::string& fromEnc, 
		   //std::string& toEnc,
		   bool forCGI = false);

//----- replace xml encoding charset -----//
bool
OPENSOAP_API
replaceXmlEncoding(std::string& soapMsg, const std::string& toEnc);

std::string
OPENSOAP_API
createResponseSoapMsgAsResult(const std::string& result);

std::string
OPENSOAP_API
createResponseSoapMsgAsMessageId(const std::string& messageId);

int
OPENSOAP_API
createResponseEnvelopeAsResult(const std::string& result, 
                               OpenSOAPEnvelopePtr& request);

int
OPENSOAP_API
createResponseEnvelopeAsMessageId(const std::string& messageId, 
                                  OpenSOAPEnvelopePtr& request);

int
OPENSOAP_API
convertEnvelopeToString(OpenSOAPEnvelopePtr& request, std::string& response,
                        const char* charEnc);

#endif /* SOAP_MESSAGEFUNC_H */
