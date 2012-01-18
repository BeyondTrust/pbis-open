/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlDoc.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef XML_DOC_H
#define XML_DOC_H

#include <string>
#include <vector>
#include <iostream>

//for libxml2
#include <libxml/tree.h>

namespace OpenSOAP {
    typedef xmlNode XmlElem;
    
    class XmlDoc {
      public:
        XmlDoc();
        virtual ~XmlDoc();
        void serialize(std::string& str) const;
        void serialize(std::vector<char>& bytearray) const;
        void serialize(std::ostream& ost) const;
        void deserialize(const std::string& str);
        void deserialize(const std::vector<char>& bytearray);
        void deserialize(const std::istream& ist);
        
        XmlElem* getRootElem() { return rootElem; }
        xmlDocPtr& getDoc() { return doc; }

        std::string toString() const {
            std::string str;
            serialize(str);
            return str;
        }
        
        //private:
    protected:
        virtual void parse();
        //void deserialize();
        
        xmlDocPtr doc;
        XmlElem* rootElem;
    };
}

#endif //XML_DOC_H
