/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ServerCommon.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef ServerCommon_H
#define ServerCommon_H

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif // HAVE_CONFIG_H

#include <string>
#include <OpenSOAP/Defines.h>

#if !defined(HAVE_DAEMON) && !defined(WIN32)
// daemon declare
extern "C" int daemon(int nochdir, int noclose);
#endif // !HAVE_DAEMON && !WIN32

//use timestamp in log file
#define USE_TIMESTAMP

#ifdef USE_TIMESTAMP
extern std::string OPENSOAP_API
printTimestamp();
#define PRINT_TIMESTAMP << printTimestamp()
#else //USE_TIMESTAMP
#define PRINT_TIMESTAMP 
#endif //USE_TIMESTAMP/else

//log output macro
#define LOGPRINT(l) std::cerr PRINT_TIMESTAMP << l


#if defined(WIN32)
#define CloseSocket(s) closesocket(s)
#else //WIN32
#define CloseSocket(s) ::close(s)
#endif //defeind(WIN32)/else

namespace OpenSOAP {

    // for LOG
    extern const std::string OPENSOAP_VAR TAG_ERR;
    extern const std::string OPENSOAP_VAR TAG_WRN;
    extern const std::string OPENSOAP_VAR TAG_INF;
    extern const std::string OPENSOAP_VAR TAG_CRTCL_ERR;
    
    //keyword
    extern const char OPENSOAP_VAR FILE_PATH_DELMIT;
    extern const int OPENSOAP_VAR DEFAULT_PORT;

    namespace Result {
        extern const std::string OPENSOAP_VAR SUCCESS;
        extern const std::string OPENSOAP_VAR FAILURE;
    }

    namespace ResultString {
        extern const std::string OPENSOAP_VAR SUCCESS;
        extern const std::string OPENSOAP_VAR FAILURE;
    }
    namespace BoolString {
        extern const std::string OPENSOAP_VAR STR_TRUE;
        extern const std::string OPENSOAP_VAR STR_FALSE;
    }
    namespace EntryStatus {
        extern const std::string OPENSOAP_VAR NOENTRY;
        extern const std::string OPENSOAP_VAR NOVALUE;
    }    

    namespace ChnlCommon {
        extern const std::string OPENSOAP_VAR CMD;
        extern const std::string OPENSOAP_VAR OPEN;
        extern const std::string OPENSOAP_VAR CLOSE;
        extern const std::string OPENSOAP_VAR RESULT;
        extern const std::string OPENSOAP_VAR SUCCESS;
        extern const std::string OPENSOAP_VAR FAILURE;
        extern const std::string OPENSOAP_VAR CHNL_TYPE;
        extern const std::string OPENSOAP_VAR UNIX_SOCKET;
        extern const std::string OPENSOAP_VAR INET_SOCKET;
        extern const std::string OPENSOAP_VAR FIFO;
        extern const std::string OPENSOAP_VAR WAIT;
    
        extern const std::string OPENSOAP_VAR OPERATION;
        extern const std::string OPENSOAP_VAR POP;
        extern const std::string OPENSOAP_VAR PUSH;
        extern const std::string OPENSOAP_VAR REF;
        extern const std::string OPENSOAP_VAR EXPIRE;
        
        extern const std::string OPENSOAP_VAR ITEM_DELMIT;
        extern const std::string OPENSOAP_VAR KEY_DELMIT;
    }

    //-------------------------------------------
    //common func.
    //read/write for network i/f(Socket/FIFO)
    extern int OPENSOAP_API
    read(int aFd, std::string& aData);
    extern int OPENSOAP_API
    write(int aFd, const std::string& aData);
#if !defined(WIN32)
    extern int read2(int aFd, std::string& aData);
#endif
    //-------------------------------------------
    
    // Service Connection Type
    // added 2002.03.11
    extern const std::string OPENSOAP_VAR SRV_CONNCT_SOCKET;
    extern const std::string OPENSOAP_VAR SRV_CONNCT_STDIO;
    extern const std::string OPENSOAP_VAR SRV_CONNCT_FIFO;
    extern const std::string OPENSOAP_VAR SRV_CONNCT_IPC;
    extern const std::string OPENSOAP_VAR SRV_CONNCT_MODULE;
    extern const std::string OPENSOAP_VAR SRV_CONNCT_COM;
    // added 2003/06/12
    extern const std::string OPENSOAP_VAR SRV_CONNCT_HTTP;
    // etc...
    
