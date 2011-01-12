/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: TTL.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include <stdexcept>

#if defined(WIN32)
#include <process.h>
#else
#endif

/* Common Library */
#include "SOAPMessageFunc.h"
#include "FileIDFunc.h"

#include "StringUtil.h"

/* Server Common Library */
#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "TTL.h"
#include "SrvConf.h"
#include "RequestMessage.h"
#include "SSMLInfo.h"
#include "SSMLInfoFunc.h"
#include "SpoolAdopter.h"
#include "ProcessInfo.h"

#include "AppLogger.h"

//#define DEBUG

using namespace OpenSOAP;
using namespace std;

//windows—pmutex‚h‚cƒJƒEƒ“ƒg
#if defined(WIN32)
int TTL::mtCount = 0;
#endif

// Constructor
// 
TTL::TTL(string filePass)
{
    static char METHOD_LABEL[] = "TTL::TTL:";

  watchInterval = 1;
  // Multimap Key: ExpireTime,
  //        Value: Structure[msgID,fileID,startTime,ttl,backwardPath]
  mapRecord m;

  time_t expireTime;

  ttlMap.clear();
#if 1
  tableFile = srvConf.getAsyncTablePath() + filePass;
#else
  SrvConf* srvConf = new SrvConf();
  tableFile = srvConf->getAsyncTablePath() + filePass;
  delete srvConf;
#endif

  //debug
  AppLogger::Write(APLOG_DEBUG9,"%s tableFile=[%s]",
                   METHOD_LABEL,
                   tableFile.c_str());

  // Read from Backup File
  //
  min.open(tableFile.c_str(), ios::in);
  if (min.fail()) {
      AppLogger::Write(APLOG_INFO, "%s %s [%s]",
                       METHOD_LABEL,
                       "table file not found",
                       tableFile.c_str());
  }
  else {
      AppLogger::Write(APLOG_DEBUG, "%s %s [%s]",
                       METHOD_LABEL,
                       "load table file",
                       tableFile.c_str());

      while (!min.eof() && !min.fail()) {
          min >> expireTime;
          min >> m.msgID;
          min >> m.fileID;
          min >> m.startTime;
          min >> m.ttl;
          min >> m.backwardPath;
          if (!min.eof()) {
              //swap
              if (m.backwardPath == ".") {m.backwardPath = "";}

              AppLogger::Write(
                  APLOG_DEBUG,
                  "%s %s %s=[%s] %s[%s] %s[%s] %s[%s] %s[%d%s] %s[%s]",
                  METHOD_LABEL,
                  "load table file",
                  "Expire Datetime:", asctime(localtime(&expireTime)),
                  "msgID:", m.msgID.c_str(),
                  "fileID:",  m.fileID.c_str(),
                  "Start Datetime", asctime(localtime(&m.startTime)),
                  "TTL:", m.ttl, "sec",
                  "BackwardPath:", m.backwardPath.c_str());

              ttlMap.insert(pair<time_t, mapRecord>(expireTime, m));
          }
      }
      min.close();
  }

  // Thread Initialize
#if defined(WIN32)
  if (mtCount > 10000) {
      mtCount = 0;
  }
  string mtId = "TTL" + StringUtil::toString(mtCount++);
  ttl_mutex = CreateMutex(NULL, FALSE, mtId.c_str());
#else
  pthread_mutex_init(&ttl_mutex, NULL);
#endif

#if defined(WIN32)
  long status = _beginthread(watcher, 0, this);
#else
  pthread_create(&watcher_thread, NULL, watcher, this);
#endif
}

void
TTL::setChnlDesc(const ChannelDescriptor& chnlDesc)
{
    chnlDesc_ = chnlDesc;
}

