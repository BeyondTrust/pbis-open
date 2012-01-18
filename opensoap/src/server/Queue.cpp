/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Queue.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <iostream>
#include "MsgAttrHandler.h"
#include "ServerCommon.h"
#include "ChannelDescriptor.h"
#include "MsgDrvChnlSelector.h"
#include "Queue.h"
#include "StringUtil.h"
//e#include "SoapMessageSerializer.h"
#include "DataRetainer.h"
#include "FileIDFunc.h"
#include "SrvConf.h"
#include "AppLogger.h"
#include "TraceLog.h"

#include <OpenSOAP/ErrorCode.h>

//#define DEBUG

using namespace OpenSOAP;
using namespace std;

#if defined(WIN32)
int Queue::mtCount_ = 0;
#endif


//trace
extern TraceLog		*tlog;

//const member 
//const int OpenSOAP::Queue::MAX_POP_THREAD_NUMBER = 4;

// コンストラクタ
// 引数：テーブルバックアップファイルのパスつき名前
//
Queue::Queue(std::string filePass, bool isFwdQueue)
    : srvConf(0)
{
    static char METHOD_LABEL[] = "Queue::Queue:";

    TableRecord tableRecord;
    isFwdQueue_ = isFwdQueue;
    
    queueTable.clear();
    
    srvConf = new SrvConf();
    tableFile = srvConf->getAsyncTablePath() + filePass;

    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s tableFile=[%s]",
                     METHOD_LABEL,
                     tableFile.c_str());
    
    //load table file
    tin.open(tableFile.c_str(), ios::in);
    if (tin.fail()) {
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

        while (!tin.eof() && !tin.fail()) {
            tin >> tableRecord.fileID;
            tin >> tableRecord.proccessFlag;
            tin >> tableRecord.retryCount;
            //tableRecord.proccessFlag = INITIAL_STATE;
            if (!tin.eof()) {
#ifdef DEBUG
                AppLogger::Write(APLOG_DEBUG9,"%s %s %s %d %d",METHOD_LABEL
                                 ,"init: ",tableRecord.fileID.c_str()
                                 ,tableRecord.proccessFlag
                                 ,tableRecord.retryCount);
#endif
                queueTable.push_back(tableRecord);    
            }
        }
        tin.close();
    }
    
    // Mutex，条件変数，スレッドの初期化
#if defined(WIN32)
    if (mtCount_ > 10000) {
        mtCount_ = 0;
    }
    std::string mtId = (isFwdQueue ? "FWDQUEUEMTX" : "QUEUEMTX") 
        + StringUtil::toString(mtCount_++);
    queue_mutex = CreateMutex(NULL, FALSE, mtId.c_str());
    //queue_mutex = CreateMutex(NULL, FALSE, "QUEUEMTX");
    string semId = (isFwdQueue ? "FWDQUEUESEM" : "QUEUESEM") 
        + StringUtil::toString(mtCount_);
    queue_cond = CreateSemaphore(NULL, 0, 99999, semId.c_str());
    //queue_semaphore = CreateSemaphore(NULL, 0, 99999, "QUEUESEM");
#else
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
#endif

    for (int thrd = 0; thrd < MAX_POP_THREAD_NUMBER ; thrd++) {
#if defined(WIN32)
	pop_thread[thrd] = (HANDLE)_beginthread(pop, 0, this);
#else
        pthread_create(&pop_thread[thrd], NULL, pop, this);
#endif
    }

#if 0 //next step
    //init member
    SoapMessageSeializer::setSoapMessageSpoolPath(
        srvConf->getSoapMessageSpoolPath()
        );
#endif
}

// テーブルファイルの書き出し関数
//
int 
Queue::fileOut()
{
    static char METHOD_LABEL[] = "Queue::fileOut:";

    tout.open(tableFile.c_str(), ios::out);
    if (tout.fail()) {
        AppLogger::Write(APLOG_ERROR, "%s %s [%s]",
                         METHOD_LABEL,
                         "Table_file output open error",
                         tableFile.c_str());
        return 1;
        //exit(1);
    }

    for (unsigned int i=0; i<queueTable.size(); i++) {
        tout 
            << queueTable[i].fileID 
            << "\t" 
            << queueTable[i].proccessFlag
            << "\t" 
            << queueTable[i].retryCount
            << endl ;
        tout.flush();
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s %s=[%s] %s=(%d) %s=(%d)",METHOD_LABEL
                         ,"ID",queueTable[i].fileID.c_str()
                         ,"tFlag",queueTable[i].proccessFlag
                         ,"Retry",queueTable[i].retryCount);
#endif
    }

    tout.close();
    return 0;
}

