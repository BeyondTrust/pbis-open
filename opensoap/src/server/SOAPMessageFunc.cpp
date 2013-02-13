/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SOAPMessageFunc.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#include <io.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <netdb.h>
#endif

#include <sys/time.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <stdexcept>

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

#include "SOAPMessageFunc.h"
#include "MsgAttrHandler.h"
//#include "SOAPException.h"
#include "XmlModifier.h"
#include "StringUtil.h"
#include "ServerCommon.h"
#include "SoapDef.h"
#include "AppLogger.h"

#include <OpenSOAP/Envelope.h>
#include <OpenSOAP/String.h>

using namespace OpenSOAP;
using namespace std;

//#define DEBUG

/* error message macro */
/*
#define ERROR_MSG(error, message); \
fprintf(stderr, \
		"%s: Cannot %s\n" \
		" ---> OpenSOAP Error Code: 0x%04x\n", \
                "SOAPException::toFaultMessage", \
		(message),\
		(error));
*/
/* error return macro */
/*
#define ERROR_RETURN(error, message); \
if (OPENSOAP_FAILED((error))) { \
	ERROR_MSG(error, message) \
	return (error); \
}
*/

static
int
ERROR_RETURN(int err, const char* msg)
{
    if (OPENSOAP_FAILED(err)) {
        AppLogger::Write(APLOG_ERROR,"%s error. code=(%x)\n", msg, err);
    }
    return err;
}

static
void
PrintEnvelope(OpenSOAPEnvelopePtr env,
              const char *label,
              const char *charEnc) 
{
    OpenSOAPByteArrayPtr envBuf = NULL;
    const unsigned char *envBeg = NULL;
    size_t envSz = 0;
    
    OpenSOAPByteArrayCreate(&envBuf);
    OpenSOAPEnvelopeGetCharEncodingString(env, charEnc, envBuf);
    OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
    
    fprintf(stderr, "\n=== %s envelope begin ===\n", label);
    fwrite(envBeg, 1, envSz, stderr);
    fprintf(stderr, "\n=== %s envelope  end  ===\n", label);
    
    OpenSOAPByteArrayRelease(envBuf);
}

string OPENSOAP_API
createSOAPFaultMessage(string faultcodeStr, string faultstringStr,
			string faultactorStr, string detailStr)
{
    static char METHOD_LABEL[] = "createSOAPFaultMessage: ";

    static string SOAP_VERSION("1.1");
    static const string SOAP_VERSION_1_1("1.1");
    static const string SOAP_VERSION_1_2("1.2");

    static const char NAMESPACE_PREFIX_1_1[] = "SOAP-ENV";
    static const char NAMESPACE_PREFIX_1_2[] = "env";

    string NAMESPACE_PREFIX = 
        ((SOAP_VERSION == SOAP_VERSION_1_1)? NAMESPACE_PREFIX_1_1 :
         (SOAP_VERSION == SOAP_VERSION_1_2)? NAMESPACE_PREFIX_1_2 : "");
        
    int error = OPENSOAP_NO_ERROR;

    /* create envelope */
    OpenSOAPEnvelopePtr response = NULL;
    
    error = OpenSOAPEnvelopeCreateMB(SOAP_VERSION.c_str(),
                                     NAMESPACE_PREFIX.c_str(),
                                     //NULL, 
                                     &response);
    ERROR_RETURN(error, "create envelope");
	
    /* add body response block */
    OpenSOAPBlockPtr body = NULL;
    string tagFault(NAMESPACE_PREFIX);
    tagFault += ":Fault";
    error = OpenSOAPEnvelopeAddBodyBlockMB(response, tagFault.c_str(), &body);
    ERROR_RETURN(error, "add body part");
	
    /* faultcode */
    OpenSOAPStringPtr faultcodeOssp = NULL;

    string faultCode(NAMESPACE_PREFIX);
    faultCode += ":";
    faultCode += faultcodeStr;
    error = OpenSOAPStringCreateWithMB(faultCode.c_str(), &faultcodeOssp);
    ERROR_RETURN(error, "create string: faultcode");
	
    error = OpenSOAPBlockSetChildValueMB(body,
                                         "faultcode",
                                         "string",
                                         &faultcodeOssp);
    ERROR_RETURN(error, "set parameter: faultcode");

    error = OpenSOAPStringRelease(faultcodeOssp);
    ERROR_RETURN(error, "release string: faultcode");

    /* faultstring */
    OpenSOAPStringPtr faultstringOssp = NULL;

    error = OpenSOAPStringCreateWithMB(faultstringStr.c_str(),
                                       &faultstringOssp);
    ERROR_RETURN(error, "create string: faultstring");
	
    error = OpenSOAPBlockSetChildValueMB(body,
                                         "faultstring",
                                         "string",
                                         &faultstringOssp);
    ERROR_RETURN(error, "set parameter: faultstring");

    error = OpenSOAPStringRelease(faultstringOssp);
    ERROR_RETURN(error, "release string: faultstring");

    /* faultactor */
    if (!faultactorStr.empty()) {
        OpenSOAPStringPtr faultactorOssp = NULL;

        error = OpenSOAPStringCreateWithMB(faultactorStr.c_str(), 
                                           &faultactorOssp);
        ERROR_RETURN(error, "create string: faultactor");
        
        error = OpenSOAPBlockSetChildValueMB(body,
                                             "faultactor",
                                             "string",
                                             &faultactorOssp);
        ERROR_RETURN(error, "set parameter: faultactor");

        error = OpenSOAPStringRelease(faultactorOssp);
        ERROR_RETURN(error, "release string: faultactor");
    }
	   
    /* detail */
    if (!detailStr.empty()) {
        OpenSOAPStringPtr detailOssp = NULL;
        
        error = OpenSOAPStringCreateWithMB(detailStr.c_str(), &detailOssp);
        ERROR_RETURN(error, "create string: detail");
        
        error = OpenSOAPBlockSetChildValueMB(body,
                                             "detail",
                                             "string",
                                             &detailOssp);
        ERROR_RETURN(error, "set parameter: detail");

        error = OpenSOAPStringRelease(detailOssp);
        ERROR_RETURN(error, "release string: detail");
    }

    PrintEnvelope(response, "response", NULL);

    /* convert OpenSOAPEnvelope -> string */

    OpenSOAPByteArrayPtr bArray = NULL;
    error = OpenSOAPByteArrayCreate(&bArray);
    ERROR_RETURN(error, "create bytearray");
    
    error = OpenSOAPEnvelopeGetCharEncodingString(response,
                                                  NULL,
                                                  bArray);
    ERROR_RETURN(error, "get charencoding string");

    const unsigned char *envBeg = NULL;
    size_t envSz = 0;
    error = OpenSOAPByteArrayGetBeginSizeConst(bArray, &envBeg, &envSz);
    ERROR_RETURN(error, "get byte array begin size");

    string faultMessage((const char*)envBeg, envSz);

    error = OpenSOAPByteArrayRelease(bArray);
    ERROR_RETURN(error, "release bytearray");

    error = OpenSOAPEnvelopeRelease(response);
    ERROR_RETURN(error, "release envelope");

    return faultMessage;
}

/*
  in: 
  ret: bool, exist operation
  throw: SOAPMsgIsInvalidException
 */
bool OPENSOAP_API
extractOperation(MsgAttrHandler& msgAttrHndl,
                 string& operationName,
                 string* nsPtr) 
{
    string query;
    vector<string> values;
    
    // get operation name
    query = "/Envelope/Body/?";
    if (0 > msgAttrHndl.queryXml(query, values)) {
        AppLogger::Write(APLOG_ERROR,"extractOperation: operation not exists.");
        return false;
    }
  
#ifdef DEBUG
    cerr << "SOAPMessageFunc::isItMsgID:values[0] = " << values[0] << endl;
#endif
    operationName = values[0];
    values.clear();

    // added 2002.03.11
    // get namespace
    if (nsPtr) {
        query = "/Envelope/Body/" + operationName +  ",xmlns=?";
        if (0 > msgAttrHndl.queryXml(query, values)) {
            AppLogger::Write(APLOG_ERROR
                            ,"extractOperation: namespace not set.");
        }
        else {
            *nsPtr = values[0];
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG5,"%s[%s] %s=[%s]"
                            ,"extractOperation: operation",operationName.c_str()
                            ,"namespace",(*nsPtr).c_str());
#endif
        }
        values.clear();
    }
    return true;
}// end of extractOperationAndParam()


