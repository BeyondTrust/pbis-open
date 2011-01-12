/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ChannelSelector.cpp,v $
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
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <iostream>
#include <fcntl.h>

#include "MapUtil.h"
#include "ServerCommon.h"
#include "ChannelSelector.h"
#include "SrvConf.h"

//temp.
#if defined(HAVE_FIFO_IMPL)
#include "FIFODescriptor.h"
#include "FIFOSelector.h"
#endif

#include "AppLogger.h"
#include "Exception.h"
#include "SrvErrorCode.h"

using namespace OpenSOAP;
using namespace std;

//for LOG
static const string CLASS_SIG = "ChannelSelector";


ChannelSelector::ChannelSelector()
{
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
}

ChannelSelector::ChannelSelector(
#if defined(WIN32)
				 const int addr)
#else
                                 const string& addr)
#endif
  : socketAddr_(addr)
{
  AppLogger::Write(APLOG_DEBUG9,"%s::%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
}

ChannelSelector::~ChannelSelector()
{
  AppLogger::Write(APLOG_DEBUG9,"%s::~%s",CLASS_SIG.c_str(),CLASS_SIG.c_str());
}

void 
ChannelSelector::setSocketAddr(
#if defined(WIN32)
			       const int addr)
#else
                               const string& addr)
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
}

