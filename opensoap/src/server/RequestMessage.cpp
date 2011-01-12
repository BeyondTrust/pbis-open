/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: RequestMessage.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <sys/types.h>

#include <stdexcept>

#include "RequestMessage.h"
#include "ServerCommon.h"
#include "DataRetainer.h"
#include "MsgAttrHandler.h"
#include "XmlModifier.h"
#include "StringUtil.h"

#include "SignatureFunc.h"

#include "SOAPMessageFunc.h"

#include "SoapDef.h"
#include "AppLogger.h"

using namespace std;
using namespace OpenSOAP;

//#define DEBUG

#ifdef DEBUG
//#include "Stopwatch.h"
#endif //DEBUG

//constant values
const int RequestMessage::DEFAULT_HOPCOUNT_VAL = -1;

//
//constructor
//
RequestMessage::RequestMessage(const string& spoolPath)
    : addedSignature_(false)
    , resultStatus_(FINISH_NOT_YET)
    , hasTTLEntry_(false)
    , hasHopcount_(false)
    , hasTTLSecond_(false)
    , hasTTLHoptimes_(false)
    , hasTTLAsyncSecond_(false)
    , modified_(false)
    , forFinalReceiver_(false)
    , forwardPathArray_(0)
    , receivedPathArray_(0)
    , receivedTimeArray_(0)
    , dataSpoolPath(spoolPath)
/*
        std::vector<std::string>* forwardPathArray_;
        std::vector<std::string>* receivedPathArray_;
        std::vector<std::string>* receivedTimeArray_;
*/
{
    forwardPathArray_ = new vector<string>;
    receivedPathArray_ = new vector<string>;
    receivedTimeArray_ = new vector<string>;
}

//
//destructor
//
RequestMessage::~RequestMessage()
{
    delete forwardPathArray_;
    delete receivedPathArray_;
    delete receivedTimeArray_;

}

//
//extract soap message from storage and set method call
//
int
RequestMessage::extractStorage(const std::string& id)
{
    static char METHOD_LABEL[] = "RequestMessage::extractStorage: ";

    int ret = OPENSOAP_NO_ERROR;
    
    //update id
    storageId_ = id;

    //convert to SOAPMessage from fileID
    DataRetainer dr(dataSpoolPath);
    dr.SetId(id);
    dr.Decompose();

    string requestStr;
    dr.GetSoapEnvelope(requestStr);
    ret = setSoapMessage(requestStr);
    
    return ret;
}

//
//store modified soap message to storage 
// ex. before push to queue
//
int
RequestMessage::storeStorage()
{
    static char METHOD_LABEL[] = "RequestMessage::storeStorage: ";

    int ret = OPENSOAP_NO_ERROR;
    //not modified, no necessity to store
    if (!modified_) {
        return ret;
    }

    //flag off
    modified_ = false;

    //convert to SOAPMessage from fileID
    DataRetainer dr(dataSpoolPath);
    dr.SetId(storageId_);
    dr.Decompose();

    dr.UpdateSoapEnvelope(soapMessage_);

    return OPENSOAP_NO_ERROR;
}

//
//init. for cleared by setSoapMessage()
//
void
RequestMessage::initMember()
{
    async_ = false;
    forwardPathArray_->clear();
    messageId_ = "";//messageId_.clear();
    hopcount_ = DEFAULT_HOPCOUNT_VAL;
    receivedPathArray_->clear();
    ttlSecond_ = 0;
    ttlAsyncSecond_ = 0;
    ttlHoptimes_ = 0;
    backwardPath_ = "";//backwardPath_.clear();
    responseMsg_ = false;
    methodName_ = "";//methodName_.clear();
    methodNamespace_ = "";//methodNamespace_.clear();
    hasHopcount_ = false;
    hasTTLSecond_ = false;
    hasTTLHoptimes_ = false;
    hasTTLAsyncSecond_ = false;
}