/*
  check Messaage ID
 */
bool OPENSOAP_API
isItMsgID(MsgAttrHandler& msgAttrHndl, bool& itIsMsgID, string& msgId)
{
    itIsMsgID = false;
    
    string query;
    vector<string> values;

    query = "/Envelope/Header/opensoap-header-block/message_id=?";
    //not exists message_id return isItMsgID is false
    if (0 > msgAttrHndl.queryXml(query, values)) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s%s"
                        ,"<Envelope><Header><opensoap-header-block><message_id>"
                        ," does not contain.");
#endif //DEBUG
        return true;
    }
    msgId = values[0];
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"<Envelope><Header><opensoap-header-block><message_id>"
                    ,msgId.c_str());
#endif
    
    // 2003/06/17 <message_id> tag exists but no value...invalid message?
    if (msgId.empty()) {
        AppLogger::Write(APLOG_DEBUG5,"<message_id> tag exist but no value.");
        return false;
    }
    values.clear();

    // modified check logic
    // old check style is <Body/> with no value 

    // <response_msg>
    // <message_id> exists and <async> not contain -> isItMsg ture
    query = "/Envelope/Header/opensoap-header-block/async=?";
    if (0 > msgAttrHndl.queryXml(query, values)) {
        itIsMsgID = true;
        return true;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"SOAPMessageFunc::isItMsgID:values[0]",values[0].c_str());
#endif

    if (values[0].empty()) {
        AppLogger::Write(APLOG_ERROR,"<async> tag exist but no value.");
    }
    else if (values[0] == "false") {
        // if has <message_id> and <ture> is false
        // it's messageID, return true
        itIsMsgID = true;
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"=====  isItMsgID() end =====");
#endif

    return true;
}

/*
 check sync or async
 */
bool OPENSOAP_API
getAsyncInfoFromHeader(MsgAttrHandler& msgAttrHndl,
		       bool& asyncHeaderExist, bool& isAsync)
{
  asyncHeaderExist = false;
  isAsync = false;
  
  string query("/Envelope/Header/opensoap-header-block/async=?");
  vector<string> values;
  if (0 > msgAttrHndl.queryXml(query, values)) {
    isAsync = false;
    return true;
  }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s%s"
                    ,"SOAPMessageFunc::getAsyncInfoFromHeader:values[0]"
                    ,values[0].c_str());
#endif

  if (values[0] == "true") {
    asyncHeaderExist = true;
    isAsync = true;
  }
  else if (values[0] == "false") {
    asyncHeaderExist = true;
    isAsync = false;
  }
  else {
    return true;
  }

  return true;
}// end of getAsyncInfoFromHeader()

/*
  check forward
 */
bool OPENSOAP_API
getFwdInfoFromHeader(MsgAttrHandler& msgAttrHndl,
		     bool& fwdPathExist, vector<string>& fwdPath,
		     bool& hopCountExist, unsigned int& hopCount)
{
    fwdPathExist = false;
    hopCountExist = false;
    fwdPath.clear();

    string query;
    vector<string> values;
    vector<string>::iterator iter;

    query = "/Envelope/Header/opensoap-header-block/forwarder/forward_path=??";  
    if (0 <= msgAttrHndl.queryXml(query, values)) {
        fwdPathExist = true;
        for (iter = values.begin(); iter != values.end(); iter++) {
            fwdPath.push_back(*iter);
        }
    }
  
    values.clear();
    query = "/Envelope/Header/opensoap-header-block/forwarder/hopcount=?";  
    if (0 <= msgAttrHndl.queryXml(query, values)) {
        hopCountExist = true;    
        hopCount = atoi(values[0].c_str());
    }
    
    return true;
}// end of getFwdInfoFromHeader()

/*
   check Received Path
 */
bool OPENSOAP_API
getReceivedPathInfo(const string& soapMsg, 
		    bool& receivedPathExist, 
		    vector<string>& receivedPath) 
{
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"=====  getReceivedInfo() begin =====");
#endif

  receivedPathExist = false;
  receivedPath.clear();
  
  string query;
  MsgAttrHandler msgAttrHndl;
  msgAttrHndl.setMsg(soapMsg);
  //MsgAttrHandler msgAttrHndl(soapMsg);
  vector<string> values;
  vector<string>::iterator iter;
  
  query = "/Envelope/Header/opensoap-header-block/received_path/url=??";  
  if (0 <= msgAttrHndl.queryXml(query, values)) {
    receivedPathExist = true;
    for (iter = values.begin(); iter != values.end(); iter++) {
      receivedPath.push_back(*iter);
      AppLogger::Write(APLOG_DEBUG,"%s=[%s]"
                      ,"SOAPMessageFunc::getReceivedInfo:values",(*iter).c_str());
    }
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"=====  getReceivedInfo() end =====");
#endif

  return true;
}

/*
  check TTL
 */
bool OPENSOAP_API
getTTLInfo(MsgAttrHandler& msgAttrHndl,
	   bool& ttlHeaderExist, unsigned int& ttl, string& ttlType)
{
    ttlHeaderExist = false;
    string query;
    vector<string> values;
    unsigned int retTTL = 0;
    
    query = "/Envelope/Header/opensoap-header-block/ttl=?";
    if (0 > msgAttrHndl.queryXml(query, values)) {
	//edit 2002.03.18
        //ttl = 0;
        ttlType = "";
        ttlHeaderExist = false;
        return true;
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"SOAPMessageFunc::getTTLInfo: values[0]",values[0].c_str());
#endif

    retTTL = atoi(values[0].c_str());
    ttlHeaderExist = true;
    values.clear();
  
    query = "/Envelope/Header/opensoap-header-block/ttl,type=?";
    //query = "/Envelope/Header/opensoap-header-block/ttl=?";
    if (0 > msgAttrHndl.queryXml(query, values)) {
        //ttl = 0;
        ttlType = "";
        //ttlHeaderExist = false;
        return true;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"SOAPMessageFunc::getTTLInfo: values[0]",values[0].c_str());
#endif

    if (values[0] == "async-second") {
        ttl = retTTL;
        ttlHeaderExist = true;
    }
    return true;
}// end of getTTLInfo()


//create fault message for timeout
string OPENSOAP_API
makeTimeoutMessage()
{
  
  string faultcodeStr("SOAP-ENV:Server.Timeout");
  string faultstringStr("Time Out occured in Server");
  string faultactorStr("");

  string serverName;
  size_t len = 128;  
  char* hostname = new char[len];
  int ret = gethostname(hostname, len);
  if (ret < 0) {
    serverName = "localhost";
  }
  else {
    hostent* hostEntPtr;
    hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif
    serverName = hostEntPtr->h_name;
  }
  delete[] hostname;
  
  string official_hostname = serverName;
  
  string detailStr = string("This FAULT is created by ") 
    + official_hostname + string(".");
  
  string soapMsgToIf
    = createSOAPFaultMessage(faultcodeStr, faultstringStr,
			     faultactorStr, detailStr);
  
  return soapMsgToIf;
}


//create fault message for operation not exist
string OPENSOAP_API
makeOperationNotFoundMessage()
{
  string faultcodeStr("SOAP-ENV:Client.OperationNotFound");
  string faultstringStr("Request Operation is not Treated in Server");
  string faultactorStr("");
  
  string serverName;
  size_t len = 128;  
  char* hostname = new char[len];
  int ret = gethostname(hostname, len);
  if (ret < 0) {
    serverName = "localhost";
  }
  else {
    hostent* hostEntPtr;
    hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif
    serverName = hostEntPtr->h_name;
  }
  delete[] hostname;
  
  string official_hostname = serverName;
  
  string detailStr = string("This FAULT is created by ") 
    + official_hostname + string(".");
  
  string soapMsgToIf
    = createSOAPFaultMessage(faultcodeStr, faultstringStr,
			     faultactorStr, detailStr);
  return soapMsgToIf;
}

//create fault message for invalid request message
string OPENSOAP_API
makeInvalidRequestMessage()
{
  string faultcodeStr("SOAP-ENV:Client.RequestInvalid");
  string faultstringStr("Request from Client is Invalid");
  string faultactorStr("");
  
  string serverName;
  size_t len = 128;  
  char* hostname = new char[len];
  int ret = gethostname(hostname, len);
  if (ret < 0) {
    serverName = "localhost";
  }
  else {
    hostent* hostEntPtr;
    hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif
    serverName = hostEntPtr->h_name;
  }
  delete[] hostname;
  
  string official_hostname = serverName;
  
  string detailStr = string("This FAULT is created by ") 
    + official_hostname + string(".");
  
  string soapMsgToIf
    = createSOAPFaultMessage(faultcodeStr, faultstringStr,
			     faultactorStr, detailStr);
  return soapMsgToIf;
}

