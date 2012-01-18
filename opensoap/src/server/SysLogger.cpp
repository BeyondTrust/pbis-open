#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <syslog.h>    //syslog

#include "SysLogger.h"
#include "Exception.h"

using namespace std;
using namespace OpenSOAP;

static const std::string CLASS_SIG = "SysLogger::";

//initialize 

/*============================================================================
SysLogger CLASS 
=============================================================================*/
SysLogger::SysLogger(){
	//set default syslog-option 
	SyslogOption=LOG_ODELAY|LOG_PID;
	//set default syslog-facility 
	SyslogFacility=LOG_USER;
	//set default syslog-defautl-log-level
	SyslogLevel=LOG_ERR;
	//set default output-log-level
	OutputLevel=LOG_INFO;
	Initialize();
	openlog(NULL,SyslogOption,SyslogFacility);
}

SysLogger::~SysLogger(){
	closelog();
}

void SysLogger::Initialize(){
	//½é´ü²½½èÍý
//	openlog(NULL,SyslogOption,SyslogFacility);
	return ;
}

void SysLogger::SetSyslogOption(int option){
	SyslogOption=option;
}
void SysLogger::SetSyslogFacility(int facility){
	SyslogFacility=facility;
}
void SysLogger::SetSyslogLevel(int level){
	SyslogLevel=level;
}
int SysLogger::GetSyslogOption(){
	return SyslogOption;
}
int SysLogger::GetSyslogFacility(){
	return SyslogFacility;
}
int SysLogger::GetSyslogLevel(){
	return SyslogLevel;
}

void SysLogger::Open(){
//	openlog(NULL,SyslogOption,SyslogFacility);
}
void SysLogger::Close(){
//	closelog();
}

//Ìá¤êÃÍ:write size
int SysLogger::Write(string str){
	return Write(LOG_INFO,str.c_str());
}
//Ìá¤êÃÍ:write size
int SysLogger::Write(const char * fmt,va_list args){
	return Write(LOG_INFO,fmt,args);
}

//Ìá¤êÃÍ:write size
int SysLogger::Write(const char * fmt,...){
	int size=0;
	va_list args;
	va_start(args, fmt);
	try{
		size=Write(LOG_INFO,fmt,args);
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
//Ìá¤êÃÍ:write size
int SysLogger::Write(int level,std::string str){
	return Write(level,str.c_str());
}
//Ìá¤êÃÍ:write size
int SysLogger::Write(int level,const char * fmt,va_list args){
	int size=0;
	string wfmt;
	

	if (fmt!=NULL) {
		wfmt=Logger::MsgLevel2Str(level);
		wfmt+=',';
		wfmt+=fmt;
		size=vsnprintf(NULL,0,wfmt.c_str(),args);
		if (level > OutputLevel){
			return size;
		}
		vsyslog(SyslogFacility|level,wfmt.c_str(),args);
	}
	return size;

}

//Ìá¤êÃÍ:write size
int SysLogger::Write(int level,const char * fmt,...){
	int size;
	va_list args;
	va_start(args, fmt);
	try{
		size=Write(level,fmt,args);
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

//Ìá¤êÃÍ:write size
int SysLogger::Write(int level,time_t t,const char * hostname
			,const char * processname
			,pid_t pid,pthread_t tid
			,const char * fmt,va_list args){
	return Write(level,fmt,args);
}


//Ìá¤êÃÍ:write size
int SysLogger::RawWrite(int level,const char * fmt,va_list args){
	return Write(level,fmt,args);
}

//Ìá¤êÃÍ:write size
int SysLogger::RawWrite(int level,const char * fmt,...){
	int size;
	va_list args;
	va_start(args, fmt);
	try{
		size=Write(level,fmt,args);
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

