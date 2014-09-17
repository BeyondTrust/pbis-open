/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SSMLAttrHandler.cpp,v $
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

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
//for shm
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <iostream>
#include <string>

#include "MapUtil.h"
#include "StringUtil.h"
#include "ServerCommon.h"
#include "SSMLAttrMgrProtocol.h"
#include "SSMLAttrHandler.h"
#include "SrvConf.h"
#include "AppLogger.h"
#include "Exception.h"
#include "SrvErrorCode.h"

using namespace OpenSOAP;
using namespace std;

// for LOG
static const string CLASS_SIG = "SSMLAttrHandler";

SSMLAttrHandler::SSMLAttrHandler()
{
  SrvConf srvConf;
  socketAddr_ = srvConf.getSocketPath() + SSMLATTRMGR_SOCKET_ADDR;
}

SSMLAttrHandler::~SSMLAttrHandler()
{

}

int 
SSMLAttrHandler::queryXml(const string& query, 
                          vector<string>& values,
                          const SSMLType ssmlType)
{
    static char METHOD_LABEL[] = "SSMLAttrHandler::queryXml: ";

    //check shm buffer
    if (getShmCache(query, values, ssmlType)) {
        return 0;
    }

    int sockfd = connectManager();
    if (0 > sockfd) {
		AppLogger::Write(APLOG_ERROR,"%s%s"
			,METHOD_LABEL 
            ,"connect manager failed"
            );
        return -1;
    }
  
    //送信メッセージ作成
    MapUtil writeDataMap(ITEM_DELMIT, VAL_DELMIT);
    writeDataMap.insert(make_pair(CMD,SEARCH));
    writeDataMap.insert(make_pair(QUERY,query));

    //extend 2004/01/08
    if (ssmlType == INTERNAL_SERVICES) {
        //extend for internalservice
        writeDataMap.insert(make_pair(SSML_TYPE, SSML_TYPE_INTERNAL));
    }

#ifdef DEBUG
    writeDataMap.spy();
#endif /* DEBUG */
    
    //メッセージ送信
    if (0 > write(sockfd, writeDataMap.toString())) {
		AppLogger::Write(APLOG_ERROR,"%s%s"
			,METHOD_LABEL 
            ,"write to manager failed."
            );
        CloseSocket(sockfd);
        return -1;
    }
    
    string readBuf;
    if (0 > read(sockfd, readBuf)) {
		AppLogger::Write(APLOG_ERROR,"%s%s"
			,METHOD_LABEL 
            ,"read from manager failed."
            );
        CloseSocket(sockfd);
        
        return -1;
    }

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                    ,"read",readBuf.c_str());
#endif /* DEBUG */

    MapUtil readDataMap(ITEM_DELMIT, VAL_DELMIT);
    readDataMap.insertItem(readBuf);

    //結果取り出し
    string result = readDataMap[RESULT];
    if (SUCCESS != result) {
#ifdef DEBUG
        AppLogger::Write(APLOG_ERROR,"%s%s=[%s]",METHOD_LABEL 
            ,"result failure. result"
            ,result.c_str()
            );
#endif /* DEBUG */
        
        CloseSocket(sockfd);
        return -1;
    }

    //解答取り出し
    string answer = readDataMap[ANSWER];
    
    string::size_type begIdx = 0;
    string::size_type endIdx = 0;
    for (;;) {
        begIdx = answer.find_first_not_of(SUB_ITEM_DELMIT, endIdx);
        if (begIdx == string::npos) {
#ifdef DEBUG
            AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"END");
#endif /* DEBUG */
            break;
        }
        endIdx = answer.find_first_of(SUB_ITEM_DELMIT, begIdx);
        if (endIdx == string::npos) {
            endIdx = answer.length();
        }
        string token = answer.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                        ,"token",token.c_str());
#endif /* DEBUG */

        values.push_back(token);
    }

    CloseSocket(sockfd);
    return 0;
}

