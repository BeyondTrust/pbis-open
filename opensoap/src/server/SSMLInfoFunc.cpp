/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLInfoFunc.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>

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

#include "SSMLInfoFunc.h"
#include "SSMLAttrHandler.h"

#include "ServerCommon.h"
#include "StringUtil.h"
#include "AppLogger.h"
#include "Exception.h"

using namespace OpenSOAP;
using namespace std;

bool
getOperationNameAndType(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getOperationNameAndType:";
    //-----------------------------------
    // outline
    //-----------------------------------
    // check <operation> elements
    // get index of <operation> matched ssmlInfo.getOperaionName()
    // check <operation type> attributes
    // get operation type at that index.
    //-----------------------------------
    bool operationExist = false;

    //extract operation name from SSML
    string query;
    //prepare query string
    if (ssmlInfo.getNamespace().empty()) {
        query = "/SSML/service/operation=??"; 
    }
    else {
        query = "/SSML/service,nsuri='";
        query += ssmlInfo.getNamespace();
        query += "'/operation=??";
    }
  
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                    ,"ssmlType",ssmlInfo.getSSMLType());
#endif //DEBUG

    //get operation list
    if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
        //comment out until method implimented for pop async response
/* 
        cerr
#ifdef USE_TIMESTAMP
            << printTimestamp()
#endif
            << TAG_WRN
            << "SSMLInfo: any operations not found." << endl;
*/
        return operationExist;
    }
  
    vector<string>::const_iterator pos;
    unsigned int operationNum = 0;

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                    ,"ssmlInfo.operationName"
                    ,ssmlInfo.getOperationName().c_str());
#endif //DEBUG

    for (pos = values.begin(); pos != values.end(); ++pos, ++operationNum) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                        ,"values",(*pos).c_str());
#endif //DEBUG
        if (*pos == ssmlInfo.getOperationName()) {
            operationExist = true;
            break;
        }
    }

    //extract operation type from SSML
    //vector<string> values1;
    if (operationExist) {
        //prepare query string
        if (ssmlInfo.getNamespace().empty()) {
            query = "/SSML/service/operation,type=??"; 
        }
        else {
            query = "/SSML/service,nsuri='";
            query += ssmlInfo.getNamespace();
            query += "'/operation,type=??";
        }
        
        values.clear();
        if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                            ,"operation exists but type attribute not found.");
            operationExist = false;
        }
        else {
            ssmlInfo.setOperationType(values[operationNum]);
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"values[operationNum]",values[operationNum].c_str());
#endif //DEBUG
        }
    }
    return operationExist;
}

void
getSyncTTL(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getSyncTTL:";
    //added namespace attr. 2004/01/19
    string baseQuery;
    if (ssmlInfo.getNamespace().empty()) {
        baseQuery = "/SSML/service/connection,name="; 
    }
    else {
        baseQuery = "/SSML/service,nsuri='";
        baseQuery += ssmlInfo.getNamespace();
        baseQuery += "'/connection,name=";
    }
    //end: 2004/01/19

    baseQuery += ssmlInfo.getOperationType();

    //extract TTL values
    string query = baseQuery + "/synchronizedTTL=??";
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;
    if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
        AppLogger::Write(APLOG_INFO,"%s%s=[%s]%s",METHOD_LABEL
                     ,"query",query.c_str(),"not found.");

        ssmlInfo.setTTLHoptimes(0);
        return;
    }

    //extract TTL types
    query = baseQuery + "/synchronizedTTL,count=??";
    vector<string> typeValues;
    ssmlAttrHndl.queryXml(query, typeValues, ssmlInfo.getSSMLType());
    //matching types
    if (typeValues.size() != values.size()) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                     ,"synchronizedTTL elements and attributes missmatch");
        return;
    }

    unsigned int uiVal = 0;    
    for(int i=0; i < typeValues.size(); i++) {
        StringUtil::fromString(values[i], uiVal);
        if (typeValues[i] == "second") {
            ssmlInfo.setSyncTTL(uiVal);
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                     ,"TTL-second",ssmlInfo.getSyncTTL());
#endif //DEBUG
        }
        else if (typeValues[i] == "hoptimes") {
            ssmlInfo.setTTLHoptimes(uiVal);
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                     ,"TTL-hoptimes",ssmlInfo.getTTLHoptimes());
#endif //DEBUG
        }
        else {
            AppLogger::Write(APLOG_WARN,"%s%s %s=[%s] %s=[%s]",METHOD_LABEL
                     ,"synchronizedTTL type invalid."
                     ,"type",typeValues[i].c_str(),"value",values[i].c_str());
            return;
        }
    }
}

