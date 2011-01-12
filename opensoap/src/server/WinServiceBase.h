/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: WinServiceBase.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#ifndef WinServiceBase_H
#define WinServiceBase_H

#include <string>

class WinServiceBase
{
protected:

	std::string execProgName;
	std::string logFileName;
	std::string extParm;

   static SERVICE_STATUS          ServiceStatus; 
   static SERVICE_STATUS_HANDLE   ServiceStatusHandle; 
   static HANDLE thread;
   DWORD lasterror;
   WinServiceBase() { _this=this; };
   HANDLE hev;
   static void ServiceThread_(void *);
   static void __stdcall CtrlHandler(DWORD);
   static void __stdcall ServiceStart_ (DWORD argc, LPTSTR *argv); 

   virtual void ServiceThread(void);            // service proc. thread
   virtual void ServicePause(BOOL pause) {};       // for pause
   virtual void ServiceStop();                     // for stop
   virtual void ServiceInterrogate() {};           // for info
   virtual void ServiceControl(DWORD opcode) {};   // 
   virtual DWORD ServiceInit(int argc, LPTSTR *argv) 
      { return NO_ERROR; };  // initialize
   // need unique service name 
   virtual LPTSTR GetName()=0;

public:
   static void main(int argc, char *argv[]);
   static WinServiceBase *_this;

};

#endif //WinServiceBase_H
