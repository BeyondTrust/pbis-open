/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: connection.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <OpenSOAP/Defines.h>

extern
void OPENSOAP_API
sendStdout(std::vector<std::string> soapMsg);

extern
void OPENSOAP_API
recvStdin(std::vector<std::string>& soapMsg);

/* 標準出力に string クラスを送る */
extern
void OPENSOAP_API
sendStdout(const std::string& soapMsg);
//sendStdout(std::string soapMsg);

/* 標準入力を読み取った結果を string クラスに入れる */
extern
void OPENSOAP_API
recvStdin(std::string& soapMsg);

extern
int OPENSOAP_API
openFIFO(std::string fifoName, int flags);

extern
void OPENSOAP_API
sendFIFO(int fd, std::vector<std::string> soapMsg);

extern
void OPENSOAP_API
recvFIFO(int fd, std::vector<std::string>& soapMsg);


/* FIFO に string クラスを送る */
extern
void OPENSOAP_API
sendFIFO(int fd, std::string soapMsg);

/* FIFO を読み取った結果を string クラスに入れる */
extern
void OPENSOAP_API
recvFIFO(int fd, std::string& soapMsg);

extern
int OPENSOAP_API
openSocket(const std::string& hostname, unsigned int port);

extern
#if defined(WIN32)
int OPENSOAP_API
#else
ssize_t
#endif
sendSocket(int fd, const std::string& msg);

extern
#if defined(WIN32)
int OPENSOAP_API
#else
ssize_t
#endif
recvSocket(int fd, std::string& msg);

#endif /* CONNECTION_H */

