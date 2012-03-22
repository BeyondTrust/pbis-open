/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConfAttrMgr.cpp,v $
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

#if defined(WIN32)
#include <process.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <stdexcept>
#include <string>
#include <iostream>
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
#include "ServerCommon.h"
//#include "SrvConfAttrMgrProtocol.h"
#include "SSMLAttrMgrProtocol.h"
#include "XmlQuery.h"
#include "SrvConfAttrMgr.h"
#include "AppLogger.h"
#include "Exception.h"
#include "ProcessInfo.h"

using namespace OpenSOAP;
using namespace std;

//const std::string SrvConfAttrMgr::CONF_FILE_SUFFIX = "conf";

// for LOG
static const std::string CLASS_SIG = "SrvConfAttrMgr";

//UNIXドメインソケットのアドレスを指定する
SrvConfAttrMgr::SrvConfAttrMgr()
{
#if defined(WIN32)
  socketAddr_ = SRVCONFATTRMGR_SOCKET_ADDR;
#else
  socketAddr_ = getSocketPath() + SRVCONFATTRMGR_SOCKET_ADDR;
#endif

  //mutexの初期化
#if defined(WIN32)
  loadXmlLock_ = CreateMutex(NULL, FALSE, "SRVCONFATTRMGRMTX");
#else
  int status = pthread_mutex_init(&loadXmlLock_, NULL);
  if (0 != status) {
    //throw OpenSOAPException();
    throw std::exception();
  }
#endif

  //SEM init
#if defined(WIN32)
  semid_ = CreateSemaphore(NULL, 1, 1, "SRVCONFATTRMGRSEM");
  //shm
  if (-1 == createShm()) {
    throw std::exception();
  }
#else
  key_t semKey = ftok(SRVCONFATTRMGR_SOCKET_ADDR.c_str(), 1);
  semid_ = semget(semKey, 1, IPC_CREAT|0666);
  if (-1 == semid_) {
    throw std::exception();
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
    throw std::exception();
  }
  semUn.val = 0;
  if (-1 == semctl(semid_, 0, SETVAL, semUn)) {
    throw std::exception();
  }
#endif

//  clearShm();

}

SrvConfAttrMgr::~SrvConfAttrMgr()
{
  // mutex 破棄
#if defined(WIN32)
  CloseHandle(loadXmlLock_);
#else
  int status = pthread_mutex_destroy(&loadXmlLock_);
  if (0 != status) {
    //throw OpenSOAPException();
    //throw std::exception();
	AppLogger::Write(APLOG_WARN,"%s::~%s: %s"
					,CLASS_SIG.c_str(),CLASS_SIG.c_str()
					,"mutext destroy failed.");
  }
#endif

#if defined(WIN32)
  deleteShm();
  CloseHandle(semid_);
#endif
}

//通信開始
int SrvConfAttrMgr::run()
{
  //===================================================================
  // 通信処理部分 SrvConfAttrMgr <=> SSMLAttrHandler
  //===================================================================
    static char METHOD_LABEL[] = "SrvConfAttrMgr::run: ";
	int selectflg=0;
	fd_set rfds;
	struct timeval tv;
#if defined(WIN32)
	typedef SOCKADDR SockAddr;
	typedef SOCKADDR_IN SockAddrAf;
	//SOCKADDR_IN serverAddr;
	SockAddrAf serverAddr;
	SockAddrAf clientAddr;

	SOCKET sockfd = INVALID_SOCKET;
	SOCKET newsockfd = INVALID_SOCKET;
	WSADATA data;
	if (WSAStartup(0x101,&data)) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"WSAStartup failed.");
		throw exception();
	}
	int af = AF_INET;
	int addrLen = sizeof(SockAddrAf);
	int clientLen = int();
#else
  typedef struct sockaddr SockAddr;
  typedef struct sockaddr_un SockAddrAf;

  int af = AF_UNIX;
  int sockfd = int();
  int newsockfd = int();
  SockAddrAf serverAddr;
  SockAddrAf clientAddr;
#if HAVE_TYPE_OF_ADDRLEN_AS_SOCKLEN_T
  socklen_t addrLen = socklen_t(); //socket_t();
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

#endif

  if (0 > (sockfd = socket(af, SOCK_STREAM, 0))) {
	AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"open socket failed.");
    return -1;
  }

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"socket open OK!");
#endif

  memset((char*)&serverAddr, 0, sizeof(serverAddr));

#if defined(WIN32)
	serverAddr.sin_family = af;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(socketAddr_);
