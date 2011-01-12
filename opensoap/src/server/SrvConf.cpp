/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConf.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>

#ifndef SYSCONFDIR
#if defined(WIN32)
# define SYSCONFDIR "/opensoap/etc/"
#else
# define SYSCONFDIR "/usr/local/opensoap/etc/"
#endif
#endif /* SYSCONFDIR */

#if defined(WIN32)
//#include <process.h>
#else
#include <sys/stat.h>
#include <unistd.h>
/*
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <sys/socket.h>
#include <sys/un.h>
*/
#endif

#include <stdexcept>

#include <string>
#include <iostream>

/* libxml2 */
#if defined(HAVE_LIBXML2_XMLVERSION_H)
#include <libxml2/tree.h>
#include <libxml2/parser.h>
#else
#include <libxml/tree.h>
#include <libxml/parser.h>
#endif /* !HAVE_LIBXML2_XMLVERSION_H */

#include "ServerCommon.h"
#include "SSMLAttrMgrProtocol.h"
#include "XmlQuery.h"
#include "SrvConf.h"
#include "StringUtil.h"
#include "AppLogger.h"
#include "Exception.h"
#include "SrvErrorCode.h"

using namespace OpenSOAP;
using namespace std;

const std::string SrvConf::SERVER_CONF_FILE = "server.conf";
std::string SrvConf::xmlFilePath_="";
xmlDocPtr SrvConf::confDocPtr_=NULL;
xmlDocPtr SrvConf::confNowDocPtr_=NULL;
time_t    SrvConf::conftime_=0;

// for LOG
static const std::string CLASS_SIG = "SrvConf";

//XMLファイルを指定する
SrvConf::SrvConf()
//: xmlFilePath_(SYSCONFDIR)
//, confDocPtr_(NULL)
{
  //ディレクトリパスが/で終るようにしておく
  if (conftime_ == 0) {
    xmlFilePath_ = SYSCONFDIR;
    if (FILE_PATH_DELMIT != xmlFilePath_.at(xmlFilePath_.length() - 1)) {
      xmlFilePath_ += FILE_PATH_DELMIT;
    }
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"%s::%s %s=[%s]"
                  ,CLASS_SIG.c_str(),CLASS_SIG.c_str()
                  ,"xmlFilePath_",xmlFilePath_.c_str());
#endif /* DEBUG */

  //server.confファイルのロード
  if (!confDocPtr_ && !loadXml()) {
      throw Exception (-1,OPENSOAPSERVER_SERVERCONF_ERROR,APLOG_ERROR
                      ,__FILE__,__LINE__);
  }
}

SrvConf::~SrvConf()
{
  //DOM構造開放
//OKANO  if (confDocPtr_) {
//OKANO    xmlFreeDoc(confDocPtr_);
//OKANO  }
}

bool
SrvConf::loadXml()
{
  static char METHOD_LABEL[] = "SrvConf::loadXml: ";
  /* parse ssml begin */
  /* COMPAT: Do not generate nodes for formatting spaces */
#if 0
  LIBXML_TEST_VERSION
    xmlKeepBlanksDefault(0);
#endif
  xmlNodePtr node1 = NULL;
  
  /* build an XML tree from a file */
  xmlDocPtr doc    = NULL;
  xmlDocPtr tmpdoc = NULL;
  struct stat st;
  std::string srvConfFilePath;
  srvConfFilePath = xmlFilePath_ + SERVER_CONF_FILE;
  
  if (stat((const char*)(srvConfFilePath.c_str()),&st)==-1) {
    AppLogger::Write(APLOG_DEBUG,"%s%s %s %s",METHOD_LABEL
                    ,"file not found. read ",srvConfFilePath.c_str(),"faild.");
    return false;
  }

  // set configuration ctime(change time)
  conftime_ = st.st_ctime;
  // load configuration (for startup)
  doc = loadConf((const char*)(srvConfFilePath.c_str()));
  if (doc) {
	tmpdoc = confDocPtr_;
    confDocPtr_ = doc;
    xmlFree(tmpdoc);
  }
  // load configuration (for update/newest)
  doc = loadConf((const char*)(srvConfFilePath.c_str()));
  if (doc) {
	tmpdoc = confNowDocPtr_;
    confNowDocPtr_ = doc;
    xmlFree(tmpdoc);
  }

  if (confDocPtr_ == NULL || confNowDocPtr_ == NULL) {
    return false;
  }
  

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG5,"%s%s",METHOD_LABEL,"add doc.");
#endif /* DEBUG */
  
  return true;
}

