/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConf.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SrvConf_H
#define SrvConf_H

#include <string>
#include <vector>
#include <libxml/tree.h>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP {

    class OPENSOAP_CLASS SrvConf {
        
        static const std::string SERVER_CONF_FILE; // = "server.conf";

    public:
        SrvConf();
        virtual ~SrvConf();
        
        std::string getLogPath();
        std::string getSoapMessageSpoolPath();
        std::string getAsyncTablePath();
        std::string getPIDPath();
        std::string getSocketPath();
        std::string getSecKeyPath();
        std::string getSSMLPath();
        std::string getSSMLInternalServicesPath(); //added 2004/01/04
        //std::string getSSMLReplyToPath();        //added 2004/01/04
        std::vector<std::string> getBackwardUrls();
        
        std::string getForwarderUrl();
        long getForwarderTimeout();
        std::string getAddSignature();
        bool isAddSignatureTrue();
        
        // 2003/06/03
        long getLimitSOAPMessageSize();
        
        //timeout for invoke
        long getLimitTTLSecond();
        //limit for forwarding
        long getLimitTTLHoptimes();
        //expire for TTLTable
        long getLimitTTLAsync();

        std::string getDefaultCharEncoding();

        int query(const std::string& queryStr, 
                  std::vector<std::string>& values);
        int cquery(const std::string& queryStr
                  ,std::vector<std::string>& values);
       
    protected:
        //読み込み対象XMLファイル
        static std::string xmlFilePath_;
        
        //DOMデータ
        static xmlDocPtr confDocPtr_;
        //DOMデータ
        static xmlDocPtr confNowDocPtr_;
        static time_t    conftime_;
        
        //XMLファイルの読み込み
        bool loadXml();
        xmlDocPtr loadConf(std::string file);
        int query_(xmlDocPtr doc
                  ,const std::string& queryStr
                  ,std::vector<std::string>& values);
    };

} // end of namespace OpenSOAP


#endif /* SrvConf_H */