//
//parse soap message and element deploy to member
//
int
RequestMessage::setSoapMessage(const string& soapMsg)
{
    static char METHOD_LABEL[] = "RequestMessage::setSoapMessage: ";

    int ret = OPENSOAP_NO_ERROR;

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"soapMsg",soapMsg.c_str());
#endif //DEBUG

    //initialize data
    soapMessage_ = soapMsg;
    initMember();

    MsgAttrHandler msgAttrHndl(soapMessage_);

    //query string
    string baseQuery = "/" + OpenSOAP::SoapTag::ENVELOPE;
    string query;

    //-------------
    //check common
    //-------------
    query = baseQuery + "/??";
    vector<string> values;
    msgAttrHndl.queryXml(query, values);
    if (values.size() <= 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s %s=[%s]",METHOD_LABEL
					,"<Envelope> does not have child element."
					,"in msg_id",storageId_.c_str());
        return OPENSOAP_PARAMETER_BADVALUE;
    }
    else {
        vector<string>::iterator itr;
        bool hasBody = false;
        bool hasHeader = false;
        bool hasOther = false;
        for(itr = values.begin(); itr != values.end(); itr++) {
#ifdef DEBUG
			AppLogger::Write(APLOG_ERROR,"%s%s<%s>",METHOD_LABEL
						,"RequestMessage: <Envelope> has ",(*itr).c_str());
#endif //DEBUG
            if (*itr == OpenSOAP::SoapTag::BODY) {
                hasBody = true;
            }
            else if (*itr == OpenSOAP::SoapTag::HEADER) {
                hasHeader = true;
            }
            else {
                hasOther = true;
				AppLogger::Write(APLOG_WARN,"%s%s=[%s] %s=[%s]",METHOD_LABEL
								,"<Envelope> has unexpected child."
								,(*itr).c_str()
								,"in msg_id",storageId_.c_str());
            }
        }

        if (!hasBody) {
            //
			AppLogger::Write(APLOG_WARN,"%s%s %s=[%s]",METHOD_LABEL
							,"<Envelope> has unexpected child."
							,"in msg_id",storageId_.c_str());
        }            
        if (hasHeader) {
            //
        }            
        if (hasOther) {
            //
        }            
    }

    //-------------
    //Header parts
    //-------------
    baseQuery += "/";
    baseQuery += OpenSOAP::SoapTag::HEADER;
    baseQuery += "/";
    baseQuery += OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
    baseQuery += "/";

    //check async_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::ASYNC;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);

    if ((values.size() > 0) && (OpenSOAP::BoolString::STR_TRUE == values[0])) {
        async_ = true;
    }
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s %s=[%s] %s=[%s] %s=[%s]",METHOD_LABEL
					,"check async_:"
					,"query",query.c_str()
					,"value",(values.size()>0?values[0].c_str():"<none>")
					,"compare",(OpenSOAP::BoolString::STR_TRUE).c_str());
#endif //DEBUG

    //check forwardPathArray_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::FORWARDER;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::FORWARD_PATH;
    query += "=??";
    
    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        *forwardPathArray_ = values;
    }

    //check messageId_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        messageId_ = values[0];
    }

    //check hopcount_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::FORWARDER;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::HOPCOUNT;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        int hopCount = 0;
        StringUtil::fromString(values[0], hopCount);
        if (hopCount >= 0) {
            hasHopcount_ = true;
            hopcount_ = hopCount;
        }
        else {
            //ignore hopcount
			AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
							,"hopcount is minus value. ignore it.");
        }
    }
        
    //check receivedPathArray_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::URL;
    query += "=??";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        *receivedPathArray_ = values;
    }

    //check receivedTimeArray_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH;
    query += "/";
    query += OpenSOAP::ExtSoapHeaderTag::TIME;
    query += "=??";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        *receivedTimeArray_ = values;
    }

    //check relation of receivedPathArray_ and receivedTimeArray_
    if (receivedPathArray_->size() != receivedTimeArray_->size()) {
		AppLogger::Write(APLOG_DEBUG9,"%s%s%s",METHOD_LABEL
						,"number of <received_path> children elements "
						,"<url> and <time> is not same.");
        return OPENSOAP_MEM_OUTOFRANGE;
    }

    //check ttlSecond
    //check ttlHoptimes
    //check ttlAsyncSecond
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::TTL;
    query += "=??";

    values.clear();
    //extract ttl array
    msgAttrHndl.queryXml(query, values);
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
						,"ttl array size",values.size());
