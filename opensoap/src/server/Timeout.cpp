/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Timeout.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <iostream>
#include <stdexcept>

#if defined(WIN32)
#include <process.h>
#include "StringUtil.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

#include "Timeout.h"
#include "ServerCommon.h"
#include "AppLogger.h"

using namespace OpenSOAP;
using namespace std;

#if defined(WIN32)
int Timeout::mtCount_ = 0;
#endif

//===========================================
// コンストラクタ
//-------------------------------------------
// 引数: タイムアウト時間(秒) 
//===========================================
Timeout::Timeout(int timeout_second)
  : timeoutSecond_(timeout_second)
    , targetThread_(0)
{
  initMutex();
}

//===========================================
// コンストラクタ
//-------------------------------------------
// 引数: タイムアウト時間(秒) 
// 引数: タイマー監視対象スレッドID
//===========================================
Timeout::Timeout(int timeout_second, 
#if defined(WIN32)
				HANDLE
#else
				pthread_t
#endif
					thr_id)
  : timeoutSecond_(timeout_second)
    , targetThread_(thr_id)
{
  initMutex();
}

//===========================================
// デストラクタ
//===========================================
Timeout::~Timeout()
{
  destroyMutex();
  //added 2003/02/03
#if !defined(WIN32)
  pthread_detach(targetThread_);
#endif
}

//===========================================
// mutex初期化処理
//===========================================
void Timeout::initMutex()
{
#if defined(WIN32)
	if (mtCount_ > 10000) {
		mtCount_ = 0;
	}
	std::string mtId = "TIMEOUTMTX" + StringUtil::toString(mtCount_++);
	isRunning_mutex_ = CreateMutex(NULL, FALSE, mtId.c_str());
#else
  int status = pthread_mutex_init(&isRunning_mutex_, NULL);
  if (0 != status) {
    //throw OpenSOAPException();
    throw std::exception();
  }
#endif
}

//===========================================
// mutex破棄処理
//===========================================
void Timeout::destroyMutex()
{
#if defined(WIN32)
	CloseHandle(isRunning_mutex_);
#else
  int status = pthread_mutex_init(&isRunning_mutex_, NULL);
  if (0 != status) {
    //throw OpenSOAPException();
    throw std::exception();
  }
#endif
}

//===========================================
// タイムアウト対象スレッド指定
//-------------------------------------------
// 引数: タイマー監視対象スレッドID
//===========================================
void Timeout::setTargetThread(
#if defined(WIN32)
							HANDLE
#else
							pthread_t
#endif
								thr_t)
{
  targetThread_ = thr_t;
}

//===========================================
// タイムアウト時間指定(秒)
//-------------------------------------------
// 引数: タイムアウト時間(秒) 
//===========================================
void Timeout::setTimeout(int timeout_second)
{
  timeoutSecond_ = timeout_second;
}


//===========================================
// タイマー処理
//-------------------------------------------
// 引数: Timeoutインスタンスのポインタ
//===========================================
ThrFuncReturnType
Timeout::runTimeout(ThrFuncArgType arg)
{
    static char METHOD_LABEL[] = "Timeout::runTimeout: ";

#ifdef DEBUG
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                    ,"Timeout::runTimeout() called");
#endif
    
    //インスタンス情報取得
    // ※スタティックメソッドのため,引数によりインスタンスを受取り
    //   これを介してインスタンスメンバにアクセスする
    Timeout* that = (Timeout*)arg;
    if (NULL == that) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"invalid argument...");
    }

    //指定時間休止
#if defined(WIN32)
    Sleep(that->timeoutSecond_*1000);
#else
    sleep(that->timeoutSecond_);
#endif

    //-------------------------------
    //ここから以降はタイムアウト処理
    //-------------------------------
    //  タイムアウト前に処理完了の場合は上記sleep()中に
    //  finish()内で本スレッドが中止される
    
    // lock mutex
    Thread::lockMutex(that->isRunning_mutex_);

    //実行状態チェック
    if (that->isRunning_) {
#ifdef DEBUG
        //debug
        AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                        ,"### pthread cancel call!! ###");
#endif //DEBUG
        
        //timeout proc.

