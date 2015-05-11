/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrMgr.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
//for shm
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <iostream>
#include <string>

#include <stdexcept>
#include <errno.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

/* libxml2 */
#if defined(HAVE_LIBXML2_XMLVERSION_H) 
#include <libxml2/tree.h>
#include <libxml2/parser.h>
#else
#include <libxml/tree.h>
#include <libxml/parser.h>
#endif /* !HAVE_LIBXML2_XMLVERSION_H */

#include "MapUtil.h"
#include "StringUtil.h"
#include "ServerCommon.h"
#include "SSMLAttrMgrProtocol.h"
#include "XmlQuery.h"
#include "SSMLAttrMgr.h"
#include "SrvConf.h"
#include "AppLogger.h"
#include "Exception.h"
#include "ProcessInfo.h"

using namespace OpenSOAP;
using namespace std;

const string SSMLAttrMgr::SSML_FILE_SUFFIX = "ssml";

SSMLAttrMgr::SSMLAttrMgr()
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::SSMLAttrMgr: ";

    SrvConf* srvConf = new SrvConf();
    xmlDir_ = srvConf->getSSMLPath();
    //extend 2003/12/11
    intrnlSrvcDir = srvConf->getSSMLInternalServicesPath();
    //replyToDir = srvConf->getSSMLReplyToPath();
    
#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
					,"xmlDir_",xmlDir_.c_str());
#endif //DEBUG

    socketAddr_ = srvConf->getSocketPath() + SSMLATTRMGR_SOCKET_ADDR;

    delete srvConf;

    //init. mutex
    int status = pthread_mutex_init(&loadXmlLock_, NULL);
    if (0 != status) {
        throw Exception(-1,OPENSOAPSERVER_THREAD_ERROR
						,APLOG_ERROR,__FILE__,__LINE__);
    }

    //SEM init
    key_t semKey = ftok(SSMLATTRMGR_SOCKET_ADDR.c_str(), 1);
    semid_ = semget(semKey, 1, IPC_CREAT|0666);
    if (-1 == semid_) {
        throw Exception(-1,OPENSOAPSERVER_SEM_GET_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
    }

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
    union semun semUn;
#else
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short int *array;
        struct seminfo *__buf;
    } semUn;
