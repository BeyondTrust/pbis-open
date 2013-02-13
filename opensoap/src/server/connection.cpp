/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: connection.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <sys/types.h>

#if defined(WIN32)
#include <windows.h>
#include <io.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>

#include "connection.h"
#include "AppLogger.h"
#include "Exception.h"
#include "SrvErrorCode.h"

using namespace std;
using namespace OpenSOAP;

void OPENSOAP_API
sendStdout (std::vector<std::string> soapMsg)
{
  for (std::vector<std::string>::iterator iter = soapMsg.begin();
       iter != soapMsg.end(); iter++) {
    cout << *iter << endl;
  }
}

void OPENSOAP_API
recvStdin (std::vector<std::string>& soapMsg)
{
  const unsigned int bufLength = 10240;
  char buf[bufLength];
  
  while (fgets(buf, bufLength, stdin)) {
//      if (strncmp(buf, "QUIT", 4) == 0) {
//        break;
//      } 
    soapMsg.push_back (std::string (buf));
  }
}

void OPENSOAP_API
sendStdout (const std::string& soapMsg)
{
  cout << soapMsg << endl;
}

void OPENSOAP_API
recvStdin (std::string& soapMsg)
{
  const unsigned int bufLength = 10240;
  char buf[bufLength];
  
  while (fgets(buf, bufLength, stdin)) {
    //soapMsg += std::string(buf);
	soapMsg += buf;
  }
}


int OPENSOAP_API
openFIFO (std::string fifo_name, int flags)
{
#if defined(WIN32)
  int fd = _open (fifo_name.c_str(), flags);
#else
  int fd = open (fifo_name.c_str(), flags);
#endif
  if (fd < 0) {
	AppLogger::Write(APLOG_ERROR,"FIFO open failed !!");
  }
  return fd;
}

void OPENSOAP_API
sendFIFO (int fd, std::vector<std::string> soapMsg)
{
  AppLogger::Write(APLOG_DEBUG9,"sendFIFO");

  for (std::vector<std::string>::iterator iter = soapMsg.begin();
       iter != soapMsg.end(); iter++) {
    AppLogger::Write(APLOG_DEBUG9,"%s",(*iter).c_str());
    if (iter == soapMsg.end() - 1) {
#if defined(WIN32)
		Sleep(100);
#else
      usleep(100000);
#endif
    }
    write (fd, iter->c_str(), iter->length());
  }
#if !defined(WIN32)
  fsync(fd);
#endif
}

void OPENSOAP_API
recvFIFO (int fd, std::vector<std::string>& soapMsg)
{
  AppLogger::Write(APLOG_DEBUG9,"FIFO recv begin");

#if !defined(WIN32)
  fsync(fd);
#endif
  std::vector<char>::size_type        bufLength = 10240L;
  std::vector<char>   buf(bufLength, '\0');
  
  std::vector<char>::size_type readLen;
  
  //for g++-3.0
  //readLen = read(fd, buf.begin(), buf.size());  
  readLen = read(fd, &*buf.begin(), buf.size());  
  do {
    //for g++-3.0
    //std::string recv_str (buf.begin(), readLen);
    std::string recv_str (&*buf.begin(), readLen);
    AppLogger::Write(APLOG_DEBUG9,"readLen = %d:%s",readLen,recv_str.c_str());
    soapMsg.push_back (recv_str);
#if !defined(WIN32)
    fsync(fd);
#endif
    //for g++-3.0
    //readLen = read(fd, buf.begin(), buf.size());  
    readLen = read(fd, &*buf.begin(), buf.size());  
  } while (readLen > 0);
#if !defined(WIN32)
  fsync(fd);
#endif

  AppLogger::Write(APLOG_DEBUG9,"FIFO recv end");

}

/* FIFO に string クラスを送る */
void OPENSOAP_API
sendFIFO (int fd, std::string soapMsg)
{
  AppLogger::Write(APLOG_DEBUG9,"sendFIFO");

  write (fd, soapMsg.c_str(), soapMsg.length());
#if !defined(WIN32)
  fsync(fd);
#endif
}

