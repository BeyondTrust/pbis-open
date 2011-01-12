#ifndef ProcessInfo_H
#define ProcessInfo_H

#include <pthread.h>
#include <map>
#include <string>
#include "ThreadInfo.h"

#define _StaticClass

#ifdef _StaticClass
	#define STATIC_OPTION static
#else
	#define STATIC_OPTION
#endif

#define PSTAT_NORMAL	0	// normal
#define PSTAT_INIT		1	// initialize
#define PSTAT_WAIT		2	// wait
#define PSTAT_WAITTERM	3	// wait termination
#define PSTAT_HUP		4	// wait termination
#define PSTAT_USR1		5	// wait termination
#define PSTAT_USR2		6	// wait termination

namespace OpenSOAP {

class ProcessInfo {
	private:
		STATIC_OPTION std::string ProcessName;
		STATIC_OPTION std::string HostName;
		STATIC_OPTION pthread_mutex_t MutexFlag;
		STATIC_OPTION pthread_mutex_t UpdateLockFlag;
		STATIC_OPTION int LockCount;
		STATIC_OPTION std::map<pthread_t,ThreadInfo *> ThreadInfoMap;
		STATIC_OPTION int ProcessStatus;
		STATIC_OPTION int ThreadCount;
		

	private:
		STATIC_OPTION ThreadInfo * AddThreadInfo(pthread_t tid);

	public:
		ProcessInfo();
		virtual ~ProcessInfo();

		//スレッドリストにスレッド情報を追加する。
		STATIC_OPTION ThreadInfo * AddThreadInfo();
		//スレッドリストからスレッド情報を取得する。
		STATIC_OPTION ThreadInfo * GetThreadInfo();	
		STATIC_OPTION ThreadInfo * GetThreadInfo(pthread_t tid);
		//スレッドリストからスレッド情報を削除する。
		STATIC_OPTION void DelThreadInfo();	
		STATIC_OPTION void DelThreadInfo(pthread_t tid);
		//現在のスレッドリストを表示する。
		STATIC_OPTION void PrintProcessInfo();	
		STATIC_OPTION void PrintThreadInfo();	
		//現在のスレッドリストをチェックする。
		STATIC_OPTION void CheckThreadInfo();	//pthread_check&delete item
		STATIC_OPTION void CheckThreadInfo(pthread_t tid);
		//プロセス名の設定/取得
		STATIC_OPTION void SetProcessName(char * str);
		STATIC_OPTION std::string GetProcessName();
		//ホスト名を設定/取得
		STATIC_OPTION void SetHostName(const char * str);
		STATIC_OPTION std::string GetHostName();
		//スレッド数を取得
		STATIC_OPTION int GetThreadCount();
		//スレッド状態の設定/取得 
		// PSTAT_NORMAL/PSTAT_INIT/PSTAT_WAIT/PSTAT_WAITERM
		STATIC_OPTION void SetProcessStatus(int pstat);
		STATIC_OPTION int GetProcessStatus();
		//スレッドリストのデータ更新用ロック）
		STATIC_OPTION void Lock();	
		STATIC_OPTION void UnLock();	
		//参照用ロック（書き込み(追加/削除)を不可にする）
		STATIC_OPTION void UpdateLock();	
		STATIC_OPTION void UpdateUnLock();	

}; //class ProcessInfo

} //namespace OpenSOAP

#endif //ProcessInfo_H