#endif
    
    if (-1 == semctl(semid_, 0, GETVAL, semUn)) {
        throw Exception(-1,OPENSOAPSERVER_SEM_CTL_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
    }
    semUn.val = 0;
    if (-1 == semctl(semid_, 0, SETVAL, semUn)) {
        throw Exception(-1,OPENSOAPSERVER_SEM_CTL_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
    }

    //load SSML files
    if (0 != loadXml()) {
        throw Exception(-1,OPENSOAPSERVER_SSML_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
    }
}

SSMLAttrMgr::~SSMLAttrMgr()
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::~SSMLAttrMgr: ";

    //release xml document into xmlList_
    list<xmlDocPtr>::iterator pos;
    for (pos = xmlList_.begin(); pos != xmlList_.end(); ++pos) {
        xmlFreeDoc(*pos);
    }
    //extend 2003/12/11
    for (pos = intrnlSrvcList.begin(); pos != intrnlSrvcList.end(); ++pos) {
        xmlFreeDoc(*pos);
    }
    for (pos = replyToList.begin(); pos != replyToList.end(); ++pos) {
        xmlFreeDoc(*pos);
    }
    
    //release mutex
    int status = pthread_mutex_destroy(&loadXmlLock_);
    if (0 != status) {
        AppLogger::Write(APLOG_ERROR,"%s%s"
                         ,METHOD_LABEL 
                         ,"mutext destroy failed"
            );
    }
}

int 
SSMLAttrMgr::run()
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::run: ";

    //===================================================================
    // 通信処理部分 SSMLAttrMgr <=> SSMLAttrHandler
    //===================================================================
	int selectflg=0;
	fd_set rfds;
	struct timeval tv;
    typedef struct sockaddr SockAddr;
    typedef struct sockaddr_un SockAddrAf;
    
    int af = AF_UNIX;
    int sockfd = int();
    int newsockfd = int();
    SockAddrAf serverAddr;
    SockAddrAf clientAddr;
#if HAVE_TYPE_OF_ADDRLEN_AS_SOCKLEN_T
    socklen_t addrLen = socklen_t(); 
    socklen_t clientLen = socklen_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_SIZE_T
    size_t addrLen = size_t(); 
    size_t clientLen = size_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_INT
    int addrLen = int();
    int clientLen = int();
#endif

    if (0 > (sockfd = socket(af, SOCK_STREAM, 0))) {
        throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_OPEN_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"socket open OK!");
#endif

    memset((char*)&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sun_family = af;
    strcpy(serverAddr.sun_path, socketAddr_.c_str());
    addrLen = sizeof(serverAddr);
    
    unlink(socketAddr_.c_str());
  
    if (0 > bind(sockfd, (SockAddr*)&serverAddr, addrLen)) {
        throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_BIND_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"bind OK!");
#endif

    if (0 > listen(sockfd, 5)) {
        throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_LISTEN_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"listen OK!");
#endif
    
    for (;ProcessInfo::GetProcessStatus()!=PSTAT_WAITTERM;) {
        clientLen = sizeof(clientAddr);

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"accept ready!");
#endif
    
		// roop for connection and signal
		do
		{
			tv.tv_sec=1;
			tv.tv_usec=0;
			FD_ZERO(&rfds);
			FD_SET(sockfd,&rfds);
			selectflg=select(FD_SETSIZE,&rfds,NULL,NULL,&tv);
			switch (ProcessInfo::GetProcessStatus()){
				case PSTAT_WAITTERM:
					if (0 < sockfd) {
						CloseSocket(sockfd);
					}
					return 0;
					break;
				case PSTAT_NORMAL:
					break;
				case PSTAT_INIT:
					break;
				case PSTAT_WAIT:
					break;
				default:
					break;
			}
			
			if (selectflg==-1){
				Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_SELECT_ERROR
						,APLOG_ERROR,__FILE__,__LINE__);
				if (e.GetSysErrNo()!=EINTR){
					AppLogger::Write(LOG_DEBUG,"[%s:%d]select error[%d]=[%d:%s]"
									,__FILE__,__LINE__,selectflg
									,e.GetSysErrNo()
									,e.GetSysErrText().c_str());
					throw (e);
				}
			}
		} while(FD_ISSET(sockfd,&rfds)==0);
        newsockfd = accept(sockfd, (SockAddr*)&clientAddr, &clientLen);
        if (0 > newsockfd) {
            Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_LISTEN_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
            AppLogger::Write(e);
        }

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s %s=(%d)",METHOD_LABEL
                        ,"socket open OK!","fd",newsockfd);
#endif

        //スレッドに渡す必要のあるデータ
        ThrData* thrData = new ThrData();
        thrData->that = this;
        thrData->sockfd = newsockfd;

        //通信スレッド作成
        pthread_t thrId;
        int status = pthread_create(&thrId,
                                    NULL,
                                    connectionThread,
                                    (void*)thrData);
        if (0 != status) {
            Exception e(-1,OPENSOAPSERVER_THREAD_CREATE_ERROR
                        ,APLOG_ERROR,__FILE__,__LINE__);
            AppLogger::Write(e);
        }

        pthread_detach(thrId);
    } // end for (;;)
}

int
SSMLAttrMgr::loadXml()
{
    int ret = 0;
    ret = loadXml(xmlDir_, xmlList_);
    if (0 != ret) {
        return ret;
    }
//added 2004/01/04
    ret = loadXml(intrnlSrvcDir, intrnlSrvcList);
    if (0 != ret) {
        return ret;
    }
#if 0
    ret = loadXml(replyToDir, replyToList);
    if (0 != ret) {
        return ret;
    }
#endif //if 0
}