//----- create Falut Messgage of No Entry -----//
string OPENSOAP_API
makeNoEntryMessage()
{
    string faultcodeStr("SOAP-ENV:Server.NoEntry");
    string faultstringStr("No Corresponding Entry in Response Spool");
    string faultactorStr("");
    
    string serverName;
    size_t len = 128;  
    char* hostname = new char[len];
    int ret = gethostname(hostname, len);
    if (ret < 0) {
        serverName = "localhost";
    }
    else {
        hostent* hostEntPtr;
        hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif
        serverName = hostEntPtr->h_name;
    }
    delete[] hostname;
    
    string official_hostname = serverName;
    
    string detailStr = string("This FAULT is created by ") 
        + official_hostname + string(".");
    
    string soapMsgToIf
        = createSOAPFaultMessage(faultcodeStr, faultstringStr,
                                 faultactorStr, detailStr);
    return soapMsgToIf;
}

//----- create Falut Messgage in the case of -----//
//----- Response from Serive is Empty        -----//
string OPENSOAP_API
makeResponseIsEmptyMessage()
{
  string faultcodeStr("SOAP-ENV:Server.ResponseIsEmpty");
  string faultstringStr("Response from Service is Empty");
  string faultactorStr("");
  
  string serverName;
  size_t len = 128;  
  char* hostname = new char[len];
  int ret = gethostname(hostname, len);
  if (ret < 0) {
    serverName = "localhost";
  }
  else {
    hostent* hostEntPtr;
    hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif
    serverName = hostEntPtr->h_name;
  }
  delete[] hostname;
  
  string official_hostname = serverName;
  
  string detailStr = string("This FAULT is created by ") 
    + official_hostname + string(".");
  
  string soapMsgToIf
    = createSOAPFaultMessage(faultcodeStr, faultstringStr,
			     faultactorStr, detailStr);
  return soapMsgToIf;
}

//create fault message for limit size over
string OPENSOAP_API
makeLimitSizeOverMessage(long limitSize)
{
  string unitStr;
  if (limitSize >= 1024L) {
    unitStr = "KB";
    limitSize /= 1024L;

    if (limitSize >= 1024L) {
      unitStr = "MB";
      limitSize /= 1024L;
    }
  }
  string faultcodeStr("SOAP-ENV:Server.LimitSizeOver");
  string faultstringStr("Message Size over Limit ");
  faultstringStr += StringUtil::toString(limitSize);
  faultstringStr += unitStr;
  string faultactorStr("");

  string serverName;
  size_t len = 128;  
  char* hostname = new char[len];
  int ret = gethostname(hostname, len);
  if (ret < 0) {
    serverName = "localhost";
  }
  else {
    hostent* hostEntPtr;
    hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif
    serverName = hostEntPtr->h_name;
  }
  delete[] hostname;
  
  string official_hostname = serverName;
  
  string detailStr = string("This FAULT is created by ") 
    + official_hostname + string(".");
  
  string soapMsgToIf
    = createSOAPFaultMessage(faultcodeStr, faultstringStr,
			     faultactorStr, detailStr);
  
  return soapMsgToIf;
}

//create fault message
string OPENSOAP_API
makeFaultMessage(string& faultcode, string& faultstring)
{
    string faultactorStr("");
  
    string serverName;
    size_t len = 128;  
    char* hostname = new char[len];
    int ret = gethostname(hostname, len);
    if (ret < 0) {
        serverName = "localhost";
    }
    else {
        hostent* hostEntPtr;
        hostEntPtr = gethostbyname(hostname);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif
        serverName = hostEntPtr->h_name;
    }
    delete[] hostname;
    
    string official_hostname = serverName;
    
    string detailStr = string("This FAULT is created by ") 
        + official_hostname + string(".");
    
    string soapMsgToIf
        = createSOAPFaultMessage(faultcode, faultstring,
                                 faultactorStr, detailStr);
    return soapMsgToIf;
}

#if 0
//----- attach message ID into request SOAP message ----------//
//----- create response SOAPMessage contained message ID -----//
//----- use in MsgDrv ----------------------------------------//
bool OPENSOAP_API
attachMsgID(MsgAttrHandler& msgAttrHndl,
            const string& fileIdOfRequestSoapMsg,
            const string& operationName,
            string& response)
{
    //for return string
    //string responseContainedMessageId;

    //check current request message contained <message_id>
    string msgIDContent = getMsgIDContent(msgAttrHndl);

    // already message_id exist 
    // so not create new message_id and not attach to request messsage
    // only create message_id contained response message
    if (msgIDContent.empty()) {
        //
        //create new <message_id>
        //
        string dot(".");
  
        string serverName = getLocalhostName();
        
        //use fileIDOfMsgFromIf for message received timestamp 
        // fileId format is yyyymmddhhmmssxxxxx(=timestamp + 5byte sequence)
        //CAUTION!! if changed create fileID logic, modify this...
        string receivedDateTime = fileIdOfRequestSoapMsg;
  
        msgIDContent = operationName + dot 
            + serverName + dot + receivedDateTime;
  
        // attach messageID
        xmlDocPtr soapMsgDoc = NULL;
        xmlNsPtr ns;
        xmlNodePtr node1, node2;
        
        /* build an XML tree from a string */
        // need modified use API 
        
        const string& requestSoapMsg = msgAttrHndl.getMsg();
        char *soapMsgChar = new char[requestSoapMsg.length()+1];
        memcpy (soapMsgChar, requestSoapMsg.c_str(), requestSoapMsg.length());
        soapMsgChar[requestSoapMsg.length()] = 0x00;
        
        soapMsgDoc = xmlParseMemory(soapMsgChar, requestSoapMsg.length());
        if (soapMsgDoc == NULL) {
            cerr << "attachMsgID: parsing soapMsg = [" << soapMsgChar
                 << "] failed." << endl;
            delete[] soapMsgChar;
            return false;
        }
        delete[] soapMsgChar;

        /* check document (root node) */
        node1 = xmlDocGetRootElement(soapMsgDoc);
        if (node1 == NULL) {
            //fprintf(stderr, "empty document\n");
            cerr << "attachMsgID: empty document" << endl;
            xmlFreeDoc(soapMsgDoc);
            return false;
        }
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s=[%s]","node1->name",node1->name);
#endif

        /* check namespace */
        ns = xmlSearchNsByHref(soapMsgDoc, node1,
                               (xmlChar*)
                               ("http://schemas.xmlsoap.org/soap/envelope/"));
        if (ns == NULL) {
            //fprintf(stderr, "wrong namespace of Envelope\n");
            cerr << "attachMsgID: wrong namespace of Envelope" << endl;
            xmlFreeDoc(soapMsgDoc);
            return false;
        }
        
        /* check document type (root node) */
        if (strcmp((const char*)(node1->name), "Envelope") != 0) {
            //fprintf(stderr, "wrong document type");
            cerr << "attachMsgID: wrong document type" << endl;
            xmlFreeDoc(soapMsgDoc);
            return false;
        }
  
        node2 = node1->xmlChildrenNode;
        if (node2 == NULL) {
            cerr << "node1->xmlChildrenNode == NULL" << endl;
            return false;
        }
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s=[%s]","node2->name",node2->name);
#endif
        
        xmlNewChild(node2->xmlChildrenNode, NULL, (xmlChar*)("message_id"),
                    (xmlChar*)(msgIDContent.c_str()));
        

        /* free memory */
        xmlFreeDoc(soapMsgDoc);

        /* output doc tree */
        xmlChar* mem_ptr;
        int size;
        
        //update requestSoapMsg attached message_id
        xmlDocDumpMemory(soapMsgDoc, &mem_ptr, &size);
        msgAttrHndl.setMsg(string((char*)mem_ptr, size));
        
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                        ,"attachMsgID:New attached requestSoapMsg"
                        ,msgAttrHndl.getMsg().c_str());
#endif

    } //case: request not contained message_id 
    
    //
    //create message ID for response to TransI/F
    //
    LIBXML_TEST_VERSION xmlKeepBlanksDefault(0);
    xmlDocPtr msgIDDoc;
    xmlAttrPtr attr1, attr2;
    xmlNodePtr body;
    xmlNsPtr soapEnvNs;
  
    /* make xml element */ 
    msgIDDoc = xmlNewDoc((xmlChar*)("1.0"));
    attr1 = xmlNewDocProp(msgIDDoc, (xmlChar*)("encoding"), 
                          (xmlChar*)("UTF-8")); /* ??? */
    attr2 = xmlNewDocProp(msgIDDoc, (xmlChar*)("standalone"), 
                          (xmlChar*)("no")); /* ??? */
  
    /* make Envelope element */
    msgIDDoc->children = xmlNewDocNode(msgIDDoc, NULL, 
                                       (xmlChar*)("Envelope"), NULL);
    xmlSetProp(msgIDDoc->children, (xmlChar*)("encodingStyle"),
               (xmlChar*)("http://schemas.xml.soap.org/soap/encoding/"));
    soapEnvNs = xmlNewNs(msgIDDoc->children,
                         (xmlChar*)("http://schemas.xmlsoap.org/soap/envelope/"),
                         (xmlChar*)("SOAP-ENV"));
    xmlSetNs(msgIDDoc->children, soapEnvNs);

    /* make Header element */
    xmlNodePtr header;
    header = xmlNewChild(msgIDDoc->children, NULL, (xmlChar*)("Header"), NULL);
    
    xmlNodePtr extBlock 
        = xmlNewChild(header, NULL, (xmlChar*)("opensoap-header-block"), NULL);
    xmlNsPtr opensoapExtNs 
        = xmlNewNs(extBlock, (xmlChar*)("http://header.opensoap.jp/1.0/"),
                   (xmlChar*)("opensoap-header"));
    xmlSetNs(extBlock, opensoapExtNs);
    
    xmlNewChild(extBlock, NULL, (xmlChar*)("message_id"),
                (xmlChar*)(msgIDContent.c_str()));
    
    /* make Body element */
    body = xmlNewChild(msgIDDoc->children, NULL, (xmlChar*)("Body"), NULL);
    xmlSetNs(body, soapEnvNs);
    
    /* 2003/06/12 */
    xmlNewChild(body, NULL, (xmlChar*)("Result"),//("Async"),
                (xmlChar*)(OpenSOAP::Result::SUCCESS.c_str()));
    /*
      xmlNewChild(body, NULL, (xmlChar*)("Response"),
      (xmlChar*)("OK"));
    */

    /* output doc tree */
    xmlChar* mem_ptr;
    int size;

    //generate new response message contained message_id
    xmlDocDumpMemory(msgIDDoc, &mem_ptr, &size);
    response = string((char*)mem_ptr, size);
    
    /* free memory */
    xmlFreeDoc(msgIDDoc);

    return true;
}
#endif //if 0

