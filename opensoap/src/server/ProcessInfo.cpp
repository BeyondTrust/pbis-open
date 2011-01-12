#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#include "StringUtil.h"
#include "ProcessInfo.h"
#include "AppLogger.h"

using namespace std;
using namespace OpenSOAP;

typedef struct _pstat_list {
	int id;
	char * str;
}PSTAT_LIST;

PSTAT_LIST pstat_list[]={
	{PSTAT_NORMAL,"normal"},
	{PSTAT_INIT,"initialize"},
	{PSTAT_WAIT,"wait"},
	{PSTAT_WAITTERM,"wait termination"},
	{PSTAT_HUP,"catch sighup"},
	{PSTAT_USR1,"catch sigusr1"},
	{PSTAT_USR2,"catch sigusr2"},
	{-1,NULL}
};
	
//
#ifdef _StaticClass
	pthread_mutex_t ProcessInfo::MutexFlag=PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t ProcessInfo::UpdateLockFlag=PTHREAD_MUTEX_INITIALIZER;
	std::map<pthread_t,ThreadInfo *> ProcessInfo::ThreadInfoMap;
	std::string ProcessInfo::ProcessName;
	std::string ProcessInfo::HostName;
	int	ProcessInfo::LockCount=0;
	int	ProcessInfo::ThreadCount=0;
	int	ProcessInfo::ProcessStatus=PSTAT_NORMAL;
#else
	ProcessInfo GProcessInfo;
#endif

//リスト初期化新規作成処理
ProcessInfo::ProcessInfo(){
	LockCount=0;
	ThreadCount=0;
	ProcessStatus=PSTAT_NORMAL;
}

//リスト初期化削除処理
//処理：
//　リストに結合されているデータをすべて削除する。
//　AutoDeleteが1の場合はrootも削除する。
ProcessInfo::~ProcessInfo(){
}

//リストに自スレッドの情報を追加する。
//既に追加されている場合は既にある情報へのポインタが返される。
ThreadInfo * ProcessInfo::AddThreadInfo(){
	return AddThreadInfo(pthread_self());
}

ThreadInfo * ProcessInfo::AddThreadInfo(pthread_t tid){
	ThreadInfo * ti=NULL;
	map<pthread_t,ThreadInfo *>::iterator tis;
	
	if (ProcessStatus==PSTAT_WAITTERM) {
		return NULL;
	}
	tis=ThreadInfoMap.find(tid);
	if ( tis!=ThreadInfoMap.end()){
		ti=tis->second ;
	}
	else {
		ti=new ThreadInfo();
		if (!ti) {	return (NULL);	}
		
		Lock();
		ThreadInfoMap.insert(pair<pthread_t,ThreadInfo *>(tid,ti));
		ThreadCount++;
		UnLock();
	}
	return ti;
}

ThreadInfo * ProcessInfo::GetThreadInfo(){
	return ProcessInfo::GetThreadInfo(pthread_self());
}