    // Socket Address 
#if defined(WIN32)
    extern const int OPENSOAP_VAR MSGDRV_SOCKET_ADDR;
    //extern const int OPENSOAP_VAR SYNC_SRVDRV_SOCKET_ADDR;
    //extern const int OPENSOAP_VAR ASYNC_SRVDRV_SOCKET_ADDR; 
    //extern const int OPENSOAP_VAR SRV_SOCKET_ADDR;
    //extern const int OPENSOAP_VAR FIFOSRV_SOCKET_ADDR;
    //extern const int OPENSOAP_VAR FWDER_SOCKET_ADDR;
    extern const int OPENSOAP_VAR IDMGR_SOCKET_ADDR;
    extern const int OPENSOAP_VAR SSMLATTRMGR_SOCKET_ADDR;
    extern const int OPENSOAP_VAR SRVCONFATTRMGR_SOCKET_ADDR;
    extern const int OPENSOAP_VAR FWDQUEUE_SOCKET_ADDR;
    extern const int OPENSOAP_VAR REQQUEUE_SOCKET_ADDR;
    extern const int OPENSOAP_VAR RESSPOOL_SOCKET_ADDR;
    extern const int OPENSOAP_VAR TTLMGR_SOCKET_ADDR;

    // path to temporal file directory
    extern const std::string OPENSOAP_VAR OPENSOAP_TMP_PATH;
    extern const std::string OPENSOAP_VAR SERVER_CONF_TAG;
#else
    extern const std::string MSGDRV_SOCKET_ADDR;
    extern const std::string SYNC_SRVDRV_SOCKET_ADDR;
    extern const std::string ASYNC_SRVDRV_SOCKET_ADDR;
    extern const std::string SRV_SOCKET_ADDR;
    extern const std::string FIFOSRV_SOCKET_ADDR;
    extern const std::string FWDER_SOCKET_ADDR;
    extern const std::string IDMGR_SOCKET_ADDR;
    extern const std::string SSMLATTRMGR_SOCKET_ADDR;
    extern const std::string SRVCONFATTRMGR_SOCKET_ADDR;
    extern const std::string FWDQUEUE_SOCKET_ADDR;
    extern const std::string REQQUEUE_SOCKET_ADDR;
    extern const std::string RESSPOOL_SOCKET_ADDR;
    extern const std::string TTLMGR_SOCKET_ADDR;
#endif

    // PID File
    extern const std::string MSGDRV_PID_FILE;
    extern const std::string SYNC_SRVDRV_PID_FILE;
    extern const std::string ASYNC_SRVDRV_PID_FILE;
    extern const std::string SRV_PID_FILE;
    extern const std::string FIFOSRV_PID_FILE;
    extern const std::string FWDER_PID_FILE;
    extern const std::string IDMGR_PID_FILE;
    extern const std::string SSMLATTRMGR_PID_FILE;
    extern const std::string SRVCONFATTRMGR_PID_FILE;
    extern const std::string FWDQUEUE_PID_FILE;
    extern const std::string REQQUEUE_PID_FILE;
    extern const std::string RESSPOOL_PID_FILE;
    extern const std::string TTLMGR_PID_FILE;
    extern const std::string OPENSOAPMGR_PID_FILE;
    
    // table
    extern const std::string OPENSOAP_VAR REQQUEUE_TABLE_FILE;
    extern const std::string OPENSOAP_VAR FWDQUEUE_TABLE_FILE;
    extern const std::string OPENSOAP_VAR RESSPOOL_TABLE_FILE;
    extern const std::string OPENSOAP_VAR TTL_TABLE_FILE;
    
    // Private Key
    extern const std::string OPENSOAP_VAR SECURITY_DATA_PRIV_KEY;

    // SOAP message
    
} // end of namespace OpenSOAP


#endif /* ServerCommon_H */
