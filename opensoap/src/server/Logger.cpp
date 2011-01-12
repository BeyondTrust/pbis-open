/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: Logger.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>	//getpid
#include <stdarg.h>		//vsnprintf,vfprintf
#include <pthread.h>	//pthread_self
#include <time.h>		//time
#include <unistd.h>		//getpid


#include "Logger.h"
#include "Exception.h"
#include "ProcessInfo.h"

using namespace std;
using namespace OpenSOAP;

static const char LOG_LEVEL_UNKNOWN[] 	= "debug";

typedef struct _logcode {
  char *name;
  int   val;
} LOGCODE;

LOGCODE logcodes[] =
{
  { "alert", LOG_ALERT },
  { "crit", LOG_CRIT },
  { "debug", LOG_DEBUG },
  { "emerg", LOG_EMERG },
  { "err", LOG_ERR },
  { "error", LOG_ERR },
  { "info", LOG_INFO },
  { "notice", LOG_NOTICE },
  { "panic", LOG_EMERG },
  { "warn", LOG_WARNING },
  { "warning", LOG_WARNING },
  { NULL, -1 }
};


//const std::string Logger::
static const std::string CLASS_SIG = "Logger::";

/*============================================================================
Log CLASS 
=============================================================================*/
Logger::Logger(){
	//初期化処理
	OutputLevel=0x7FFFFFFF;
	//初期化処理呼び出し
	//COMMENT:server.conf読み直しを考慮したため、別関数とした。
	Initialize();
}

Logger::~Logger(){
}

void Logger::Initialize(){
	//initialize parameter
	return ;
}

void Logger::SetLogFormatType(const string& ftype){
	LogFormatType=ftype;
}
void Logger::SetLogFormatType(const char * ftype){
	LogFormatType=ftype;
}
string& Logger::GetLogFormatType(){
	return LogFormatType;
}

void Logger::SetOutputLevel(int level){
	OutputLevel=level;
}
int Logger::GetOutputLevel(){
	return OutputLevel;
}

void Logger::Lock(){
//	pthread_mutex_lock(&MutexFlag);
}

void Logger::UnLock(){
//	pthread_mutex_unlock(&MutexFlag);
}

void Logger::FFlush(){
	fflush(stderr);
}

const char * Logger::MsgLevel2Str(int level){
	int i;
	for (i=0;logcodes[i].val!=-1;i++){
		if (logcodes[i].val==level) {
	    		return logcodes[i].name;
		}
	}
	return LOG_LEVEL_UNKNOWN;
}

//インフォメーション用
int Logger::Write(std::string str){
	return Write(LOG_INFO,str.c_str());		//->Write(const char * fmt,...)
}

int Logger::Write(const char * fmt,...){
	int size;
	va_list args;
	
	va_start(args,fmt);
	try{
		size=Write(LOG_INFO,fmt,args);	//->Write(const char * fmt,va_list args)
	}
	catch(Exception e){
		va_end(args);
		e.AddFuncCallStack();
		throw e;
	}
	catch(...){
		va_end(args);
		throw Exception(-1,OPENSOAPSERVER_UNKNOWN_ERROR
						,LOG_ERR,__FILE__,__LINE__);
	}
	va_end(args);
	
	return size;
}

int Logger::Write(const char * fmt,va_list args){
	return Write(LOG_INFO,fmt,args);
}

int Logger::Write(int level,std::string str){
	return Write(level,str.c_str());
}

int Logger::Write(int level,const char * fmt,va_list args){
	time_t		t;	//=time(NULL);<-未使用
	pid_t		pid=getpid();
	pthread_t	tid=pthread_self();

	return Write(level,t,ProcessInfo::GetHostName().c_str()
				,ProcessInfo::GetProcessName().c_str(),pid,tid,fmt,args);
}

int Logger::Write(int level,const char * fmt,...){
	int size;
	va_list args;
	
	va_start(args,fmt);
	try{
		size=Write(level,fmt,args);	//->Write(const char * fmt,va_list args)
	}
	catch(Exception e){
		va_end(args);
		e.AddFuncCallStack();
		throw e;
	}
	catch(...){
		va_end(args);
		throw Exception(-1,OPENSOAPSERVER_UNKNOWN_ERROR
						,LOG_ERR,__FILE__,__LINE__);
	}
	va_end(args);
	
	return size;
}

int Logger::Write(int level
		,time_t t
		,const char * hostname
		,const char * processname
		,pid_t pid
		,pthread_t tid
		,const char * fmt
		,va_list args
		){
	char buff[128];
	string wfmt;
	struct tm tm;
	int size=0;
	int len=0;

	if(fmt==NULL){
		throw Exception(-1,OPENSOAP_PARAMETER_BADVALUE
						,LOG_ERR,__FILE__,__LINE__);
	}

	struct timeval tv;
	gettimeofday(&tv,NULL);
	//initialize
	tm=*localtime((time_t *)(&tv.tv_sec));
	try{
		size=sprintf(buff,"%04d/%02d/%02d,%02d:%02d:%02d"				//19byte
				,tm.tm_year+1900
				,tm.tm_mon+1
				,tm.tm_mday
				,tm.tm_hour
				,tm.tm_min
				,tm.tm_sec
				);
		size+=sprintf(buff+size,".%06ld",tv.tv_usec);					// 7byte
		len=snprintf(buff+size,22,",%s",hostname);						//21byte
		size+=(len>=22 || len< 0)? 21 : len;
		len=snprintf(buff+size,22,",%s",processname);					//21byte
		size+=(len>=22 || len< 0)? 21 : len;
		size+=sprintf(buff+size,",%05ld",pid);							// 6byte
#ifdef WIN32
		size+=sprintf(buff+size,",%10ld",tid);							//11byte
#endif
		size+=sprintf(buff+size,",%s,",Logger::MsgLevel2Str(level));	//10byte
		wfmt=buff;
		wfmt+=fmt;
		wfmt+="\n";
		size+=RawWrite(level,wfmt.c_str(),args);
	}
	catch(Exception e){
		e.AddFuncCallStack();
		throw e;
	}
	catch(...){
		throw Exception(-1,OPENSOAPSERVER_UNKNOWN_ERROR
						,LOG_ERR,__FILE__,__LINE__);
	}
	return size;
}

//直接指定した文字を書き込む場合用
int Logger::RawWrite(int level,const char * fmt,va_list args){
	int wsize=0;
	int size=0;

	if (fmt!=NULL) {
		size=vsnprintf(NULL,0,fmt,args);
		if (level > OutputLevel){
			return size;
		}
		if ((wsize=vfprintf(stderr,fmt,args))!=size){
			//write error
			Exception e(-1,OPENSOAP_IO_ERROR,LOG_ERR,__FILE__,__LINE__);
			e.SetErrText("stderr output error");
			throw e;
		}
	}
	return size;
}

int Logger::RawWrite(int level,const char * fmt,...){
	int size=0;
	va_list args;

	va_start(args,fmt);
	try{
		size=RawWrite(level,fmt,args);
	}
	catch(Exception e){
		va_end(args);
		e.AddFuncCallStack();
		throw e;
	}
	catch(...){
		va_end(args);
		throw Exception(-1,OPENSOAPSERVER_UNKNOWN_ERROR
						,LOG_ERR,__FILE__,__LINE__);
	}
	va_end(args);

	return size;
}

// End of Logger.cpp
