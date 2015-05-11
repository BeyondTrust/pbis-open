/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTL.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef TTL_H
#define TTL_H

#if defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <fstream>
#include <map>
#include <string>
#include <ctime>

#include "ThreadDef.h"

#include "SrvConf.h"

namespace OpenSOAP {
    class ChannelDescriptor;
    class RequestMessage;
  
    class TTL {
    private:
        SrvConf srvConf;
        
#if defined(WIN32)
	static int mtCount;
#endif
	ThrFuncHandle watcher_thread;
	ThrMutexHandle ttl_mutex;

        std::string tableFile;
        std::ifstream min;
        std::ofstream mout;

        ChannelDescriptor chnlDesc_;  
    
  public:
        int watchInterval;
        //int ttlMaxValue;
    
  public:
        TTL(std::string filePass);
        ~TTL();
        void setChnlDesc(const ChannelDescriptor& chnlDesc);
        
        bool execPushOpr();
        bool execRefOpr();
        

    private:
        int fileOut();
        static 
        ThrFuncReturnType watcher(ThrFuncArgType arg);
        
        int sendEraseSignal(std::string msgID, std::string fileID);
    
        int push(const std::string& msgID, const std::string& fileID,
                 const time_t startTime, 
                 const int ttl, const std::string& backwardPath);
        std::string refBackwardPath(std::string msgID);

        //long getInvokeTTL(const RequestMessage& requestMessage);

    private:
        struct mapRecord{
            std::string msgID;
            std::string fileID;
            time_t startTime;
            int ttl;
            std::string backwardPath;
        };
    
        std::multimap<time_t, mapRecord> ttlMap;
    };
}

#endif /* TTL_H */