//----- attach message ID into SOAP message -----//
//----- return message ID -----//
//----- use in SrvDrv -----//
string OPENSOAP_API
attachOrUpdateMsgID(string& soapMsg, string msgIDContent)
{
    XmlModifier xmlMod(soapMsg);
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string val;
    int result;
    string newMsg;

    MsgAttrHandler msgAttrHndl(soapMsg);
    //if message ID alread exist, update messageID
    //else attach message ID
    string msgIDContentCurrent = getMsgIDContent(msgAttrHndl);
    if (msgIDContentCurrent.size() > 0) {
    }
    else {
        // add header 
        //fmt = "/Envelope/?";
        //val = "Header";
        fmt = "/Envelope/Header=?";
        val = "";
        //modified for Duplicate error : <Header>TAG
        //result = xmlMod.attach(fmt, val, NULL);
        result = xmlMod.attachNoDuplicate(fmt, val, NULL);
        if (result != 0) {
            cerr << "attach failed. code=(" << result << ")" << endl;
        }
        newMsg = xmlMod.toString();
#ifdef DEBUG
        cerr << "####################################" << endl;
        cerr << "newMsg=[" << newMsg << "]" << endl;
        cerr << "####################################" << endl;
#endif
        // add header block
        fmt = "/Envelope/Header/?";
        val = "opensoap-header-block";
        nsDef.href = "http://header.opensoap.jp/1.0/";
        nsDef.prefix = "opensoap-header";
        result = xmlMod.attach(fmt, val, &nsDef);
        if (result != 0) {
            cerr << "attach failed. code=(" << result << ")" << endl;
        }
        newMsg = xmlMod.toString();
#ifdef DEBUG
        cerr << "####################################" << endl;
        cerr << "newMsg=[" << newMsg << "]" << endl;
        cerr << "####################################" << endl;
#endif
        // add message ID
        fmt = "/Envelope/Header/opensoap-header-block/message_id=?";
        val = msgIDContent;
        result = xmlMod.attach(fmt, val, &nsDef);
        if (result != 0) {
            cerr << "attach failed. code=(" << result << ")" << endl;
        }
        newMsg = xmlMod.toString();
#ifdef DEBUG
        cerr << "####################################" << endl;
        cerr << "newMsg=[" << newMsg << "]" << endl;
        cerr << "####################################" << endl;
#endif
        soapMsg = newMsg;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"===== attachOrUpdateMsgID() end =====");
#endif

    return msgIDContent;
}


//----- attach backward path to response message,                 -----//
//----- if this server is the final receiver of async fwd message -----//
//----- This is used by SrvDrv -----//
bool OPENSOAP_API
attachBackwardPathToResponse(string& responseMsg,
    //MsgAttrHandler& responseMsgAttrHndl,
                             MsgAttrHandler& requestMsgAttrHndl)
    //string& msgFromService, 
    //const string& msgFromMsgDrv)
{
    static char METHOD_LABEL[] = "attachBackwardPathToResponse: ";

    MsgAttrHandler responseMsgAttrHndl(responseMsg);
    XmlModifier xmlMod(responseMsg);
    //XmlModifier xmlMod(responseMsgAttrHndl.getMsg());
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string val;
    int result;
  
    string query;
    vector<string> values;
  
    // check if response has backward path
    values.clear();
    query = "/Envelope/Header/opensoap-header-block/backward_path=??";
    if (0 > responseMsgAttrHndl.queryXml(query, values)) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s%s=[%s]%s",METHOD_LABEL
                        ,"query",query.c_str(),"is not found.");
#endif //DEBUG
    }
  
  // if there is no backward path in Response message,
  // add backward path of request message,
  // otherwise do nothing
  if(values.size() > 0) {
#ifdef DEBUG      
      AppLogger::Write(APLOG_DEBUG5,"%s%s",METHOD_LABEL
                      ,"Response Message already has <backward_path>.");
#endif //DEBUG
    return true;
  }
  
  // retain backward_path from Request message
  values.clear();
  query = "/Envelope/Header/opensoap-header-block/backward_path=??";
  if (0 > requestMsgAttrHndl.queryXml(query, values)) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"%s%s=[%s]%s",METHOD_LABEL
                        ,"query",query.c_str(),"is not found.");
#endif //DEBUG
  }

  // return error, if no backward_path in Request message
  if (values.size() == 0) {
      AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"Request Message has no <backward_path>.");
      return false;
  }
  
  // add backward_path
  fmt = "/Envelope/Header/opensoap-header-block/backward_path=?";
  for(vector<string>::iterator iter = values.begin();
      iter != values.end(); iter++) {
    val = *iter;
    result = xmlMod.attach(fmt, val, NULL);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=[%d]",METHOD_LABEL
                        ,"backward_path attach failed."
                        ,"code",result);
      return false;
    }
  }// loop of for(iter)


  // add <response_msg> with value of "true"
  fmt = "/Envelope/Header/opensoap-header-block/response_msg=?";
  val = "true";
  result = xmlMod.attach(fmt, val, NULL);
  if (result != 0) {
      AppLogger::Write(APLOG_ERROR,"%s%s %s=[%d]",METHOD_LABEL
                      ,"response_msg attach failed."
                      ,"code",result);
    return false;
  }
  
  responseMsgAttrHndl.setMsg(xmlMod.toString());
  //msgFromService = newMsg;

  return true;
}// end of attachBackwardPathToResponse()

