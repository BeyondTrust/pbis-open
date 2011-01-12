/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Spool.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#include "StringUtil.h"
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <iostream>

/* Common Library */
#include "SOAPMessageFunc.h"
#include "FileIDFunc.h"

/* Server Common Library */
#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "ThreadDef.h"
#include "StringUtil.h"

#include "Spool.h"
#include "SrvConf.h"
#include "AppLogger.h"

//#define DEBUG

using namespace OpenSOAP;
using namespace std;

//for windows mutex uniq.id
#if defined(WIN32)
int Spool::mtCount = 0;
#endif

//args: file path of spool data map
Spool::Spool(const string& filePass)//:NoEntryMsg("No Entry")
{
    static char METHOD_LABEL[] = "Spool::Spool:";

    // initiate the string which is returned in the case of no entry

    // Multimap（Key:msgID，Value:struct of records[member:fileID]）
    MapRecord m;

    string msgID;

    spoolMap.clear();
    SrvConf* srvConf = new SrvConf();
    tableFile = srvConf->getAsyncTablePath() + filePass;
    vector<string> urls = srvConf->getBackwardUrls();
    if (urls.size() > 0) {
        myselfUrl_ = urls[0];
    }
    else {
        myselfUrl_ = "http://";
        myselfUrl_ += getLocalhostName();
        myselfUrl_ += "/cgi-bin/soapInterface.cgi";
    }
    delete srvConf;

    //load backup file
    min.open(tableFile.c_str(), ios::in);
    if (min.fail()) {
        AppLogger::Write(APLOG_INFO, "%s %s [%s]",
                         METHOD_LABEL,
                         "map file not found",
                         tableFile.c_str());
    }
    else {
        //debug
        AppLogger::Write(APLOG_DEBUG, "%s %s [%s]",
                         METHOD_LABEL,
                         "load map file",
                         tableFile.c_str());

        while (!min.eof() && !min.fail()) {
            min >> msgID;
            min >> m.fileID;
            if (!min.eof()) {
                AppLogger::Write(APLOG_DEBUG, 
                                 "%s init: msgId=[%s] fileId=[%s]",
                                 METHOD_LABEL,
                                 msgID.c_str(),
                                 m.fileID.c_str());
                spoolMap.insert(pair<string, MapRecord>(msgID, m));    
            }
        }
        min.close();
    }

    //init mutex
#if defined(WIN32)
    if (mtCount > 10000) {
        mtCount = 0;
    }
    string mtId = "Spool" + StringUtil::toString(mtCount++);
    spool_mutex = CreateMutex(NULL, FALSE, mtId.c_str());
#else
    pthread_mutex_init(&spool_mutex, NULL);
#endif
    //pthread_cond_init(&spool_cond, NULL);
    //pthread_create(&pop_thread, NULL, pop, this);
}


Spool::~Spool()
{
    //pthread_join(pop_thread, NULL);
    //release mutex
#if defined(WIN32)
    CloseHandle(spool_mutex);
#else
    int status = pthread_mutex_destroy(&spool_mutex);
    if (0 != status) {
        //throw OpenSOAPException();
        throw exception();
    }
#endif
}

//records info. output to backup file
//ret: normal=0, error=1
int 
Spool::fileOut()
{
    static char METHOD_LABEL[] = "Spool::fileOut: ";

    mout.open(tableFile.c_str(), ios::out);
    if (mout.fail()) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s]",
                         METHOD_LABEL,
                         "Map_file output open error",
                         tableFile.c_str());
        return 1;
    }

    multimap<string, MapRecord>::iterator itr = spoolMap.begin();
    multimap<string, MapRecord>::iterator itrEnd = spoolMap.end();
    for (; itr != itrEnd; itr++) {
        mout << itr->first << "\t" << itr->second.fileID << endl ;
        AppLogger::Write(APLOG_DEBUG, "%s msgId=[%s] filId=[%s]",
                         METHOD_LABEL,
                         itr->first.c_str(),
                         itr->second.fileID.c_str());
        mout.flush();
    }
    
    mout.close();
    return 0;
}