// Backup File Output
// Return Value [success:0, failure:1]
//
int
TTL::fileOut()
{
    static char METHOD_LABEL[] = "TTL::fileOut:";

    mout.open(tableFile.c_str(), ios::out);
    if (mout.fail()) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s]",
                         METHOD_LABEL,
                         "Map_file output open error",
                         tableFile.c_str());
        return 1;
    }
    
    multimap<time_t, mapRecord>::iterator itr = ttlMap.begin();
    multimap<time_t, mapRecord>::iterator itrEnd = ttlMap.end();
    for (; itr != itrEnd; itr++) {
        mout << itr->first << "\t" 
             << itr->second.msgID << "\t" << itr->second.fileID << "\t" 
             << itr->second.startTime << "\t" << itr->second.ttl << "\t"
             << (itr->second.backwardPath.empty()?".":itr->second.backwardPath)
             << endl ;
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG, "%s %s=(%d) %s=[%s] %s=[%s] %s=(%d) %s=(%d) %s=[%s]",
                         METHOD_LABEL,
                         "expireTime",
                         itr->first,
                         "messae_id",
                         itr->second.msgID.c_str(),
                         "fileId",
                         itr->second.fileID.c_str(),
                         "startTime",
                         itr->second.startTime,
                         "ttl",
                         itr->second.ttl,
                         "backward_path",
                         itr->second.backwardPath.c_str());
#endif //DEBUG

        mout.flush();
    }

  mout.close();
  return 0;
}


// Push Method
// Return Value [success:0, failure: not 0]
//
int 
TTL::push(const string& msgID, const string& fileID, 
          const time_t startTime,
          const int ttl, const string& backwardPath)
{
    static char METHOD_LABEL[] = "TTL::push:";

    if (msgID.empty()) {
        return 1;
    }

    mapRecord m;

    m.msgID = msgID;
    m.fileID = fileID;
    m.startTime = startTime;
    m.ttl = ttl;
    m.backwardPath = backwardPath;

    // Mutex Lock
    Thread::lockMutex(ttl_mutex);
    
    // Insert to Multimap
    ttlMap.insert(pair<time_t, mapRecord>(startTime + ttl, m)); 

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG, "%s %s=(%d)",
                     METHOD_LABEL,
                     "map size",
                     ttlMap.size());
#endif //DEBUG

    // File Output
    int sf = fileOut();
    if (sf != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "Map file write error");
        return 2;
    }
    
    // Mutex Unlock
    Thread::unlockMutex(ttl_mutex);

    return 0;
}


// Watcher Thread
//
ThrFuncReturnType
TTL::watcher(ThrFuncArgType arg)
{
    static char METHOD_LABEL[] = "TTL::watcher:";

    // Pointer to Instance
    TTL* that = (TTL*)arg;
    if (NULL == that) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "Invalid argument");
        ReturnThread(NULL);
    }
    
    while (ProcessInfo::GetProcessStatus()!=PSTAT_WAITTERM) {
#if defined(WIN32)
        Sleep(that->watchInterval * 1000);
#else
        sleep(that->watchInterval);
#endif

        // Serch the Expirable Record in the TTLTable
        multimap<time_t, mapRecord>::iterator p = that->ttlMap.begin();
        multimap<time_t, mapRecord>::iterator pEnd = that->ttlMap.end();
        
        if (p != pEnd && p->first < time(NULL)) {
            
            // Mutex Lock
            Thread::lockMutex(that->ttl_mutex);
            
            for (; p != pEnd && p->first < time(NULL); p++) {
                // Find the Expirable Record
                
                AppLogger::Write(APLOG_INFO, "%s %s=[%s]",
                                 METHOD_LABEL,
                                 "TimeOut msgID",
                                 p->second.msgID.c_str());

                // Send the Signal to Queue,SpoolManager
                // for erase request message and queue table entry,
                // or response message and spool table entry.
                // **** no implimented ****
                int se = 
                    that->sendEraseSignal(p->second.msgID, p->second.fileID);
                //that->eraseSignalToFIFO(p->second.msgID, p->second.fileID);
                if (OPENSOAP_FAILED(se)) {
                    AppLogger::Write(APLOG_ERROR, "%s %s %s=[%s] %s=[%s]",
                                     METHOD_LABEL,
                                     "send Erase Signal failed.",
                                     "msgID",
                                     p->second.msgID.c_str(),
                                     "fileID",
                                     p->second.fileID.c_str());
                }
                //delete record
                that->ttlMap.erase(p);

                AppLogger::Write(APLOG_DEBUG9, "%s %s=(%d)",
                                 METHOD_LABEL,
                                 "check map erase: map size",
                                 that->ttlMap.size());
            }
            
            // Backup File Output
            int sf = that->fileOut();
            if (sf != 0) {
                AppLogger::Write(APLOG_ERROR, "%s %s",
                                 METHOD_LABEL,
                                 "Map file write error");
            }
            
            // Mutex unlock
            Thread::unlockMutex(that->ttl_mutex);

    	}
        //else if (p != pEnd && p->first > time(NULL)) {
    	//  cerr << "Not expired yet" << endl;
        //}
    }
    ReturnThread(NULL);
}