#endif //DEBUG
    if (values.size() > 0) {
        long* ttlVal = new long[values.size()];
        vector<string> typeValues;
        query = baseQuery + OpenSOAP::ExtSoapHeaderTag::TTL;
        query += ",";
        query += OpenSOAP::ExtSoapHeaderAttributes::TYPE;
        query += "=??";
        
        msgAttrHndl.queryXml(query, typeValues);
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
							,"ttl-type array size",typeValues.size());
#endif //DEBUG

        //check miss match size
        if (values.size() != typeValues.size()) {
			AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
							,"extract ttl and type failed.");
            delete[] ttlVal;
            return OPENSOAP_MEM_OUTOFRANGE;
        }

        //compare type and get each value
        for(int i=0; i<values.size();i++) {
            StringUtil::fromString(values[i], ttlVal[i]);
            if (typeValues[i] == 
                OpenSOAP::ExtSoapHeaderAttributes::SECOND) {
                
                if (ttlVal[i] >0) {
                    hasTTLSecond_ = true;
                    ttlSecond_ = ttlVal[i];
#ifdef DEBUG
					AppLogger::Write(APLOG_DEBUG9,"%s%s=(%ld)",METHOD_LABEL
								,"set ttlSecond_",ttlVal[i]);
#endif //DEBUG
                }
                else {
                    //ignore
					AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
									,"ttl second value is minus. ignore it.");
                }
            }
            else if (typeValues[i] == 
                     OpenSOAP::ExtSoapHeaderAttributes::HOPTIMES) {

                if (ttlVal[i] >0) {
                    hasTTLHoptimes_ = true;
                    ttlHoptimes_ = ttlVal[i];
#ifdef DEBUG
					AppLogger::Write(APLOG_DEBUG9,"%s%s=(%ld)",METHOD_LABEL
								,"set ttlHoptimes_",ttlVal[i]);
#endif //DEBUG
                }
                else {
                    //ignore
					AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
									,"ttl hoptimes value is minus. ignore it.");
                }
            }
            else if (typeValues[i] == 
                     OpenSOAP::ExtSoapHeaderAttributes::ASYNCSECOND) {
                
                if (ttlVal[i] >0) {
                    hasTTLAsyncSecond_ = true;
                    ttlAsyncSecond_ = ttlVal[i];
#ifdef DEBUG
					AppLogger::Write(APLOG_DEBUG9,"%s%s=(%ld)",METHOD_LABEL
								,"set ttlAsyncSecond_",ttlVal[i]);
#endif //DEBUG
                }
                else {
                    //ignore
					AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
									,"ttl second value is minus. ignore it.");
                }
            }

            delete[] ttlVal;
        }
    }

    //check backwardPathArray_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::BACKWARD_PATH;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {
        backwardPath_ = values[0];
    }

    //check responseMsg_
    query = baseQuery + OpenSOAP::ExtSoapHeaderTag::RESPONSE_MSG;
    query += "=?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0 && OpenSOAP::BoolString::STR_TRUE == values[0]) {
        responseMsg_ = true;

        //check must item
        if (messageId_.empty()) {
			AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
				,"<response_msg> contained but <message_id> not contained");
            return OPENSOAP_PARAMETER_BADVALUE;
        }
    }

    //-------------
    //Body parts
    //-------------

    baseQuery = "/";
    baseQuery += OpenSOAP::SoapTag::ENVELOPE;
    baseQuery += "/";
    baseQuery += OpenSOAP::SoapTag::BODY;
    baseQuery += "/";

    //check method
    query = baseQuery + "?";

    values.clear();
    msgAttrHndl.queryXml(query, values);
    if (values.size() > 0) {

        methodName_ = values[0];
        //check Namespace
        query = baseQuery + methodName_ + "," + OpenSOAP::XMLDef::XMLNS + "=?";

        values.clear();
        msgAttrHndl.queryXml(query, values);

        if (values.size() > 0) {
            methodNamespace_ = values[0];
        }
        else {
			AppLogger::Write(APLOG_WARN,"%s%s %s=[%s] %s=[%s]",METHOD_LABEL
							,"method namespace not found."
							,"method",methodName_.c_str()
							,"in msg_id",storageId_.c_str());
            //return OPENSOAP_PARAMETER_BADVALUE;
        }
    }
    else {
		AppLogger::Write(APLOG_WARN,"%s%s %s=[%s]",METHOD_LABEL
						,"method not found."
						,"in msg_id",storageId_.c_str());
        //return OPENSOAP_PARAMETER_BADVALUE;
    }

    return OPENSOAP_NO_ERROR;
}