//regist new record
//arg: message_id, response fileID
//ret: normal=0, error=not0
int 
Spool::push(const string& msgID, string fileID)
{
    static char METHOD_LABEL[] = "Spool::push:";

    int ret = OPENSOAP_NO_ERROR;

    MapRecord m;
    
    m.fileID = fileID;
    
    // lock Mutex
    Thread::lockMutex(spool_mutex);

    //insert msgID+record into multimap
    spoolMap.insert(pair<string, MapRecord>(msgID, m)); 

    AppLogger::Write(APLOG_DEBUG5, "%s %s=(%d)",
                     METHOD_LABEL,
                     "map size",
                     spoolMap.size());

    //output to backup file
    ret = fileOut();
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "Map file write error");
        return ret;
    }

    //unlock mutex
    Thread::unlockMutex(spool_mutex);

    return OPENSOAP_NO_ERROR;
}

string
Spool::del(const string& msgID)
{
    static char METHOD_LABEL[] = "Spool::del: ";

    string fileID = pop(msgID);
    if (EntryStatus::NOENTRY == fileID) {
        return fileID;
    }

    AppLogger::Write(APLOG_DEBUG, "%s %s for message_id=[%s] fileId=[%s]",
                     METHOD_LABEL,
                     "delete response file",
                     msgID.c_str(),
                     fileID.c_str());

    if (0 != deleteFileID(fileID, srvConf.getSoapMessageSpoolPath())) {
        AppLogger::Write(APLOG_ERROR, "%s %s fileId=[%s]",
                         METHOD_LABEL,
                         "request message delete failed.",
                         fileID.c_str());
        return Result::FAILURE;
    }

    return fileID;
}

// extract entry from internal table
string 
Spool::pop(const string& msgID, bool undelete)
{
    static char METHOD_LABEL[] = "Spool::pop: ";

    //init. value is "No Entry"
    string fileID = OpenSOAP::EntryStatus::NOENTRY; //NoEntryMsg;
  
    // mutex lock
    Thread::lockMutex(spool_mutex);

    // 渡されたメッセージIDをKeyとして，レコードをひとつ抽出．
    // 発見されたレコードは消去．(when undelete is not true)
    // なければ返すstringは初期値のまま．
    // 
    multimap<string, MapRecord>::iterator p = spoolMap.find(msgID);
    if (p != spoolMap.end()) {
        fileID = p->second.fileID;

        //
        AppLogger::Write(APLOG_DEBUG5, "%s %s=[%s]",
                         METHOD_LABEL,
                         "Poped FileID",
                         fileID.c_str());

        //check undelete flag: undelete is true no erase data
        if (!undelete) {
            spoolMap.erase(p);
        }
    }
  
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG, "%s %s=(%d)",
                     METHOD_LABEL,
                     "map size",
                     spoolMap.size());
#endif //DEBUG

    // バックアップファイル書き出し
    int ret = fileOut();
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "Map file write error");
    }

    // mutext unlock
    Thread::unlockMutex(spool_mutex);
    
    //return : fileID / "No Entry"
    return fileID;
}


void 
Spool::setChnlDesc(const ChannelDescriptor& chnlDesc)
{
  chnlDesc_ = chnlDesc;
}

bool 
Spool::execPushOpr()
{
    static char METHOD_LABEL[] = "Spool::execPushOpr:";

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9, "%s %s",
                     METHOD_LABEL, 
                     "called.");
#endif //DEBUG
  
    //read from MsgDrv
    //string fileIDOfMsgFromMsgDrv;
    string recvMsg;
    if (0 > chnlDesc_.read(recvMsg)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "read failed from MsgDrv");
        return false;
    }
  
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "READ From MsgDrv=[",
                     recvMsg.c_str());
