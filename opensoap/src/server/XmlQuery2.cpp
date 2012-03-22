/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlQuery2.cpp,v $
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

//  #include "ServerCommon2.h"
#include "ServerCommon.h"
#include "SSMLAttrMgrProtocol.h"
#include "XmlQuery2.h"
#include "AppLogger.h"

//#define DEBUG

using namespace OpenSOAP;
using namespace std;

// for LOG
static const std::string CLASS_SIG = "XmlQuery2";

//parse dom
XmlQuery2::XmlQuery2(const std::string& query)
  : next_(NULL)
  , isTarget_(false)
  , isMulti_(false)
{
  std::string queryStr(query);

  //split Node using NODE_DELMIT(="/")
  std::string::size_type begIdx = 0;
  std::string::size_type endIdx = 0;

  begIdx = queryStr.find_first_not_of(NODE_DELMIT);
  if (begIdx == std::string::npos) {
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s",CLASS_SIG.c_str(),__func__
					,"END");
//    cerr << "END" << endl;
#endif /* DEBUG */
	AppLogger::Write(APLOG_ERROR,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"invalid query data. query",query.c_str());
//    cerr << TAG_ERR << CLASS_SIG << "::" << CLASS_SIG
//	 << ": invalid query data. query=[" << query << "]" << endl;
    throw std::exception();
  }
  
  endIdx = queryStr.find_first_of(NODE_DELMIT, begIdx);
  if (endIdx == std::string::npos) {
    endIdx = queryStr.length();
  }
  //target data for current tree-level
  std::string curQuery = queryStr.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"curQuery",curQuery.c_str());
//  cerr << "curQuery=[" << curQuery << "]" << endl;
#endif /* DEBUG */

  //data for next tree-level 
  queryStr = queryStr.substr(endIdx);

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"next queryStr",queryStr.c_str());
//  cerr << "next queryStr=[" << queryStr << "]" << endl;
#endif /* DEBUG */

  //-------------------------------------------------
  //if next tree-level exists, set instance into next_
  //-------------------------------------------------
  begIdx = queryStr.find_first_not_of(NODE_DELMIT);
  if (begIdx != std::string::npos) {
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"next Query create.",queryStr.c_str());
//    cerr << "next Query create." << endl;
#endif /* DEBUG */

    next_ = new XmlQuery2(queryStr);
  }

  //parsing data
  //split tag-name and attributes
  begIdx = curQuery.find_first_not_of(ATTR_DELMIT);
  if (begIdx == std::string::npos) {
	AppLogger::Write(APLOG_ERROR,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"invalid current query data. query",curQuery.c_str());
//    cerr << TAG_ERR << CLASS_SIG << "::" << CLASS_SIG
//	 << ": invalid current query data. query=[" << curQuery << "]" << endl;
    throw std::exception();
  }

  endIdx = curQuery.find_first_of(ATTR_DELMIT, begIdx);
  if (endIdx == std::string::npos) {
    endIdx = curQuery.length();
  }
  //tag-name
  std::string nameToken = curQuery.substr(begIdx, (endIdx-begIdx));
  addName(nameToken);

  for (;;) {
    begIdx = curQuery.find_first_not_of(ATTR_DELMIT, endIdx);
    if (begIdx == std::string::npos) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s",CLASS_SIG.c_str(),__func__
					,"END");
//      cerr << "END" << endl;
#endif /* DEBUG */
      break;
    }
    endIdx = curQuery.find_first_of(ATTR_DELMIT, begIdx);
    if (endIdx == std::string::npos) {
      endIdx = curQuery.length();
    }
    std::string attrToken = curQuery.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
	AppLogger::Write(APLOG_ERROR,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"attrToken",attrToken.c_str());
#endif /* DEBUG */

    addAttributes(attrToken);
  }


}