int 
RequestMessage::pushReceivedPathStackToHeader(const string& myselfUrl)
{
    static char METHOD_LABEL[] = 
        "RequestMessage::pushReceivedPathStackToHeader: ";

    //flag on
    modified_ = true;

    //use storageId_ for message received timestamp 
    //id format is YYYYMMDDhhmmss + 5byte sequence (ex. 2003042410253600001)
    //CAUTION!! if changed create storageId_ logic, modify this...
    string receivedDateTime = storageId_.substr(0, strlen("YYYYMMDDhhmmss"));
    
    //-----------------------------------------
    // XmlModifier
    //-----------------------------------------
    XmlModifier xmlMod(getSoapMessage());
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string baseFmt;
    string val;
    
    int ret = OPENSOAP_NO_ERROR;

    //--------------------
    // attach <Header>
    //--------------------
    baseFmt = "/";
    baseFmt += OpenSOAP::SoapTag::ENVELOPE;
    baseFmt += "/";
    baseFmt += OpenSOAP::SoapTag::HEADER;
    fmt = baseFmt + "=?";

    val = "";
    
    //namespace: if NULL , inherit parent NS
    ret = xmlMod.attachNoDuplicate(fmt, val, NULL);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
						,"attach <Header> tag failed.","code",ret);
        return OPENSOAP_IO_WRITE_ERROR;
    }

    //--------------------
    // attach <opensoap-header-block>
    //--------------------
    baseFmt += "/";
    baseFmt += OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
    fmt = baseFmt + "=?";

    val = "";
    nsDef.href = OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER;
    nsDef.prefix = OpenSOAP::ExtSoapHeaderNamespace::OPENSOAP_HEADER_PREFIX;
  
    ret = xmlMod.attachNoDuplicate(fmt, val, &nsDef);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
				,"attach <opensoap-header-block> tag failed.","code",ret);
        return OPENSOAP_IO_WRITE_ERROR;
    }

    //--------------------
    // attach <received_path>
    //--------------------
    baseFmt += "/";
    baseFmt += OpenSOAP::ExtSoapHeaderTag::RECEIVED_PATH;
    fmt = baseFmt + "=?";
    val = "";
    
    //namespace: if NULL , inherit parent NS
    ret = xmlMod.attach(fmt, val, NULL);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
				,"attach <received_path> tag failed.","code",ret);
        return OPENSOAP_IO_WRITE_ERROR;
    }

    //----------------------------------
    //simple add
    //----------------------------------
    fmt = baseFmt + "/";
    fmt += OpenSOAP::ExtSoapHeaderTag::URL;
    fmt += "=?";
    val = myselfUrl;
    
    //ret = xmlMod.attach(fmt, val);
    ret = xmlMod.attach(fmt, val, NULL);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
				,"attach <received_path><url> tag failed.","code",ret);
        return OPENSOAP_IO_WRITE_ERROR;
    }

    //----------------------------------
    //simple add
    //----------------------------------
    fmt = baseFmt + "/";
    fmt += OpenSOAP::ExtSoapHeaderTag::TIME;
    fmt += "=?";
    val = receivedDateTime;
    
    //ret = xmlMod.attach(fmt, val);
    ret = xmlMod.attach(fmt, val, NULL);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
				,"attach <received_path><time> tag failed.","code",ret);
        return OPENSOAP_IO_WRITE_ERROR;
    }