//----- attach TTL to response message,                 -----//
//----- if this server is the final receiver of async fwd message -----//
//----- This is used by SrvDrv -----//
bool OPENSOAP_API
attachTTLToResponse(string& msgFromService, 
		    const string& msgFromMsgDrv)
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"===== attachTTLToResponse() begin =====");
#endif

  XmlModifier xmlMod(msgFromService);
  XmlModifier::NameSpaceDef nsDef;
  string fmt;
  string val;
  int result;
  
  string query;
  MsgAttrHandler msgAttrHndlRes;
  msgAttrHndlRes.setMsg(msgFromService);
  //MsgAttrHandler msgAttrHndlRes(msgFromService);
  MsgAttrHandler msgAttrHndlReq;
  msgAttrHndlReq.setMsg(msgFromMsgDrv);
  //MsgAttrHandler msgAttrHndlReq(msgFromMsgDrv);
  vector<string> values;
  
  // check if response has ttl
  values.clear();
  query = "/Envelope/Header/opensoap-header-block/ttl=?";
  if (0 > msgAttrHndlRes.queryXml(query, values)) {
    AppLogger::Write(APLOG_INFO,"%s%s%s"
                    ,"attachTTLToResponse:"
                    ,"<Header><opensoap-header-block><ttl>"
                    ,"not found in Response message.");
  }
  
  // if there is no ttl path in Response message,
  // add backward path of request message,
  // otherwise do nothing
  if (values.size() > 0) {
    AppLogger::Write(APLOG_INFO,"%s%s%s"
                    ,"attachTTLTo Response:"
                    ,"<Header><opensoap-header-blcok><ttl>"
                    ,"already exists in Response message.");
    return true;
  }
  
  // retain ttl from Request message
  values.clear();
  query = "/Envelope/Header/opensoap-header-block/ttl=?";
  if (0 > msgAttrHndlReq.queryXml(query, values)) {
    AppLogger::Write(APLOG_INFO,"%s%s%s"
                    ,"attachTTLToResponse:"
                    ,"<Header><opensoap-header-block><ttl>"
                    ,"not found in Request message.");
  }
  
  // if no ttl in Request message, ttl becomes defalut value
  if (values.size() <= 0) {
    return true;
  }
  
  // add ttl
  fmt = "/Envelope/Header/opensoap-header-block/ttl=?";
  val = values[0];
  result = xmlMod.attach(fmt, val, NULL);
  if (result != 0) {
    AppLogger::Write(APLOG_ERROR,"%s(%d)"
                    ,"attach <ttl> to Response failed.",result);
    return false;
  }
  
  // retain type of ttl from Request message
  values.clear();
  query = "/Envelope/Header/opensoap-header-block/ttl,type=?";
  if (0 > msgAttrHndlReq.queryXml(query, values)) {
    AppLogger::Write(APLOG_INFO,"%s%s%s"
                    ,"attachTTLToResponse:"
                    ,"<Header><opensoap-header-block><ttl type>"
                    ,"attribute not found in Request message.");
  }
  
  // if no type of ttl in Request message, type of ttl becomes defalut value
  if (values.size() > 0) {
    // add type of ttl
    fmt = "/Envelope/Header/opensoap-header-block/ttl,type=?";
    val = values[0];
    result = xmlMod.attach(fmt, val);
    if (result != 0) {
      AppLogger::Write(APLOG_INFO,"%s(%d)"
                      ,"attach <ttl type> attribute to Response failed."
                      ,result);
      return false;
    }
  }

  //change to attached message
  msgFromService = xmlMod.toString();

#ifdef DEBUG
  cerr << "####################################" << endl;
  cerr << "newMsg=[" << msgFromService << "]" << endl;
  cerr << "####################################" << endl;
#endif

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"===== attachTTLToResponse() end =====");
#endif

  return true;
}// end of attachTTLToResponse()

//----- attach received_path -----//
bool OPENSOAP_API
attachReceivedPath(string& soapMsg, 
		   const string& url)
{
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"===== attachReceivedPath() begin =====");
#endif

    string receivedPathTime;
    struct tm	logtime;
    struct timeval now_time;
    time_t now_sec;
    char	time_buf[32];
    char	ms_str[8];
    
    gettimeofday(&now_time, NULL);
    now_sec=now_time.tv_sec;
    localtime_r(&now_sec,&logtime);
    strftime(time_buf, 31, "%Y%m%d%H%M%S", &logtime);
    sprintf(ms_str, "%0.3f", 
            ((float)now_time.tv_usec/(float)1000000.0));
    strcat(time_buf, &ms_str[2]);

    string receivedDateTime = time_buf;
  
    //-----------------------------------------
    // XmlModifier
    //-----------------------------------------
    XmlModifier xmlMod(soapMsg);
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string val;
    string newMsg;
    
    int result = 0;
    // add header 
    //fmt = "/Envelope/?";
    //val = "Header";
    fmt = "/Envelope/Header=?";
    val = "";
    //modified for Duplicate error : <Header>TAG
    //result = xmlMod.attach(fmt, val, NULL);
    result = xmlMod.attachNoDuplicate(fmt, val, NULL);
    if (result != 0) {
        cerr << "attach failed. code=(" << result << ")" << endl;
    }
    newMsg = xmlMod.toString();
#ifdef DEBUG
    cerr << "####################################" << endl;
    cerr << "newMsg=[" << newMsg << "]" << endl;
    cerr << "####################################" << endl;
#endif
    // add header block
    fmt = "/Envelope/Header/?";
    val = "opensoap-header-block";
    nsDef.href = "http://header.opensoap.jp/1.0/";
    nsDef.prefix = "opensoap-header";
    result = xmlMod.attach(fmt, val, &nsDef);
    if (result != 0) {
        cerr << "attach failed. code=(" << result << ")" << endl;
    }
    newMsg = xmlMod.toString();
#ifdef DEBUG
    cerr << "####################################" << endl;
    cerr << "newMsg=[" << newMsg << "]" << endl;
    cerr << "####################################" << endl;
#endif


    fmt = "/Envelope/Header/opensoap-header-block/?";
    val = "received_path";
    
    //namespace: if NULL , inherit parent NS
    result = xmlMod.attach(fmt, val, NULL);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR,"attach failed. code=(%d)",result);
    }
    newMsg = xmlMod.toString();

#ifdef DEBUG
    cerr << "####################################" << endl;
    cerr << "newMsg=[" << newMsg << "]" << endl;
    cerr << "####################################" << endl;
#endif

    //----------------------------------
    //simple add
    //----------------------------------
    fmt = "/Envelope/Header/opensoap-header-block/received_path/url=?";
    val = url;
  
    result = xmlMod.attach(fmt, val);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR,"attach failed. code=(%d)",result);
    }
    newMsg = xmlMod.toString();
    
#ifdef DEBUG
    cerr << "####################################" << endl;
    cerr << "newMsg=[" << newMsg << "]" << endl;
    cerr << "####################################" << endl;
#endif

    //----------------------------------
    //simple add
    //----------------------------------
    fmt = "/Envelope/Header/opensoap-header-block/received_path/time=?";
    val = receivedDateTime; //fileIDOfMsgFromIf;
    
    result = xmlMod.attach(fmt, val);
    if (result != 0) {
        AppLogger::Write(APLOG_ERROR,"attach failed. code=(%d)",result);
    }
    newMsg = xmlMod.toString();

#ifdef DEBUG
    cerr << "####################################" << endl;
    cerr << "newMsg=[" << newMsg << "]" << endl;
    cerr << "####################################" << endl;
#endif
    
    soapMsg = newMsg;

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"===== attachReceivedPath() end =====");
#endif

    return true;
}

//----- attach or update backward_path  -----//
bool OPENSOAP_API
updateBackwardPath(string& soapMsg, 
		   const string& backwardURL)
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"===== updateBackwardPath() begin =====");
#endif

  //-----------------------------------------
  // XmlModifier
  //-----------------------------------------
  XmlModifier xmlMod(soapMsg);
  XmlModifier::NameSpaceDef nsDef;
  string fmt;
  string val;
  string newMsg;
  
  int result = 0;
  //----------------------------------
  //update
  //----------------------------------
  fmt = "/Envelope/Header/opensoap-header-block/backward_path=?";
  val = backwardURL;
  
  result = xmlMod.update(fmt, val);
  if (result != 0) {
    //if update target backward_path not exist, create and attach
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"attach failed. code=(%d)",result);
#endif

    fmt = "/Envelope/Header/opensoap-header-block/backward_path=?";
    val = backwardURL;
    
    //append 3rd. argument for inherit parent Namespace 2003.01.08
    result = xmlMod.attach(fmt, val, NULL);
    if (result != 0) {
      cerr << "attach failed. code=(" << result << ")" << endl;
    }
  }
  
  newMsg = xmlMod.toString();

