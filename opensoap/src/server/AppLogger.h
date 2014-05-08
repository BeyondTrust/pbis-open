#ifndef AppLogger_H
#define AppLogger_H

#include <stdarg.h>
#include <string>
#include <time.h>
#include <pthread.h>

#include "SysLogger.h"
#include "FileLogger.h"
#include "MsgInfo.h"
#include "Exception.h"
#include "SrvErrorCode.h"

typedef pthread_t apploglockid_t;

namespace OpenSOAP {

class AppLogger {
	private:
		static string AppLogBase[2];	//basename (server.conf)
		static string AppLogFormat[2];	//Log format type
		static Logger * AppLogPtr[2];	//Logger Class Pointer
		static int AppLogNum;			//Number of Logger class 
		static pthread_mutex_t AppLogLock;	//thread lock flag 
		static struct tm LogDate;
		static bool AppLogUpdating;		//Updating flag 
		static bool AppLogInitNG;		//initialize NG flag 
		static bool AppLogFirstInit;	//First initialize flag 
		static apploglockid_t	AppLogLockThread;	//Lock thread id

	public:
		AppLogger();
		AppLogger(const char * general,const char * detail);
		~AppLogger();
		
	private:
		static Logger * CreateLog(const char * base,int type);
		static Logger * CreateLogger(const char * base);
		static SysLogger * CreateSysLogger(const char * base);
		static FileLogger * CreateFileLogger(const char * base);

		static void Lock() ;
		static void UnLock() ;
		static bool isMyLock();

		static void Write(int level,const char * fmt,va_list args);

		//server.conÇ©ÇÁÇÃì«Ç›çûÇ›éxâáä÷êî
		static int GetConfAttrString(const char * key,std::string& buff);
		static int GetConfAttrInt(const char * key,int * i);
		static int GetConfAttrStringWithBase(
						const char * base,const char * key,std::string& buff);
		static int GetConfAttrIntWithBase(
						const char * base,const char * key,int * i);
		static int GetConfLogType(const char * base,int * i);
		static int GetConfLogFormatType(const char * base,std::string& buff);


	public:
		static void Initialize();
		static void Update() ;
		static void Update(const char * log1,const char * log2);

		static void Write(Exception e);
		static void Write(std::string str);
		static void Write(const char * fmt,...);
		static void Write(int level,std::string str);
		static void Write(int level,const char * fmt,...);

		static int RotateCheck();
		static void Rotate();
		static apploglockid_t GetLockThread();		//Current lock thread id
		static void UnLock(apploglockid_t tid);		//force unlock
		static apploglockid_t GetCurrentThreadID();
}; 

}//end of namespace OpenSOAP

#endif /* Logger_H */