void
getAsyncTTL(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getAsyncTTL:";
    //added namespace attr. 2004/01/19
    string query;
    if (ssmlInfo.getNamespace().empty()) {
        query = "/SSML/service/connection,name="; 
    }
    else {
        query = "/SSML/service,nsuri='";
        query += ssmlInfo.getNamespace();
        query += "'/connection,name=";
    }
    //end: 2004/01/19

    query += ssmlInfo.getOperationType();
    query += "/asynchronizedTTL=?";
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;
    
    if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]%s",METHOD_LABEL
                     ,"query",query.c_str(),"not found.");
        ssmlInfo.setAsyncTTL(0);
    }
    else {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"async ttl",values[0].c_str());
#endif //DEGUG
        unsigned int uival = 0;
        StringUtil::fromString(values[0], uival);
        ssmlInfo.setAsyncTTL(uival);
    }
}


void
getHostname(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getHostname:";
    //added namespace attr. 2004/01/19
    string query;
    if (ssmlInfo.getNamespace().empty()) {
        query = "/SSML/service/connection,name="; 
    }
    else {
        query = "/SSML/service,nsuri='";
        query += ssmlInfo.getNamespace();
        query += "'/connection,name=";
    }
    //end: 2004/01/19

    query += ssmlInfo.getOperationType();
    query += "/Socket,hostname=?";
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;
  
    if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]%s",METHOD_LABEL
                     ,"query",query.c_str(),"not found.");
        ssmlInfo.setHostname("");
    }
    else {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"hostname",values[0].c_str());
#endif //DEGUG
        ssmlInfo.setHostname(values[0]);
    }
}

void
getPort(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getPort:";
    //added namespace attr. 2004/01/19
    string query;
    if (ssmlInfo.getNamespace().empty()) {
        query = "/SSML/service/connection,name="; 
    }
    else {
        query = "/SSML/service,nsuri='";
        query += ssmlInfo.getNamespace();
        query += "'/connection,name=";
    }
    //end: 2004/01/19

    query += ssmlInfo.getOperationType();
    query += "/Socket,port=?";
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;
    
    if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]%s",METHOD_LABEL
                     ,"query",query.c_str(),"not found.");
        ssmlInfo.setPort(0);
    }
    else {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"port",values[0].c_str());
#endif //DEGUG
        unsigned int uiVal = 0;
        StringUtil::fromString(values[0], uiVal);
        ssmlInfo.setPort(uiVal);
    }
}

//added 2002.03.11
//modified 2003.10.20
void
getExecProg(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getExecProg:";
    //added namespace attr. 2004/01/19
    string queryBase;
    if (ssmlInfo.getNamespace().empty()) {
        queryBase = "/SSML/service/connection,name="; 
    }
    else {
        queryBase = "/SSML/service,nsuri='";
        queryBase += ssmlInfo.getNamespace();
        queryBase += "'/connection,name=";
    }
    //end: 2004/01/19

    queryBase += ssmlInfo.getOperationType();
    queryBase += "/StdIO/exec,";

    //extract "prog"
    string query = queryBase + "prog=?";
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;

    //execprog string
    string execProgStr;

    int rc = 0;
    rc = ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType());
    if (0 > rc) {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]%s",METHOD_LABEL
                     ,"query",query.c_str(),"get failed.");
        ssmlInfo.setExecProg("");
        return;
    }
    else if (values.size() > 0) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"execprog",values[0].c_str());
#endif //DEGUG

        execProgStr = values[0];
    }

    //extract "option"
    query = queryBase + "option=?";
    values.clear();
    rc = ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType());
    if (0 == rc && values.size() > 0) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"option",values[0].c_str());
#endif //DEGUG
        execProgStr += " ";
        execProgStr += values[0];
    }

    //set execprog
    ssmlInfo.setExecProg(execProgStr);

}

void
getConnectionMethod(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getConnectionMethod:";
    //added namespace attr. 2004/01/19
    string query;
    if (ssmlInfo.getNamespace().empty()) {
        query = "/SSML/service/connection,name="; 
    }
    else {
        query = "/SSML/service,nsuri='";
        query += ssmlInfo.getNamespace();
        query += "'/connection,name=";
    }
    //end: 2004/01/19

    query += ssmlInfo.getOperationType();
    query += "/??";
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;
    
    if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]%s",METHOD_LABEL
                     ,"query",query.c_str(),"not found.");
        ssmlInfo.setConnectionMethod("");
    }
    
    vector<string>::const_iterator pos;
    unsigned int operationNum = 0;
    for (pos = values.begin(); pos != values.end(); ++pos, ++operationNum) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]%s",METHOD_LABEL
                     ,"values",(*pos).c_str(),", ");
        cerr << "values = " << *pos << ", ";