xmlDocPtr
SrvConf::loadConf(std::string file){
  static char METHOD_LABEL[] = "SrvConf::loadConf: ";
  xmlDocPtr  doc  = NULL;
  xmlNodePtr node = NULL;
  doc = xmlParseFile((const char*)(file.c_str()));
  if (doc == NULL) {
    AppLogger::Write(APLOG_DEBUG,"%s%s %s %s",METHOD_LABEL
                    ,"parsing file",file.c_str(),"faild.");
    return NULL;
  }
  
  /* check document (root node) */
  node = xmlDocGetRootElement(doc);
  if (node == NULL) {
    AppLogger::Write(APLOG_DEBUG,"%s%s %s %s",METHOD_LABEL
                    ,"empty document",file.c_str(),"faild.");
    xmlFreeDoc(doc);
    return NULL;
  }
  return doc;
}

int
SrvConf::query(const std::string& queryStr, vector<string>& values) 
{
	return query_(confDocPtr_,queryStr,values);
}

int
SrvConf::cquery(const std::string& queryStr, vector<string>& values) 
{
  xmlDocPtr doc=NULL;
  struct stat st;
  std::string srvConfFilePath = xmlFilePath_ + SERVER_CONF_FILE;

  if (stat((const char*)(srvConfFilePath.c_str()),&st)==0 && conftime_ != st.st_ctime) {
    doc = loadConf((const char*)(srvConfFilePath.c_str()));
    if (doc) {
      xmlFreeDoc(confNowDocPtr_);
      confNowDocPtr_ = doc;
      conftime_ = st.st_ctime;
    }
  }
  
  return query_(confNowDocPtr_,queryStr,values);
}

int
SrvConf::query_(xmlDocPtr doc,const std::string& queryStr, vector<string>& values) 
{
  static char METHOD_LABEL[] = "SrvConf::query_: ";
  XmlQuery xmlQuery(queryStr);
  
  xmlNodePtr node = xmlDocGetRootElement(doc);

  int ret = xmlQuery.getValue(doc, node, values);

  if (ret == 0) {
    ret = values.size();
  }
  return ret;
}

string 
SrvConf::getLogPath()
{
  static const string qry = "/server_conf/log/path=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
    if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
      retStr += FILE_PATH_DELMIT;
    }
  }
  return retStr;
}

string 
SrvConf::getSoapMessageSpoolPath()
{
  static const string qry = "/server_conf/spool/soap_message/path=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
    if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
      retStr += FILE_PATH_DELMIT;
    }
  }
  return retStr;
}

string 
SrvConf::getAsyncTablePath()
{
  static const string qry = "/server_conf/spool/async_table/path=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
    if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
      retStr += FILE_PATH_DELMIT;
    }
  }
  return retStr;
}

string 
SrvConf::getPIDPath()
{
  static const string qry = "/server_conf/run/pid/path=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
    if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
      retStr += FILE_PATH_DELMIT;
    }
  }
  return retStr;
}

string 
SrvConf::getSocketPath()
{
  static const string qry = "/server_conf/run/socket/path=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
    if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
      retStr += FILE_PATH_DELMIT;
    }
  }
  return retStr;
}

string 
SrvConf::getSecKeyPath()
{
  static const string qry = "/server_conf/security/keys/path=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
    if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
      retStr += FILE_PATH_DELMIT;
    }
  }
  return retStr;
}

string 
SrvConf::getSSMLPath()
{
  static const string qry = "/server_conf/ssml/path=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
    if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
      retStr += FILE_PATH_DELMIT;
    }
  }
  return retStr;
}

//added 2004/01/04
string 
SrvConf::getSSMLInternalServicesPath()
{
    static const string qry = "/server_conf/ssml/internalServices/path=?";
    vector<string> values;
    int ret = query(qry, values);
    string retStr;
    if (ret > 0) {
        retStr = values[0];
        if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
            retStr += FILE_PATH_DELMIT;
        }
    }
    return retStr;
}

#if 0
//added 2004/01/04
string 
SrvConf::getSSMLReplyToPath()
{
    string qry = "/server_conf/ssml/replyTo/path=?";
    vector<string> values;
    int ret = query(qry, values);
    string retStr;
    if (ret > 0) {
        retStr = values[0];
        if (!retStr.empty() && retStr[retStr.length()-1] != FILE_PATH_DELMIT) {
            retStr += FILE_PATH_DELMIT;
        }
    }
    return retStr;
}
#endif //if 0