//modified 2004/01/04
int
SSMLAttrMgr::loadXml(const string& dirPath, list<xmlDocPtr>& xmlList)
//SSMLAttrMgr::loadXml()
{ 
    static char METHOD_LABEL[] = "SSMLAttrMgr::loadXml: ";

    // mutex lock
    Thread::lockMutex(loadXmlLock_);

    typedef struct direct Dir;

    DIR* dp = NULL;
    struct dirent* dir = NULL;
    //指定ディレクトリを開く
    //if (NULL == (dp = opendir(xmlDir_.c_str()))) {
    if (NULL == (dp = opendir(dirPath.c_str()))) {
        AppLogger::Write(APLOG_ERROR,"%s%s[%s]"
                         ,METHOD_LABEL 
                         ,"cannot open directory."
                         ,dirPath.c_str()
            );
        //unlock mutex
        Thread::unlockMutex(loadXmlLock_);
        return -1;
    }

    string ssmlFilePath;
  
    while (NULL != (dir = readdir(dp))) {
        if (0 == dir->d_ino) {
            cerr << "0 == dir->d_ino" << endl;
            continue;
        }

        //full path file name
        //ssmlFilePath = xmlDir_ + dir->d_name;
        ssmlFilePath = dirPath + dir->d_name;
        //addXmlList(ssmlFilePath);
        addXmlList(ssmlFilePath, xmlList);
    }
    closedir(dp);

    // shm buffering clear
    clearShm();

    //unlock mutex
    Thread::unlockMutex(loadXmlLock_);

    return 0;
}

bool
//SSMLAttrMgr::addXmlList(const string& ssmlFilePath)
SSMLAttrMgr::addXmlList(const string& ssmlFilePath, list<xmlDocPtr>& xmlList) 
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::addXmlList: ";

    //check file suffix
    if (SSML_FILE_SUFFIX != ssmlFilePath.substr(ssmlFilePath.rfind(".")+1)) {
        return false;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s %s=[%s]",METHOD_LABEL
                    ,"xmlParseFile","file",ssmlFilePath.c_str());
#endif /* DEBUG */    

    /* parse ssml begin */
    /* COMPAT: Do not generate nodes for formatting spaces */
    LIBXML_TEST_VERSION xmlKeepBlanksDefault(0);
    xmlDocPtr doc = NULL;
    xmlNodePtr node1 = NULL;
  
    /* build an XML tree from a file */

    doc = xmlParseFile((const char*)(ssmlFilePath.c_str()));
    if (doc == NULL) {
        AppLogger::Write(APLOG_WARN,"%s%s%s%s"
                         ,METHOD_LABEL 
                         ,"parsing file "
                         ,ssmlFilePath.c_str()
                         ," failed."
            );
        return false;
    }
    
    /* check document (root node) */
    node1 = xmlDocGetRootElement(doc);
    if (node1 == NULL) {
        AppLogger::Write(APLOG_WARN,"%s%s%s"
                         ,METHOD_LABEL 
                         ,"empty document "
                         ,ssmlFilePath.c_str()
            );
        xmlFreeDoc(doc);
        return false;
    }
    //append list
    //xmlList_.push_back(doc);
    xmlList.push_back(doc);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s %s",METHOD_LABEL
                    ,"addXmlList","add doc.");
#endif /* DEBUG */

    return true;
}