#endif //DEBUG
	if (*pos == SRV_CONNCT_SOCKET ||
            *pos == SRV_CONNCT_STDIO  ||
            *pos == SRV_CONNCT_HTTP   ||
            *pos == SRV_CONNCT_FIFO   ||
            *pos == SRV_CONNCT_IPC    ||
            *pos == SRV_CONNCT_MODULE ||
            *pos == SRV_CONNCT_COM) {

            ssmlInfo.setConnectionMethod(*pos);
            break;
	}
    }
}

void
getEndPointUrl(SSMLInfo& ssmlInfo)
{
    static char METHOD_LABEL[] = "SSMLInfoFunc getEndPointUrl:";
    //added namespace attr. 2004/01/19
    string query;
    if (ssmlInfo.getNamespace().empty()) {
        query = "/SSML/service/connection,name="; 
    }
    else {
        query = "/SSML/service,nsuri='";
        query += ssmlInfo.getNamespace();
        query += "'/connection,name=";
    }
    //end: 2004/01/19

    query += ssmlInfo.getOperationType();
    query += "/HTTP/url=?";
    
    SSMLAttrHandler ssmlAttrHndl;
    vector<string> values;
  
    if (0 > ssmlAttrHndl.queryXml(query, values, ssmlInfo.getSSMLType())) {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]%s",METHOD_LABEL
                     ,"query",query.c_str(),"not exists.");
        ssmlInfo.setEndPointUrl("");
        return;
    }

#ifdef DEBUG    
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"<HTTP><url>",values[0].c_str());
#endif //DEBUG

    ssmlInfo.setEndPointUrl(values[0]);
}


extern
bool
getSSMLInfo(SSMLInfo& ssmlInfo)
{
  static char METHOD_LABEL[] = "SSMLInfoFunc getSSMLInfo:";
  /* parse ssml begin */

  // オペレーション名とタイプの取得
  bool operationExist = getOperationNameAndType(ssmlInfo);
  
  // TTL の取得
  vector<string> timeValues;
  if (operationExist) {
      //common
      getSyncTTL(ssmlInfo);
      getAsyncTTL(ssmlInfo);

      //branch by type
      getConnectionMethod(ssmlInfo);

      string connectionType = ssmlInfo.getConnectionMethod();
      if (SRV_CONNCT_SOCKET == connectionType) {
          //Socket
          getHostname(ssmlInfo);
          getPort(ssmlInfo);
      }
      else if (SRV_CONNCT_STDIO == connectionType) {
          //StdIO
          getExecProg(ssmlInfo);
      }
      else if (SRV_CONNCT_HTTP == connectionType) {
          //HTTP
          getEndPointUrl(ssmlInfo);
      }

    //-------------------------------------------------------------
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                     ,"ssmlInfo.getSyncTTL()",ssmlInfo.getSyncTTL());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getSyncTTL() = " << ssmlInfo.getSyncTTL() << endl;
    
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                     ,"ssmlInfo.getAsyncTTL()",ssmlInfo.getAsyncTTL());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getAsyncTTL() = " << ssmlInfo.getAsyncTTL() << endl;

    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                     ,"ssmlInfo.getTTLHoptimes()"
                     ,ssmlInfo.getTTLHoptimes());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getTTLHoptimes() = " << ssmlInfo.getTTLHoptimes() 
//         << endl;
    

    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"ssmlInfo.getConnectionMethod()"
                     ,ssmlInfo.getConnectionMethod().c_str());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getConnectionMethod() = " 
//	 << ssmlInfo.getConnectionMethod() << endl;

    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"ssmlInfo.getHostname()",ssmlInfo.getHostname().c_str());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getHostname() = " << ssmlInfo.getHostname() << endl;
    
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                     ,"ssmlInfo.getPort()",ssmlInfo.getPort());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getPort() = " << ssmlInfo.getPort() << endl;

    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"ssmlInfo.getExecProg()"
                     ,ssmlInfo.getExecProg().c_str());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getExecProg() = " << ssmlInfo.getExecProg() << endl;
    
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"ssmlInfo.getEndPointUrl()"
                     ,ssmlInfo.getEndPointUrl().c_str());
//    cerr << "SSMLInfoFunc::getSSMLInfo:"
//	 << "ssmlInfo.getEndPointUrl() = " << ssmlInfo.getEndPointUrl() 
//	 << endl;
#endif //DEBUG
    
  }
  /* parse ssml end */
  
  return operationExist;

}

