/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrHandler.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SSMLAttrHandler_H
#define SSMLAttrHandler_H

#include <string>
#include <vector>
#include "SSMLAttrMgrProtocol.h"

namespace OpenSOAP {

    class SSMLAttrHandler {

    public:
        SSMLAttrHandler();
        virtual ~SSMLAttrHandler();
        
        int queryXml(const std::string& query, 
                     std::vector<std::string>& values,
                     const SSMLType ssmlType = EXTERNAL_SERVICES);
        
    protected:
        
        std::string socketAddr_;

        int connectManager();
        
        bool getShmCache(const std::string& query, 
                         std::vector<std::string>& values,
                         const SSMLType ssmlType);
    };
    
} // end of namespace OpenSOAP


#endif /* SSMLAttrHandler_H */