ThrFuncReturnType
SSMLAttrMgr::connectionThread(ThrFuncArgType arg)
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::connectionThread: ";

    //check argument 
    if (!arg) {
        AppLogger::Write(APLOG_ERROR,"%s%s"
                         ,METHOD_LABEL 
                         ,"invalid argument"
            );
        ReturnThread(NULL);
    }

    ThrData* thrData = (ThrData*)arg;
    SSMLAttrMgr* that = thrData->that;
    int sockfd = thrData->sockfd;
    delete thrData;

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL,"sockfd",sockfd);
#endif /* DEBUG */
	ProcessInfo::AddThreadInfo();

	try {
		that->connectProc(sockfd);
	}
	catch(Exception e){
		e.AddFuncCallStack();
		AppLogger::Write(e);
	}
	catch(...){
		AppLogger::Write(APLOG_ERROR,"unknown error");
	}

    if (0 < sockfd) {
        CloseSocket(sockfd);
    }

	ProcessInfo::DelThreadInfo();

    ReturnThread(NULL);
}

//communication proc.
void 
SSMLAttrMgr::connectProc(int sockfd)
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::connectProc: ";

    //read data
    string readBuf;
    if (0 > read(sockfd, readBuf)) {
        AppLogger::Write(APLOG_ERROR,"%s%s"
                         ,METHOD_LABEL 
                         ,"read failed"
            );
        return;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                    ,"read",readBuf.c_str());
#endif

    MapUtil readDataMap(ITEM_DELMIT, VAL_DELMIT);
    readDataMap.insertItem(readBuf);

#ifdef DEBUG
    readDataMap.spy();
#endif /* DEBUG */

    //reply data
    string response;
    string retCode;
    MapUtil writeDataMap(ITEM_DELMIT, VAL_DELMIT);

    //check command string
    if (!readDataMap.exists(CMD)) {
        AppLogger::Write(APLOG_ERROR,"%s%s"
                         ,METHOD_LABEL 
                         ,"read command failed"
            );
        writeDataMap.insert(make_pair(RESULT, FAILURE));
        retCode = CMD + " not found";
        writeDataMap.insert(make_pair(RET_CODE, retCode));
        write(sockfd, writeDataMap.toString());

        return;
    }

    //get command
    string cmd = readDataMap[CMD];

    if (RELOAD_XML == cmd) {
        //reload xml files
        if (0 != loadXml()) {
            AppLogger::Write(APLOG_ERROR,"%s%s"
                             ,METHOD_LABEL 
                             ,"reload SSML failed"
                );
            writeDataMap.insert(make_pair(RESULT, FAILURE));
            retCode = "reload failed";
            writeDataMap.insert(make_pair(RET_CODE, retCode));
            write(sockfd, writeDataMap.toString());

            return;
        }

        writeDataMap.insert(make_pair(RESULT, SUCCESS));
        write(sockfd, writeDataMap.toString());

        return;
    }
    else if (SEARCH == cmd) {

        //check query
        if (!readDataMap.exists(QUERY)) {
            AppLogger::Write(APLOG_ERROR,"%s%s"
                             ,METHOD_LABEL 
                             ,"search query not found."
                );
            writeDataMap.insert(make_pair(RESULT, FAILURE));
            retCode = QUERY + " not found";
            writeDataMap.insert(make_pair(RET_CODE, retCode));
            write(sockfd, writeDataMap.toString());

            return;
        }

        //get query
        string query = readDataMap[QUERY];

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                        ,"search query",query.c_str());
#endif /* DEBUG */

        //extend 2004/01/04
        SSMLType ssmlType = EXTERNAL_SERVICES;
        if (readDataMap.exists(SSML_TYPE)) {
            string ssmlTypeStr = readDataMap[SSML_TYPE];
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG9,"%s%s %s=[%s]",METHOD_LABEL
                            ,"**** SSML_TYPE check"
                            ,"type",ssmlTypeStr.c_str());
#endif //DEBUG

            if (SSML_TYPE_INTERNAL == ssmlTypeStr) {
                ssmlType = INTERNAL_SERVICES;
            }
        }

        //modified 2004/01/08
        response = queryXml(query, ssmlType);

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                        ,"queryXml response",response.c_str());
#endif /* DEBUG */

        write(sockfd, response);

        return;
    }
    else {
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]"
                         ,METHOD_LABEL 
                         ,"invalid command. cmd"
                         ,cmd.c_str()
            );
        writeDataMap.insert(make_pair(RESULT, FAILURE));
        retCode = CMD + " invalid";
        writeDataMap.insert(make_pair(RET_CODE, retCode));
        write(sockfd, writeDataMap.toString());
        
        return;
    }
}

