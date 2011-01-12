/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ServerCommon.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if defined(WIN32)
#include <io.h>
#include <winsock.h>
#else
#include <unistd.h>
#endif // WIN32

#include <iostream>
#include <time.h>
#include <cstdio>

#include "ServerCommon.h"
#include "SrvConfAttrHandler.h"
#include "AppLogger.h"

#if !defined(HAVE_DAEMON) && !defined(WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// daemon define
extern "C"
int
daemon(int nochdir,
       int noclose) {
  pid_t pid = fork();
  
  if (pid < 0) {
    /* fork failed */
    return -1;
  }
  else if (pid != 0) {
    /* parent is exit */
    exit(0); 
  }
  
  /* set session leader */
  if (-1 == setsid()) {
    return -1;
  }

  if (!nochdir) {
    /* chdir / */
    chdir("/");
  }
  
  if (!noclose) {
    int fd = open("/dev/null", O_RDWR);
    if (-1 != fd) {
      dup2(fd, 0);
      dup2(fd, 1);
      dup2(fd, 2);
    }
  }
  
  return 0;
}

#endif // !HAVE_DAEMON && !WIN32

using namespace OpenSOAP;
using namespace std;

//=============================================================

// for LOG
const string OpenSOAP::TAG_ERR = "<E>: ";
const string OpenSOAP::TAG_WRN = "<W>: ";
const string OpenSOAP::TAG_INF = "<I>: ";
const string OpenSOAP::TAG_CRTCL_ERR = "<Critical ERROR>: ";
//keyword
const char OpenSOAP::FILE_PATH_DELMIT = '/';
const int OpenSOAP::DEFAULT_PORT = 80;

// Service Connection Type
const string OpenSOAP::SRV_CONNCT_SOCKET = "Socket";
const string OpenSOAP::SRV_CONNCT_STDIO = "StdIO";
const string OpenSOAP::SRV_CONNCT_FIFO   = "FIFO";
const string OpenSOAP::SRV_CONNCT_IPC    = "IPC";
const string OpenSOAP::SRV_CONNCT_MODULE = "Module";
const string OpenSOAP::SRV_CONNCT_COM    = "COM";
const string OpenSOAP::SRV_CONNCT_HTTP    = "HTTP";


  // Socket Address 
#if defined(WIN32)
const int OpenSOAP::MSGDRV_SOCKET_ADDR = 7101;
//const int OpenSOAP::SYNC_SRVDRV_SOCKET_ADDR = 7102;
//const int OpenSOAP::ASYNC_SRVDRV_SOCKET_ADDR = 7103;
//const int OpenSOAP::SRV_SOCKET_ADDR = 7104;
//const int OpenSOAP::FIFOSRV_SOCKET_ADDR = 7105;
//const int OpenSOAP::FWDER_SOCKET_ADDR = 7106;
const int OpenSOAP::IDMGR_SOCKET_ADDR = 7107;
const int OpenSOAP::SSMLATTRMGR_SOCKET_ADDR = 7108;
const int OpenSOAP::SRVCONFATTRMGR_SOCKET_ADDR = 7109;
const int OpenSOAP::FWDQUEUE_SOCKET_ADDR = 7110;
const int OpenSOAP::REQQUEUE_SOCKET_ADDR = 7111;
const int OpenSOAP::RESSPOOL_SOCKET_ADDR = 7112;
const int OpenSOAP::TTLMGR_SOCKET_ADDR = 7113;

// path to temporal file directory
const string OpenSOAP::OPENSOAP_TMP_PATH = "/OpenSOAP/var/";
const string OpenSOAP::SERVER_CONF_TAG = "/server_conf";
#else
  
const string OpenSOAP::MSGDRV_SOCKET_ADDR = "MsgDrv";
const string OpenSOAP::SYNC_SRVDRV_SOCKET_ADDR = "SyncSrvDrv";
const string OpenSOAP::ASYNC_SRVDRV_SOCKET_ADDR = "AsyncSrvDrv";
const string OpenSOAP::SRV_SOCKET_ADDR = "Srv";
const string OpenSOAP::FIFOSRV_SOCKET_ADDR = "FIFOSrv";
const string OpenSOAP::FWDER_SOCKET_ADDR = "Fwder";
const string OpenSOAP::IDMGR_SOCKET_ADDR = "IdManager";
const string OpenSOAP::SSMLATTRMGR_SOCKET_ADDR = "SSMLAttrMgr";
const string OpenSOAP::SRVCONFATTRMGR_SOCKET_ADDR = "SrvConfAttrMgr";
const string OpenSOAP::FWDQUEUE_SOCKET_ADDR = "FwdQueue";
const string OpenSOAP::REQQUEUE_SOCKET_ADDR = "ReqQueue";
const string OpenSOAP::RESSPOOL_SOCKET_ADDR = "ResSpool";
const string OpenSOAP::TTLMGR_SOCKET_ADDR = "TTLManager";
#endif

