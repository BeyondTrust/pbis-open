#include <stdio.h>
#include <string.h>
#include <sys/types.h>	//getpid()
#include <unistd.h>		//getpid()
#include <sys/time.h>	//gettimeofday
#include "ThreadInfo.h"
#include "MsgInfo.h"

//using namespace
#include "ServerDef.h"

using namespace std;
using namespace OpenSOAP;

ThreadInfo::ThreadInfo() {
	int i;

	// 初期化
	ProcessID=0;
	ThreadID=0;

	// Process&Thread ID設定
	for (i=0;i<3 && !ProcessID ;i++) {
		ProcessID=getpid();
	}
	for (i=0;i<3 && !ThreadID ;i++) {
		ThreadID=pthread_self();
	}
	gettimeofday(&CreateTime,NULL);
	msginfo=new MsgInfo();
	ThreadDataPtr=this;
}

ThreadInfo::ThreadInfo(ThreadInfo& ti){
	ProcessID=ti.ProcessID;
	ThreadID=ti.ThreadID;
	MsgInfo * msg=ti.msginfo;
	msginfo=new MsgInfo();
	*msginfo=*msg;
	ThreadDataPtr=this;
}

ThreadInfo::~ThreadInfo(){
	if (msginfo) delete msginfo;
	msginfo=NULL;
}

MsgInfo * ThreadInfo::GetMsgInfo(){
    return ThreadDataPtr->msginfo;
}

pid_t ThreadInfo::GetPID(){
	return ThreadDataPtr->ProcessID;
}

pthread_t ThreadInfo::GetTID(){
	return ThreadDataPtr->ThreadID;
}

int ThreadInfo::SetMsgInfo(MsgInfo * mi){
	if (!mi) return -1;
	if (ThreadDataPtr->msginfo) delete ThreadDataPtr->msginfo;
	ThreadDataPtr->msginfo=mi;
	return 0;
}

struct timeval * ThreadInfo::GetCreateTime(){
	return &(ThreadDataPtr->CreateTime);
}