string 
SSMLAttrMgr::queryXml(const string& query, const SSMLType ssmlType) 
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::queryXml: ";
    
    XmlQuery* xmlQuery = new XmlQuery(query);

    //mutex lock check.

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s %s=(%d)",METHOD_LABEL
                    ,"XmlQuery create done.","doc size",xmlList_.size());
#endif /* DEBUG */

    vector<string> values;
    list<xmlDocPtr>::const_iterator pos;
    //extend 2004/01/08
    list<xmlDocPtr>& targetList = 
        (ssmlType == EXTERNAL_SERVICES) ? xmlList_ :
        (ssmlType == INTERNAL_SERVICES) ? intrnlSrvcList :
        replyToList; //"REPLY_TO"

    for (pos = targetList.begin(); pos != targetList.end(); ++pos) {
        xmlDocPtr doc = *pos;
        xmlNodePtr node = xmlDocGetRootElement(doc);
        int ret = xmlQuery->getValue(doc, node, values);
        
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s %s=(%d) %s %d %s",METHOD_LABEL
                        ,"XmlQuery","ret",ret
                        ,"isMulti is",xmlQuery->isMulti(),".");
        //xmlQuery->spy();
#endif /* DEBUG */
        
        if (0 == ret && !xmlQuery->isMulti()) {
            //if (0 == xmlQuery->getValue(doc, node, values)) {
            break;
        }
    }
    
    MapUtil writeDataMap(ITEM_DELMIT, VAL_DELMIT);

    if (0 < values.size()) {
        
        writeDataMap.insert(make_pair(RESULT, SUCCESS));
        string answer;
        vector<string>::const_iterator vpos;
        for (vpos = values.begin(); vpos != values.end(); ++vpos) {
            if (!answer.empty()) {
                answer += SUB_ITEM_DELMIT;
            }
            answer += *vpos;
        }
        writeDataMap.insert(make_pair(ANSWER, answer));


        //shm buffering
        addShm(query, answer, ssmlType); //modified 2004/01/20
    }
    else {
        
        writeDataMap.insert(make_pair(RESULT, FAILURE));
        string retCode("querey failed");
        writeDataMap.insert(make_pair(RET_CODE, retCode));
    }

    delete xmlQuery;
    
    return writeDataMap.toString();
}

void
SSMLAttrMgr::clearShm()
{
    //clear all shm data
    clearShm(EXTERNAL_SERVICES);
    clearShm(INTERNAL_SERVICES);
    clearShm(REPLY_TO);
}

void 
SSMLAttrMgr::clearShm(const SSMLType& ssmlType)
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::clearShm: ";

    // SEM lock
    struct sembuf semBuf;
    // semVal -> 1
    semBuf.sem_num = 0;
    semBuf.sem_op = 1;
    semBuf.sem_flg = 0;
    if (-1 == semop(semid_, &semBuf, 1)) {
        AppLogger::Write(APLOG_ERROR,"%s%s"
                         ,METHOD_LABEL 
                         ,"semop up failed."
            );
        return;
    }

    //SHM clear
    char* shm = NULL;
    const int SHMSZ = 196608;

    //extend 2004/01/20
    string shmKeyPath(SSMLATTRMGR_SOCKET_ADDR);
    shmKeyPath += StringUtil::toString(ssmlType);
    key_t shm_key = ftok(shmKeyPath.c_str(), 0);
    int shmid = 0;
    
    if ((shmid = shmget(shm_key, SHMSZ, IPC_CREAT|0666)) < 0) {
        Exception e(-1,OPENSOAPSERVER_SHM_GET_ERROR
                    ,APLOG_ERROR,__FILE__,__LINE__);
        AppLogger::Write(e);
        return;
    }
    
    if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*)-1) {
        Exception e(-1,OPENSOAPSERVER_SHM_AT_ERROR
                    ,APLOG_ERROR,__FILE__,__LINE__);
        AppLogger::Write(e);
        return ;
    }

    *shm = 0x00;
  
    shmdt(shm);

    // SEM unlock
    // semVal -> 0
    semBuf.sem_num = 0;
    semBuf.sem_op = -1;
    semBuf.sem_flg = 0;
    if (-1 == semop(semid_, &semBuf, 1)) {
        Exception e(-1,OPENSOAPSERVER_SEM_OP_ERROR
                    ,APLOG_ERROR,__FILE__,__LINE__);
        AppLogger::Write(e);
        return;
    }
}

