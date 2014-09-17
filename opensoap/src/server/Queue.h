/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Queue.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <fstream>
#include <vector>
#include <string>

#include "ThreadDef.h"

#define MAX_POP_THREAD_NUMBER 1
//#define INITIAL_STATE 0
//#define PROGRESS 1

namespace OpenSOAP {
    
    class ChannelDescriptor;
    class SrvConf;
    
    class Queue {
    public:
        Queue(std::string filePass, bool isFwdQueue);
        ~Queue(); 

        void setChnlDesc(const ChannelDescriptor& chnlDesc);
        bool execPushOpr();
        bool execExpireOpr();

    protected:
        //static const int MAX_POP_THREAD_NUMBER;
        typedef enum {
            INITIAL_STATE = 0,
            PROGRESS_STATE,
            DONE_SUCCESS_STATE,
            DONE_FAILURE_STATE,
            IGNORE_STATE,
        } ProcessStatus;

    protected:
        SrvConf* srvConf;
        ThrFuncHandle pop_thread[MAX_POP_THREAD_NUMBER];
        
#if defined(WIN32)
	static int mtCount_;
#endif 
        ThrMutexHandle queue_mutex;

	//semaphore for windows
	//cond_wait for pthread
        ThrCondHandle queue_cond;

	//HANDLE queue_semaphore;
        
        std::string tableFile;
        std::ifstream tin;
        std::ofstream tout;
        bool isFwdQueue_;
        struct TableRecord{
            //string msgID;
            std::string fileID;
            int proccessFlag; // INITIAL_STATE, PROGRESS
            int retryCount;
        };
        
        std::vector<TableRecord> queueTable;
        ChannelDescriptor chnlDesc_;  
    private:
        int push(const std::string& fileID);
        int del(const std::string& fileID);

        int fileOut();
        static 
        ThrFuncReturnType pop(ThrFuncArgType arg);
        
        int popOutToNextDrv(const std::string& fileID);
        int queueEraseAndFileOut(const std::string& fileID);
        
    };
}
#endif // QUEUE_H
