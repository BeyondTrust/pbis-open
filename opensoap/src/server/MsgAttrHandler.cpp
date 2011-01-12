/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgAttrHandler.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <string>
#include <iostream>
#include <string>

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

#include "MapUtil.h"
#include "ServerCommon.h"
#include "SSMLAttrMgrProtocol.h"
#include "XmlQuery.h"
#include "MsgAttrHandler.h"
#include "AppLogger.h"
#include "Exception.h"
#include "SrvErrorCode.h"

using namespace OpenSOAP;
using namespace std;

// for LOG
static const std::string CLASS_SIG = "MsgAttrHandler";

MsgAttrHandler::MsgAttrHandler()
  : docPtr_(NULL)
{
  
}

MsgAttrHandler::MsgAttrHandler(const std::string& xmlMsg)
  : docPtr_(NULL)
{
  if (0 != setMsg(xmlMsg)) {
	throw Exception(-1,OPENSOAPSERVER_UNKNOWN_ERROR
					,ERR_ERROR,__FILE__,__LINE__);
//    throw std::exception();
  }
}

MsgAttrHandler::~MsgAttrHandler()
{
  if (docPtr_) {
    xmlFreeDoc(docPtr_);
  }
}

int 
MsgAttrHandler::setMsg(const std::string& xmlMsg)
{
    //set string data
    xmlStr = xmlMsg;
    
    //clear xmlDoc
    if (docPtr_) {
        xmlFreeDoc(docPtr_);
    }
    /* COMPAT: Do not generate nodes for formatting spaces */

    LIBXML_TEST_VERSION    xmlKeepBlanksDefault(0);
    
    char* xmlMsgChar = new char[xmlMsg.length()+1];
    memcpy (xmlMsgChar, xmlMsg.c_str(), xmlMsg.length());
    xmlMsgChar[xmlMsg.length()] = 0x00;
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9
                    ,"XXXXXXXXX MsgAttrHandler::xmlParseMem XXXXXXXXXXXX");
#endif

    docPtr_ = xmlParseMemory(xmlMsgChar, xmlMsg.length());
    //docPtr_ = xmlParseMemory((char*)xmlMsg.c_str(), xmlMsg.length());
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9
                 ,"XXXXXXXXX MsgAttrHandler::xmlParseMem done. XXXXXXXXXXXX");
#endif
    
    if (NULL == docPtr_) {
		AppLogger::Write(ERR_ERROR,"%s%s"
			,CLASS_SIG.c_str() 
            ,"::setMsg: xmlParseMemory returned NULL"
            );
        delete[] xmlMsgChar;
        return -1;
    }

    /* check document (root node) */
    xmlNodePtr node1 = xmlDocGetRootElement(docPtr_);
    if (node1 == NULL) {
		AppLogger::Write(ERR_ERROR,"%s%s"
			,CLASS_SIG.c_str() 
            ,"::setMsg: empty document"
            );
        xmlFreeDoc(docPtr_);
        docPtr_ = NULL;
        delete[] xmlMsgChar;
        return -1;
    }
    
    delete[] xmlMsgChar;
    
    return 0;
}

int MsgAttrHandler::queryXml(const std::string& query, 
			      std::vector<std::string>& values)
{
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s::%s %s=[%s]"
                    ,CLASS_SIG.c_str(),"queryXml()"
                    ,"arg",query.c_str());
#endif /* DEBUG */

    XmlQuery xmlQuery(query);
    
    xmlNodePtr node = xmlDocGetRootElement(docPtr_);
    
    int ret = xmlQuery.getValue(docPtr_, node, values);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s::%s %s=(%d)"
                    ,CLASS_SIG.c_str(),"queryXml()"
                    ,"ret",ret);
#endif /* DEBUG */

    
    return ret;
}