#ifdef DEBUG
  cerr << "####################################" << endl;
  cerr << "newMsg=[" << newMsg << "]" << endl;
  cerr << "####################################" << endl;
#endif

  soapMsg = newMsg;

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"===== updateBackwardPath() end =====");
#endif

  return true;
}// end of updateBackwardPath()



//attach message ID
//use in Service
bool OPENSOAP_API
attachMsgID(string& soapMsg, const string& msgIDContent)
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"===== attachMsgID() begin =====");
#endif

  //attach message ID
  //-----------------------------------------
  // XmlModifier
  //-----------------------------------------
  XmlModifier xmlMod(soapMsg);
  XmlModifier::NameSpaceDef nsDef;
  string fmt;
  string val;
  string newMsg;
  
  int result = 0;
  //----------------------------------
  fmt = "/Envelope/?";
  val = "Header";
  

  result = xmlMod.attach(fmt, val, NULL);
  if (result != 0) {
    AppLogger::Write(APLOG_WARN,"attach failed. code=(%d)",result);
  }
  newMsg = xmlMod.toString();

#ifdef DEBUG
  cerr << "####################################" << endl;
  cerr << "newMsg=[" << newMsg << "]" << endl;
  cerr << "####################################" << endl;
#endif

  //----------------------------------
  // <opensoap-header:opensoap-header-block 
  //    xmlns:opensoap-header="http://header.opensoap.jp/1.0/" mustUnderstand="1">
  //----------------------------------
  //Namespace(new) add
  //----------------------------------
  
  fmt = "/Envelope/Header/?";
  val = "opensoap-header-block";
  nsDef.href = "http://header.opensoap.jp/1.0/";
  nsDef.prefix = "opensoap-header";
  
  result = xmlMod.attach(fmt, val, &nsDef);
  if (result != 0) {
    AppLogger::Write(APLOG_WARN,"attach failed. code=(%d)",result);
  }
  newMsg = xmlMod.toString();

#ifdef DEBUG
  cerr << "####################################" << endl;
  cerr << "newMsg=[" << newMsg << "]" << endl;
  cerr << "####################################" << endl;
#endif

  //----------------------------------
  // <opensoap-header:opensoap-header-block 
  //    xmlns:opensoap-header="http://header.opensoap.jp/1.0/" mustUnderstand="1">
  //----------------------------------
  //attributes add
  //----------------------------------
  fmt = "/Envelope/Header/opensoap-header-block,mustUnderstand=?";
  val = "1";
  
  result = xmlMod.attach(fmt, val);
  if (result != 0) {
    AppLogger::Write(APLOG_WARN,"attach failed. code=(%d)",result);
  }
  newMsg = xmlMod.toString();

#ifdef DEBUG
  cerr << "####################################" << endl;
  cerr << "newMsg=[" << newMsg << "]" << endl;
  cerr << "####################################" << endl;
#endif

  //----------------------------------
  // <opensoap-header:message_id>
  //   ServiceName.ServerName.RecieveDateTime
  // </opensoap-header:message_id>
  //----------------------------------
  //Namespace(ref) add
  //----------------------------------
  fmt = "/Envelope/Header/opensoap-header-block/message_id=?";
  val = msgIDContent;

  result = xmlMod.attach(fmt, val, NULL);
  if (result != 0) {
    AppLogger::Write(APLOG_WARN,"attach failed. code=(%d)",result);
  }
  newMsg = xmlMod.toString();

#ifdef DEBUG
  cerr << "####################################" << endl;
  cerr << "newMsg=[" << newMsg << "]" << endl;
  cerr << "####################################" << endl;
#endif

  //----------------------------------
  // <opensoap-header:response_msg>
  //   true
  // </opensoap-header:response_msg>
  //----------------------------------
  //Namespace(ref) add
  //----------------------------------
  fmt = "/Envelope/Header/opensoap-header-block/response_msg=?";
  val = "true";

  result = xmlMod.attach(fmt, val, NULL);
  if (result != 0) {
    AppLogger::Write(APLOG_WARN,"attach failed. code=(%d)",result);
  }
  newMsg = xmlMod.toString();

#ifdef DEBUG
  cerr << "####################################" << endl;
  cerr << "newMsg=[" << newMsg << "]" << endl;
  cerr << "####################################" << endl;
#endif

  soapMsg = newMsg;

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"===== attachMsgID() end =====");
#endif

  return true;
}

//get messageID
string OPENSOAP_API
getMsgIDContent(MsgAttrHandler& msgAttrHndl)
{
  string msgIDContent("");
  string query;
  vector<string> values;
  
  query = "/Envelope/Header/opensoap-header-block/message_id=?";
  if (0 > msgAttrHndl.queryXml(query, values)) {
    return msgIDContent;
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"%s%s"
                  ,"SOAPMessageFunc::getMsgIDContent:values[0]",values[0].c_str());
#endif

  msgIDContent = values[0];

  return msgIDContent;
}

// check response message
// use in MsgDrv, Forwarder
bool OPENSOAP_API
isItResponseMessage(MsgAttrHandler& msgAttrHndl)
{
    bool itIsResponseMessage = false;
    string query;
    vector<string> values;
    
    query = "/Envelope/Header/opensoap-header-block/response_msg=?";
    if (0 > msgAttrHndl.queryXml(query, values)) {
        return itIsResponseMessage;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s%s"
                    ,"SOAPMessageFunc::isItResponseMessage:values[0]"
                    ,values[0].c_str());
#endif

    if (values[0] == "true") {
        itIsResponseMessage = true;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=(%d)"
                    ,"isItResponseMessage",itIsResponseMessage);
#endif

    return itIsResponseMessage;
}// end of isItResponseMessage()

//----- attach header for Service communication -----//
string OPENSOAP_API
attachHeaderForService(const string& soapMsg,
		       const string charEnc)
{
  string newSOAPMsg;
  //vector<string> operationNames;
  string operationName;
  string endString("\r\n");

  MsgAttrHandler msgAttrHndl(soapMsg);
  extractOperation(msgAttrHndl, operationName);
  
  newSOAPMsg = string("POST ") + operationName
    + string(" HTTP/1.1") + endString;
  newSOAPMsg += string("Content-Type: text/xml; charset=")//UTF-8") 
    + (charEnc.empty() ? "UTF-8" : charEnc)
    + endString;
  newSOAPMsg += string("Content-Length: ") 
    + StringUtil::toString(soapMsg.length()) + endString;
  newSOAPMsg += endString;
  newSOAPMsg += soapMsg;
  
  return newSOAPMsg;
  
}// end of attachHeaderForService(string soapMsg)

//----- remove header for Service communication -----//
string OPENSOAP_API
removeHeaderForService(string& recvMsg)
{
  char delim = '<';
  string::iterator openBraceLoc
    = find (recvMsg.begin(), recvMsg.end (), delim);
  if (openBraceLoc == recvMsg.end()) {
    AppLogger::Write(APLOG_WARN
                    ,"HTTP Message does not contain SOAP Message !");
    cerr << "HTTP Message does not contain SOAP Message !" << endl;
    return string ("");
  }
  string soapMsg = string(openBraceLoc, recvMsg.end());
  
  return soapMsg;
}// end of removeHeaderForService (string& recvMsg)


//----- return hostname with FQDN -----//
string OPENSOAP_API
getLocalhostName() 
{
  char localhostname[128];
  int ret0 = gethostname(localhostname, sizeof(localhostname));
  if (ret0 == 0) {

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"localhostname",localhostname);
#endif

  }
  else {
    AppLogger::Write(APLOG_ERROR,"gethostname() failed !!");
  }
  string officialHostname = localhostname;
  
  hostent* hostEntPtr;
  hostEntPtr = gethostbyname(localhostname);
  if (hostEntPtr != NULL) {

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=[%s]","hostEntPtr",hostEntPtr);
#endif

    if (hostEntPtr->h_name != NULL) {

#ifdef DEBUG
      AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                    ,"hostEntPtr->h_name",hostEntPtr->h_name);
#endif

      officialHostname = hostEntPtr->h_name;
    }
  }
  
  return officialHostname;
}