void XmlQuery2::addName(const std::string& nameToken)
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"arg",nameToken.c_str());
//  cerr << CLASS_SIG << "::addName() arg=[" << nameToken << "]" << endl;
#endif /* DEBUG */

  //split attribute condition using VAL_DELMIT(="=")
  std::string::size_type begIdx = 0;
  std::string::size_type endIdx = 0;

  begIdx = nameToken.find_first_not_of(VAL_DELMIT);
  if (begIdx == std::string::npos) {
	AppLogger::Write(APLOG_ERROR,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"invalid name token data. nameToken",nameToken.c_str());
//    cerr << TAG_ERR << CLASS_SIG << "::" << CLASS_SIG
//	 << ": invalid name token data. nameToken=[" << nameToken << "]" << endl;
    throw std::exception();
  }
  
  endIdx = nameToken.find_first_of(VAL_DELMIT, begIdx);
  if (endIdx == std::string::npos) {
    endIdx = nameToken.length();
  }
  
  //name
  std::string name = nameToken.substr(begIdx, (endIdx-begIdx));

  //split condition
  begIdx = nameToken.find_first_not_of(VAL_DELMIT, endIdx);
  if (begIdx == std::string::npos) {
      //no target
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s",CLASS_SIG.c_str(),__func__
					,"no name condition.");
//    cerr << CLASS_SIG << "::addName: no name condition." << endl;
#endif /* DEBUG */

    //added proc. for get child tag names #2001/09/10
    if (QUESTION_MARK == name) {
        //question target
      isTarget_ = true;
      node_.setName(name);
    } 
    else if (MULTI_QUESTION_MARK == name) {
        //multi 
      isTarget_ = true;
      isMulti_ = true;
      //next step no use MULTI_QUESTION_MARK
      node_.setName(QUESTION_MARK);
    }
    else {
      node_.setName(name);
    }
    return;
  }
  //normally name setting
  node_.setName(name);

  endIdx = nameToken.find_first_of(VAL_DELMIT, begIdx);
  if (endIdx == std::string::npos) {
    endIdx = nameToken.length();
  }
  std::string cond = nameToken.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"name cond",cond.c_str());
//  cerr << CLASS_SIG << "::addName: name cond=[" << cond << "]" << endl;
#endif /* DEBUG */

  if (QUESTION_MARK == cond) {
      //question target
    isTarget_ = true;
    node_.setValue(cond);
  } 
  else if (MULTI_QUESTION_MARK == cond) {
      //multi
    isTarget_ = true;
    isMulti_ = true;
    //next step no use MULTI_QUESTION_MARK
    node_.setValue(QUESTION_MARK);
  }
  else {
    node_.setValue(cond);
  }
}

void XmlQuery2::addAttributes(const std::string& attrToken)
{
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"arg",attrToken.c_str());
//  cerr << CLASS_SIG << "::addAttributes() arg=[" << attrToken << "]" << endl;
#endif /* DEBUG */

  //split attributes using VAL_DELMIT(="=")
  std::string::size_type begIdx = 0;
  std::string::size_type endIdx = 0;
  
  begIdx = attrToken.find_first_not_of(VAL_DELMIT);
  if (begIdx == std::string::npos) {
    AppLogger::Write(APLOG_ERROR,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"invalid attr token data. attrToken",attrToken.c_str());
//    cerr << TAG_ERR << CLASS_SIG << "::" << CLASS_SIG
//	 << ": invalid attr token data. attrToken=[" << attrToken << "]" << endl;
    throw std::exception();
  }
  
  endIdx = attrToken.find_first_of(VAL_DELMIT, begIdx);
  if (endIdx == std::string::npos) {
    endIdx = attrToken.length();
  }
  
  DataSet attrDataSet;

  //attributes name
  std::string attrName = attrToken.substr(begIdx, (endIdx-begIdx));
  attrDataSet.setName(attrName);

  //split condition
  begIdx = attrToken.find_first_not_of(VAL_DELMIT, endIdx);
  if (begIdx == std::string::npos) {
      //no target
    AppLogger::Write(APLOG_ERROR,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"no attr condition. attr",attrName.c_str());
//    cerr << TAG_ERR << CLASS_SIG 
//	 << "::addAttributes: no attr condition. attr=[" 
//	 << attrName << "]" << endl;
    throw std::exception();
  }

  endIdx = attrToken.find_first_of(VAL_DELMIT, begIdx);
  if (endIdx == std::string::npos) {
    endIdx = attrToken.length();
  }
  std::string cond = attrToken.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=[%s]",CLASS_SIG.c_str(),__func__
					,"attr cond",cond.c_str());