//push to internal queue object and store backup 
int 
Queue::push(const string& ID)
{
    static char METHOD_LABEL[] = "Queue::push:";
    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - create Table Cell Info with ID(=fileId of request soap message)
    // - exclusive lock
    // - append cell info to queue object 
    // - store queue object status to Table File(=req_queue_table)
    // - semaphore count up(=signal to waiting pop queue thread)
    // - exclusive unlock
    // - return
    //-----------------------------------------------------

    //TraceLog instance
    TraceLog tlog_local(*tlog);
#ifdef DEBUG
    tlog_local.SetModule(__FILE__);
    tlog_local.SetFunction(METHOD_LABEL);
#endif //DEBUG

    //for TraceLog
    MsgInfo* msgInfo = ProcessInfo::GetThreadInfo()->GetMsgInfo();
    msgInfo->SetRequestID(ID);

#ifdef DEBUG
    //trace
    tlog_local.SetComment("IN >>");
    tlog_local.TraceUpdate();
#endif //DEBUG

    // queue stored cell info 
    TableRecord tableRecord;
    tableRecord.fileID = ID; //=fileId of request soap message
    tableRecord.proccessFlag = int(INITIAL_STATE);
    tableRecord.retryCount = 0;
    
    try {
        // exclusive lock
        Thread::lockMutex(queue_mutex);
    }
    catch (exception e) {
        return OPENSOAP_FILE_ERROR;
    }
    
    //append to queue object
    queueTable.push_back(tableRecord);
        
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s %s %s=(%d)",METHOD_LABEL
                     ,"push","vector size",queueTable.size());
#endif
        
    //store internal queue object status for backup
    //save to req_queue_table(next stage: save to DBMS)
    int sf = fileOut();
    if (sf != 0) {
        AppLogger::Write(APLOG_ERROR,"%s %s",METHOD_LABEL
                         ,"Table file write error");
        // exclusive unlock
        try {
            // exclusive lock
            Thread::unlockMutex(queue_mutex);
        }
        catch (exception e) {
            return OPENSOAP_FILE_ERROR;
        }

        return OPENSOAP_IO_WRITE_ERROR;
    }
        
    //semaphore count up (signal to waiting pop queue thread)
#if defined(WIN32)
    // semaphore up(for windows)
    ReleaseSemaphore(queue_cond, 1, NULL);
#else
    // for pthread
    int sc = pthread_cond_signal(&(queue_cond));
    if (sc != 0) {
        AppLogger::Write(APLOG_ERROR, "%s %s ret=(%d)",
                         METHOD_LABEL,
                         "pthread_cond_signal failed.",
                         sc);
    }
#ifdef DEBUG
    else {
        AppLogger::Write(APLOG_DEBUG9, "%s %s",
                         METHOD_LABEL,
                         "push thread condition value sent");
    }
#endif //DEBUG

#endif //WIN32


    //sleep(10);
        
    // exclusive unlock
    try {
        // exclusive lock
        Thread::unlockMutex(queue_mutex);
    }
    catch (exception e) {
        return OPENSOAP_FILE_ERROR;
    }

#ifdef DEBUG
    //trace
    tlog_local.SetComment("OUT <<");
    tlog_local.TraceUpdate();
#endif //DEBUG

    return OPENSOAP_NO_ERROR;
}


//
//queue data pop thread
//
ThrFuncReturnType
Queue::pop(ThrFuncArgType arg)
{
    static char METHOD_LABEL[] = "Queue::pop: ";
    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - infinity loop
    //-----------------------------------------------------
    string ID;
    int ret = OPENSOAP_NO_ERROR;
    
#if defined(WIN32)
    DWORD tid = GetCurrentThreadId();
#else
    pthread_t tid = pthread_self();
#endif
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s(%ld) %s",METHOD_LABEL
                     ,"thread ID",tid,"created");
