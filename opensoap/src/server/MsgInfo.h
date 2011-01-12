/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: MsgInfo.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef _MsgInfo_H
#define _MsgInfo_H

#include <string>
#include <vector>

using namespace std;

namespace OpenSOAPv1_2_00 {
    
    class MsgInfo {
    private:
        std::string RequestID;
        std::string ResponseID;
        
    public:
        //header item
        std::string messageId;
        bool async;
        bool hasForwardPath;
        std::vector<std::string> forwardPathArray;
        bool hasHopcount;
        int hopcount;
        std::vector<std::string> receivedPathUrlArray;
        std::vector<std::string> receivedPathTimeArray;
        bool hasTTLSecond;
        long ttlSecond;
        bool hasTTLHoptimes;
        long ttlHoptimes;
        bool hasTTLAsyncSecond;
        long ttlAsyncSecond;
        std::string backwardPath;
        bool isResponseMsg;
        bool undelete;
        
        bool in;
        bool operationExist;
        bool hasExtHeader;
        bool hasExtHeaderBlock;
        
        std::string methodName;
        std::string methodNamespace;

        void setAsync(const std::string& s);
        void setIn(const std::string& s);
        void setUndelete(const std::string& s);
        
    protected:
        static const std::string DELIMITER;
        
    public:
        MsgInfo();
        virtual ~MsgInfo();
        MsgInfo(MsgInfo& mi);
        
        void SetRequestID(const std::string& str);
        void SetResponseID(const std::string& str);
        
        void SetRequestID(const char * str);
        void SetResponseID(const char * str);
        
        const string& GetRequestID() const;
        const string& GetResponseID() const;
        
        virtual string toString() const;
        virtual string toString(int level) const;
        
    }; //class MsgInfo

} //namespace OpenSOAPv1_2_00

#endif //_MsgInfo_H