//  cerr << CLASS_SIG << "::addAttributes: attr cond=[" << cond << "]" << endl;
#endif /* DEBUG */

  if (QUESTION_MARK == cond) {
      //question target
      isTarget_ = true;
      attrDataSet.setValue(cond);
  } 
  else if (MULTI_QUESTION_MARK == cond) {
      //multi
      isTarget_ = true;
      isMulti_ = true;
      attrDataSet.setValue(QUESTION_MARK);
  }
  else {
    attrDataSet.setValue(cond);
  }

  //add attributes
  attributes_.push_back(attrDataSet);
}

XmlQuery2::~XmlQuery2()
{
    if (next_) { delete next_; }
}

//values getter
//return true in success
//¦set RootNode into RootQuery
//int XmlQuery2::getValue(xmlDocPtr& doc, xmlNodePtr node)
int XmlQuery2::getValue(xmlDocPtr& doc, xmlNodePtr node,
		       std::vector<std::string>& values)
{
  //bool result = false;
  int result = -101; // -101 = target not found

  //temp.buffer
  std::string answer;

  int i = 0;

  while (NULL != node) {
    
    //check node name
    if (QUESTION_MARK == node_.getName()) {
      //request tag name 
      answer = (const char*)node->name;
    }
    else {
      if (0 != strcmp((const char*)node->name, node_.getName().c_str())) {
          //target node
	node = node->next;
	continue;
      }

      //check node element
      if (!node_.isNullValue()) {
          //target element exists
          xmlChar* element = 
              xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
          string elementString;
          if (element) {
              elementString = (const char*)element;
              xmlFree(element);
          }
          if (QUESTION_MARK == node_.getValue()) {
              //is target, set buffer
              answer = elementString;
              //hasAnswer = true;
          }
          else if (elementString != node_.getValue()) {
              //check condition
              //not target
              node = node->next;
              continue;
          }
      }
     
      //check attributes
      std::vector<DataSet>::const_iterator pos;
      bool isValidAttr = true;
      for (pos = attributes_.begin(); pos != attributes_.end(); ++pos) {
	
          xmlChar* attr = xmlGetProp(node, 
                                     (const xmlChar*)pos->getName().c_str());
          if (NULL == attr) {
              return -103; // -103 = target attr not exists
          }
          string attrString((const char*)attr);
          xmlFree(attr);
          if (QUESTION_MARK == pos->getValue()) {
              //is target, set buffer
              answer = attrString;
          }
          else if (attrString != pos->getValue()) {
              //condition check
              //not target
              isValidAttr = false;
              break;
          }
      } // end for ();
      if (!isValidAttr) {
          node = node->next;
          continue;
      }

      //has child, next step
      if (next_) {
	
	if (isTarget_) {
      AppLogger::Write(APLOG_WARN,"%s::%s:%s",CLASS_SIG.c_str(),__func__
					,"current instance is target, but next_ has...");
//	  cerr << TAG_WRN << CLASS_SIG << 
//	    "::getValue: current instance is target, but next_ has..." << endl;
	}
	return next_->getValue(doc, node->xmlChildrenNode, values);
      }
    }

    values.push_back(answer);
    result = 0; // 0 = SUCCESS

    //exit, if not multi
    if (!isMulti_) {
      break;
    }

    //next node
    node = node->next;
    
    i++;
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=(%d)",CLASS_SIG.c_str(),__func__
					,"return",result);
//  cerr << "return =(" << result << ")" << endl;
#endif

  return result;
}

