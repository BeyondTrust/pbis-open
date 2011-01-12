/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: IdManager.cpp,v $
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
#include <windows.h>
#include <process.h>
#include <time.h>
#else /* ! WIN32 */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#if HAVE_UUID_H
#  include <uuid.h>
#else
#  if HAVE_UUID_UUID_H
#    include <uuid/uuid.h>
#  endif
#endif

#endif /* WIN32 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

#include "ServerCommon.h"
#include "IdManager.h"
#include "SrvConf.h"
#include "AppLogger.h"
#include "SrvErrorCode.h"
#include "Exception.h"
#include "ProcessInfo.h"

using namespace OpenSOAP;
using namespace std;

//for LOG
static const std::string CLASS_SIG = "IdManager";

IdManager::IdManager()
{
  SrvConf srvConf;
#if defined(WIN32)
  socketAddr_ = IDMGR_SOCKET_ADDR;
#else
  socketAddr_ = srvConf.getSocketPath() + IDMGR_SOCKET_ADDR;
#endif

}

IdManager::~IdManager()
{
  
}

void IdManager::run()
{
  //===================================================================
  // connection proc.
  //===================================================================
	int selectflg=0;
	fd_set rfds;
	struct timeval tv;
#if defined(WIN32)
	typedef SOCKADDR SockAddr;
	typedef SOCKADDR_IN SockAddrAf;
	SockAddrAf serverAddr;
	SockAddrAf clientAddr;

	SOCKET sockfd = INVALID_SOCKET;
	SOCKET newsockfd = INVALID_SOCKET;
	WSADATA data;
	if (WSAStartup(0x101,&data)) {
		cerr << "** E **: WSAStartup failed." << endl;
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

  //connection standby to DataRetainer
  if (0 > (sockfd = socket(af, SOCK_STREAM, 0))) {
	Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_CREATE_ERROR,ERR_ERROR,
				__FILE__,__LINE__);
	throw (e);
  }
#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG7,"%s::%s:%s"
                  ,CLASS_SIG.c_str(),"run","socket open OK!");
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

  mode_t newMask = 0x000;
  mode_t curMask = umask(newMask);
#endif

  if (0 > bind(sockfd, (SockAddr*)&serverAddr, addrLen)) {
	Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_BIND_ERROR,ERR_ERROR,
				__FILE__,__LINE__);
	throw (e);
  }


#if !defined(WIN32)
  umask(curMask);
#endif

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG7,"%s::%s:%s"
                  ,CLASS_SIG.c_str(),"run","bind OK!");
#endif

  if (0 > listen(sockfd, 5)) {
	Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_LISTEN_ERROR,ERR_ERROR,
				__FILE__,__LINE__);
	throw (e);
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG7,"%s::%s:%s"
                  ,CLASS_SIG.c_str(),"run","listen OK!");
#endif
    
  for (;ProcessInfo::GetProcessStatus()!=PSTAT_WAITTERM;) {
    clientLen = sizeof(clientAddr);

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG7,"%s::%s:%s"
                  ,CLASS_SIG.c_str(),"run","accept ready!");
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
				return;
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
		Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_ACCEPT_ERROR,ERR_ERROR,
				__FILE__,__LINE__);
		AppLogger::Write(e);
		throw (e);
    }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG7,"%s::%s:%s"
                  ,CLASS_SIG.c_str(),"run","accept OK!");
#endif

    if (0 > write(newsockfd, getNewId())) {
		Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_WRITE_ERROR,ERR_ERROR,
				__FILE__,__LINE__);
		AppLogger::Write(e);
    }

    CloseSocket(newsockfd);
  }
}


//modified 2004/01/04
std::string IdManager::getNewId()
{
#ifdef HAVE_UUID_H
  /* FreeBSD5 uuid library */
  uuid_t uuid;
  uint32_t st;
  uuid_create(&uuid,&st);
  if (st==uuid_s_ok) {
    char * str=NULL;
    uuid_to_string(&uuid,&str,&st);
    if (st==uuid_s_ok){
        char uuidStr[36+1];
        strcpy(uuidStr,str);
	free(str);
        return uuidStr;
    }
    if (str){
      free(str);
    }
  }
  //error
  return "";
#else /* HAVE_UUID_H */
#  ifdef HAVE_UUID_UUID_H
  /* Linux e2fsprogs uuid library */
  uuid_t uuid;
  uuid_generate(uuid);
  if (!uuid_is_null(uuid)) {
    char uuidStr[36+1];
    uuid_unparse(uuid, uuidStr);
    return uuidStr;
  }
  else {
    //error
    return "";
  }
#  else /* ! HAVE_UUID_H && ! HAVE_UUID_UUID_H */
  // OpenSOAP original ID creater
  // ID is the current system time as YYYYMMDDHH24MMSS
  const int BSIZE = 4096;
  char buf[BSIZE];
  time_t cur_time;
  time(&cur_time);
  struct tm* cur_tm = localtime(&cur_time);
  //size_t len = strftime(buf, BSIZE, "%Y%m%d%H%M%S", cur_tm);
  strftime(buf, BSIZE, "%Y%m%d%H%M%S", cur_tm);
  std::string mainId(buf);
  
  //compare with the previous ID
  // - reset the sequence number if the time is changed
  if (mainId != prevId_) {
    idSequence_ = 0;
  }
  //store the history
  prevId_ = mainId;
#  if defined(WIN32)
  _snprintf(buf, BSIZE, "%s%05d", mainId.c_str(), ++idSequence_);
#  else
  snprintf(buf, BSIZE, "%s%05d", mainId.c_str(), ++idSequence_);
#  endif

  //return DataRetainer::Id(buf);
  return buf;
#  endif /* ! HAVE_UUID_H && ! HAVE_UUID_UUID_H */
#endif /* HAVE_UUID_H */
}

// End of IdManager.cpp