#if defined(WIN32)
	if (0 == TerminateThread(that->targetThread_, 0)) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"thread cancel error...");
	}
	else {
#ifdef DEBUG
        AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                        ,"Timeout:: thread canceled.");
#endif //DEBUG
	    that->targetThreadIsCanceled_ = true;
	}
#else //defined(WIN32)
        int status = pthread_cancel(that->targetThread_);
        if (0 != status) {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                            ,"pthread cancel error...");
        }
        else {
            that->targetThreadIsCanceled_ = true;
        }
        //added 2003/01/31
        //pthread_detach(that->targetThread_);

#endif //defined(WIN32)/else
        
        that->isRunning_ = false;
    }

#ifdef DEBUG
    // for debug
    else {
        AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                ,"## Timeout::runTimeout : not running. may be finished ##");
    }
#endif
//fprintf(stderr,"[%s:%s:%d][%ld]\n",__FILE__,__func__,__LINE__,AppLogger::GetLockThread());

    //release mutex
    Thread::unlockMutex(that->isRunning_mutex_);
    
#ifdef DEBUG
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                    ,"Timeout::runTimeout() return");
#endif
    
    ReturnThread(NULL);
}

//===========================================
//タイムアウト監視開始
//-------------------------------------------
// 返り値： ステータス
//          ０:     正常
//          ０以外: エラー
//===========================================
int 
Timeout::start()
{
    static char METHOD_LABEL[] = "Timeout::start: ";

#ifdef DEBUG
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                    ,"Timeout::start() called");
#endif

    //処理対象スレッド設定チェック
    if (0 == targetThread_) {
        AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                        ,"target thread not set");
        return -1;
    }
    
    // lock mutex
    Thread::lockMutex(isRunning_mutex_);

    // フラグ設定
    isRunning_ = true;
    targetThreadIsCanceled_ = false;
  
    //タイマースレッド開始
#if defined(WIN32)
    timerThr_id_ = (HANDLE)_beginthread(runTimeout, 0, this);
#else
    int status = pthread_create(&timerThr_id_, 
                                NULL,
                                runTimeout, // static method
                                this);
#endif

    // release mutex
    Thread::unlockMutex(isRunning_mutex_);

#ifdef DEBUG
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                    ,"Timeout::start() return");
#endif

    return 0;
}

//===========================================
//タイムアウト監視終了
//-------------------------------------------
// 返り値： ステータス
//          ０:     正常
//          ０以外: エラー
//===========================================
int 
Timeout::finish()
{
    static char METHOD_LABEL[] = "Timeout::finish: ";

#ifdef DEBUG
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                    ,"called");
#endif

    // lock mutex
    Thread::lockMutex(isRunning_mutex_);

    // isRunning_ のチェックを行なってからにするか???
    if (isRunning_) {
        // タイムアウト前に処理終了なので,
        // タイマー処理スレッドを中止する
#if defined(WIN32)
	if ( 0 == TerminateThread(timerThr_id_, 0)) {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                            ,"thread cancel error");
        }
#else
        int status = pthread_cancel(timerThr_id_);
        if (0 != status) {
            AppLogger::Write(APLOG_ERROR,"%s%s",METHOD_LABEL
                            ,"pthread cancel error");
        }
#ifdef DEBUG
        else {
            AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                            ,"thread cancel OK!!");
        }
#endif
        //added 2003/01/31
        pthread_detach(timerThr_id_);
#endif
    
        // フラグ解除
        isRunning_ = false;
    }
#ifdef DEBUG
    else {
        AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                        ,"not Running!!");
    }
#endif

    // release mutex
    Thread::unlockMutex(isRunning_mutex_);

#ifdef DEBUG
    //debug
    AppLogger::Write(APLOG_DEBUG9,"%s%s",METHOD_LABEL
                    ,"return");
#endif
  
  return 0;
}
 
// ターゲットスレッドがタイムアウトでキャンセルされたか否か？
// 
// true:  キャンセルされている。
// false: キャンセルされていない。
// 
bool
Timeout::targetThreadIsCanceled()
{
  return targetThreadIsCanceled_;
}
//--- End of Timeout.cpp ---