void XmlQuery2::spy() const
{
  cerr << CLASS_SIG << "::spy() #####" << endl;
  cerr << CLASS_SIG << "::node_:" << endl;
  node_.spy();

  cerr << "  isTarget_:" << isTarget_ << endl;
  cerr << "  isMulti_:" << isMulti_ << endl;

  cerr << CLASS_SIG << "::attributes_:" << endl;
  std::vector<DataSet>::const_iterator pos;
  for (pos = attributes_.begin(); pos != attributes_.end(); ++pos) {
    cerr << " -------------" << endl;   
    pos->spy();
  }
  
  if (next_) { next_->spy(); }
}


    
int 
XmlQuery2::attach(xmlDocPtr& doc, 
                  //xmlNodePtr parentNode,
                  xmlNodePtr node,
                  xmlNodePtr parent,
                  const std::string& value, 
                  bool updateFlg,
                  bool useNs,
                  const NameSpaceDef* nsDef,
                  int insPos)
{
    static char METHOD_LABEL[] = "XmlQuery2::attach: ";

    if (!node && !parent) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
					,"node and parent are null.");
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "node and parent are null." << endl;
        return -1;
    }

    //xmlNodePtr node = parentNode->xmlChildrenNode;
    
    //bool result = false;
    int result = -101; // -101 = target not found

    //temp buffer
    std::string answer;

#ifdef DEBUG
    cerr << METHOD_LABEL << "node_.getName()=[" << node_.getName() << "]"
         << endl;
    cerr << "          node_.getValue()=[" << node_.getValue() << "]"
         << endl;
    cerr << "          node_.isNullValue()=(" << node_.isNullValue() << ")"
         << endl;
    cerr << "          updateFlg=(" << updateFlg << ")"
         << endl;
    cerr << "          value=[" << value << "]" 
         << endl;
    cerr << "-----------------------------" << endl;
    node_.spy();
    cerr << "-----------------------------" << endl;
#endif //DEBUG

    if (QUESTION_MARK == node_.getName()) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"Q.M.!! node_.getName",node_.getName().c_str());
//        cerr << METHOD_LABEL << "Q.M.!! node_.getName=["
//             << node_.getName() << "]" << endl;
#endif //DEBUG
        //request tag name 
        //answer = (const char*)node->name;

        //create new empty tag

        //create new node
        xmlNodePtr newNode = xmlNewNode(NULL, (xmlChar*)value.c_str());
    
        if (useNs) {
            xmlNsPtr newNs = NULL;
            if (NULL == nsDef) {
                //default
                xmlNsPtr* nsPtrList = 
                    xmlGetNsList(doc, (parent ? parent : node));
                if (nsPtrList) {
                    newNs = *nsPtrList;
                    //added 2004/02/13(Fri ;-)
                    xmlFree(nsPtrList);
                }
            }
            else {
                if (NULL != parent) {
                    newNs = xmlSearchNsByHref(doc,
                                              //(node ? node : parent),
                                              parent,
                                              (xmlChar*)nsDef->href.c_str());
                }
                if (!newNs) {
                    newNs = xmlNewNs(newNode, 
                                     (xmlChar*)nsDef->href.c_str(),
                                     (xmlChar*)nsDef->prefix.c_str());
                }
            }
            xmlSetNs(newNode, newNs);
        }
        if (node) {
            if (-1 == insPos) {
                xmlAddPrevSibling(node, newNode);
            }
            else {
                for (xmlNodePtr next = node->next;
                     NULL != next; 
                     next = node->next) {
                    node = next;
                }
                xmlAddNextSibling(node, newNode);
            }
        }
        else if (parent) {
            xmlAddChild(parent, newNode);
        }
        else {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
					,"node and parent are NULL.");
