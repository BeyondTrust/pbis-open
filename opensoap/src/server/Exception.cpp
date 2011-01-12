#include <stdio.h>
#include <errno.h>
#include <string>
#include <pthread.h>
#include <string.h>
#include <time.h>	//time()
#include <sys/types.h>	//getpid()
#include <unistd.h>		//getpid()
#include <stdarg.h>		//vsnprintf

#include "Exception.h"
#include "StringUtil.h"
#include "ErrorUtil.h"

#undef AddFuncCallStack

extern int errno;

using namespace std;
using namespace OpenSOAP;

Exception::Exception() {
	Initialize();
}

Exception::Exception(int errcode 
					,int detailcode
					,int level
					) {
	Initialize();
	SetExceptionData(errcode,detailcode,level,NULL,0);
}
Exception::Exception(int errcode 
					,int detailcode
					,int level
					,const char * modname
					,int id) {
	Initialize();
	SetExceptionData(errcode,detailcode,level,modname,id);
}
Exception::Exception(int errcode 
					,int detailcode
					,int level
					,const string& modname
					,int id) {
	Initialize();
	SetExceptionData(errcode,detailcode,level,modname.c_str(),id);
}

Exception::Exception(const Exception& e) {
	tm=e.tm;
	pid=e.pid;
	tid=e.tid;
	szFuncStack=e.szFuncStack;
	szMsg=e.szMsg;
	nErrCode=e.nErrCode;
	nDetailCode=e.nDetailCode;
	nSysErrNo=e.nSysErrNo;
	nErrLevel=e.nErrLevel;
}

Exception::~Exception() {
}
void Exception::Initialize(){
	std::time(&tm);
	pid=getpid();
	tid=pthread_self();
	szFuncStack="";
	nErrCode=0;
	nDetailCode=0;
	nSysErrNo=errno;
	nErrLevel=LOG_ERR;
	errno=0;
	szMsg="";
}

void Exception::SetExceptionData(int errcode 
								,int detailcode
								,int level
								,const char * modname
								,int id
								){
    if (modname!=NULL) {
		szFuncStack+=modname;
		szFuncStack += "(";
		szFuncStack += StringUtil::toString(id) ;
		szFuncStack += ")";
	}
	else {
		szFuncStack.empty();
	}
	nErrCode=errcode;
	nDetailCode=detailcode;
	nErrLevel=level;
}

void Exception::SetExceptionData(int errcode 
								,int detailcode
								,int level
								,const string& modname
								,int id
								){
	SetExceptionData(errcode,detailcode,level,modname.c_str(),id);
}

void Exception::AddFuncCallStack(const std::string& str){
	AddFuncCallStack(str.c_str());
}
void Exception::AddFuncCallStack(const char * str){
	if(szFuncStack.length()>0){
		szFuncStack += "<-";
	}
	szFuncStack += str;
}
void Exception::AddFuncCallStack(const std::string& modname,const int id){
	AddFuncCallStack(modname.c_str(),id);
}
void Exception::AddFuncCallStack(const char * modname,const int id){
	if(szFuncStack.length()>0){
		szFuncStack += "<-";
	}
	szFuncStack += modname ;
	szFuncStack += "(";
	szFuncStack += StringUtil::toString(id) ;
	szFuncStack += ")";
}
string& Exception::GetFuncCallStack(){
	return szFuncStack;
}
void Exception::SetErrNo(int n){
	nErrCode=n;
}
int Exception::GetErrNo(){
	return nErrCode;
}
void Exception::SetErrText(const char * fmt,...){
	char buff[1024];
	va_list args;

	va_start(args,fmt);
	vsnprintf(buff,sizeof(buff)-1,fmt,args);
	szMsg=buff;
	va_end(args);
}

void Exception::SetErrText(const std::string& str){
	szMsg=str;
}
string& Exception::GetErrText(){
	return szMsg;
}
void Exception::SetDetailErrNo(int n){
	nDetailCode=n;
}
int Exception::GetDetailErrNo(){
	return nDetailCode;
}
string Exception::GetDetailErrText(){
	string str=ErrorUtil::toString(nDetailCode);
	return str;
}
void Exception::SetSysErrNo(int n){
	nSysErrNo=n;
}
int Exception::GetSysErrNo(){
	return nSysErrNo;
}
string Exception::GetSysErrText(){
	string str=strerror(nSysErrNo);
	return str;
}
void Exception::SetErrLevel(int n){
	nErrLevel=n;
}
int Exception::GetErrLevel(){
	return nErrLevel;
}