int 
ChannelSelector::connectChannelManager()
{
    static char METHOD_LABEL[] = "ChannelSelector::connectChannelManager: ";

#if defined(WIN32)
    typedef SOCKADDR SockAddr;
    typedef SOCKADDR_IN SockAddrAf;

    SockAddrAf servAddr;
    
    SOCKET sockfd = INVALID_SOCKET;
    WSADATA data;
    if (WSAStartup(0x101,&data)) {
		Exception e(-1,OPENSOAPSERVER_NETWORK_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
		e.SetErrText("WSAStartup failed.")
        throw e;
    }
    int af = AF_INET;
    int addrLen = sizeof(SockAddrAf);
    int servAddrLen = sizeof(servAddr);

    servAddr.sin_family = af;
    PHOSTENT phe = gethostbyname("localhost");
    memcpy(&(servAddr.sin_addr), phe->h_addr, phe->h_length);
    servAddr.sin_port = htons(socketAddr_);
#else //defined(WIN32)
    typedef struct sockaddr SockAddr;
    typedef struct sockaddr_un SockAddrAf;
    
    int sockfd = int();
    SockAddrAf servAddr;
    int af = AF_UNIX;
    
    memset((char*)&servAddr, 0, sizeof(servAddr));
    servAddr.sun_family = af;
    strcpy(servAddr.sun_path, socketAddr_.c_str());
    
#if HAVE_TYPE_OF_ADDRLEN_AS_SOCKLEN_T
    socklen_t servAddrLen = socklen_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_SIZE_T
    size_t servAddrLen = size_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_INT
    int servAddrLen = int();
#endif
    
    servAddrLen = sizeof(servAddr);

#endif //defined(WIN32)/else

    if (0 > (sockfd = socket(af, SOCK_STREAM, 0))) {
		throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_OPEN_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
    }
    
	AppLogger::Write(APLOG_DEBUG7,"%s: %s",METHOD_LABEL,"socket open OK!");
    
    if (0 > connect(sockfd, (SockAddr*)&servAddr, servAddrLen)) {
		throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_CONNECT_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
    }

	AppLogger::Write(APLOG_DEBUG7,"%s: %s",METHOD_LABEL,"connect OK!");

    return sockfd;
}

string ChannelSelector::makeSendOpenMessage(const int waitSecond)
{
  return "";
}

string ChannelSelector::makeSendCloseMessage()
{
  return "";
}

int 
ChannelSelector::open(ChannelDescriptor& chnlDesc, const int waitSecond)
{
    static char METHOD_LABEL[] = "ChannelSelector::open: ";

    // timeout 値のチェック
    if (ChannelDescriptor::NO_WAIT != waitSecond &&
        ChannelDescriptor::WAIT_NOLIMIT != waitSecond &&
        0 > waitSecond) {
		AppLogger::Write(APLOG_ERROR,"%s%s"
						,METHOD_LABEL
						,"invalid timeout second"
						);
        return -1;
    }
    
    int sockfd = connectChannelManager();

    if (0 > sockfd) {

        return sockfd;
    }

    string sendMessage = makeSendOpenMessage(waitSecond);
    
	AppLogger::Write(APLOG_DEBUG7,"%s: %s=[%s]"
					,METHOD_LABEL,"sendMessage",sendMessage.c_str());

    if (0 > write(sockfd, sendMessage)) {
		Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_WRITE_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
		AppLogger::Write(e);
        return -1;
    }

	AppLogger::Write(APLOG_DEBUG7,"%s: %s",METHOD_LABEL,"write done.");

    string recvMessage;
    int recvLen = read(sockfd, recvMessage);

	AppLogger::Write(APLOG_DEBUG7,"%s: %s=[%s] %s=[%d]"
					,METHOD_LABEL,"read",recvMessage.c_str(),"len",recvLen);

    MapUtil mapUtil(OpenSOAP::ChnlCommon::ITEM_DELMIT, 
                    OpenSOAP::ChnlCommon::KEY_DELMIT);
    mapUtil.insertItem(recvMessage);
    //結果情報
    if (!mapUtil.exists(OpenSOAP::ChnlCommon::RESULT)) {
		AppLogger::Write(APLOG_ERROR,"%s%s%s=[%s]"
						,METHOD_LABEL
						,"invalid read message. "
						,"RESUT not found. msg"
						,recvMessage.c_str()
						);
        return -1;
    }
    //チャンネルタイプ情報
    if (!mapUtil.exists(OpenSOAP::ChnlCommon::CHNL_TYPE)) {
		AppLogger::Write(APLOG_ERROR,"%s%s%s=[%s]"
						,METHOD_LABEL
						,"invalid read message. "
						,"CHNL_TYPE not found. msg"
						,recvMessage.c_str()
						);
        return -1;
    }

    string result = mapUtil[OpenSOAP::ChnlCommon::RESULT];
    string type = mapUtil[OpenSOAP::ChnlCommon::CHNL_TYPE];
    if (OpenSOAP::ChnlCommon::SUCCESS == result) {

        if (OpenSOAP::ChnlCommon::UNIX_SOCKET == type) {
            chnlDesc.setReadFd(sockfd);
            chnlDesc.setWriteFd(sockfd);
        }
        else if (OpenSOAP::ChnlCommon::FIFO == type) {
            //※※※※※※※※※※※※
            //この部分は書き換え必要
            //※※※※※※※※※※※※
#if defined(HAVE_FIFO_IMPL)
            FIFODescriptor fifoDesc; 
            FIFOSelector* fifoSelector = new FIFODownsideSelector(6500);
            if (0 != fifoSelector->open(fifoDesc, waitSecond)) {
				AppLogger::Write(APLOG_ERROR,"%s%s"
								,METHOD_LABEL
								,"fifo open failed"
								);
                return -1;
            }

            chnlDesc.setReadFd(fifoDesc.getReadFIFO());
            chnlDesc.setWriteFd(fifoDesc.getWriteFIFO());
            chnlDesc.setId(fifoDesc.getId());
            
            delete fifoSelector;
#endif
        }
        else if (OpenSOAP::ChnlCommon::INET_SOCKET == type) {
            // not support
			AppLogger::Write(APLOG_WARN,"%s%s%s=[%d]"
							,METHOD_LABEL
							,"invalid read message. "
							,"not supported CHNL_TYPE . CHNL_TYPE"
							,type.c_str()
							);
            return -1;
        }
        else {
			AppLogger::Write(APLOG_ERROR,"%s%s%s=[%d]"
							,METHOD_LABEL
							,"invalid read message. "
							,"ilegal CHNL_TYPE . CHNL_TYPE"
							,type.c_str()
							);
            return -1;
        }
    } 
    else if (OpenSOAP::ChnlCommon::FAILURE == result) {
        return 1;
    }
    else {
		AppLogger::Write(APLOG_ERROR,"%s%s=[%s]"
						,METHOD_LABEL
						,"invalid result. RESULT"
						,result.c_str()
						);
        return -1;
    }
    
    return 0;
}

int 
ChannelSelector::close(ChannelDescriptor& chnlDesc)
{
    static char METHOD_LABEL[] = "ChannelSelector::close: ";

#if 1    
    //close main socket
    //need to customize in other communication channel type support.
    CloseSocket(chnlDesc.getWriteFd());
    //close(chnlDesc.getReadFd());

    return 0;
#else //if 1
    int sockfd = connectChannelManager();
    if (0 > sockfd) {
        return sockfd;
    }
    
    string sendMessage = makeSendCloseMessage();
    
    if (0 > write(sockfd, sendMessage)) {
		Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_WRITE_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
		AppLogger::Write(e);
        CloseSocket(sockfd);
        return -1;
    }

    string recvMessage;
    int recvLen = read(sockfd, recvMessage);
    
    MapUtil mapUtil(OpenSOAP::ChnlCommon::ITEM_DELMIT,
                    OpenSOAP::ChnlCommon::KEY_DELMIT);
    mapUtil.insertItem(recvMessage);
    if (!mapUtil.exists(OpenSOAP::ChnlCommon::RESULT)) {
		AppLogger::Write(APLOG_ERROR,"%s%s=[%s]"
						,METHOD_LABEL
						,"invalid read message. msg"
						,recvMessage.c_str()
						);
        CloseSocket(sockfd);
        return -1;
    }

    string result = mapUtil[OpenSOAP::ChnlCommon::RESULT];

    if (OpenSOAP::ChnlCommon::SUCCESS == result) {
        
        if (mapUtil.exists(OpenSOAP::ChnlCommon::CHNL_TYPE) &&
            OpenSOAP::ChnlCommon::FIFO == mapUtil[OpenSOAP::ChnlCommon::CHNL_TYPE]) {
            //※※※※※※※※※※※※
            //この部分は書き換え必要
            //※※※※※※※※※※※※
#if defined(HAVE_FIFO_IMPL)
            FIFODescriptor fifoDesc; 
            fifoDesc.setReadFIFO(chnlDesc.getReadFd());
            fifoDesc.setWriteFIFO(chnlDesc.getWriteFd());
            fifoDesc.setId(chnlDesc.getId());
            
            FIFOSelector* fifoSelector = new FIFODownsideSelector(6500);
            fifoSelector->close(fifoDesc);
            delete fifoSelector;
#endif
        }
        else {
            
            //socke close
            CloseSocket(chnlDesc.getWriteFd());
        }
    } 
    else if (OpenSOAP::ChnlCommon::FAILURE == result) {
        CloseSocket(sockfd);
        return 1;
    }
    else {
		AppLogger::Write(APLOG_ERROR,"%s%s=[%s]"
						,METHOD_LABEL
						,"invalid result. result"
						,result.c_str()
						);
        CloseSocket(sockfd);
        return -1;
    }
  
    CloseSocket(sockfd);
    return 0;
#endif //if 1/else
}


// End of ChannelSelector.cpp
