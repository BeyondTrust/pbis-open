/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ChannelManager.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <sys/types.h>

#if HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(WIN32)
#include <windows.h>
#include <process.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include <sys/stat.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <errno.h>

#include "ServerCommon.h"
#include "ChannelManager.h"
#include "SrvConf.h"
#include "ProcessInfo.h"
#include "Exception.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

/*
  ChannelManager::
*/

//for LOG
static const std::string CLASS_SIG = "ChannelManager";

ChannelManager::ChannelManager()
{
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
  setMaskFlag = false;
}

ChannelManager::~ChannelManager()
{
  AppLogger::Write(APLOG_DEBUG9,"%s::~%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
}

void
#if defined(WIN32)
ChannelManager::setMask(int newMask)
#else
ChannelManager::setMask(mode_t newMask)
#endif
{
  newMask_ = newMask;
  setMaskFlag = true;
}

bool 
ChannelManager::setSocketAddr(
#if defined(WIN32)
			      const int addr)
#else
			      const std::string& addr)
#endif
{
  AppLogger::Write(APLOG_DEBUG9,"%s::%s arg=[%s]"
  					,CLASS_SIG.c_str(),__func__,addr.c_str());

#if defined(WIN32)
  socketAddr_ = addr;
#else
  SrvConf srvConf;
  socketAddr_ = srvConf.getSocketPath() + addr;
#endif

  return true;
}

void 
ChannelManager::run()
{
    static char METHOD_LABEL[] = "ChannelManager::run: ";

  //===================================================================
  // communication proc. : ChannelManager <-> ChannelSelector
  //===================================================================
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
		Exception e(-1,OPENSOAPSERVER_NETWORK_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
		e.SetErrText("WSAStartup failed.")
        throw e;
    }
    int af = AF_INET;
    int addrLen = sizeof(SockAddrAf);
    int clientLen = int();
#else

    typedef struct sockaddr SockAddr;
    typedef struct sockaddr_un SockAddrAf;
    
    int sockfd = int();
    int newsockfd = int();
    SockAddrAf serverAddr;
    SockAddrAf clientAddr;
    int af = AF_UNIX;
#if HAVE_TYPE_OF_ADDRLEN_AS_SOCKLEN_T
    socklen_t addrLen = socklen_t(); //socket_t();
    socklen_t clientLen = socklen_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_SIZE_T
    // Solaris 2.6 does not have socklen_t
    size_t addrLen = size_t(); 
    size_t clientLen = size_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_INT
    // Solaris 2.6 does not have socklen_t
    int addrLen = int(); 
    int clientLen = int();
#endif

#endif
  
    if (0 > (sockfd = socket(af, SOCK_STREAM, 0))) {
		throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_OPEN_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
    }
	AppLogger::Write(APLOG_DEBUG9,"%s::%s %s",CLASS_SIG.c_str()
					,__func__,": socket open OK!");
    
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
    
    if (setMaskFlag == true) {
        curMask = umask(newMask_);
    }
#endif

    if (0 > bind(sockfd, (SockAddr*)&serverAddr, addrLen)) {
		throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_BIND_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
    }
#if !defined(WIN32)
    if (setMaskFlag == true) {
        umask(curMask);
    }
#endif
	AppLogger::Write(APLOG_DEBUG5,"%s::%s %s",CLASS_SIG.c_str()
					,__func__,": bind OK!");

    if (0 > listen(sockfd, 5)) {
		throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_LISTEN_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
    }

	AppLogger::Write(APLOG_DEBUG5,"%s::%s %s",CLASS_SIG.c_str()
					,__func__,": listen OK!");
    
    for (;ProcessInfo::GetProcessStatus()!=PSTAT_WAITTERM;) {
        clientLen = sizeof(clientAddr);

		AppLogger::Write(APLOG_DEBUG5,"%s::%s %s",CLASS_SIG.c_str()
						,__func__,": select ready!");

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
			throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_ACCEPT_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
        }
#if defined(WIN32)
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG7,"#### Accept info. ####");
		AppLogger::Write(APLOG_DEBUG7,"  b1=[%d]"
						,(short)clientAddr.sin_addr.S_un.S_un_b.s_b1);
		AppLogger::Write(APLOG_DEBUG7,"  b2=[%d]"
						,(short)clientAddr.sin_addr.S_un.S_un_b.s_b2);
		AppLogger::Write(APLOG_DEBUG7,"  b3=[%d]"
						,(short)clientAddr.sin_addr.S_un.S_un_b.s_b3);
		AppLogger::Write(APLOG_DEBUG7,"  b4=[%d]"
						,(short)clientAddr.sin_addr.S_un.S_un_b.s_b4);