int 
TTL::sendEraseSignal(string msgID, string fileID)
{
    static char METHOD_LABEL[] = "TTL::sendEraseSignal:";

    // Queue Manager << fileID
    // Spool Manager << msgID

    try {
        //expire queue
        string queueExpireResult("---");

        //expire spool
        SpoolAdopter spoolAdptr;
        string resultOrFileID = spoolAdptr.expire(msgID);
        string spoolExpireResult;
        if (EntryStatus::NOENTRY == resultOrFileID) {
            //response not created yet 
            spoolExpireResult = "response not created yet.";
            //debug
            AppLogger::Write(APLOG_DEBUG9, "%s %s",
                             METHOD_LABEL,
                             "result is NOENTRY");
        }
        else if (Result::FAILURE == resultOrFileID) {
            AppLogger::Write(APLOG_ERROR, "%s %s %s=[%s]",
                             METHOD_LABEL,
                             "Spool Manager expire id failed.",
                             "targetId",
                             msgID.c_str());
            return OPENSOAP_FILE_ERROR;
        }
        else {
            spoolExpireResult = "deleted response. fileID=";
            spoolExpireResult += resultOrFileID;
            //debug
            AppLogger::Write(APLOG_DEBUG9, "%s %s",
                             METHOD_LABEL,
                             "result is SUCCESS");
        }

#if 0
        AppLogger::Write(APLOG_INFO, "%s %s %s=[%s] %s=[%s] %s=[%s]",
                         METHOD_LABEL,
                         "TTL Timeouted.",
                         "expire request fileID", fileID.c_str(),
                         "message_id", msgID.c_str(),
                         "status", queueExpireResult.c_str());
#endif //if 0
        
        AppLogger::Write(APLOG_INFO, "%s %s %s=[%s] %s=[%s]",
                         METHOD_LABEL,
                         "TTL Timeouted.",
                         "expire response message_id", msgID.c_str(),
                         "status", spoolExpireResult.c_str());
    }
    catch (runtime_error re) {
        return OPENSOAP_FILE_ERROR;
    }
    catch (exception e) {
        return OPENSOAP_FILE_ERROR;
    }

    return OPENSOAP_NO_ERROR;
}

//check internal table entry message_id
string 
TTL::refBackwardPath(string msgID)
{
    //init. value "No Entry"
    string backwardPath(OpenSOAP::EntryStatus::NOENTRY);

    // Mutex lock
    Thread::lockMutex(ttl_mutex);

    // Find the BackwardPath correponding to message_id
    multimap<time_t, mapRecord>::iterator p = ttlMap.begin();
    for (; p != ttlMap.end(); p++) {
        if (p->second.msgID == msgID) {
            //set value when entry exist
            backwardPath = p->second.backwardPath;
            break;
        }
    }

    // Mutex unlock
    Thread::unlockMutex(ttl_mutex);

    /* require ???
    if (backwardPath.empty()) {
        backwardPath = OpenSOAP::EntryStatus::NOVALUE;
    }
    */

    return backwardPath;
}

// Destructor
//
TTL::~TTL()
{
    //Mutext‚Ì”jŠü
#if defined(WIN32)
    CloseHandle(ttl_mutex);
#else
    pthread_join(watcher_thread, NULL);
    int status = pthread_mutex_destroy(&ttl_mutex);
    if (0 != status) {
        //throw OpenSOAPException();
        throw exception();
    }
#endif
}