void 
SSMLAttrMgr::addShm(const string& query, const string& value, 
                    const SSMLType& ssmlType)
{
    static char METHOD_LABEL[] = "SSMLAttrMgr::addShm: ";
        
    // SEM lock
    struct sembuf semBuf;
    // semVal -> 1
    semBuf.sem_num = 0;
    semBuf.sem_op = 1;
    semBuf.sem_flg = 0;
    if (-1 == semop(semid_, &semBuf, 1)) {
        Exception e(-1,OPENSOAPSERVER_SEM_OP_ERROR
                    ,APLOG_ERROR,__FILE__,__LINE__);
        AppLogger::Write(e);
        return;
    }

    char* shm = NULL;
    const int SHMSZ = 196608;

    //extend 2004/01/20
    string shmKeyPath(SSMLATTRMGR_SOCKET_ADDR);
    shmKeyPath += StringUtil::toString(ssmlType);
    key_t shm_key = ftok(shmKeyPath.c_str(), 0);
    int shmid = 0;
    
    if ((shmid = shmget(shm_key, SHMSZ, IPC_CREAT|0666)) < 0) {
        Exception e(-1,OPENSOAPSERVER_SHM_GET_ERROR
                    ,APLOG_ERROR,__FILE__,__LINE__);
        AppLogger::Write(e);
        return;
    }

    if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*)-1) {
        Exception e(-1,OPENSOAPSERVER_SHM_AT_ERROR
                    ,APLOG_ERROR,__FILE__,__LINE__);
        AppLogger::Write(e);
        return ;
    }

//    memset(shmBuf, 0x00, SHMSZ);
//    strncpy(shmBuf, shm, SHMSZ);
//    string newBuf(shmBuf);
    string newBuf(shm);
    newBuf += "|";
    newBuf += query;
    newBuf += "|";
    newBuf += value;
    
    //newBuf.length >= SHMSZ -> delete cache
    while (newBuf.length() >= SHMSZ) {
        string::size_type begIdx = 0;
        const string delimit = "|";
        begIdx = newBuf.find_first_of(delimit);
        if (begIdx != string::npos) {
            //cerr << "pos=(" << begIdx << ")" << endl;
            string buf = newBuf.substr(begIdx+1, newBuf.length()-begIdx-1);
            newBuf = buf;
        }
    }
    strcpy(shm, newBuf.c_str());
    
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s] %s=(%d) %s",METHOD_LABEL
                    ,"addShm",shm,"len",strlen(shm),":196608");
#endif

    shmdt(shm);

    // SEM unlock
    // semVal -> 0
    semBuf.sem_num = 0;
    semBuf.sem_op = -1;
    semBuf.sem_flg = 0;
    if (-1 == semop(semid_, &semBuf, 1)) {
        Exception e(-1,OPENSOAPSERVER_SEM_OP_ERROR
                    ,APLOG_ERROR,__FILE__,__LINE__);
        AppLogger::Write(e);
        return;
    }
}


// End of SSMLAttrMgr.cpp
