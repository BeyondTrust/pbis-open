/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLInfo.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SSML_INFO_H
#define SSML_INFO_H

#include <string>
#include "SSMLAttrMgrProtocol.h"

namespace OpenSOAP {
    class SSMLInfo 
    {
    protected:
        std::string _operationName;
        std::string _namespace;
        std::string _operationType;
        std::string _execProg;
        unsigned int _asyncTTL;
        unsigned int _syncTTL;
        unsigned int _hoptimesTTL;
        //std::string _ttlCount;
        std::string _connectionMethod;
        std::string _sendFIFOName;
        std::string _recvFIFOName;
    
        std::string _hostname;
        unsigned int _port;
    
        std::string _endPointUrl;
        SSMLType _ssmlType;     //added 2004/01/04
        
    public:
        SSMLInfo();
//      ~SSMLInfo();
        const std::string& getOperationName() const;
        void setOperationName(const std::string& operationName);
        
        const std::string& getNamespace() const;
        void setNamespace(const std::string& ns);
        
        const std::string& getOperationType() const;
        void setOperationType(const std::string& operationType);
        
        const std::string& getExecProg() const;
        void setExecProg(const std::string& execProg);
        
        unsigned int getSyncTTL() const;
        void setSyncTTL(const unsigned int ttl);
        
        unsigned int getAsyncTTL() const;
        void setAsyncTTL(const unsigned int ttl);
        
        unsigned int getTTLHoptimes() const;
        bool setTTLHoptimes(const unsigned int ttl);
        
        const std::string& getConnectionMethod() const;
        bool setConnectionMethod(const std::string& connectionMethod);
        
        const std::string& getSendFIFOName() const;
        void setSendFIFOName(const std::string& sendFIFOName);
    
        const std::string& getRecvFIFOName() const;
        void setRecvFIFOName(const std::string& recvFIFOName);
        
        const std::string& getHostname() const;
        void setHostname(const std::string& hostname);
        
        unsigned int getPort() const;
        void setPort(const unsigned int port);

        //added 2003/06/18
        const std::string& getEndPointUrl() const;
        void setEndPointUrl(const std::string& endpointUrl);

        //added 2004/01/04
        const SSMLType& getSSMLType() const;
        void setSSMLType(const SSMLType ssmlType);
        
    };
  
}// end of namespace OpenSOAP

#endif /* SSML_INFO_H */