int SSMLAttrHandler::connectManager()
{
  static char METHOD_LABEL[] = "SSMLAttrHandler::connectManager: ";
  typedef struct sockaddr SockAddr;
  typedef struct sockaddr_un SockAddrAf;

  int af = AF_UNIX;
  int sockfd = int();
  SockAddrAf servAddr;

  memset((char*)&servAddr, 0, sizeof(servAddr));
  servAddr.sun_family = AF_UNIX;
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

  if (0 > (sockfd = socket(af, SOCK_STREAM, 0))) {
	AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
        ,"open socket failed."
        );
    return -1;
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"socket open OK!");
#endif
  

  if (0 > connect(sockfd, (SockAddr*)&servAddr, servAddrLen)) {
	AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
        ,"connect failed."
        );
    return -1;
  }

#ifdef DEBUG
  AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"connect OK!");
#endif

    return sockfd;
}

bool SSMLAttrHandler::getShmCache(const string& query,
                                  vector<string>& values,
                                  const SSMLType ssmlType)
{
  static char METHOD_LABEL[] = "SSMLAttrHandler::getShmCache: ";
    // SEM check
  key_t semKey = ftok(SSMLATTRMGR_SOCKET_ADDR.c_str(), 1);
  
  int semid = semget(semKey, 1, 0666);
  if (-1 == semid) {
    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"semget failed.");
    return false;
  }
  struct sembuf semBuf;
  semBuf.sem_op = 0;
  semBuf.sem_num = 0;
  semBuf.sem_flg = 0;
  if (-1 == semop(semid, &semBuf, 1)) { // if sem val not 0, process blocked.
    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"semop failed.");
    return false;
  }

  const int SHMSZ = 196608;
  char* shm = NULL;

  string shmKeyPath(SSMLATTRMGR_SOCKET_ADDR);
  shmKeyPath += StringUtil::toString(ssmlType);
  key_t shm_key = ftok(shmKeyPath.c_str(), 0);
  int shmid = 0;

  if ((shmid = shmget(shm_key, SHMSZ, IPC_CREAT|0666)) < 0) {
    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"shmget failed.");
    return false;
  }

  if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*)-1) {
    AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL,"shmat failed.");
    return false;
  }

//  memset(shmBuf, 0x00, SHMSZ);
//  strncpy(shmBuf, shm, SHMSZ);
//  string data(shmBuf);
  string data(shm);

  string::size_type begIdx = 0;
  string::size_type endIdx = 0;
  const string delimit = "|";
  for (;;) {
    begIdx = data.find_first_not_of(delimit, endIdx);
    if (begIdx == string::npos) {
#ifdef DEBUG
      AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"END");
#endif /* DEBUG */
      break;
    }
    endIdx = data.find_first_of(delimit, begIdx);
    if (endIdx == string::npos) {
      endIdx = data.length();
    }
    string key = data.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL,"key",key.c_str());
#endif /* DEBUG */

    begIdx = data.find_first_not_of(delimit, endIdx);
    if (begIdx == string::npos) {

#ifdef DEBUG
      AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"END");
#endif /* DEBUG */
      break;
    }
    endIdx = data.find_first_of(delimit, begIdx);
    if (endIdx == string::npos) {
      endIdx = data.length();
    }
    string val = data.substr(begIdx, (endIdx-begIdx));

#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL,"val",val.c_str());
#endif /* DEBUG */
    
    if (key == query) {
      string::size_type begIdx = 0;
      string::size_type endIdx = 0;
      for (;;) {
	begIdx = val.find_first_not_of(SUB_ITEM_DELMIT, endIdx);
	if (begIdx == string::npos) {
#ifdef DEBUG
      AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL,"END");
#endif /* DEBUG */
	  break;
	}
	endIdx = val.find_first_of(SUB_ITEM_DELMIT, begIdx);
	if (endIdx == string::npos) {
	  endIdx = val.length();
	}
	string token = val.substr(begIdx, (endIdx-begIdx));
	
#ifdef DEBUG
    AppLogger::Write(APLOG_DEBUG9,"%s%s=[%s]",METHOD_LABEL
                    ,"token",token.c_str());
#endif /* DEBUG */
	
	values.push_back(token);
      }
      break;
    }
  }

  shmdt(shm);

  return values.size() > 0 ? true : false;
}