// PID File
const string OpenSOAP::MSGDRV_PID_FILE = "msgDrvCreator.pid";
const string OpenSOAP::SYNC_SRVDRV_PID_FILE = "srvDrvCreator_sync.pid";
const string OpenSOAP::ASYNC_SRVDRV_PID_FILE = "srvDrvCreator_async.pid";
const string OpenSOAP::SRV_PID_FILE = "Srv.pid";
const string OpenSOAP::FIFOSRV_PID_FILE = "FIFOSrv.pid";
const string OpenSOAP::FWDER_PID_FILE = "fwderCreator.pid";
const string OpenSOAP::IDMGR_PID_FILE = "idManager.pid";
const string OpenSOAP::SSMLATTRMGR_PID_FILE = "ssmlAttrMgr.pid";
const string OpenSOAP::SRVCONFATTRMGR_PID_FILE = "srvConfAttrMgr.pid";
const string OpenSOAP::FWDQUEUE_PID_FILE = "queueManager_fwd.pid";
const string OpenSOAP::REQQUEUE_PID_FILE = "queueManager_req.pid";
const string OpenSOAP::RESSPOOL_PID_FILE = "spoolManager.pid";
const string OpenSOAP::TTLMGR_PID_FILE = "ttlManager.pid";
const string OpenSOAP::OPENSOAPMGR_PID_FILE = "OpenSOAPMgr.pid";

// table
const string OpenSOAP::REQQUEUE_TABLE_FILE = "req_queue_table";
const string OpenSOAP::FWDQUEUE_TABLE_FILE = "fwd_queue_table";
const string OpenSOAP::RESSPOOL_TABLE_FILE = "res_spool_table";
const string OpenSOAP::TTL_TABLE_FILE = "ttl_table";

// Private Key
const string OpenSOAP::SECURITY_DATA_PRIV_KEY = "privKey.pem";
//=============================================================

const string OpenSOAP::Result::SUCCESS("success");
const string OpenSOAP::Result::FAILURE("failure");

const string OpenSOAP::BoolString::STR_TRUE("true");
const string OpenSOAP::BoolString::STR_FALSE("false");

const string OpenSOAP::EntryStatus::NOENTRY("No Entry");
const string OpenSOAP::EntryStatus::NOVALUE("No Value");

//ChnlCommon
const string OpenSOAP::ChnlCommon::CMD = "CMD";
const string OpenSOAP::ChnlCommon::OPEN = "OPEN";
const string OpenSOAP::ChnlCommon::CLOSE = "CLOSE";
const string OpenSOAP::ChnlCommon::RESULT = "RESULT";
const string OpenSOAP::ChnlCommon::SUCCESS = "SUCCESS";
const string OpenSOAP::ChnlCommon::FAILURE = "FAILURE";
const string OpenSOAP::ChnlCommon::CHNL_TYPE = "CHNL_TYPE";
const string OpenSOAP::ChnlCommon::UNIX_SOCKET = "UNIX_SOCKET";
const string OpenSOAP::ChnlCommon::INET_SOCKET = "INET_SOCKET";
const string OpenSOAP::ChnlCommon::FIFO = "FIFO";
const string OpenSOAP::ChnlCommon::WAIT = "WAIT";

const string OpenSOAP::ChnlCommon::OPERATION = "OPERATION";
const string OpenSOAP::ChnlCommon::POP = "POP";
const string OpenSOAP::ChnlCommon::PUSH = "PUSH";
const string OpenSOAP::ChnlCommon::REF = "REF";
const string OpenSOAP::ChnlCommon::EXPIRE = "EXPIRE";

const string OpenSOAP::ChnlCommon::ITEM_DELMIT = ":";
const string OpenSOAP::ChnlCommon::KEY_DELMIT = "=";
  

//=============================================================