// check <undelete> tag
bool OPENSOAP_API
hasUndeleteTag(MsgAttrHandler& msgAttrHndl)
{
    string query;
    vector<string> values;
    
    query = "/Envelope/Header/opensoap-header-block/undelete=?";
    if (0 > msgAttrHndl.queryXml(query, values)) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG5,"check: <undelete> tag not found.");
#endif //DEBUG
        return false;
    }
    if (values.size() > 0 && OpenSOAP::BoolString::STR_TRUE == values[0]) {
        return true;
    }
    return false;
}

// convert enc
int OPENSOAP_API
convertEncoding(string& soapMsg, 
		const string& fromEnc, 
		const string& toEnc)
{
  OpenSOAPByteArrayPtr fromStr;
  OpenSOAPByteArrayPtr toStr;
  const unsigned char* out = 0;
  int rc = OPENSOAP_NO_ERROR;

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"%s=[%s]"
                  ,"convertEncoding: soapMsg",soapMsg.c_str());
#endif

  rc = OpenSOAPByteArrayCreateWithData((const unsigned char*)soapMsg.c_str(),
				       soapMsg.length()+1,
				       &fromStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_ERROR,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayCreateWithData failed"
					,rc);
    return rc;
  }

  rc = OpenSOAPByteArrayCreate(&toStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_ERROR,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayCreate failed",rc);
    return rc;
  }
  
  rc = OpenSOAPStringConvertCharEncoding(fromEnc.c_str(),
					 fromStr,
					 toEnc.c_str(),
					 toStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_ERROR,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPStringConvertCharEncoding failed"
					,rc);
    return rc;
  }

  rc = OpenSOAPByteArrayBeginConst(toStr, &out);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_ERROR,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayBeginConst failed"
					,rc);
    return rc;
  }

#ifdef DEBUG
  cerr << "ConvStr=[" << out << "]" << endl;
#endif

  soapMsg = (const char*)out;
  
  rc = OpenSOAPByteArrayRelease(fromStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_WARN,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayRelease failed"
                    ,rc);
//    return rc;
  }

  // bugged anything...
  rc = OpenSOAPByteArrayRelease(toStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_WARN,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayRelease failed"
                    ,rc);
//    return rc;
  }

  return OPENSOAP_NO_ERROR;
}

// convert enc
int OPENSOAP_API
convertEncoding(OpenSOAPByteArrayPtr fromStr,
		const string& fromEnc, 
		const string& toEnc,
                string& soapMsg)
{
  OpenSOAPByteArrayPtr toStr;
  const unsigned char* out = 0;
  int rc = OPENSOAP_NO_ERROR;

  rc = OpenSOAPByteArrayCreate(&toStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_ERROR,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayCreate failed"
                    ,rc);
    return rc;
  }

 rc = OpenSOAPStringConvertCharEncoding(fromEnc.c_str(),
					 fromStr,
					 toEnc.c_str(),
					 toStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_ERROR,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPStringConvertCharEncoding failed"
                    ,rc);
    return rc;
  }

  rc = OpenSOAPByteArrayBeginConst(toStr, &out);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_ERROR,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayBeginConst failed"
                    ,rc);
    return rc;
  }

#ifdef DEBUG
  cerr << "ConvStr=[" << out << "]" << endl;
#endif

  soapMsg = (const char*)out;

  rc = OpenSOAPByteArrayRelease(toStr);
  if (OPENSOAP_NO_ERROR != rc) {
    AppLogger::Write(APLOG_WARN,"%s=(%d)"
                    ,"convertEncoding: OpenSOAPByteArrayRelease failed"
                    ,rc);
//    return rc;
  }

  return OPENSOAP_NO_ERROR;
}

//get charset for convert enc
bool OPENSOAP_API
getEncodingCharset(const string& soapMsg,
		   string& fromEnc, 
		   //string& toEnc,
		   bool forCGI)
{
  const string DEFAULT_ENC("UTF-8");

  // check Content-Type: charset="?"
  const string CONTENT_TYPE_KEY("Content-Type:");
  const string CHARSET_KEY("charset=");
  const char sQuot = '\'';
  const char dQuot = '"';
  char keepQuot = '\0';
  const char xmlBeginKey = '<';
  //const char xmlFirstLineEndKey = '>';
  
  //toEnc = DEFAULT_ENC;

  if (forCGI) {
    char* contentTypeEnv = 0;
    contentTypeEnv = getenv("CONTENT_TYPE");
    if (contentTypeEnv) {
      //parse charset
      string contentType(contentTypeEnv);
      int fromIdx = contentType.find(CHARSET_KEY);
      if (string::npos != fromIdx) {
	fromIdx += strlen(CHARSET_KEY.c_str());
	keepQuot = contentType.at(fromIdx);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"******** %s=[%c] ********"
                    ,"KEEP_QUOTE",keepQuot);
#endif //DEBUG

	switch(keepQuot) {
	case sQuot:
	case dQuot:
	  fromIdx += 1; // for 'or"
	  break;
	default:
	  keepQuot = ' ';
	  break;
	}

	int toIdx = contentType.find(dQuot, fromIdx);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=(%d) %s=(%d) %s=[%s]"
                    ,"From",fromIdx,"To",toIdx
                    ,"charset"
                    ,(contentType.substr(fromIdx, (toIdx - fromIdx))).c_str());
#endif
	fromEnc = contentType.substr(fromIdx, (toIdx - fromIdx));
      }
    }
    else {
      //default from_enc=UTF-8
      fromEnc = DEFAULT_ENC;
    }
  }
  else {
    int xmlBeginIdx = soapMsg.find(xmlBeginKey);
    if (string::npos == xmlBeginIdx) {
      //empty xml
      return false;
    }

    //parse charset into HttpHeader
    int fromIdx = soapMsg.find(CONTENT_TYPE_KEY);
    int toIdx = string::npos;
    if (string::npos == fromIdx || xmlBeginIdx < fromIdx) {
      // default fromEnc = "UTF-8"
      fromEnc = DEFAULT_ENC;
      AppLogger::Write(APLOG_DEBUG,"**** 1 ****");
    }
    else {
      fromIdx = soapMsg.find(CHARSET_KEY, fromIdx);
      if (string::npos == fromIdx || xmlBeginIdx < fromIdx) {
	// default fromEnc = "UTF-8"
	fromEnc = DEFAULT_ENC;
        AppLogger::Write(APLOG_DEBUG,"**** 2 ****");
      }
      else {
	fromIdx += strlen(CHARSET_KEY.c_str());
	//added 2003/06/12
	keepQuot = soapMsg.at(fromIdx);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"******** %s=[%c] ********"
                    ,"KEEP_QUOTE",keepQuot);
#endif //DEBUG

	switch(keepQuot) {
	case sQuot:
	case dQuot:
	  fromIdx += 1; // for 'or"
	  break;
	default:
	  keepQuot = ' ';
	  break;
	}
	toIdx = soapMsg.find(keepQuot, fromIdx);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG5,"%s=(%d) %s=(%d) %s=[%s]"
                    ,"From",fromIdx,"To",toIdx
                    ,"charset"
                    ,(soapMsg.substr(fromIdx, (toIdx - fromIdx))).c_str());
#endif
	fromEnc = soapMsg.substr(fromIdx, (toIdx - fromIdx));
      }
    }
  }
  return true;
}

//replace xml encoding charset
bool OPENSOAP_API
replaceXmlEncoding(string& soapMsg, const string& toEnc)
{
  const string ENCODING_KEY("encoding=");
  const char dQuot = '"';
  const char xmlBeginKey = '<';
  const char xmlFirstLineEndKey = '>';

  int xmlBeginIdx = soapMsg.find(xmlBeginKey);
  if (string::npos == xmlBeginIdx) {
    //empty xml
    return false;
  }

  //parse xml encoding
  int xmlFirstLineEndIdx = soapMsg.find(xmlFirstLineEndKey, xmlBeginIdx);
  if (string::npos == xmlFirstLineEndIdx) {
    //error
    AppLogger::Write(APLOG_ERROR
                    ,"replaceXmlEncoding: Invalid xml message from Serivce!");
    return false;
  }
  
  //rewrite encoding to "UTF-8"
  int fromIdx = soapMsg.find(ENCODING_KEY, xmlBeginIdx);
  if (string::npos != fromIdx && fromIdx < xmlFirstLineEndIdx) {
    fromIdx = soapMsg.find(dQuot, fromIdx);
    if (string::npos == fromIdx || xmlFirstLineEndIdx < fromIdx) {
      //no qQuot
      AppLogger::Write(APLOG_ERROR
          ,"replaceXmlEncoding: no double quotation marks in xml encoding");
      return false;
    }
    fromIdx += 1;
    int toIdx = soapMsg.find(dQuot, fromIdx);
    soapMsg.replace(fromIdx, (toIdx - fromIdx), toEnc);
  }

  return true;
}


