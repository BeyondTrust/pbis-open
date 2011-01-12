/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlQuery.cpp,v $
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

#include "XmlQuery.h"
#include "ServerCommon.h"
#include "SSMLAttrMgrProtocol.h"
#include "SoapDef.h"
#include "AppLogger.h"

//#define DEBUG

using namespace OpenSOAP;
using namespace std;
    
//parsing DOM
XmlQuery::XmlQuery(const string& query)
    : next_(NULL)
    , isTarget_(false)
    , isMulti_(false)
    , attributes_(0)
    //, values_(0)
{
    static char METHOD_LABEL[] = "XmlQuery::XmlQuery: ";

    //2003/08/26
    attributes_ = new std::vector<DataSet>;

    string queryStr(query);

    //split Node using NODE_DELMIT(="/")
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    //for quote block
    string::size_type quoteIdx = 0;

    begIdx = queryStr.find_first_not_of(NODE_DELMIT);
    if (begIdx == string::npos) {
		AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
					,"invalid query data. query",query.c_str());
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "invalid query data. query=[" << query << "]" 
//           << endl;
        throw exception();
    }

    endIdx = findDelmitPos(queryStr, begIdx, NODE_DELMIT);
    if (endIdx == string::npos) {
		AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
					,"invalid query data. query",query.c_str());
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "invalid query data. query=[" << query << "]" 
//            << endl;
        throw exception();
    }

    //target data for current tree-level
    string curQuery = queryStr.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"curQuery",curQuery.c_str());
//    cerr << "curQuery=[" << curQuery << "]" << endl;
#endif /* DEBUG */
    
    //data for next tree-level 
    queryStr = queryStr.substr(endIdx);

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"next queryStr",queryStr.c_str());
//    cerr << "next queryStr=[" << queryStr << "]" << endl;
#endif /* DEBUG */

    //-------------------------------------------------
    //if next tree-level exists, set instance into next_
    //-------------------------------------------------
    begIdx = queryStr.find_first_not_of(NODE_DELMIT);
    if (begIdx != string::npos) {
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"next Query create.",queryStr.c_str());
//        cerr << "next Query create." << endl;
#endif /* DEBUG */
        
        next_ = new XmlQuery(queryStr);
    }
    
    //parsing data
    //split tag-name and attributes
    begIdx = curQuery.find_first_not_of(ATTR_DELMIT);
    if (begIdx == string::npos) {
		AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
					,"invalid current query data. query",curQuery.c_str());
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "invalid current query data. query=[" << curQuery << "]" 
//            << endl;
        throw exception();
    }
    
    endIdx = curQuery.find_first_of(ATTR_DELMIT, begIdx);
    if (endIdx == string::npos) {
        endIdx = curQuery.length();
    }
    //tag-name
    string nameToken = curQuery.substr(begIdx, (endIdx-begIdx));
    addName(nameToken);
    
    for (;;) {
        begIdx = curQuery.find_first_not_of(ATTR_DELMIT, endIdx);
        if (begIdx == string::npos) {
            break;
        }
        endIdx = curQuery.find_first_of(ATTR_DELMIT, begIdx);
        if (endIdx == string::npos) {
            endIdx = curQuery.length();
        }
        string attrToken = curQuery.substr(begIdx, (endIdx-begIdx));
        
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"attrToken",attrToken.c_str());
//        cerr << "attrToken=[" << attrToken << "]" << endl;
#endif /* DEBUG */
        
        addAttributes(attrToken);
    }
}

//Destructor
XmlQuery::~XmlQuery()
{
  if (next_) { delete next_; }
  delete attributes_;
}