#ifdef DEBUG
    cerr << "####################################" << endl;
    cerr << "attach <received_path> newMsg=[" 
         << xmlMod.toString() << "]" << endl;
    cerr << "####################################" << endl;

//    Stopwatch sw;
//    sw.start("reset soapMessage");
#endif

#if 1 // choise each logic ...
    ret = setSoapMessage(xmlMod.toString());
#else
    receivedPathArray_->insert(receivedPathArray_->begin(), myselfUrl);
    receivedTimeArray_->insert(receivedTimeArray_->begin(), receivedDateTime);
#endif 
    
#ifdef DEBUG
//    sw.stop();
#endif

    return ret;
}

int 
RequestMessage::popForwardPathStackFromHeader()
{
    static char METHOD_LABEL[] = 
        "RequestMessage::popForwardPathStackFromHeader: ";

    //flag on
    modified_ = true;

    // soapMessage_ and forwardPathArray_ modified.

    //delete first forward_path
    XmlModifier xmlMod(getSoapMessage());
    string delFmt = "/" + 
        OpenSOAP::SoapTag::ENVELOPE + "/" +
        OpenSOAP::SoapTag::HEADER + "/" + 
        OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK + "/" +
        OpenSOAP::ExtSoapHeaderTag::FORWARDER + "/" +
        OpenSOAP::ExtSoapHeaderTag::FORWARD_PATH + "=?";
    
    int ret = xmlMod.del(delFmt);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
						,"forward_path remove failed.");
        return OPENSOAP_IO_WRITE_ERROR;
    }

    //reset soap message
    ret = setSoapMessage(xmlMod.toString());

    //check finalreceiver
    if (forwardPathArray_->empty()) {
        forFinalReceiver_ = true;
    }

    return ret;
}

int 
RequestMessage::decrementHopcountInHeader()
{
    static char METHOD_LABEL[] = "RequestMessage::decrementHopcountInHeader: ";

    //flag on
    modified_ = true;

    // soapMessage_ and hopcount_ modified.

    //-----------------------------------------
    // XmlModifier
    //-----------------------------------------
    XmlModifier xmlMod(getSoapMessage());
    XmlModifier::NameSpaceDef nsDef;
    string fmt = "/";
    fmt += OpenSOAP::SoapTag::ENVELOPE;
    fmt += "/";
    fmt += OpenSOAP::SoapTag::HEADER;
    fmt += "/";
    fmt += OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK;
    fmt += "/";
    fmt += OpenSOAP::ExtSoapHeaderTag::FORWARDER;
    fmt += "/";
    fmt += OpenSOAP::ExtSoapHeaderTag::HOPCOUNT;
    fmt += "=?";
    string val = StringUtil::toString(hopcount_ -1);
    
    int ret = OPENSOAP_NO_ERROR;

    ret = xmlMod.update(fmt, val);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
						,"decrease hopcount failed.","code",ret);
        return OPENSOAP_IO_WRITE_ERROR;
    }
  
    //modify original message
    ret = setSoapMessage(xmlMod.toString());

#ifdef DEBUG
    cerr << "####################################" << endl;
    cerr << "newMsg=[" << getSoapMessage() << "]" << endl;
    cerr << "####################################" << endl;
#endif

    return ret;
}

int 
RequestMessage::attachOrSwapBackwardPathInHeader(const string& myselfUrl)
{
    static char METHOD_LABEL[] = 
        "RequestMessage::attachOrSwapBackwardPathInHeader: ";

    //flag on
    modified_ = true;

    // soapMessage_ and backwardPath_ modified.

    //-----------------------------------------
    // XmlModifier
    //-----------------------------------------
    XmlModifier xmlMod(getSoapMessage());
    XmlModifier::NameSpaceDef nsDef;
    string fmt;
    string val;
  
    int ret = 0;
    //----------------------------------
    //update
    //----------------------------------
    fmt = "/" + OpenSOAP::SoapTag::ENVELOPE + "/" + 
        OpenSOAP::SoapTag::HEADER + "/" +
        OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK + "/" +
        OpenSOAP::ExtSoapHeaderTag::BACKWARD_PATH + "=?";

    ret = xmlMod.update(fmt, myselfUrl);
    if (ret != 0) {
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG5,"%s%s",METHOD_LABEL
						,"update failed.");
#endif //DEBUG
        //if update target backward_path not exist, create and attach
        //append 3rd. argument for inherit parent Namespace 2003.01.08
        ret = xmlMod.attach(fmt, myselfUrl, NULL);
        if (ret != 0) {
			AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
							,"attach failed.");
            return OPENSOAP_IO_WRITE_ERROR;
        }
    }
    //reset soapmessage
    ret = setSoapMessage(xmlMod.toString());

    return ret;
}

