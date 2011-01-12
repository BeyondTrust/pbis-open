/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: RequestMessage.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef REQUEST_MESSAGE_H
#define REQUEST_MESSAGE_H

#include <string>
#include <vector>

#include <OpenSOAP/Defines.h>

namespace OpenSOAP
{
    class OPENSOAP_CLASS RequestMessage {
    public:
        typedef enum {
            FINISH_NOT_YET = 0,
            FINISH_SUCCESS,
            FINISH_FAILURE,
        } FinishStatus;

    public:
        RequestMessage(const std::string& spoolPath);
        ~RequestMessage();
        
        int extractStorage(const std::string& id);
        int storeStorage();
        
        //check message pattern
        bool isRequestOfPopResponse() const;
        bool isReturnedResponse() const;
        bool isAsync() const;

    protected:
        int setSoapMessage(const std::string& soapMsg);

    public:
        const std::string& getSoapMessage() const { return soapMessage_; }

        std::vector<std::string>& getReceivedPathArray();
        std::vector<std::string>& getReceivedTimeArray();
        std::vector<std::string>& getForwardPathArray();
        const std::string& getStorageId() const { return storageId_; }

        //edit message
        int pushReceivedPathStackToHeader(const std::string& myselfUrl);
        int popForwardPathStackFromHeader();

        int decrementHopcountInHeader();
        int attachOrSwapBackwardPathInHeader(const std::string& myselfUrl);

        int attachMessageIdInHeader(const std::string& messageId);

        int addSignature(const std::string& key);

        const std::string& getMethodName() const;
        const std::string& getMethodNamespace() const;
        const std::string& getMessageId() const;

        const std::string& getBackwardPath() const {return backwardPath_;}

        //zero is valid value
        bool hasHopcount() const { 
            return hasHopcount_;
            //return (hopcount_ == DEFAULT_HOPCOUNT_VAL) ? false : true;
        }
        long getHopcount() const;
        bool hasTTLSecond() const { return hasTTLSecond_; }
        long getTTLSecond() const;
        bool hasTTLHoptimes() const { return hasTTLHoptimes_; }
        long getTTLHoptimes() const;
        long getTTLAsyncSecond() const;
        bool hasTTLAsyncSecond() const { return hasTTLAsyncSecond_; }

        //relate TTLTable info.
        bool hasTTLEntry() const {return hasTTLEntry_;}
        const std::string& getBackwardPathInTTL() const {
            return backwardPathInTTL_;
        }
        bool forFinalReceiver() const {return forFinalReceiver_;}
        void setBackwardPathInTTL(const std::string& bwPath);

        //result 
        // check
        FinishStatus getResultStatus() const {return resultStatus_;}
        // set
        void finishSuccess() {resultStatus_ = FINISH_SUCCESS;}
        void finishFailure() {resultStatus_ = FINISH_FAILURE;}

        //debug
        void spy() const;

    protected:
        static const int DEFAULT_HOPCOUNT_VAL; //-1

        void initMember();

        //tool
      std::string dataSpoolPath;

        //Storage Id
        std::string storageId_;

        //SOAP Message
        std::string soapMessage_;

        //status
        bool addedSignature_;

        //Header parts
        bool async_;
        //can't use object in Win32 DLL interface, so declare as pointer.
        //std::vector<std::string> forwardPathArray_;
        //std::vector<std::string> receivedPathArray_;
        //std::vector<std::string> receivedTimeArray_;
        std::vector<std::string>* forwardPathArray_;
        std::vector<std::string>* receivedPathArray_;
        std::vector<std::string>* receivedTimeArray_;
        std::string messageId_;
        bool hasHopcount_;
        bool hasTTLSecond_;
        bool hasTTLHoptimes_;
        bool hasTTLAsyncSecond_;
        long hopcount_;
        long ttlSecond_;
        long ttlAsyncSecond_;
        long ttlHoptimes_;
        std::string backwardPath_;
        bool responseMsg_;

        //Body parts
        std::string methodName_;
        std::string methodNamespace_;

        //Extend parts
        bool hasTTLEntry_;
        std::string backwardPathInTTL_;
        bool forFinalReceiver_;

        //Result status
        FinishStatus resultStatus_;

        //modified flag
        bool modified_;
    };
}

#endif //REQUEST_MESSAGE_H