//            LOGPRINT(TAG_ERR)
//                << METHOD_LABEL
//                << "node and parent are NULL."
//                << endl;
            return -1;
        }
        return 0;
    }

    if (!node_.isNullValue() 
        && QUESTION_MARK == node_.getValue()
        && false == updateFlg) {

        //create new node
        xmlNodePtr newNode 
            = xmlNewNode(NULL, (xmlChar*)node_.getName().c_str());
        //set value
        xmlNodeSetContent(newNode, (xmlChar*)value.c_str());
    
        //namespace proc.
        if (useNs) {
            xmlNsPtr newNs = NULL;
            if (NULL == nsDef) {
                //default
                xmlNsPtr* nsPtrList = 
                    xmlGetNsList(doc, (parent ? parent : node));
                if (nsPtrList) {
                    newNs = *nsPtrList;
                    //added 2004/02/13(Fri ;-)
                    xmlFree(nsPtrList);
                }
                //newNs = xmlNewNs(newNode, NULL, NULL);
            }
            else {
                if (NULL != parent) {
                    newNs = xmlSearchNsByHref(doc,
                                              //(node ? node : parent),
                                              parent,
                                              (xmlChar*)nsDef->href.c_str());
                }
                if (!newNs) {
                    newNs = xmlNewNs(newNode, 
                                     (xmlChar*)nsDef->href.c_str(),
                                     (xmlChar*)nsDef->prefix.c_str());
                }
            }
            xmlSetNs(newNode, newNs);
        }
        if (node) {
            //add in front of current level first node 
            if (-1 == insPos) {
                xmlAddPrevSibling(node, newNode);
            }
            else {
                for (xmlNodePtr next = node->next; 
                     NULL != next; 
                     next = node->next) {
                    node = next;
                }
                xmlAddNextSibling(node, newNode);
            }
        }
        else if (parent) {
            xmlAddChild(parent, newNode);
        }
        else {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
					,"node and parent are NULL.");
//            LOGPRINT(TAG_ERR)
//                << METHOD_LABEL
//                << "node and parent are NULL."
//                << endl;
            return -1;
        }

        return 0;
    }

    //for update or attributes modify
    while (NULL != node) {
    
        //check node name

        if (0 != strcmp((const char*)node->name, node_.getName().c_str())) {
            node = node->next;
            continue;
        }

        //if has no child-node, current node is target
        //if (!next_) {
        if (!node_.isNullValue()) {
            //in case adding node 
            if (QUESTION_MARK == node_.getValue() && updateFlg) {
                xmlNodeSetContent(node, (xmlChar*)value.c_str());
                return 0;
            }
            else {
                //has condition
                xmlChar* element = 
                    xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
                string elementString;
                if (element) {
                    elementString = (const char*)element;
                    xmlFree(element);
                }
                if (elementString != node_.getValue()) {
                    //check condition
                    //not target node
                    node = node->next;
                    continue;
                }
            }
        }
        //attributes proc.
        //check attributes
        std::vector<DataSet>::const_iterator pos;
        bool isValidAttr = true;
        const DataSet* targetDataSet = NULL;
        for (pos = attributes_.begin(); pos != attributes_.end(); ++pos) {
      
            if (QUESTION_MARK == pos->getValue()) {
                //is target, set buffer
                //answer = (const char*)attr;
                //hasAnswer = true;
                targetDataSet = &*pos;
            }
            else {
                xmlChar* attr = 
                    xmlGetProp(node, (const xmlChar*)pos->getName().c_str());
                if (NULL == attr) {
                    return -103; // -103 = target attr not exists
                }
                string attrString((const char*)attr);
                xmlFree(attr);
                if (attrString != pos->getValue()) {
                    //condition check
                    //not target
                    isValidAttr = false;
                    break;
                }
            }
        } // end for ();
        if (!isValidAttr) {
            node = node->next;
            continue;
        }
        //if attributes setter, exit proc. now.
        if (targetDataSet) {
            //namespace proc.
            xmlNsPtr newNs = NULL;	
            if (useNs) {
                if (NULL == nsDef) {
                    //default
                    xmlNsPtr* nsPtrList = 
                        xmlGetNsList(doc, (parent ? parent : node));
                    if (nsPtrList) {
                        newNs = *nsPtrList;
                        //added 2004/02/13(Fri ;-)
                        xmlFree(nsPtrList);
                    }
                }
                else {
                    newNs = xmlSearchNsByHref(doc,
                                              node,
                                              (xmlChar*)nsDef->href.c_str());
                    if (!newNs) {
                        newNs = xmlNewNs(node, 
                                         (xmlChar*)nsDef->href.c_str(),
                                         (xmlChar*)nsDef->prefix.c_str());
                    }
                }
                //xmlSetNs(newNode, newNs);
            }
            xmlSetNsProp(node, newNs,
                         (xmlChar*)targetDataSet->getName().c_str(),
                         (xmlChar*)value.c_str());

            return 0;
        } 

        //now, this node is target..
        //has child, to next
        if (next_) {
            if (isTarget_) {
                AppLogger::Write(APLOG_WARN,"%s%s",METHOD_LABEL
					,"current instance is target, but next_ has...");
//                LOGPRINT(TAG_WRN)
//                    << METHOD_LABEL
//                    << "current instance is target, but next_ has..." 
//                    << endl;
            }

            return next_->attach(doc, node->children, node,
                                 value, updateFlg, useNs, nsDef,
                                 insPos);
        }
        result = 0; // 0 = SUCCESS

        //exit, if not multi
        if (!isMulti_) {
            break;
        }
        //next node
        node = node->next;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
					,"return",result);