#endif //DEBUG
        //check socket client by local process
        //ip addr. is "127.0.0.1" OK
        if (127 != (short)clientAddr.sin_addr.S_un.S_un_b.s_b1 ||
            0 != (short)clientAddr.sin_addr.S_un.S_un_b.s_b2 ||
            0 != (short)clientAddr.sin_addr.S_un.S_un_b.s_b3 ||
            1 != (short)clientAddr.sin_addr.S_un.S_un_b.s_b4) {
			Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_ACCEPT_ERROR
						,APLOG_ERROR,__FILE__,__LINE__);
			e.SetErrText("accept from invalid client. IP is not \"127.0.0.1\"");
			throw e;
        }
#endif //defined(WIN32)

		AppLogger::Write(APLOG_DEBUG7,"%s::%s %s fd=(%d)",CLASS_SIG.c_str()
						,__func__,": accept OK!",newsockfd);

        // ・int newsockfd;
        // ・ChannelManager* this;
        ThrData* thrData = new ThrData();
        thrData->that = this;
        thrData->sockfd = newsockfd;

        //create new thread
#if defined(WIN32)
	HANDLE thrId;
	thrId = (HANDLE)_beginthread(connectionThread, 0, thrData);
#else
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
#endif
    } // end for (;;)
}


ThrFuncReturnType
ChannelManager::connectionThread(ThrFuncArgType aArg)
{
  AppLogger::Write(APLOG_DEBUG7,"%s::%s",CLASS_SIG.c_str(),__func__);
  //引渡し情報のチェック
  if (!aArg) {
	AppLogger::Write(APLOG_ERROR,"%s%s"
					,CLASS_SIG.c_str()
					,"::connectionThread: invalid argument"
					);
    ReturnThread(NULL);
  }

  //引渡し情報の展開
  ThrData* thrData = (ThrData*)aArg;
  ChannelManager* that = thrData->that;
  int sockfd = thrData->sockfd;
  delete thrData;

  //プロセス状況依存作成チェック
  if (!(ProcessInfo::AddThreadInfo())){
	AppLogger::Write(APLOG_INFO,"%s%s"
					,CLASS_SIG.c_str()
					,"::connectionThread: Wait termination"
					);
	if (0 < sockfd) {
		// program busy or waiting termination
		CloseSocket(sockfd);
	}
    ReturnThread(NULL);
  }

  AppLogger::Write(APLOG_DEBUG7,"%s::%s: sockfd=(%d)"
  					,CLASS_SIG.c_str(),__func__,sockfd);

  try{
    that->doProc(sockfd);
  }
  catch(Exception e){
    e.AddFuncCallStack();
	AppLogger::Write(e);
  }
  catch(...){
	AppLogger::Write(APLOG_ERROR,"unknown error !");
  }

  if (0 < sockfd) {
      CloseSocket(sockfd);
  }

  ProcessInfo::DelThreadInfo();
  ReturnThread(NULL);
}

void 
ChannelManager::doProc(int sockfd)
{
  AppLogger::Write(APLOG_DEBUG7,"%s::%s: sockfd=(%d)"
  					,CLASS_SIG.c_str(),__func__,sockfd);

}