#else
  serverAddr.sun_family = af;
  strcpy(serverAddr.sun_path, socketAddr_.c_str());
  addrLen = sizeof(serverAddr);

  unlink(socketAddr_.c_str());
#endif

  if (0 > bind(sockfd, (SockAddr*)&serverAddr, addrLen)) {
	AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"bind failed");
    return -1;
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"bind OK!");
#endif

  if (0 > listen(sockfd, 5)) {
	AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"listen failed");
    return -1;
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
					,ERR_ERROR,__FILE__,__LINE__);
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
      AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"accept failed");
      return -1;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s %s=(%d)",METHOD_LABEL
                    ,"accept OK!","fd",newsockfd);
#endif

    //スレッドに渡す必要のあるデータ
    ThrData* thrData = new ThrData();
    thrData->that = this;
    thrData->sockfd = newsockfd;

    //通信スレッド作成
#if defined(WIN32)
	long status = _beginthread(connectionThread, 0, thrData);
#else
    pthread_t thrId;
    int status = pthread_create(&thrId,
				NULL,
				connectionThread,
				(void*)thrData);
    if (0 != status) {
      AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                      ,"create thread failed");
      //perror("CreateThread");
      return -1;
    }
    pthread_detach(thrId);
#endif
  } // end for (;;)
  return 0;
}


//XMLファイルの読み込み
int SrvConfAttrMgr::reloadXml()
{ 
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str()
                      ,"reloadXml()");
#endif /* DEBUG */
  int rtn=0;

  // mutex lock
  Thread::lockMutex(loadXmlLock_);

  try{
    //SrvConf::loadXml()
    loadXml();

    // shm buffering clear
    clearShm();
  }
  catch(Exception e){
     e.AddFuncCallStack();
     Thread::unlockMutex(loadXmlLock_);
     throw (e);
  }
  catch(...){
     Thread::unlockMutex(loadXmlLock_);
     throw ;
  }
  // release lock
  Thread::unlockMutex(loadXmlLock_);
  
  return 0;
}

//通信処理スレッド
#if defined(WIN32)
void
#else
void* 
#endif
SrvConfAttrMgr::connectionThread(void* arg)
{
#ifdef DEBUG
  static char METHOD_LABEL[] = "SrvConfAttrMgr::connectionThread: ";
  AppLogger::Write(APLOG_DEBUG9,METHOD_LABEL);
#endif /* DEBUG */

  //引渡し情報のチェック
  if (!arg) {
    AppLogger::Write(APLOG_ERROR,"%s::%s:%s",CLASS_SIG.c_str()
                    ,"connectionThread","invalid argument");

    ReturnThread(NULL);
  }

  //引渡し情報の展開
  ThrData* thrData = (ThrData*)arg;
  SrvConfAttrMgr* that = thrData->that;
  int sockfd = thrData->sockfd;
  delete thrData;
  ProcessInfo::AddThreadInfo();
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                  ,"sockfd",sockfd);
#endif /* DEBUG */
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

//通信処理
void SrvConfAttrMgr::connectProc(int sockfd)
{
  static char METHOD_LABEL[] = "SrvConfAttrMgr::connectProc: ";
  bool except=false;
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                  ,"arg",sockfd);
#endif /* DEBUG */

  //データ受信
  std::string readBuf;
  if (0 > read(sockfd, readBuf)) {
    AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL
                  ,"read",readBuf.c_str());
    return;
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                  ,"read",readBuf.c_str());
#endif

  //受信データの解析

  MapUtil readDataMap(ITEM_DELMIT, VAL_DELMIT);
  readDataMap.insertItem(readBuf);

#ifdef DEBUG
  readDataMap.spy();
#endif /* DEBUG */

  //返信データ
  std::string response;
  std::string retCode;
  MapUtil writeDataMap(ITEM_DELMIT, VAL_DELMIT);

  
  //コマンドチェック
  if (!readDataMap.exists(CMD)) {
    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                    ,"read failed");
    writeDataMap.insert(make_pair(RESULT, FAILURE));
    retCode = CMD + " not found";
    writeDataMap.insert(make_pair(RET_CODE, retCode));
    write(sockfd, writeDataMap.toString());

    return;
  }
  //コマンド内容取り出し
  std::string cmd = readDataMap[CMD];

  if (RELOAD_XML == cmd) {
    //ファイルのリロード処理
    try{
      reloadXml();
    }
    catch(Exception e){
      AppLogger::Write(e);
      except=true;
    }
    catch(...){
      except=true;
    }
    if (except) {
      writeDataMap.insert(make_pair(RESULT, FAILURE));
      retCode = "reload failed";
      writeDataMap.insert(make_pair(RET_CODE, retCode));
      write(sockfd, writeDataMap.toString());
      Exception except(-1,OPENSOAPSERVER_RUNTIME_ERROR
                      ,APLOG_ERROR,__FILE__,__LINE__);
      except.SetErrText("reload conf file failed.");
      throw (except);
    }
    writeDataMap.insert(make_pair(RESULT, SUCCESS));
    write(sockfd, writeDataMap.toString());

    return;
  }
  else if (SEARCH == cmd) {
    //サーチデータチェック
    if (!readDataMap.exists(QUERY)) {
      AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                      ,"search query not found.");

      writeDataMap.insert(make_pair(RESULT, FAILURE));
      retCode = QUERY + " not found";
      writeDataMap.insert(make_pair(RET_CODE, retCode));
      write(sockfd, writeDataMap.toString());

      return;
    }
    //サーチ内容取り出し
    std::string query = readDataMap[QUERY];

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                      ,"search query",query.c_str());
#endif /* DEBUG */

    response = queryXml(query);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                      ,"queryXml response",response.c_str());
