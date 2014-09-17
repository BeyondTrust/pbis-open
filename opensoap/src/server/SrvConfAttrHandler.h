/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConfAttrHandler.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SrvConfAttrHandler_H
#define SrvConfAttrHandler_H

#include <string>
#include <vector>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {

    class OPENSOAP_CLASS SrvConfAttrHandler {
        
    public:
	SrvConfAttrHandler();
        virtual ~SrvConfAttrHandler();
        int queryXml(const std::string& query, std::vector<std::string>& values);
        
        //XMLファイルの再読み込み命令
        int reloadXml();
        
        static const std::string SERVER_CONF_TAG; //for query string
        
    protected:

#if defined(WIN32)
	int
#else
        //UNIXドメインソケットアドレス
        std::string 
#endif
    	socketAddr_;
        
        int connectManager();
        
        bool getShmCache(const std::string& query, 
                         std::vector<std::string>& values);
        
    };

} // end of namespace OpenSOAP


#endif /* SrvConfAttrHandler_H */