void 
XmlQuery::addName(const string& nameToken)
{
    static char METHOD_LABEL[] = "XmlQuery::addName: ";

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"arg",nameToken.c_str());
//    cerr << METHOD_LABEL << "::addName() arg=[" << nameToken << "]" << endl;
#endif /* DEBUG */

    //split attribute condition using VAL_DELMIT(="=")
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    
    begIdx = nameToken.find_first_not_of(VAL_DELMIT);
    if (begIdx == string::npos) {
		AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
					,"invalid name token data. nameToken",nameToken.c_str());
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "invalid name token data. nameToken=[" 
//            << nameToken << "]" 
//            << endl;
        throw exception();
    }
    
    endIdx = nameToken.find_first_of(VAL_DELMIT, begIdx);
    if (endIdx == string::npos) {
        endIdx = nameToken.length();
    }
  
    //name
    string name = nameToken.substr(begIdx, (endIdx-begIdx));
    //node_.setName(nameToken.substr(begIdx, (endIdx-begIdx)));
    
    //split condition
    begIdx = nameToken.find_first_not_of(VAL_DELMIT, endIdx);
    if (begIdx == string::npos) {
        //no target
#ifdef DEBUG
	    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
					,"no name condition.");
        cerr << METHOD_LABEL << "::addName: no name condition." << endl;
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
    if (endIdx == string::npos) {
        endIdx = nameToken.length();
    }
    string cond = nameToken.substr(begIdx, (endIdx-begIdx));
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"name cond",cond.c_str());
//    cerr << METHOD_LABEL << "::addName: name cond=[" << cond << "]" << endl;
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
        node_.setValue(QUESTION_MARK);
    }
    else {
        node_.setValue(cond);
    }
}

void 
XmlQuery::addAttributes(const string& attrToken)
{
    static char METHOD_LABEL[] = "XmlQuery::addAttributes: ";

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"arg",attrToken.c_str());
//    cerr << METHOD_LABEL << "::addAttributes() arg=[" 
//         << attrToken << "]" << endl;
#endif /* DEBUG */

    //split attributes using VAL_DELMIT(="=")
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
  
    begIdx = attrToken.find_first_not_of(VAL_DELMIT);
    if (begIdx == string::npos) {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
					,"invalid attr token data. attrToken",attrToken.c_str());
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "invalid attr token data. attrToken=[" 
//            << attrToken << "]" 
//            << endl;
        throw exception();
    }
  
    endIdx = attrToken.find_first_of(VAL_DELMIT, begIdx);
    if (endIdx == string::npos) {
        endIdx = attrToken.length();
    }
  
    DataSet attrDataSet;

    //attributes name
    string attrName = attrToken.substr(begIdx, (endIdx-begIdx));
    attrDataSet.setName(attrName);

    //split condition
    begIdx = attrToken.find_first_not_of(VAL_DELMIT, endIdx);
    if (begIdx == string::npos) {
        //no target
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
					,"no attr condition. attr",attrName.c_str());
//        LOGPRINT(TAG_ERR)
//            << METHOD_LABEL
//            << "no attr condition. attr=[" 
//            << attrName << "]" 
//            << endl;
        throw exception();
    }
    
    endIdx = attrToken.find_first_of(VAL_DELMIT, begIdx);
    if (endIdx == string::npos) {
        endIdx = attrToken.length();
    }
    string cond = attrToken.substr(begIdx, (endIdx-begIdx));
    
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"attr cond",cond.c_str());
//    cerr << METHOD_LABEL << "::addAttributes: attr cond=[" 
//         << cond << "]" << endl;
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
    attributes_->push_back(attrDataSet);
}

