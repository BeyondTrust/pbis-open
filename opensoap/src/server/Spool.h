/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Spool.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SPOOL_H
#define SPOOL_H

#if defined(WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <fstream>
#include <map>
#include <string>

#include "SrvConf.h"

namespace OpenSOAP {
    class ChannelDescriptor;
  
    class Spool {
    private:
        SrvConf srvConf;
        std::string myselfUrl_;

#if defined(WIN32)
	HANDLE pop_thread;
	HANDLE spool_mutex;
	static int mtCount;
#else
        pthread_t pop_thread;
        pthread_mutex_t spool_mutex;
        //pthread_cond_t spool_cond;   
#endif
        std::string tableFile;
        std::ifstream min;
        std::ofstream mout;
    
        ChannelDescriptor chnlDesc_;  
    
        //const std::string NoEntryMsg;

    public:
        Spool(const std::string& filePass);
    
        ~Spool(); 
        void setChnlDesc(const ChannelDescriptor& pushChnlDesc);
        bool execPushOpr();
        bool execPopOpr();
        bool execExpireOpr();
    
    private:
        int fileOut();
        int push(const std::string& msgID, std::string fileID);
        std::string pop(const std::string& msgID, bool undelete = false);
        std::string del(const std::string& msgID);
        
    private:
        struct MapRecord{
            //string msgID;
            std::string fileID;
        };
        
        std::multimap<std::string, MapRecord> spoolMap;
    };
}

#endif  /* SPOOL_H */