#endif

    //
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    int cnt = 0;
    string msgIDContent;
    string fileIDOfMsgFromMsgDrv;

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
        }
        cnt++;
    }

    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s %s=[%s] %s=[%s]",
                     METHOD_LABEL,
                     "msgIDContent", msgIDContent.c_str(),
                     "fileIDOfMsgFromMsgDrv", fileIDOfMsgFromMsgDrv.c_str());

    // regist to spool
    string result(Result::SUCCESS);
    int ret = push(msgIDContent, fileIDOfMsgFromMsgDrv);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "push to spool failed");
        result = Result::FAILURE;
    }
  
    //result return to MsgDrv
    if (0 > chnlDesc_.write(result)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "write failed to MsgDrv");
        return false;
    }
  
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]", 
                     METHOD_LABEL,
                     "WRITE Result To MsgDrv",
                     result.c_str());
#endif

  return true;
}

//pop requirement from MsgDrv
bool 
Spool::execPopOpr()
{
    static char METHOD_LABEL[] = "Spool::execPopOpr:";

    //---------------------------------
    // outline
    //---------------------------------
    // recv fileID from MsgDrv
    // convert to RequestMsg from fileID
    // extract message_id and undelete tag
    // compare message_id with internal table
    //   matched : - return fileID of response
    //             - erase that entry when undelete is not true 
    //   no match : create fault and convert to fileID, return it
    //---------------------------------

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s",
                     METHOD_LABEL,
                     "called!!");

    // read FileID of request from MsgDrv 
    string recvMsg;
    if (0 > chnlDesc_.read(recvMsg)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "read failed from MsgDrv");
        return false;
    }
    
    
    //
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    int cnt = 0;
    string msgIDContent;
    bool undelete(false);

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
            StringUtil::fromString(recvMsg.substr(begIdx, (endIdx-begIdx)),
                                   undelete);
            break;
        }
        cnt++;
    }

    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s %s=[%s] %s=(%d)",
                     METHOD_LABEL,
                     "msgIDContent", msgIDContent.c_str(),
                     "undelete", undelete);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "READ message_id From MsgDrv",
                     recvMsg.c_str());
#endif //DEBUG
  
    // pop spool
    string fileIDOfResMsg = pop(msgIDContent, undelete);
    
    // In the case of no entry in spool
    // create SOAP fault
    if (fileIDOfResMsg == OpenSOAP::EntryStatus::NOENTRY) {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG, "%s %s",
                         METHOD_LABEL,
                         "Create No Entry Fault");
#endif //DEBUG
        string resMsg =
            createSOAPFaultMessage("SOAP-ENV:Server.EntryNotCreatedYet",
                                   "Response Entry Not Created Yet",
                                   myselfUrl_,
                                   "");
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG, "%s %s=[%s]",
                         METHOD_LABEL,
                         "Fault",
                         resMsg.c_str());
#endif //DEBUG

        fileIDOfResMsg 
            = convertToFileID(resMsg, srvConf.getSoapMessageSpoolPath());
    }
  
    // MsgDrv に該当するMsg ID をもったレスポンスを返す。
    
    if (0 > chnlDesc_.write(fileIDOfResMsg)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "write to MsgDrv failed.");
        return false;
    }
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "WRITE FileID To MsgDrv",
                     fileIDOfResMsg.c_str());
#endif //DEBUG

    return true;
}


bool 
Spool::execExpireOpr()
{
    static char METHOD_LABEL[] = "Spool::execExpireOpr: ";

    //read from MsgDrv
    string msgIDFromMsgDrv;
    if (0 > chnlDesc_.read(msgIDFromMsgDrv)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "read failed from MsgDrv");
        return false;
    }
  
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "READ msgID From MsgDrv",
                     msgIDFromMsgDrv.c_str());
#endif

    // regist to spool
    string result = del(msgIDFromMsgDrv);
  
    //result return to MsgDrv
    if (0 > chnlDesc_.write(result)) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "write failed to MsgDrv");
        return false;
    }
  
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "WRITE Result To MsgDrv",
                     result.c_str());
#endif

    return true;
}

// End of Spool.cpp