ThreadInfo * ProcessInfo::GetThreadInfo(pthread_t tid){
	map<pthread_t,ThreadInfo *>::iterator tis;
	ThreadInfo * ti=NULL;
	
	tis=ThreadInfoMap.find(tid);
	if ( tis!=ThreadInfoMap.end()){
		ti=tis->second ;
	}
	return ti;
}
void ProcessInfo::DelThreadInfo(){
	ProcessInfo::DelThreadInfo(pthread_self());
}
void ProcessInfo::DelThreadInfo(pthread_t tid){
	Lock();
	ThreadInfo * ti=ProcessInfo::GetThreadInfo(tid);
	if (ti) {
		delete ti;
	}
	ThreadInfoMap.erase(tid);
	ThreadCount--;
	UnLock();
}
void ProcessInfo::PrintProcessInfo(){
	time_t t;
	struct tm tm;
	time(&t);
	tm=*(localtime(&t));
	
	fprintf(stderr
			,"ProcessInfo Printing-------------------------------------\n");
	fprintf(stderr," OutputTime     =[%04d/%02d/%02d %02d:%02d:%02d]\n"
			,tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday
			,tm.tm_hour,tm.tm_min,tm.tm_sec
			);
	fprintf(stderr," ProcessName    =[%s]\n",ProcessName.c_str());
	fprintf(stderr," HostName       =[%s]\n",HostName.c_str());
	fprintf(stderr," ThreadCount    =[%d]\n",ThreadCount-1);
	fprintf(stderr," ProcessStatus\t=[%s]\n"
			,pstat_list[GetProcessStatus()].str);
}
void ProcessInfo::PrintThreadInfo(){
	ThreadInfo * ti=NULL;
	map<pthread_t,ThreadInfo *>::iterator tis;
	int cnt=1;
	struct timeval * tv;
	struct tm tm;	
	// root以外を削除
	fprintf(stderr
			,"ThreadInfo Printing--------------------------------------\n");
	fprintf(stderr,"%s %s %s %s %s %s %s\n"
			,"   No","   pid","     tid"
			,"  day","    time","    usec"
			,"id"
			);
	UpdateLock();
	for (tis=ThreadInfoMap.begin() ; tis!=ThreadInfoMap.end() ; tis++){
		ti=tis->second;
		tv=ti->GetCreateTime();
		tm=*(localtime((const time_t *)&(tv->tv_sec)));
		fprintf(stderr,"%5d %6d %8ld %02d/%02d %02d:%02d:%02d %8ld %s\n"
			,cnt,ti->GetPID(),ti->GetTID()
			,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec
			,tv->tv_usec
			,ti->GetMsgInfo()->GetRequestID().c_str()
			);
		cnt++;
	}
	UpdateUnLock();
}

void ProcessInfo::CheckThreadInfo(){
	map<pthread_t,ThreadInfo *>::iterator tis;
	for (tis=ThreadInfoMap.begin() ; tis!=ThreadInfoMap.end() ; tis++){
		CheckThreadInfo((tis->second)->GetTID());
	}
}

void ProcessInfo::CheckThreadInfo(pthread_t tid){
#if defined(linux) || defined(__linux__)
	struct stat fstatus;
	map<pthread_t,ThreadInfo *>::iterator tis;
	string filename="/proc/";
	string szPID;
	
	tis=ThreadInfoMap.find(tid);
	szPID=StringUtil::toString((tis->second)->GetPID());
	filename += szPID;
	if (stat(filename.c_str(),&fstatus)){
		AppLogger::Write(LOG_WARNING
						,"Thread not found[%d:%d].Thread info deleted."
						,(tis->second)->GetPID(),(tis->second)->GetTID());
		DelThreadInfo(tid);
	}
#endif
}

void ProcessInfo::SetProcessName(char * str){
	ProcessName=basename((char *)str);
}
std::string ProcessInfo::GetProcessName(){
	return ProcessName;
}
void ProcessInfo::SetHostName(const char * str){
	if (str) {
		HostName=str;
	} else {
		char hoststr[256];
		gethostname(hoststr,sizeof(hoststr));
		HostName=hoststr;
	}
}
std::string ProcessInfo::GetHostName(){
	return HostName;
}
int ProcessInfo::GetThreadCount(){
	return ThreadCount;
}
void ProcessInfo::SetProcessStatus(int pstat){
	ProcessStatus=pstat;
}
int ProcessInfo::GetProcessStatus(){
	return ProcessStatus;
}

void ProcessInfo::Lock(){
	pthread_mutex_lock(&MutexFlag);
}
void ProcessInfo::UnLock(){
	pthread_mutex_unlock(&MutexFlag);
}

void ProcessInfo::UpdateLock(){
	pthread_mutex_lock(&UpdateLockFlag);
	if (!LockCount) {
		Lock();
	}
	LockCount++;
	pthread_mutex_unlock(&UpdateLockFlag);
}
void ProcessInfo::UpdateUnLock(){
	pthread_mutex_lock(&UpdateLockFlag);
	LockCount--;
	if (!LockCount) {
		UnLock();
	}
	pthread_mutex_unlock(&UpdateLockFlag);
}