bool 
TTL::execPushOpr()
{
    static char METHOD_LABEL[] = "TTL::execPushOpr:";

    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    //-----------------------------------------------------
    int ret = OPENSOAP_NO_ERROR;
    
    //read fileId of request soap message from MsgDrv
    string recvMsg;
    if (0 > chnlDesc_.read(recvMsg)) {
        AppLogger::Write(APLOG_ERROR,"%s %s",METHOD_LABEL
                         ,"read message id failed");
        return false;
    }

#ifdef DEBUG  
    AppLogger::Write(APLOG_DEBUG9,"%s %s=[%s]",
                     METHOD_LABEL,
                     "read message id from MsgDrv", 
                     recvMsg.c_str());
#endif //DEBUG
    //
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    int cnt = 0;
    string msgIDContent;
    string fileIDOfMsgFromMsgDrv;
    long ttl = 0;
    string backwardPath;


    for (;;) {
        begIdx = recvMsg.find_first_not_of(",", endIdx);
        if (begIdx == string::npos) {
            break;
        }
        endIdx = recvMsg.find_first_of(",", begIdx);
        if (endIdx == string::npos) {
            endIdx = recvMsg.length();
        }
        switch (cnt) {
        case 0:
            msgIDContent = recvMsg.substr(begIdx, (endIdx-begIdx));
            break;
        case 1:
            fileIDOfMsgFromMsgDrv = recvMsg.substr(begIdx, (endIdx-begIdx));
            break;
        case 2:
            StringUtil::fromString(recvMsg.substr(begIdx, (endIdx-begIdx)), ttl);
            break;
        case 3:
            backwardPath = recvMsg.substr(begIdx, (endIdx-begIdx));
            break;
        }
        cnt++;
    }

    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s %s=[%s] %s=[%s] %s=(%d) %s=[%s]",
                     METHOD_LABEL,
                     "msgIDContent", msgIDContent.c_str(),
                     "fileIDOfMsgFromMsgDrv", fileIDOfMsgFromMsgDrv.c_str(),
                     "ttl", ttl,
                     "backwardPath", backwardPath.c_str());

    string result(Result::SUCCESS);
    // regist to TTL Table
    ret = push(msgIDContent, fileIDOfMsgFromMsgDrv, time(NULL), 
               ttl, backwardPath);
    if (ret != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "Map insert failed.");
        result = Result::FAILURE;
    }
    
    //rsult return to MsgDrv
    if (0 > chnlDesc_.write(result)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "write failed to MsgDrv");
        return false;
    }

#ifdef DEBUG    
    AppLogger::Write(APLOG_DEBUG5, "%s %s=[%s]",
                     METHOD_LABEL,
                     "WRITE Result To MsgDrv" ,
                     result.c_str());
#endif //DEBUG

    return true;
}// end of execPushOpr()


bool 
TTL::execRefOpr()
{
    static char METHOD_LABEL[] = "TTL::execRefOpr: ";

    //
    //extract backward_path connected with message_id
    // - recv message_id
    // - check internal table : message_id<->backward_path
    //    return value is ..
    //     - message_id entry not exist : NOENTRY
    //     - exist and backward_path is empty : NOVALUE
    //     - else : backward_path
    // - send return value

    //read message_id from MsgDrv
    string msgIDContent;
    if (0 > chnlDesc_.read(msgIDContent)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "read failed from MsgDrv");
        return false;
    }

#ifdef DEBUG  
    AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                     METHOD_LABEL,
                     "READ <message_id> From MsgDrv",
                     msgIDContent.c_str());
#endif //DEBUG
    
    //return : backward_path / "No Entry"
    //(return : backward_path / "No Value" / "No Entry")
    string backwardPath = refBackwardPath(msgIDContent);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                     METHOD_LABEL,
                     "BackwardPath",
                     backwardPath.c_str());
#endif //DEBUG
  
    //sent result to MsgDrv
    if (0 > chnlDesc_.write(backwardPath)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "write failed to MsgDrv");
        return false;
    }
  
#ifdef DEBUG
    AppLogger::Write(APLOG_ERROR, "%s %s=[%s]",
                     METHOD_LABEL,
                     "WRITE To MsgDrv",
                     backwardPath.c_str());
#endif //DEBUG

    return true;
} // end of execRefOpr()

// End of TTL.cpp