#endif
    
    //need access from static method
    Queue* that = (Queue*)arg;
    if (NULL == that) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "invalid argument");
        ReturnThread(NULL);
    }
    
    //main loop for pop thread
    while (ProcessInfo::GetProcessStatus() != PSTAT_WAITTERM) {

#if defined(WIN32)
        //check semaphore first in windows system
        //exclusive lock after get semaphore
	// semaphore down (for windows)
	WaitForSingleObject(that->queue_cond, INFINITE);
#endif
        
        //exclusive lock for queue object access
        Thread::lockMutex(that->queue_mutex);
        
#if !defined(WIN32)
        //when queue data not exists, release mutex and wait cond signal
        //vector<tableRecord>::iterator p = that->queueTable.end();
        int tableSize = that->queueTable.size();
        if (tableSize <= 0 || 
            that->queueTable[tableSize-1].proccessFlag != INITIAL_STATE) {

#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG9,"(%ld) %s"
                             ,tid,"condition value waiting...");
#endif //DEBUG

	    struct timeval	now;
	    struct timespec	timeout;

	    gettimeofday(&now, NULL);
	    timeout.tv_sec = now.tv_sec;
	    timeout.tv_nsec = now.tv_usec * 1000;
	    do {
		timeout.tv_sec += 1;
		ret = pthread_cond_timedwait(&(that->queue_cond), &(that->queue_mutex), &timeout);
	    } while((ret == ETIMEDOUT)&&(ProcessInfo::GetProcessStatus()!=PSTAT_WAITTERM));
	    if (ProcessInfo::GetProcessStatus()==PSTAT_WAITTERM){
		continue;
	    }

#ifdef DEBUG
            if (ret == 0) {
                AppLogger::Write(APLOG_DEBUG9,"(%ld) %s",tid
                                 ,"condition value received from push method");
            }
#endif //DEBUG

        }
#endif
        
        //search first record with INITIAL_STATE
        vector<TableRecord>::iterator itrRead = that->queueTable.begin();
        //skip not INITIAL_STATE
        while (itrRead->proccessFlag != INITIAL_STATE
               && itrRead != that->queueTable.end()) {
            itrRead++;
        }
    
        //target is INITIAL_STATE status
        ID = itrRead->fileID;
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s=[%s] %s=(%d)"
                         ,"Poped ID(fileID)",ID.c_str()
                         ,"Flag",itrRead->proccessFlag);
#endif
        //change status to PROGRESS
        itrRead->proccessFlag = PROGRESS_STATE;
        //current queue object status store to file
        that->fileOut();

        //exclusive unlock
        Thread::unlockMutex(that->queue_mutex);
        
        //next request message processing stage...

        //sleep(1);

        ret = that->popOutToNextDrv(ID);

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"### %s=[%s] ###"
                         ,"popOutToNextDrv",ID.c_str());
#endif //DEBUG

        if (OPENSOAP_FAILED(ret)) {
            AppLogger::Write(APLOG_ERROR,"%s%s %s=(%x)",METHOD_LABEL
                             ,"popOutToNextDrv failed.","rc",ret);
            itrRead->proccessFlag = DONE_FAILURE_STATE;
            //exclusive lock
            Thread::lockMutex(that->queue_mutex);
            that->fileOut();
            //exclusive unlock
            Thread::unlockMutex(that->queue_mutex);
            
            //skip proc. 
            continue;
        }
#ifdef DEBUG
        else {
            AppLogger::Write(APLOG_DEBUG9,"%s%s=(%ld) %s",METHOD_LABEL
                             ,"thr_id",tid,": Pop Output Successed");
        }
#endif

#if 1 //delete 2003/08/13
        //erase when response created.

        //erase target record (with lock)
        ret = that->queueEraseAndFileOut(ID);
        if (OPENSOAP_FAILED(ret)) {
            AppLogger::Write(APLOG_ERROR,"%s%s %s=(%x)",METHOD_LABEL
                             ,"queueEraseAndFileOut failed.","rc",ret);
            itrRead->proccessFlag = DONE_FAILURE_STATE;
            //exclusive lock
            Thread::lockMutex(that->queue_mutex);
            that->fileOut();
            //exclusive unlock
            Thread::unlockMutex(that->queue_mutex);
              
            //skip proc. 
            continue;
        }
#ifdef DEBUG
        else {
            AppLogger::Write(APLOG_DEBUG9,"%s%s=(%ld) %s",METHOD_LABEL
                             ,"thr_id",tid,": Record Erase Successed");
        }
#endif
        
#endif //if 0 2003/08/13
        
    } //end of while(1)
    
    ReturnThread(NULL);
}

int
Queue::del(const string& fileID)
{
    static char METHOD_LABEL[] = "Queue::del: ";

    int ret = OPENSOAP_NO_ERROR;
    //erase target record (with lock)
    ret = queueEraseAndFileOut(fileID);
    if (OPENSOAP_FAILED(ret)) {
        AppLogger::Write(APLOG_ERROR,"%s%s %s=(%x)",METHOD_LABEL
                         ,"queueEraseAndFileOut failed.","rc",ret);
    }
    return ret;
}