string OPENSOAP_API
createResponseSoapMsgAsResult(const string& result)
{
    static char METHOD_LABEL[] = "createResponseSoapMsgAsResult: ";

    int ret = OPENSOAP_NO_ERROR;

    OpenSOAPEnvelopePtr request = NULL;
    ret = createResponseEnvelopeAsResult(result, request);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"createResponseEnvelopeAsResult failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }

    string response;
    ret = convertEnvelopeToString(request, response, NULL);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"convertEnvelopeToString failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("data convert failed");
    }

    ret = OpenSOAPEnvelopeRelease(request);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                        ,"OpenSOAPEnvelopeRelease failed"
                        ,"ErrorCode",ret);
        throw runtime_error("memory fault");
    }

    return response;
}

#if 1
string OPENSOAP_API
createResponseSoapMsgAsMessageId(const string& messageId)
{
    static char METHOD_LABEL[] = "createResponseSoapMsgAsMessageId: ";

    int ret = OPENSOAP_NO_ERROR;

    OpenSOAPEnvelopePtr request = NULL;
    ret = createResponseEnvelopeAsMessageId(messageId, request);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"createResponseEnvelopeAsMessageId failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }

    string response;
    ret = convertEnvelopeToString(request, response, NULL);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"convertEnvelopeToString failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("data convert failed");
    }

    ret = OpenSOAPEnvelopeRelease(request);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
                        ,"OpenSOAPEnvelopeRelease failed");
//        throw runtime_error("memory fault");
    }

    return response;
}
#else
string OPENSOAP_API
createResponseSoapMsgAsMessageId(const string& messageId)
{
    
}
#endif



int OPENSOAP_API
createResponseEnvelopeAsResult(const string& result, 
                               OpenSOAPEnvelopePtr& request)
{
    static char METHOD_LABEL[] = "createResponseEnvelopeAsResult: ";

    int ret = OPENSOAP_NO_ERROR;

    ret = OpenSOAPEnvelopeCreateMB("1.1", NULL, &request);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPEnvelopeCreateMB failed");
        throw runtime_error("envelope create failed");
    }

    /* add body block */
    OpenSOAPBlockPtr body = NULL;
    //string tagResult("SOAP-ENV:");
    //tagResult += OpenSOAP::ExtSoapTag::RESULT;
    ret = OpenSOAPEnvelopeAddBodyBlockMB(request, 
                                         OpenSOAP::ExtSoapTag::RESULT.c_str(),
                                         //tagResult.c_str(),
                                         &body);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPEnvelopeAddBodyBlockMB failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }

    /* set value to body block */
    OpenSOAPStringPtr osstring = NULL;
    ret = OpenSOAPStringCreateWithMB(result.c_str(), &osstring);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPStringCreateWithMB failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }

    ret = OpenSOAPBlockSetValueMB(body,
                                  "string",
                                  &osstring);

/*    
    ret = OpenSOAPBlockSetChildValueMB(
        body,
        (OpenSOAP::ExtSoapTag::RESULT).c_str(),
        "string",
        &osstring);
*/
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPBlockSetChildValueMB failed");
        OpenSOAPStringRelease(osstring);
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }
    
    ret = OpenSOAPStringRelease(osstring);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
                        ,"OpenSOAPStringRelease failed");
//        OpenSOAPEnvelopeRelease(request);
//        throw runtime_error("memory fault");
    }

    /* set namespace to request block */
    ret = OpenSOAPBlockSetNamespaceMB(
        body,
        OpenSOAP::ExtSoapNamespace::RESULT.c_str(),
        OpenSOAP::ExtSoapNamespace::RESULT_PREFIX.c_str());
                                      
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                         ,"OpenSOAPBlockSetNamespaceMB(Result) failed");
        throw runtime_error("set namespace failed");
    }

    return ret;
}

int OPENSOAP_API
createResponseEnvelopeAsMessageId(const string& messageId, 
                                  OpenSOAPEnvelopePtr& request)
{
    static char METHOD_LABEL[] = "createResponseEnvelopeAsMessageId: ";

    int ret = OPENSOAP_NO_ERROR;

    //ret = createResponseEnvelopeAsResult("SUCCESS", request);
    ret = createResponseEnvelopeAsResult(OpenSOAP::Result::SUCCESS, request);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"createResponseEnvelopeAsResult failed");
        throw runtime_error("envelope create failed");
    }

    /* add opensoap header block */
    OpenSOAPBlockPtr header = NULL;
    ret = OpenSOAPEnvelopeAddHeaderBlockMB(
        request,
        (OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK).c_str(),
        &header); 
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPEnvelopeAddHeaderBlockMB failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }
    
    /* set namespace to opensoap-header block */
    ret = OpenSOAPBlockSetNamespaceMB(
        header,
        (OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER).c_str(),
        (OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER_PREFIX).c_str());
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPBlockSetNamespaceMB failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }

    /* set message_id element to opensoap-header header */
    OpenSOAPStringPtr osstring = NULL;
    ret = OpenSOAPStringCreateWithMB(messageId.c_str(), &osstring);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPStringCreateWithMB failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }
    
    ret = OpenSOAPBlockSetChildValueMB(
        header, 
        (OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID).c_str(),
        "string", 
        &osstring);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPBlockSetChildValueMB failed");
        OpenSOAPStringRelease(osstring);
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }
    
    ret = OpenSOAPStringRelease(osstring);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
                        ,"OpenSOAPStringRelease failed");
//        OpenSOAPEnvelopeRelease(request);
//        throw runtime_error("envelope create failed");
    }
		
    /* set namespace to message_id element */
    OpenSOAPXMLElmPtr messageIdElm = NULL;
    ret = OpenSOAPBlockGetChildMB(
        header, 
        (OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID).c_str(),
        &messageIdElm);

    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPBlockGetChildMB failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }

    ret = OpenSOAPXMLElmSetNamespaceMB(
        messageIdElm,
        (OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER).c_str(),
        (OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER_PREFIX).c_str());

    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"OpenSOAPXMLElmSetNamespaceMB failed");
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("envelope create failed");
    }

    return ret;
}

int OPENSOAP_API
convertEnvelopeToString(OpenSOAPEnvelopePtr& request, string& response,
                          const char* charEnc)
{
    char METHOD_LABEL[] = "convertEnvelopeToString: ";

    int ret = OPENSOAP_NO_ERROR;

    //conver Envelope->string
    OpenSOAPByteArrayPtr envBuf = NULL;
    const unsigned char *envBeg = NULL;
    size_t envSz = 0;

    ret = OpenSOAPByteArrayCreate(&envBuf);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                ,"OpenSOAPByteArrayCreate failed","ErrorCode",ret);
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("memory fault");
    }
    ret = OpenSOAPEnvelopeGetCharEncodingString(request, 
                                                charEnc,
                                                envBuf);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                ,"OpenSOAPEnvelopeGetCharEncodingString failed"
                ,"ErrorCode",ret);
        OpenSOAPByteArrayRelease(envBuf);
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("");
    }

    ret = OpenSOAPByteArrayGetBeginSizeConst(envBuf, &envBeg, &envSz);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                ,"OpenSOAPByteArrayGetBeginSizeConst failed"
                ,"ErrorCode",ret);
        OpenSOAPByteArrayRelease(envBuf);
        OpenSOAPEnvelopeRelease(request);
        throw runtime_error("");
    }

    response = string((const char*)envBeg, envSz);

    ret = OpenSOAPByteArrayRelease(envBuf);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
                ,"OpenSOAPByteArrayRelease failed"
                ,"ErrorCode",ret);
//        throw runtime_error("memory fault");
    }

/*
    ret = OpenSOAPEnvelopeRelease(request);
    if (OPENSOAP_FAILED(ret)) {
        LOGPRINT(TAG_ERR)
            << METHOD_LABEL
            << "OpenSOAPEnvelopeRelease failed. ErrorCode=(" 
            << ret << ")" 
            << endl;
        throw runtime_error("memory fault");
    }
*/

    return ret;
}

// End of SOAPMessageFunc.cpp