int 
RequestMessage::attachMessageIdInHeader(const std::string& messageId)
{
    static char METHOD_LABEL[] = 
        "RequestMessage::attachMessageIdInHeader: ";

    //flag on
    modified_ = true;
    
    // soapMessage_ and message_id_ modify

    //-----------------------------------------
    // XmlModifier
    //-----------------------------------------
    XmlModifier xmlMod(getSoapMessage());

    int ret = OPENSOAP_NO_ERROR;

    //----------------------------------
    //attach
    //----------------------------------
    string fmt = "/" + OpenSOAP::SoapTag::ENVELOPE + "/" + 
        OpenSOAP::SoapTag::HEADER + "/" +
        OpenSOAP::ExtSoapHeaderTag::OPENSOAP_HEADER_BLOCK + "/" +
        OpenSOAP::ExtSoapHeaderTag::MESSAGE_ID + "=?";

    ret = xmlMod.attach(fmt, messageId, NULL);
    if (ret != 0) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
						,"attach failed.");
        return OPENSOAP_IO_WRITE_ERROR;
    }
    //reset soapmessage
    ret = setSoapMessage(xmlMod.toString());
    
    return ret;
}

int 
RequestMessage::addSignature(const std::string& key)
{
    static char METHOD_LABEL[] = "RequestMessage::addSignature: ";

    //flag on
    modified_ = true;

    //own signature status
    if (addedSignature_) {
		AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
						,"duplication addSignature.");
        return OPENSOAP_NO_ERROR;
    }
    bool bRet = false;
    bRet = addSignatureToString(soapMessage_, key);
    if (!bRet) {
        return OPENSOAP_SEC_ERROR;
    }
    addedSignature_ = true;
    return OPENSOAP_NO_ERROR;
}

bool 
RequestMessage::isRequestOfPopResponse() const
{
    //must: have <message_id>
    //must not : not have <response_msg> -> reversed response
    //must not : not have <async> -> forwarded async request
    if (!messageId_.empty() && !async_ && !responseMsg_) {
        return true;
    }
    return false;
}

bool 
RequestMessage::isReturnedResponse() const
{
    //must: have <response_msg>true
    if (responseMsg_) {
        return true;
    }
    return false;
}

bool 
RequestMessage::isAsync() const
{
    //must : have <async>true
    if (async_) {
        return true;
    }
    return false;
}

const string& 
RequestMessage::getMethodName() const
{
    return methodName_;
}

const string& 
RequestMessage::getMethodNamespace() const
{
    return methodNamespace_;
}


const string& 
RequestMessage::getMessageId() const
{
    return messageId_;
}

long 
RequestMessage::getHopcount() const
{
    return hopcount_;
}

long 
RequestMessage::getTTLSecond() const
{
    return ttlSecond_;
}

long 
RequestMessage::getTTLAsyncSecond() const
{
    return ttlAsyncSecond_;
}

long 
RequestMessage::getTTLHoptimes() const
{
    return ttlHoptimes_;
}

vector<string>& 
RequestMessage::getReceivedPathArray()
{
    return *receivedPathArray_;
}

vector<string>& 
RequestMessage::getForwardPathArray()
{
    return *forwardPathArray_;
}

void 
RequestMessage::setBackwardPathInTTL(const std::string& bwPath)
{
    hasTTLEntry_ = true;
    backwardPathInTTL_ = bwPath;
}

void
RequestMessage::spy() const
{
    
}

// End of RequestMessage.cpp

