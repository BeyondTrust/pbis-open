/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlModifier.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

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

#include "ServerCommon.h"
#include "XmlModifier.h"

#include "XmlQuery2.h"
#include "AppLogger.h"

//#define DEBUG

using namespace OpenSOAP;
using namespace std;

// for LOG
static const std::string CLASS_SIG = "XmlModifier";

XmlModifier::XmlModifier(xmlDocPtr& doc)
    : xmlDoc(doc)
    , refObj(true)
{
    
}

XmlModifier::XmlModifier(const std::string& xmlMsg)
  : xmlDoc(NULL)
  , refObj(false)
{
  /* COMPAT: Do not generate nodes for formatting spaces */

  LIBXML_TEST_VERSION xmlKeepBlanksDefault(0);

  char* xmlMsgChar = new char[xmlMsg.length()+1];
  memcpy (xmlMsgChar, xmlMsg.c_str(), xmlMsg.length());
  xmlMsgChar[xmlMsg.length()] = 0x00;

  xmlDoc = xmlParseMemory(xmlMsgChar, xmlMsg.length());
  //xmlDoc = xmlParseMemory((char*)xmlMsg.c_str(), xmlMsg.length());
  if (NULL == xmlDoc) {
	AppLogger::Write(APLOG_ERROR,"%s::%s:%s",CLASS_SIG.c_str(),__func__
					,"xmlParseMemory returned NULL");
//    cerr << TAG_ERR << CLASS_SIG << "::" << CLASS_SIG
//	 << ": xmlParseMemory returned NULL" << endl;
	delete[] xmlMsgChar;
    throw std::exception();
  }
  /* check document (root node) */
  xmlNodePtr node1 = xmlDocGetRootElement(xmlDoc);
  if (node1 == NULL) {
      AppLogger::Write(APLOG_ERROR,"%s::%s:%s",CLASS_SIG.c_str(),__func__
                       ,"empty document");
      xmlFreeDoc(xmlDoc);
      xmlDoc = NULL;
      delete[] xmlMsgChar;
      throw std::exception();
  }

  delete[] xmlMsgChar;
}

XmlModifier::~XmlModifier()
{
    if (xmlDoc && !refObj) {
        xmlFreeDoc(xmlDoc);
  }
}

std::string 
XmlModifier::toString()
{
  /* output doc tree */
  xmlChar* memPtr;
  int size;
  xmlDocDumpMemory(xmlDoc, &memPtr, &size);
  
//modified 2003/11/03
  string retString((char*)memPtr, size);
  xmlFree(memPtr);
  return retString;
}

int
XmlModifier::attach(const std::string& fmt, const std::string& value)
{
  XmlQuery2 xmlQuery(fmt);
  return xmlQuery.attach(xmlDoc, xmlDocGetRootElement(xmlDoc), NULL,
			 value,
			 false, false, NULL);
}

int
XmlModifier::attachNoDuplicate(const std::string& fmt, 
			       const std::string& value)
{
    static char METHOD_LABEL[] = "XmlModifier::attachNoDuplicate: ";

    XmlQuery2 xmlQuery(fmt);
    std::vector<std::string> tmpBuf;
    int ret = xmlQuery.getValue(xmlDoc, xmlDocGetRootElement(xmlDoc), tmpBuf);
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG7,"%s%s=(%d) %s=[%s]",METHOD_LABEL
					,"precheck getVale rc",ret
					,"query",fmt.c_str());
//    cerr << METHOD_LABEL << "precheck getVale rc=(" << ret << "): query=["
//         << fmt << "]" << endl;
#endif //DEBUG
        if (ret == 0) {
            return 0;
        }
    return attach(fmt, value);
}

int 
XmlModifier::attach(const std::string& fmt, 
		    const std::string& value,
		    const NameSpaceDef* nsDef)
{
    static char METHOD_LABEL[] = "XmlModifier::attach: ";

    XmlQuery2 xmlQuery(fmt);
    XmlQuery2::NameSpaceDef* def = NULL;
    if (nsDef) {
        def = new XmlQuery2::NameSpaceDef;
        def->href = nsDef->href;
        def->prefix = nsDef->prefix;
    }
    //edit 2003/02/06 memory leak
    int ret = xmlQuery.attach(xmlDoc, xmlDocGetRootElement(xmlDoc), NULL,
                              value,
                              false, true, def);
    if (def) {
        delete def;
    }
    return ret;
}

int 
XmlModifier::attachNoDuplicate(const std::string& fmt, 
		    const std::string& value,
		    const NameSpaceDef* nsDef)
{
    static char METHOD_LABEL[] = "XmlModifier::attachNoDuplicate: ";

    XmlQuery2 xmlQuery(fmt);
    std::vector<std::string> tmpBuf;
    int ret = xmlQuery.getValue(xmlDoc, xmlDocGetRootElement(xmlDoc), tmpBuf);
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG7,"%s%s=(%d) %s=[%s]",METHOD_LABEL
					,"precheck getVale rc",ret
					,"query",fmt.c_str());
#endif //DEBUG
    if (ret == 0) {
        return 0;
    }
    return attach(fmt, value, nsDef);
}

int 
XmlModifier::update(const std::string& fmt, const std::string& value)
{
  XmlQuery2 xmlQuery(fmt);
  return xmlQuery.attach(xmlDoc, xmlDocGetRootElement(xmlDoc), NULL,
			 value,
			 true, false, NULL);
}

int
XmlModifier::append(const std::string& fmt, const std::string& value)
{
  XmlQuery2 xmlQuery(fmt);
  return xmlQuery.attach(xmlDoc, xmlDocGetRootElement(xmlDoc), NULL,
			 value,
			 false, false, NULL, 1);
}


int 
XmlModifier::append(const std::string& fmt, 
		    const std::string& value,
		    const NameSpaceDef* nsDef)
{
  XmlQuery2 xmlQuery(fmt);
  XmlQuery2::NameSpaceDef* def = NULL;
  if (nsDef) {
    def = new XmlQuery2::NameSpaceDef;
    def->href = nsDef->href;
    def->prefix = nsDef->prefix;
  }
  //edit 2003/02/06 memory leak
  int ret = xmlQuery.attach(xmlDoc, xmlDocGetRootElement(xmlDoc), NULL,
			    value,
			    false, true, def, 1);
  if (def) {
    delete def;
  }
  return ret;
}

int 
XmlModifier::del(const std::string& fmt)
{
  XmlQuery2 xmlQuery(fmt);
  return xmlQuery.del(xmlDoc, xmlDocGetRootElement(xmlDoc));
}


    
