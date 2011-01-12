/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: SrvConfAttrMgr.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef SrvConfAttrMgr_H
#define SrvConfAttrMgr_H

#include <string>
#if defined(WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <list>
#include <libxml/tree.h>

#include "SrvConf.h"
#include "ThreadDef.h"

namespace OpenSOAP {

  class SrvConfAttrMgr : public SrvConf {

  public:
    SrvConfAttrMgr();
    virtual ~SrvConfAttrMgr();

    //通信開始
    int run();

  private:
    //データのリロード中のロック
      ThrMutexHandle loadXmlLock_;
    //pthread_cond_t  ;

#if defined(WIN32)
    int
#else
    //UNIXドメインソケットアドレス
    std::string 
#endif
    socketAddr_;

    //読み込み対象XMLファイルディレクトリ
    //std::string xmlDir_;

    //DOMデータのリスト
    //std::list<xmlDocPtr> xmlList_;

    //XMLファイルの読み込み
    int reloadXml();

    //xmlList_中のDOM構造開放
    //void freeXmlList();

    //通信処理
    static ThrFuncReturnType connectionThread(ThrFuncArgType arg);
    void connectProc(int sockfd);

    //for shm buffer
#if defined(WIN32)
    HANDLE shmMap;
    int createShm();
    void deleteShm();
#endif
    void clearShm();
    void addShm(const std::string& query, const std::string& value);
#if defined(WIN32)
    HANDLE
#else
    int 
#endif
    semid_; // SEM ID

    //  public: // for test
    std::string queryXml(const std::string& queryStr);

    typedef struct ThrDataTag {
      SrvConfAttrMgr* that;
      int sockfd;
    } ThrData;
  };

} // end of namespace OpenSOAP


#endif /* SrvConfAttrMgr_H */