//values getter
//return true in success
//Å¶set RootNode into RootQuery
//int XmlQuery::getValue(xmlDocPtr& doc, xmlNodePtr node)
int 
XmlQuery::getValue(xmlDocPtr& doc, xmlNodePtr node,
                   vector<string>& values)
{
    static char METHOD_LABEL[] = "XmlQuery::getValue: ";

    //bool result = false;
    int result = -101; // -101 = target not found

    //temp.buffer
    string answer;
    
    int i = 0;

    while (NULL != node) {
        
        //check node name
        if (QUESTION_MARK == node_.getName()) {
            //request tag name 
            answer = (const char*)node->name;
        }
        else {
            if (0 != strcmp((const char*)node->name, node_.getName().c_str())){
                //no target node
                node = node->next;
                continue;
            }
            //check node element
            if (!node_.isNullValue()) {
                //target element exists
                xmlChar* element = xmlNodeListGetString(doc, 
                                                        node->xmlChildrenNode, 
                                                        1);
                //modified using elementString and free element [20031029]
                string elementString;
                if (element) {
                    elementString = (const char*)element;
                    xmlFree(element);
                }
                if (QUESTION_MARK == node_.getValue()) {
                    //if target result, set buffer
                    answer = elementString;
                }
                else if (node_.getValue().c_str() != elementString) {
                    //check condition
                    //no target node
                    node = node->next;
                    continue;
                }
            }
     
            //check attributes
            vector<DataSet>::const_iterator pos;
            bool isValidAttr = true;
            const  xmlChar* attr = NULL;
            xmlChar* allocatedAttr = NULL;
            string attrString;
            bool hasAttr = false;
            for (pos=attributes_->begin(); pos != attributes_->end(); ++pos) {

                //modified 2004/01/08
                string nsKey(pos->getName());
                string cmpSubStr = 
                    nsKey.substr(0, OpenSOAP::XMLDef::XMLNS.length());
                if (cmpSubStr == OpenSOAP::XMLDef::XMLNS) {

                    string nsPrefix;
                    string::size_type idx = 0;
                    idx = nsKey.find(':');
                    if (string::npos != idx) {
                        nsPrefix = nsKey.substr(idx+1, nsKey.length());
                    }
                    //end modified 2004/01/08

                    // get scoped namespace list
                    //xmlNsPtr* nsPtr = xmlGetNsList(doc, node);
                    
                    xmlNsPtr nsPtr = node->nsDef;

                    // check target element namespace prefix
                    //const xmlChar* targetPrefix = NULL;
                    //modified 2004/01/08
                    if (nsPrefix.empty() && node->ns && node->ns->prefix) {
                        nsPrefix = (char*)node->ns->prefix;
                    }
 
                    if (nsPtr) { //modified 2004/01/19
                        
                        // edited 2003/06/02
                        //xmlNsPtr tmp = *nsPtr;
                        xmlNsPtr tmp = nsPtr;
                        while(tmp) {
#ifdef DEBUG
                            AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s] %s=[%s]"
                                       ,METHOD_LABEL
                                       ,"xmlNsPtr: prefix",tmp->prefix
                                       ,"href",tmp->href);
//                            cerr << "xmlNsPtr: prefix=[" << tmp->prefix
//                                 << "] href=[" << tmp->href << "]" << endl;
#endif //DEBUG
                            if (!(tmp->prefix) && nsPrefix.empty() ||
                                nsPrefix == (const char*)tmp->prefix) {
                                
                                attr = tmp->href;
                                if (attr) {
                                    hasAttr = true;
                                    attrString = (const char*)attr;
                                }
                                break;
                            }
                            tmp = tmp->next;
                        }

#ifdef DEBUG
                        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]"
                                        ,METHOD_LABEL
                                        ,"########## ns:href",attrString.c_str()
                                        );
//                        cerr << "########## ns:href=[" 
//                             << attrString << "]" << endl;
#endif //DEBUG
                    }
                }
                else {
                    allocatedAttr = xmlGetProp(node, 
                                      (const xmlChar*)pos->getName().c_str());

                    if (allocatedAttr) {
                        hasAttr = true;
                        attrString = (const char*)allocatedAttr;
                        xmlFree(allocatedAttr);
                    }
                }
                if (false == hasAttr) {
                    return -103; // -103 = target attribute not exists
                }
                if (QUESTION_MARK == pos->getValue()) {
                    //if target result, set buffer
                    answer = attrString;
                }
                // different namespace 'aaa' and 'aaa/'
                else if (attrString != pos->getValue()) {
                    //check condition
                    //no target node
                    isValidAttr = false;
                    break;
                }
            } // end for ();
            if (!isValidAttr) {
                node = node->next;
                continue;
            }
            //now, this current node is target
            //handover to children instance
            if (next_) {
                
                if (isTarget_) {
                    AppLogger::Write(APLOG_WARN,"%s%s"
                            ,METHOD_LABEL
                            ,"current instance is target, but next_ has...");
//                    LOGPRINT(TAG_WRN)
//                        << METHOD_LABEL 
//                        << "current instance is target, but next_ has..."
//                        << endl;
                }
                return next_->getValue(doc, node->xmlChildrenNode, values);
            }
        }

        //add result
        values.push_back(answer);
        result = 0; // 0 = SUCCESS

        //exit loop if not multi
        if (!isMulti_) {
            break;
        }

        //check next node
        node = node->next;
    
        i++;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                    ,"return",result);