//    cerr << "return =(" << result << ")" << endl;
#endif //DEBUG

    return result;
}

int XmlQuery2::del(xmlDocPtr& doc, xmlNodePtr node)
{
    int result = -101; // -101 = target not found
    
    while (NULL != node) {
        
        //check node name
        if (QUESTION_MARK == node_.getName()) {
            //request tag name 
            //proc. complete
            xmlUnlinkNode(node);
            xmlFreeNode(node);
            result = 0; // 0 = SUCCESS
            break;
        }
        else {
            if (node_.getName() != (const char*)node->name) {
                //not target noden
                node = node->next;
                continue;
            }
            //check node element
            if (!node_.isNullValue()) {
                if (QUESTION_MARK == node_.getValue()) {
                    //proc. complete
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    result = 0; // 0 = SUCCESS
                    break;
                }
                //has condition
                xmlChar* element = 
                    xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
                string elementString;
                if (element) {
                    elementString = (const char*)element;
                    xmlFree(element);
                }
                if (elementString != node_.getValue()) {
                    //not target
                    //skip...
                    node = node->next;
                    continue;
                }
                //next attributes check...
            }
            
            //check attributes
            std::vector<DataSet>::const_iterator pos;
            bool isValidAttr = true;
            for (pos = attributes_.begin(); pos != attributes_.end(); ++pos) {
                if (QUESTION_MARK == pos->getValue()) {
                    //proc. complete
                    xmlUnlinkNode(node);
                    xmlFreeNode(node);
                    result = 0; // 0 = SUCCESS
                    break;
                }
                
                xmlChar* attr = 
                    xmlGetProp(node, 
                               (const xmlChar*)pos->getName().c_str());
                if (NULL == attr) {
                    //proc. imcomplete
                    return -103; // -103 = target attr. not exists
                }
                string attrString((const char*)attr);
                xmlFree(attr);
                if (attrString != pos->getValue()) {
                    //check condition
                    //not target node
                    isValidAttr = false;
                    break;
                }
            } // end for ();
            if (!isValidAttr) {
                node = node->next;
                continue;
            }
        
            //then this node is target
            //next for child
            if (next_) {
                if (isTarget_) {
                    AppLogger::Write(APLOG_WARN,"%s::%s:%s"
							,CLASS_SIG.c_str(),__func__
							,"current instance is target, but next_ has...");
//                    cerr << TAG_WRN << CLASS_SIG << 
//                        "::del: current instance is target, but next_ has..." << endl;
                }
                return next_->del(doc, node->xmlChildrenNode);
            }
        }
        
        result = 0; // 0 = SUCCESS
        
        break;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s::%s:%s=(%d)"
					,CLASS_SIG.c_str(),__func__
					,"return",result);
//    cerr << "return =(" << result << ")" << endl;
#endif
    
    return result;
}

// End of XmlQuery2.cpp