#endif /* DEBUG */
    
    write(sockfd, response);

    return;
    
  }
  else {
    AppLogger::Write(APLOG_ERROR,"%s%s %s=[%s]",METHOD_LABEL
                    ,"invalid command.","cmd",cmd.c_str());

    writeDataMap.insert(make_pair(RESULT, FAILURE));
    retCode = CMD + " invalid";
    writeDataMap.insert(make_pair(RET_CODE, retCode));
    write(sockfd, writeDataMap.toString());

    return;
  }
}

std::string SrvConfAttrMgr::queryXml(const std::string& queryStr) 
{
#ifdef DEBUG
  static char METHOD_LABEL[] = "SrvConfAttrMgr::queryXml: ";
  AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                  ,"arg",queryStr.c_str());
#endif /* DEBUG */

#if 0
  //  XmlQuery* xmlQuery = new XmlQuery(query);
  
  //mutex lock check.

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s %s=(%d)",METHOD_LABEL
                  ,"create done.","doc size",xmlList_.size());
#endif /* DEBUG */

  std::vector<std::string> values;
  std::list<xmlDocPtr>::const_iterator pos;
  for (pos = xmlList_.begin(); pos != xmlList_.end(); ++pos) {
    xmlDocPtr doc = *pos;
    xmlNodePtr node = xmlDocGetRootElement(doc);
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"### call getValue ###");
#endif /* DEBUG */

    int ret = xmlQuery->getValue(doc, node, values);

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=(%d)",METHOD_LABEL
                    ,"ret",ret);
#endif /* DEBUG */

    if (0 == ret && !xmlQuery->isMulti()) {
      //if (0 == xmlQuery->getValue(doc, node, values)) {
      break;
    }
  }
#endif
  std::vector<std::string> values;
  query(queryStr, values);
      

  MapUtil writeDataMap(ITEM_DELMIT, VAL_DELMIT);

  if (0 < values.size()) {
    writeDataMap.insert(make_pair(RESULT, SUCCESS));
    std::string answer;
    std::vector<std::string>::const_iterator vpos;
    for (vpos = values.begin(); vpos != values.end(); ++vpos) {
      if (!answer.empty()) {
	answer += SUB_ITEM_DELMIT;
      }
      answer += *vpos;
    }
    writeDataMap.insert(make_pair(ANSWER, answer));

    //shm buffering
    addShm(queryStr, answer);
  }
  else {
    writeDataMap.insert(make_pair(RESULT, FAILURE));
    std::string retCode("querey failed");
    writeDataMap.insert(make_pair(RET_CODE, retCode));
  }

  //delete xmlQuery;

  return writeDataMap.toString();
}

void SrvConfAttrMgr::clearShm()
{
//  static char METHOD_LABEL[] = "SrvConfAttrMgr::clearShm: ";
  // SEM lock
#if defined(WIN32)
	WaitForSingleObject(semid_, INFINITE);
#else
  struct sembuf semBuf;
  // semVal -> 1
  semBuf.sem_num = 0;
  semBuf.sem_op = 1;
  semBuf.sem_flg = 0;
  if (-1 == semop(semid_, &semBuf, 1)) {
	throw Exception(-1,OPENSOAPSERVER_SEM_OP_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"semop up failed");
//    return;
  }
#endif

  char* shm = NULL;

#if defined(WIN32)
	shm = (char*)MapViewOfFile(shmMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
  key_t shm_key = ftok(SRVCONFATTRMGR_SOCKET_ADDR.c_str(), 0);
  const int SHMSZ = 196608;
  int shmid = 0;

  if ((shmid = shmget(shm_key, SHMSZ, IPC_CREAT|0666)) < 0) {
	throw Exception(-1,OPENSOAPSERVER_SHM_GET_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"shmget failed.");
//    return;
  }

  if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*)-1) {
	throw Exception(-1,OPENSOAPSERVER_SHM_AT_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"shmat failed.");
//    return ;
  }