#ifdef USE_TIMESTAMP
extern
string OPENSOAP_API
printTimestamp()
{
  time_t cur_time;
  const int bufsize = 1024;
  char buf[bufsize];
  struct tm* cur_tm = NULL;
  time(&cur_time);
  cur_tm = localtime(&cur_time);
  memset(buf, 0x00, bufsize);
  //strftime(buf, bufsize, "[%b %e %H:%M:%S] ", cur_tm);
  //strftime(buf, bufsize, "[%c] ", cur_tm);
  strftime(buf, bufsize, "[%Y/%m/%d %H:%M:%S] ", cur_tm);
  return string(buf);
}
#endif //USE_TIMESTAMP

//======== common function ==============

int OPENSOAP_API
OpenSOAP::read(int fd, std::string& data)
{
    static char METHOD_LABEL[] = "OpenSOAP::read: ";

    char readBuf[4096];
    memset(readBuf, 0x00, sizeof(readBuf));
#if defined(WIN32)
    int
#else
        ssize_t 
#endif
	readLen = 0;
    data = "";

#if defined(WIN32)
    //readLen = _read(fd, readBuf, sizeof(readBuf)-1);
    readLen = recv(fd, readBuf, sizeof(readBuf), 0);
    if (readLen == SOCKET_ERROR) {
		AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"recv failed");
        return -1;
    }
#else
    readLen = ::read(fd, readBuf, sizeof(readBuf)-1);
#endif

    if (0 == readLen) {
#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"OpenSOAP::read 0 byte done.");
#endif //DEBUG
    } 
    else if (0 > readLen) {
        perror(METHOD_LABEL);
		AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
						,"read failed.","fd",fd);
        return -1;
    }
    readBuf[readLen] = 0x00;
    data += readBuf;

#ifdef DEBUG
	AppLogger::Write(APLOG_DEBUG9,"%s [%s] %s=(%d)",METHOD_LABEL
					,readBuf,"len",readLen);
#endif //DEBUG

    //return 0;
    return data.length();
}

#if !defined(WIN32)
int 
OpenSOAP::read2(int fd, std::string& data)
{
    static char METHOD_LABEL[] = "OpenSOAP::read2: ";

    //  const int READBUFSIZE = 1049600;
    const int READBUFSIZE = 1024;
    char* readBuf = new char[READBUFSIZE];

    memset(readBuf, 0x00, READBUFSIZE);
    ssize_t readLen = 0;
    data = "";

    while (1) {
        readLen = ::read(fd, readBuf, READBUFSIZE - 1);
        
        if (0 == readLen) {
#ifdef DEBUG
			AppLogger::Write(APLOG_DEBUG9,"OpenSOAP::read2 0 byte done.");
#endif //DEBUG
            break;
        } 
        else if (0 > readLen) {
			AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"read failed");
            delete[] readBuf;
            return -1;
        }

        readBuf[readLen] = 0x00;
        data += readBuf;

#ifdef DEBUG
		AppLogger::Write(APLOG_DEBUG9,"%s [%s] %s=(%d)",METHOD_LABEL
						,readBuf,"len",readLen);
#endif //DEBUG
    }

    delete[] readBuf;
  
    return data.length();
}
#endif


int OPENSOAP_API
OpenSOAP::write(int fd, const std::string& data)
{
    static char METHOD_LABEL[] = "OpenSOAP::write: ";

    char writeBuf[4096];
    memset(writeBuf, 0x00, sizeof(writeBuf));
#if defined(WIN32)
    int
#else //defined(WIN32)
    ssize_t 
#endif //defined(WIN32)/else
        writeLen = 0;

    //std::string::size_type remainLen = data.length();
    int remainLen = data.length();

    while (remainLen > 0) {
#if defined(WIN32)
        writeLen = send(fd, 
                        data.c_str()+(data.length() - remainLen),
                        remainLen,
                        0);
        if (writeLen == SOCKET_ERROR) {
			AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"send failed");
            return -1;
        } 
#else //defined(WIN32)
        writeLen = ::write(fd, 
                           data.c_str()+(data.length() - remainLen), 
                           remainLen);
#endif //defined(WIN32)/else

        if (0 > writeLen) {
            perror(METHOD_LABEL);
			AppLogger::Write(APLOG_ERROR,"%s%s %s=(%d)",METHOD_LABEL
							,"write failed.","fd",fd);
            return -1;
        } 
        else if (writeLen > remainLen) {
			AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
							,"invalid write length.");
        }
        remainLen -= writeLen;
    }
    return data.length() - remainLen;
}

// End of ServerCommon.cpp
