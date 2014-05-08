/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlDoc.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include <sstream>
#include <fstream>

#include <stdexcept>

//for libxml2
#include <libxml/parser.h>

#include "XmlDoc.h"

using namespace std;
using namespace OpenSOAP;

XmlDoc::XmlDoc()
    : doc(NULL)
    , rootElem(NULL)
{
    
}

XmlDoc::~XmlDoc()
{
    if (doc) {
        //release doc
        xmlFreeDoc(doc);
    }
}

void
XmlDoc::parse()
{
/*
    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (node == NULL) {
        throw runtime_error("invalid rootElem");
    }
*/
/*
    rootElem = new XmlElem(node);
    rootElem.parse();
*/
}

void 
XmlDoc::serialize(string& str) const
{
    vector<char> barray;
    serialize(barray);
    str = string(&(*barray.begin()), barray.size());
}

void 
XmlDoc::serialize(vector<char>& bytearray) const
{
    xmlChar* dumpArea = NULL;
    int dumpSize = 0;
    xmlDocDumpMemory(doc, &dumpArea, &dumpSize);
    //xmlDocDumpFormatMemory(doc, &dumpArea, &dumpSize, 1);
    bytearray.resize(dumpSize);
    memcpy(&(*bytearray.begin()), dumpArea, bytearray.size());
    xmlFree(dumpArea);
}

void 
XmlDoc::serialize(ostream& ost) const
{
    vector<char> barray;
    serialize(barray);
    ost.write(&(*barray.begin()), barray.size());
}

void 
XmlDoc::deserialize(const string& str)
{
    if (doc) {
        //release doc
        xmlFreeDoc(doc);
    }
    doc = xmlParseMemory(str.c_str(), str.length());    
}

void 
XmlDoc::deserialize(const vector<char>& bytearray)
{
    if (doc) {
        //release doc
        xmlFreeDoc(doc);
    }
    doc = xmlParseMemory(&(*bytearray.begin()), bytearray.size());
}

void 
XmlDoc::deserialize(const istream& ist)
{
    stringstream strst;
    strst << ist.rdbuf();
    string xmlString(strst.str());
    if (doc) {
        //release doc
        xmlFreeDoc(doc);
    }
    doc = xmlParseMemory(xmlString.c_str(), xmlString.length());
}

//------------------------------------
// End of XmlDoc.cpp
//------------------------------------