vector<string>
SrvConf::getBackwardUrls()
{
#if 1
    static const string qry = "/server_conf/backward/url=??";
    vector<string> values;
    int ret = query(qry, values);
    return values;
#else
    try {
        string protocol = getBackwardProtocol();
        vector<string> hosts = getBackwardHostnames();
        int port = getBackwardPort();    
        string cgi_location = getBackwardCgiLocation();
        vector<string>::iterator iter;
        vector<string> retUrls;
        string portStr;
        if (port != OpenSOAP::DEFAULT_PORT) {
            portStr = ":";
            portStr += StringUtil::toString(port);
        }
        if (cgi_location.at(0) != '/') {
            cgi_location.insert(0, "/");
        }
        for(iter = hosts.begin(); iter != hosts.end(); iter++) {
            retUrls.push_back(protocol + "://" + *iter + portStr + cgi_location);
        }
        return retUrls;
    }
    catch (Exception e) {
		AppLogger::Write(e);
		e.AddFuncCallStack();
		throw (e);
	}
    catch (exception e) {
		AppLogger::Write(APLOG_ERROR
			,"SrvConf::getBackwardUrls: exception caught. \"%s\"\n"
			,e.what());
        throw e;
    }
#endif //if 1
}

string
SrvConf::getForwarderUrl()
{
#if 1
    static const string qry = "/server_conf/forwarder/url=?";
    vector<string> values;
    int ret = query(qry, values);
    string retVal;
    if (ret > 0) {
        retVal = values[0];
    }
    else {
		Exception e(-1,OPENSOAPSERVER_CONF_ERROR,APLOG_ERROR,__FILE__,__LINE__);
		e.SetErrText("<forwarder><url> not found");
		throw (e);
    }
    return retVal;
#else
    try {
        string protocol = getForwarderProtocol();
        string host = getForwarderHostname();
        int port = getForwarderPort();    
        string cgi_location = getForwarderCgiLocation();
        string portStr;
        if (port != OpenSOAP::DEFAULT_PORT) {
            portStr = ":";
            portStr += StringUtil::toString(port);
        }
        if (cgi_location.at(0) != '/') {
            cgi_location.insert(0, "/");
        }
        return string(protocol + "://" + host + portStr + cgi_location);
    }
    catch (Exception e) {
		AppLogger::Write(e);
		e.AddFuncCallStack();
		throw (e);
	}
    catch (...) {
		AppLogger::Write(APLOG_WARN
			,"SrvConf::getForwarderUrl: failed.");
        throw ;
    }
#endif //if 0
}

#if 0
string 
SrvConf::getBackwardProtocol()
{
  string qry = "/server_conf/backward/protocol=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
  }
  return retStr;
}

vector<string> 
SrvConf::getBackwardHostnames()
{
  string qry = "/server_conf/backward/hostname=??";
  vector<string> values;
  int ret = query(qry, values);
  if (values.size() == 0) {
      values.push_back("localhost");
  }
  return values;
}

int
SrvConf::getBackwardPort()
{
    string qry = "/server_conf/backward/port=?";
    vector<string> values;
    int ret = query(qry, values);
    int retVal = 80;
    if (ret > 0) {
        StringUtil::fromString(values[0], retVal);
    }
    return retVal;
}

string 
SrvConf::getBackwardCgiLocation()
{
  string qry = "/server_conf/backward/cgi_location=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
  }
  return retStr;
}

string 
SrvConf::getForwarderProtocol()
{
  string qry = "/server_conf/forwarder/protocol=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
  }
  return retStr;
}

string 
SrvConf::getForwarderHostname()
{
  string qry = "/server_conf/forwarder/hostname=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
  }
  return retStr;
}

int
SrvConf::getForwarderPort()
{
    string qry = "/server_conf/forwarder/port=?";
    vector<string> values;
    int ret = query(qry, values);
    int retVal = 80;
    if (ret > 0) {
        StringUtil::fromString(values[0], retVal);
    }
    return retVal;
}

string 
SrvConf::getForwarderCgiLocation()
{
  string qry = "/server_conf/forwarder/cgi_location=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
  }
  return retStr;
}
#endif //if 0

long
SrvConf::getForwarderTimeout()
{
    static const string qry = "/server_conf/forwarder/timeout=?";
    vector<string> values;
    int ret = query(qry, values);
    long retVal = 0;
    if (ret > 0) {
        StringUtil::fromString(values[0], retVal);
    }
    return retVal;
}