/* FIFO を読み取った結果を string クラスに入れる */
void OPENSOAP_API
recvFIFO (int fd, std::string& soapMsg)
{
  AppLogger::Write(APLOG_DEBUG9,"FIFO recv begin");

#if !defined(WIN32)
  fsync(fd);
#endif
  std::vector<char>::size_type        bufLength = 10240L;
  std::vector<char>   buf(bufLength, '\0');
  
  std::vector<char>::size_type readLen;
  
  //for g++-3.0
  //readLen = read(fd, buf.begin(), buf.size());  
  readLen = read(fd, &*buf.begin(), buf.size());  
  do {
    //for g++-3.0
    //std::string recv_str (buf.begin(), readLen);
    std::string recv_str (&*buf.begin(), readLen);
    AppLogger::Write(APLOG_DEBUG9,"readLen = %d:%s",readLen,recv_str.c_str());
    soapMsg += recv_str;
#if !defined(WIN32)
    fsync(fd);
#endif
    //for g++-3.0
    //readLen = read(fd, buf.begin(), buf.size());  
    readLen = read(fd, &*buf.begin(), buf.size());  
  } while (readLen > 0);
#if !defined(WIN32)
  fsync(fd);
#endif

  AppLogger::Write(APLOG_DEBUG9,"FIFO recv end");

}

int OPENSOAP_API
openSocket (const std::string &hostname, unsigned int port)
{
    int fd;
    struct sockaddr_in    addr;
    struct hostent *hp;
  
    /*
     *  ソケットを作る。このソケットはUNIXドメインで、ストリーム型ソケット。
     */
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_CREATE_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
		AppLogger::Write(e);
		return -1;
    }
  
    /* 
     * addrの中身を0にしておかないと、bind()でエラーが起こることがある
     */
  
    memset ((char *)&addr, 0, sizeof(addr));
    
    /*
     * ソケットの名前を入れておく
     */
  
    if ((hp = gethostbyname(hostname.c_str())) == NULL) {
		AppLogger::Write(APLOG_ERROR,"No such host.");
		return -1;
    }
    memcpy(reinterpret_cast<char *>(&addr.sin_addr),
	   hp->h_addr, hp->h_length);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    /*
     *  サーバーとの接続を試みる。これが成功するためには、サーバーがすでに
     *  このアドレスをbind()して、listen()を発行していなければならない。
     */
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		Exception e(-1,OPENSOAPSERVER_NETWORK_SOCKET_CONNECT_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
		AppLogger::Write(e);
      return -1;
    }
    
    return fd;
}

#if defined(WIN32)
int OPENSOAP_API
#else
ssize_t
#endif
sendSocket (int fd, const std::string& msg)
{
#if defined(WIN32)
	int ret = send(fd, msg.c_str(), msg.length(), 0);
#else
    ssize_t ret = write(fd, msg.c_str (), msg.length ());
#endif
    return ret;
}

#if defined(WIN32)
int OPENSOAP_API
#else
ssize_t
#endif
recvSocket (int fd, std::string& msg)
{
  //std::vector<char>::size_type	bufLength = 1024L;
  std::vector<char>::size_type	bufLength = 1049600L; //1024*1024+1024
  std::vector<char>	buf(bufLength);

#if defined(WIN32)
	int ret = recv(fd, &*buf.begin(), buf.size(), 0);
#else
	//for g++-3.0
	//ssize_t ret = read(fd, buf.begin(), buf.size());
	ssize_t ret = read(fd, &*buf.begin(), buf.size());
#endif
  if (ret >= 0) {
    std::vector<char>::size_type readLen = ret;
    //for g++-3.0
    //msg =  std::string(buf.begin(), readLen);
    msg =  std::string(&*buf.begin(), readLen);
  }
  else {
	AppLogger::Write(APLOG_ERROR,"connection::recvSocket: recvSocket failed !");
    return ret;
  }

#if defined(WIN32)
  while((ret = recv(fd, &*buf.begin(), buf.size(), 0)) != 0)
#else
  //for g++-3.0
  //while((ret = read(fd, buf.begin(), buf.size())) != 0)
  while((ret = read(fd, &*buf.begin(), buf.size())) != 0)
#endif
  if (ret >= 0) {
    std::vector<char>::size_type readLen = ret;
    //for g++-3.0
    //msg += std::string(buf.begin(), readLen);
    msg += std::string(&*buf.begin(), readLen);
  }
  
  return msg.size();
}