//
//processing poped request message
//
int
Queue::popOutToNextDrv(const string& fileID) 
{
    static char METHOD_LABEL[] = "Queue::popOutToNextDrv";

#if defined(WIN32)
    DWORD tid = GetCurrentThreadId();
#else
    pthread_t tid = pthread_self();
#endif

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s",
                     METHOD_LABEL,
                     "called!!");
    //
    // connect to MsgDrv
    //
    ChannelDescriptor nextDrvChnlDesc;
    ChannelSelector* nextDrvChnlSelector = new MsgDrvChnlSelector();
    //connection open
    if (0 != nextDrvChnlSelector->open(nextDrvChnlDesc)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                         ,"MsgDrv connection open failed");
        return OPENSOAP_IO_ERROR;
    }
    if (0 > nextDrvChnlDesc.write(fileID)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                         ,"write failed to MsgDrv");
        return OPENSOAP_IO_WRITE_ERROR;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"WRITE To MsgDrv",fileID.c_str());
#endif

    string resultOfSpooling;
    if (0 > nextDrvChnlDesc.read(resultOfSpooling)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                         ,"read failed from MsgDrv");
        return OPENSOAP_IO_READ_ERROR;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                     ,"READ Result of spooling From MsgDrv"
                     ,resultOfSpooling.c_str());
#endif

    nextDrvChnlSelector->close(nextDrvChnlDesc);
    delete nextDrvChnlSelector;
    
/*
  - convert fileID to message
  - parse <Body><Result> : SUCCESS or FAILURE
  - delete from storage
    if (resultOfSpooling == Result::FAILURE) {
        return OPENSOAP_IO_WRITE_ERROR;
    }
*/
    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "recv msg",
                     resultOfSpooling.c_str());

#if 0 //next step
    SoapMessage response;
    response.setMessageId(resultOfSpooling);
    SoapMessageSerializer sms;
    sms.deserialize(response);
#else
    //
    DataRetainer dr(srvConf->getSoapMessageSpoolPath());
    dr.SetId(resultOfSpooling);
    string responseStr;
    dr.GetSoapEnvelope(responseStr);
#endif

    //debug
    AppLogger::Write(APLOG_DEBUG9, "+++ %s %s=[%s] +++",
                     METHOD_LABEL,
                     "deserialized response",
                     responseStr.c_str());
    
    MsgAttrHandler msgAttrHndl(responseStr);
    string query("/Envelope/Body/Result=?");
    vector<string> values;
    int ret = 0;
    ret = msgAttrHndl.queryXml(query, values);
    if (0 != ret || values.empty()) {
        AppLogger::Write(APLOG_ERROR, "%s %s query=[%s]",
                         METHOD_LABEL,
                         "reponse query failed",
                         query.c_str());
        return OPENSOAP_IO_WRITE_ERROR;
    }
    string result(values[0]);

    //debug
    AppLogger::Write(APLOG_DEBUG9, "%s %s=[%s]",
                     METHOD_LABEL,
                     "result in response",
                     result.c_str());
    
    if (result == Result::FAILURE) {
        AppLogger::Write(APLOG_ERROR, "%s %s",
                         METHOD_LABEL,
                         "response spooling failed.");
        
        return OPENSOAP_IO_WRITE_ERROR;
    }
    // clean up queued request soap message
    if (0 != deleteFileID(fileID,
                          srvConf->getSoapMessageSpoolPath())) {
        AppLogger::Write(APLOG_DEBUG9,"%s%s %s=[%s]",METHOD_LABEL
                         ,"queuing message delete failed."
                         ,"after MsgDrv::invoke id"
                         ,fileID.c_str());
        return OPENSOAP_IO_WRITE_ERROR;
    }
    AppLogger::Write(APLOG_DEBUG9, "%s %s id=[%s]",
                     METHOD_LABEL,
                     "deleteFileID",
                     fileID.c_str());

    // clean up response soap message
    if (0 != deleteFileID(resultOfSpooling,
                          srvConf->getSoapMessageSpoolPath())) {
        AppLogger::Write(APLOG_DEBUG9,"%s%s %s=[%s]",METHOD_LABEL
                         ,"response message delete failed."
                         ,"after MsgDrv::invoke id"
                         ,resultOfSpooling.c_str());
        return OPENSOAP_IO_WRITE_ERROR;
    }

    AppLogger::Write(APLOG_DEBUG9, "%s %s reqId=[%s] resId=[%s]",
                     METHOD_LABEL,
                     "deleteFileID",
                     fileID.c_str(),
                     resultOfSpooling.c_str());

    return OPENSOAP_NO_ERROR;
}


