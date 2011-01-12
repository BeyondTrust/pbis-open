#ifndef Exception_H
#define Exception_H

#include <stdio.h>
#include <string>
#include <exception>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>		//vsnprintf

#include "SrvErrorCode.h"

//using namespace std;

namespace OpenSOAP {

class Exception {
	public:
		time_t	tm;
		pid_t	pid;
		pthread_t	tid;
		std::string	szFuncStack;
		std::string szMsg;
		int		nErrCode;
		int		nDetailCode;
		int		nSysErrNo;
		int		nErrLevel;
		
	public:
		Exception() ;//throw(); }
		Exception(const Exception& e) ;
		Exception(int errcode ,int detailcode,int level);
		Exception(int errcode 
				,int detailcode
				,int level
				,const std::string& modname
				,int id
				);
		Exception(int errcode 
				,int detailcode
				,int level
				,const char * modname
				,int id
				);
//		Exception(const Exception&) throw();
//		Exception& operator=(const Exception&);//throw();
		~Exception();

		void Initialize();
		void SetExceptionData(	 int errcode 
								,int detailcode
								,int level
								,const std::string& funcname
								,int id
								);
		void SetExceptionData(	 int errcode 
								,int detailcode
								,int level
								,const char * funcname
								,int id
								);
		void AddFuncCallStack(const std::string& str);
		void AddFuncCallStack(const char * str);
		void AddFuncCallStack(const std::string& modname,const int id);
		void AddFuncCallStack(const char * modname,const int id);
		std::string& GetFuncCallStack();

		void SetErrNo(int n);
		int GetErrNo();

		void SetErrText(const char * fmt,...);
		void SetErrText(const std::string& str);
		std::string& GetErrText();

		void SetDetailErrNo(int n);
		int GetDetailErrNo();
		std::string GetDetailErrText();

		void SetSysErrNo(int n);
		int GetSysErrNo();
		std::string GetSysErrText();

		void SetErrLevel(int n);
		int GetErrLevel();

}; // class Exception

} // namespace OpenSOAP

#define AddFuncCallStack() AddFuncCallStack(__FILE__,__LINE__)

#endif // Logger_H