#endif

  *shm = 0x00;

#if defined(WIN32)
	UnmapViewOfFile(shm);
#else
  shmdt(shm);
#endif

  // SEM unlock
#if defined(WIN32)
	ReleaseSemaphore(semid_, 1, NULL);
#else
  // semVal -> 0
  semBuf.sem_num = 0;
  semBuf.sem_op = -1;
  semBuf.sem_flg = 0;
  if (-1 == semop(semid_, &semBuf, 1)) {
	throw Exception(-1,OPENSOAPSERVER_SEM_OP_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    cerr << TAG_ERR << CLASS_SIG << "::clearShm: semop down failed" << endl;
//    return;
  }
#endif
}

void SrvConfAttrMgr::addShm(const std::string& query, const std::string& value)
{
  static char METHOD_LABEL[] = "SrvConfAttrMgr::addShm: ";
  // SEM lock
#if defined(WIN32)
	WaitForSingleObject(semid_, INFINITE);
#else
  struct sembuf semBuf;
  // semVal -> 1
  semBuf.sem_num = 0;
  semBuf.sem_op = 1;
  semBuf.sem_flg = 0;
  if (-1 == semop(semid_, &semBuf, 1)) {
	throw Exception(-1,OPENSOAPSERVER_SEM_OP_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    cerr << TAG_ERR << CLASS_SIG << "::addShm: semop up failed" << endl;
//    return;
  }
#endif

  char* shm = NULL;
  const int SHMSZ = 196608;

#if defined(WIN32)
	shm = (char*)MapViewOfFile(shmMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
  key_t shm_key = ftok(SRVCONFATTRMGR_SOCKET_ADDR.c_str(), 0);
  int shmid = 0;

  if ((shmid = shmget(shm_key, SHMSZ, IPC_CREAT|0666)) < 0) {
	throw Exception(-1,OPENSOAPSERVER_SHM_GET_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    cerr << TAG_ERR << CLASS_SIG << "::addShm: shmget failed." << endl;
//    return;
  }

  if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*)-1) {
	throw Exception(-1,OPENSOAPSERVER_SHM_AT_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    cerr << TAG_ERR << CLASS_SIG << "::addShm: shmat failed." << endl;
//    return ;
  }
#endif

  std::string newBuf(shm);
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
    //cerr << "+++ SrvConfAttr::EditChache!!" << endl;
  }
  strcpy(shm, newBuf.c_str());

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s] %s=(%d) %s",METHOD_LABEL
                  ,"shm",shm,"len",strlen(shm),":196608");
#endif

#if defined(WIN32)
	UnmapViewOfFile(shm);
#else
  shmdt(shm);
#endif

  // SEM unlock
#if defined(WIN32)
	ReleaseSemaphore(semid_, 1, NULL);
#else
  // semVal -> 0
  semBuf.sem_num = 0;
  semBuf.sem_op = -1;
  semBuf.sem_flg = 0;
  if (-1 == semop(semid_, &semBuf, 1)) {
	throw Exception(-1,OPENSOAPSERVER_SEM_OP_ERROR,APLOG_ERROR,__FILE__,__LINE__);
//    cerr << TAG_ERR << CLASS_SIG << "::addShm: semop down failed" << endl;
//    return;
  }
#endif
}

#if defined(WIN32)
int
SrvConfAttrMgr::createShm()
{
	//lock
	WaitForSingleObject(semid_, INFINITE);
	shmMap = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE,
								0, 196608, "SRVCONFATTRMGRSHM");
	if (!shmMap) {
		AppLogger::Write(APLOG_ERROR,"%s::%s:%s",CLASS_SIG.c_str()
						,"createShm","CreateFileMapping failed. ");
//	    cerr << TAG_ERR << CLASS_SIG << "::" << "createShm: "
//		 << "CreateFileMapping failed. " << endl;
		ReleaseSemaphore(semid_, 1, NULL);
		return -1;
	}
	//unlock
	ReleaseSemaphore(semid_, 1, NULL);

	return 0;
}

void
SrvConfAttrMgr::deleteShm()
{
	//lock
	WaitForSingleObject(semid_, INFINITE);

	CloseHandle(shmMap);

	//unlock
	ReleaseSemaphore(semid_, 1, NULL);

}
#endif