//    cerr << "return =(" << result << ")" << endl;
#endif
    return result;
}

void 
XmlQuery::spy() const
{
    static char METHOD_LABEL[] = "XmlQuery::spy: ";
    cerr << METHOD_LABEL << "#####" << endl;
    cerr << METHOD_LABEL << "node_:" << endl;
    node_.spy();
    
    cerr << "  isTarget_:" << isTarget_ << endl;
    cerr << "  isMulti_:" << isMulti_ << endl;
    
    cerr << METHOD_LABEL << "attributes_:" << endl;
    vector<DataSet>::const_iterator pos;
    for (pos = attributes_->begin(); pos != attributes_->end(); ++pos) {
        cerr << " -------------" << endl;   
        pos->spy();
    }
  
    if (next_) { next_->spy(); }
}

bool 
XmlQuery::isMulti() const
{
    static char METHOD_LABEL[] = "XmlQuery::isMulti: ";
    if (isTarget_) {
        return isMulti_;
    }
    for (XmlQuery* cur = next_; NULL != cur; cur = cur->next_) {
        if (cur->isTarget_) {
            return cur->isMulti_;
        }
    }
    //warning
    AppLogger::Write(APLOG_WARN,"%s%s"
                    ,METHOD_LABEL
                    ,"target not found.");
//    LOGPRINT(TAG_WRN)
//        << METHOD_LABEL 
//        << "target not found." 
//        << endl;

    return false;
}

string::size_type
XmlQuery::findDelmitPos(const string& s, 
			string::size_type beginPos, 
			const string del)
{
    static char METHOD_LABEL[] = "XmlQuery::findDelmitPos: ";
    string::size_type endIdx = 0;
    endIdx = s.find_first_of(del, beginPos);

    if (endIdx == string::npos) {
        return s.length();
    }
    else {
        string::size_type quoteIdx = 0;
        //string::size_type quoteEndIdx = 0;

        quoteIdx = s.find_first_of("'", beginPos);
        if (quoteIdx == string::npos) {
            return endIdx;
        }
        else if (quoteIdx < endIdx) {
            quoteIdx = s.find_first_of("'", quoteIdx+1);
            if (quoteIdx == string::npos) {
                return string::npos;
            }
            return findDelmitPos(s, quoteIdx+1, del);
        }
        else {
            return endIdx;
        }
    }
}

void 
XmlQuery::DataSet::setValue(const string& aValue) 
{ 
    static char METHOD_LABEL[] = "XmlQuery::DataSet::setValue: ";
    string tmpStr(aValue);
    if (tmpStr[0] == '\'') {
        tmpStr = tmpStr.substr(1, tmpStr.length()-1);
    }
    if (tmpStr[tmpStr.length()-1] == '\'') {
        tmpStr = tmpStr.substr(0, tmpStr.length()-1);
    }
    if (!value_) { 
        value_ = new string(tmpStr);
    }
    else {
        *value_ = tmpStr;
    }
}

// End of XmlQuery.cpp

