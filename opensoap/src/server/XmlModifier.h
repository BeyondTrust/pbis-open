/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: XmlModifier.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef XmlModifier_H
#define XmlModifier_H

#include <string>
#include <vector>
#include <libxml/tree.h>
//#include <parser.h>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {
  
  class OPENSOAP_CLASS XmlModifier {

  public:
    typedef struct NameSpaceDefTag {
      std::string href;
      std::string prefix;
    } NameSpaceDef;

  public:
    XmlModifier(const std::string& xmlMsg);
    XmlModifier(xmlDocPtr& doc);
    virtual ~XmlModifier();

    //not use namespace
    int attach(const std::string& fmt, const std::string& value);
    //use namespace
    int attach(const std::string& fmt, 
	       const std::string& value,
	       const NameSpaceDef* nsDef);
    //fmt : "/Envelope/Header/opensoap-header-block/message_id=?"
    //value : "12345"
    //nsDef : NULL = parent node ns use
    
    //not use namespace
    int attachNoDuplicate(const std::string& fmt, const std::string& value);
    //use namespace
    int attachNoDuplicate(const std::string& fmt, 
			  const std::string& value,
			  const NameSpaceDef* nsDef);

    int update(const std::string& fmt, const std::string& value);

    //output XMLmessage
    std::string toString();

    //void spy() const;

    //not test
    int append(const std::string& fmt, const std::string& value);
    //use namespace
    int append(const std::string& fmt, const std::string& value,
	       const NameSpaceDef* nsDef);

    int del(const std::string& fmt); //fmt="/Envelope/xxx=?"

  protected:

    xmlDocPtr xmlDoc;
    bool refObj;
  };

} // end of namespace OpenSOAP


#endif /* XmlModifier_H */
