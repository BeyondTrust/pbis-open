/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgAttrHandler.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef MsgAttrHandler_H
#define MsgAttrHandler_H

#include <string>
#include <vector>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

/* libxml2 */
#if defined(HAVE_LIBXML2_XMLVERSION_H) 
#include <libxml2/parser.h>
#else
#include <libxml/parser.h>
#endif /* !HAVE_LIBXML2_XMLVERSION_H */

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {

    class OPENSOAP_CLASS MsgAttrHandler {
        
    public:
        MsgAttrHandler();
        MsgAttrHandler(const std::string& xmlMsg);
        virtual ~MsgAttrHandler();
        
        int setMsg(const std::string& xmlMsg);
        const std::string& getMsg() const {return xmlStr;}
        
        int queryXml(const std::string& query, 
                     std::vector<std::string>& values);

    protected:
        
        xmlDocPtr docPtr_;
	std::string xmlStr;
        
        //UNIXドメインソケットアドレス
        //std::string socketAddr_;
        
        //int connectManager();
    };
    
} // end of namespace OpenSOAP


#endif /* MsgAttrHandler_H */