string
SrvConf::getAddSignature()
{
  static const string qry = "/server_conf/add_signature=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
  }
  return retStr;
}

bool 
SrvConf::isAddSignatureTrue()
{
  if (getAddSignature() == "true") {
    return true;
  }
  else {
    return false;
  }
}

// 2003/06/03
long
SrvConf::getLimitSOAPMessageSize()
{
  static const string qry = "/server_conf/limit/soap_message_size=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  long retVal = 0;
  if (ret > 0) {
    retStr = values[0];
    StringUtil::fromString(retStr, retVal);
    //check 'K'or'M'
    switch(toupper(retStr.at(retStr.length()-1))) {
    case 'M':
      retVal *= 1024L;
      // fall through 
    case 'K':
      retVal *= 1024L;
      break;
    default:
      break;
    }

    if (0 == retVal) {
		AppLogger::Write(APLOG_WARN,"getLimitSOAPMessageSize 0 byte.");
//      cerr << "W: getLimitSOAPMessageSize 0 byte." << endl;
    }
  }
  else {
    return -1L;
  }

  return retVal;
}

// 2003/06/17
long
SrvConf::getLimitTTLSecond()
{
  static const string qry = "/server_conf/limit/ttl/synchronizedTTL=?";
  vector<string> values;
  int ret = query(qry, values);
  long retVal = 0;
  const long DEFAULT_TTL = 600; // used when invalid configure.
  if (ret > 0) {
    StringUtil::fromString(values[0], retVal);
    if (0 >= retVal) {
		AppLogger::Write(APLOG_WARN
			,"%s%s(=%ld)"
			,"getLimitTTLSecond value is zero or less value."
			,"Use default value.\n"
			,DEFAULT_TTL
			);
//      cerr << "W: getLimitTTLSecond value is zero or less value. "
//	   << "Use default value(=" << DEFAULT_TTL << ")." << endl;
      return DEFAULT_TTL;
    }
  }
  else {
		AppLogger::Write(APLOG_WARN,"%s%s(=%ld)"
			,"getLimitTTLSecond failed."
			,"Use default value.\n"
			,DEFAULT_TTL
			);
    return DEFAULT_TTL;
  }

  return retVal;
}

long
SrvConf::getLimitTTLHoptimes()
{
    static char METHOD_LABEL[] = "SrvConf::getLimitTTLHoptimes: ";

    static const string qry = "/server_conf/limit/ttl/hoptimes=?";
    vector<string> values;
    int ret = query(qry, values);
    long retVal = 0;
    const long DEFAULT_TTL = 4; // used when invalid configure.
    if (ret > 0) {
        StringUtil::fromString(values[0], retVal);
        if (0 > retVal) {
			AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
							,"minus value is not alloweding by any case.");
        }
    }
    else {
		AppLogger::Write(APLOG_WARN,"%s%s%s(=%ld).",METHOD_LABEL
			,"get hoptimes failed. "
			,"Use default value"
			,DEFAULT_TTL
			);
        return DEFAULT_TTL;
    }
    return retVal;
}

long
SrvConf::getLimitTTLAsync()
{
    static char METHOD_LABEL[] = "SrvConf::getLimitTTLAsync: ";

    static const string qry = "/server_conf/limit/ttl/asynchronizedTTL=?";
    vector<string> values;
    int ret = query(qry, values);
    long retVal = 0;
    const long DEFAULT_TTL = 3600; // used when invalid configure.
    if (ret > 0) {
        StringUtil::fromString(values[0], retVal);
        if (0 >= retVal) {
			AppLogger::Write(APLOG_WARN,"%s%s%s(=%ld).",METHOD_LABEL
				,"value is zero or less value. "
				,"Use default value"
				,DEFAULT_TTL
				);
            return DEFAULT_TTL;
        }
    }
    else {
		AppLogger::Write(APLOG_WARN,"%s%s%s(=%ld).",METHOD_LABEL
			,"asynchronizedTTL get failed. "
			,"Use default value"
			,DEFAULT_TTL
			);
        return DEFAULT_TTL;
    }
    
    return retVal;
}

string
SrvConf::getDefaultCharEncoding()
{
  static const string qry = "/server_conf/default_charencoding=?";
  vector<string> values;
  int ret = query(qry, values);
  string retStr;
  if (ret > 0) {
    retStr = values[0];
  }
  else {
    //system default
    retStr = "UTF-8";
  }
  return retStr;
}
