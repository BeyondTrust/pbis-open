#ifndef SysLogger_H
#define SysLogger_H

#include <string>
#include <syslog.h>

#include "Logger.h"

namespace OpenSOAP {

class SysLogger :public Logger {
	protected:
		int SyslogOption;
		int SyslogFacility;
		int SyslogLevel;

	public:
		SysLogger();
		~SysLogger();

		void Initialize();
		void SetSyslogOption(int option);
		void SetSyslogFacility(int facility);
		void SetSyslogLevel(int level);
		int GetSyslogOption();
		int GetSyslogFacility();
		int GetSyslogLevel();
		
	public:
		void Open();
		void Close();
		void Lock(){}
		void UnLock(){}
		//インフォメーション用
		int Write(std::string str);
		int Write(const char * fmt,...);
		int Write(const char * fmt,va_list args);
		int Write(int level,std::string str);
		int Write(int level,const char * fmt,va_list args);
		int Write(int level,const char * fmt,...);
		int Write(int level,time_t t,const char * hostname
					,const char * processname
					,pid_t pid,pthread_t tid
					,const char * fmt,va_list args);

		//直接指定した文字を書き込む場合用
		int RawWrite(int level,const char * fmt,va_list args);
		int RawWrite(int level,const char * fmt,...);

};

}//end of namespace OpenSOAP

#endif /* Logger_H */