//
//erase queue record after processing done.
//and queue object status store to file
//
int 
Queue::queueEraseAndFileOut(const string& fileID) 
{
    static char METHOD_LABEL[] = "Queue::queueEraseAndFileOut: ";
    
    int status = 0;

#if defined(WIN32)
    DWORD tid = GetCurrentThreadId();
#else
    pthread_t tid = pthread_self();
#endif

    //exclusive lock for queue object access
    try {
        Thread::lockMutex(queue_mutex);
    }
    catch (exception e) {
        return OPENSOAP_FILE_ERROR;
    }

    //erase queue record match with fileID
    vector<TableRecord>::iterator itrErase = queueTable.begin();
    while (itrErase->fileID != fileID && itrErase != queueTable.end()) {
        itrErase++;
    }
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s[%s]%s"
                     ,"File ID:",itrErase->fileID.c_str(),"erased");
#endif

    //erase record
    queueTable.erase(itrErase);
  
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s=(%d)"
                     ,"vector size",queueTable.size());
#endif

    //store queue status to file
    status = fileOut();
    if (status != 0) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                         ,"Table file write error");
        // exclusive unlock
        try {
            Thread::unlockMutex(queue_mutex);
        }
        catch (exception e) {
            return OPENSOAP_FILE_ERROR;
        }
    }

    //exclusive unlock
    try {
        Thread::unlockMutex(queue_mutex);
    }
    catch (exception e) {
        return OPENSOAP_FILE_ERROR;
    }

    return OPENSOAP_NO_ERROR;
}


// デストラクタ
//
Queue::~Queue()
{
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"Queue::~Queue Destructor");
#endif

    delete srvConf;

    for (int thrd = 0; thrd < MAX_POP_THREAD_NUMBER ; thrd++) {
#if defined(WIN32)
	//CloseHandle(pop_thread[thrd]);
#else
        pthread_join(pop_thread[thrd], NULL);
#endif
    }

#if defined(WIN32)
    CloseHandle(queue_mutex);
    CloseHandle(queue_cond);
#else
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
#endif
}

void 
Queue::setChnlDesc(const ChannelDescriptor& chnlDesc)
{
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"Queue::setChnlDesc");
#endif

  chnlDesc_ = chnlDesc;
}

bool 
Queue::execExpireOpr()
{
    static char METHOD_LABEL[] = "Queue::execExpireOpr: ";
    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - recv fileId of request soap message from MsgDrv
    // - delete from queue
    // - send result ("success"or"failure") to MsgDrv
    //-----------------------------------------------------
    int ret = OPENSOAP_NO_ERROR;

    //read fileId of request soap message from MsgDrv
    string fileIDOfMsgFromMsgDrv;
    if (0 > chnlDesc_.read(fileIDOfMsgFromMsgDrv)) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
						,"read failed. fileId of request from MsgDrv");
        return false;
    }
  
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"READ FileID From MsgDrv",fileIDOfMsgFromMsgDrv.c_str());
#endif

    //delete fileId from queue by TableRecord cell
    ret = del(fileIDOfMsgFromMsgDrv);
    string result(OpenSOAP::Result::SUCCESS);
    if (OPENSOAP_FAILED(ret)) {
        result = OpenSOAP::Result::FAILURE;
    }
    
    //send result to MsgDrv
    if (0 > chnlDesc_.write(result)) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
						,"write failed. push queue result to MsgDrv");
        return false;
    }

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"WRITE Result To MsgDrv",result.c_str());
#endif

    return true;
}

bool 
Queue::execPushOpr()
{
    static char METHOD_LABEL[] = "Queue::execPushOpr: ";

    //-----------------------------------------------------
    // processing outline
    //-----------------------------------------------------
    // - recv fileId of request soap message from MsgDrv
    // - push into queue 
    // - send result ("success"or"failure") to MsgDrv
    //-----------------------------------------------------
    int ret = 0;

    //read fileId of request soap message from MsgDrv
    string fileIDOfMsgFromMsgDrv;
    if (0 > chnlDesc_.read(fileIDOfMsgFromMsgDrv)) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
						,"read failed. fileId of request from MsgDrv");
        return false;
    }
  
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"READ FileID From MsgDrv",fileIDOfMsgFromMsgDrv.c_str());
#endif

    //push fileId into queue by TableRecord cell
    ret = push(fileIDOfMsgFromMsgDrv);
    string result(OpenSOAP::Result::SUCCESS);
    if (OPENSOAP_FAILED(ret)) {
        result = OpenSOAP::Result::FAILURE;
    }
    
    //send result to MsgDrv
    if (0 > chnlDesc_.write(result)) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
						,"write failed. push queue result to MsgDrv");
        return false;
    }

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"WRITE Result To MsgDrv",result.c_str());
#endif

    return true;
}

