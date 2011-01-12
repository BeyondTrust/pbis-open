/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: WinServiceBase.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <windows.h>
#include <iostream>
#include <string.h>
#include <process.h>
#include <string>
#include "WinServiceBase.h"

using namespace std;

SERVICE_STATUS  WinServiceBase::ServiceStatus; 
SERVICE_STATUS_HANDLE   WinServiceBase::ServiceStatusHandle; 
HANDLE WinServiceBase::thread;
WinServiceBase *WinServiceBase::_this;

void WinServiceBase::ServiceThread(void)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si,0,sizeof(si));
	si.cb=sizeof(si);

	string execProg = execProgName + " " + logFileName;
	if (!extParm.empty()) {
		execProg += " ";
		execProg += extParm;
	}
	if (!CreateProcess(NULL,
				(char*)execProg.c_str(),
				NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)) {
		int err=GetLastError();
		if (err==2) {
		}
		else {
		}
		//return;
	}
	//wait for stop status
	WaitForSingleObject(hev, INFINITE);
	// --> set non-signal automatically, and main thread will wait

	//process terminate
	TerminateProcess(pi.hProcess, 0);

	// set signal, and activate main thread
	SetEvent(hev);

}

void WinServiceBase::ServiceStop()
{
	//HANDLE ev = CreateEvent(NULL, FALSE, FALSE, _this->GetName());

	// set signal, and activate ServiceThread()
	SetEvent(hev);
	Sleep(0);

	// wait for ServiceThread() terminate
	WaitForSingleObject(hev, INFINITE);
	CloseHandle(hev);
}


void WinServiceBase::ServiceThread_(void *)
{
	// create event kernel object & set non-signal
	// use auto reset event, it's miso
	_this->hev = CreateEvent(NULL, FALSE, FALSE, _this->GetName());
   _this->ServiceThread();
}

void __stdcall WinServiceBase::CtrlHandler (DWORD Opcode) 
{     
	switch(Opcode)     
   {         
      case SERVICE_CONTROL_PAUSE: 
		//impl. for pause
         ServiceStatus.dwCurrentState = SERVICE_PAUSED;
         _this->ServicePause(TRUE);
         break; 

      case SERVICE_CONTROL_CONTINUE: 
		//impl. for continue
         ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
         _this->ServicePause(FALSE);
         break;          

      case SERVICE_CONTROL_STOP: 
		//impl. for stop
         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
         ServiceStatus.dwCheckPoint    = 0; 
         ServiceStatus.dwWaitHint      = 0;  
         _this->ServiceStop();
         if (!SetServiceStatus (ServiceStatusHandle, 
            &ServiceStatus))            
		 { 
             _this->lasterror = GetLastError(); 
		 }  
         return;          
      case SERVICE_CONTROL_INTERROGATE: 
		//set current status
         _this->ServiceInterrogate();
         break;  
      default: 
         _this->ServiceControl(Opcode);

   }      
   // set current status
   if (!SetServiceStatus (ServiceStatusHandle,  &ServiceStatus))     
      _this->lasterror = GetLastError(); 
   return; 
}  

void __stdcall WinServiceBase::ServiceStart_ (DWORD argc, LPTSTR *argv) 
{     
   ServiceStatus.dwServiceType        = SERVICE_WIN32; 
   ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
   ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | 
      SERVICE_ACCEPT_PAUSE_CONTINUE; 
   ServiceStatus.dwWin32ExitCode      = 0; 
   ServiceStatus.dwServiceSpecificExitCode = 0; 
   ServiceStatus.dwCheckPoint         = 0; 
   ServiceStatus.dwWaitHint           = 0;  
   ServiceStatusHandle = RegisterServiceCtrlHandler( 
      _this->GetName(),CtrlHandler);  
   if (ServiceStatusHandle == (SERVICE_STATUS_HANDLE)0)     
   { 
      _this->lasterror=GetLastError();
	  return;     
   }  
   // impl. for initialize
   _this->lasterror=_this->ServiceInit(argc,argv);
   // error proc.
   if (_this->lasterror != NO_ERROR)     
   { 
      ServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
      ServiceStatus.dwCheckPoint         = 0; 
      ServiceStatus.dwWaitHint           = 0; 
      ServiceStatus.dwWin32ExitCode      = _this->lasterror; 
      ServiceStatus.dwServiceSpecificExitCode = _this->lasterror;  
      SetServiceStatus (ServiceStatusHandle, &ServiceStatus); 
      return;     
   }   // initialize done. report running status.
   ServiceStatus.dwCurrentState       = SERVICE_RUNNING; 
   ServiceStatus.dwCheckPoint         = 0; 
   ServiceStatus.dwWaitHint           = 0;  
   if (!SetServiceStatus (ServiceStatusHandle, &ServiceStatus))     
   {
      _this->lasterror = GetLastError(); 
   }
   // proc. service
   thread=(HANDLE)_beginthread(ServiceThread_,0,NULL);
   return; 
} 

void WinServiceBase::main(int argc, char *argv[])
{
   SERVICE_TABLE_ENTRY   DispatchTable[] =     
   { 
      { TEXT(""), ServiceStart_      }, 
      { NULL,              NULL                }     
   };  
   DispatchTable[0].lpServiceName=_this->GetName();
   if (argc>1 && !stricmp(argv[1],"delete"))
   {
      SC_HANDLE scm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
      if (!scm) 
	  {
		cerr << "can't open SCM" << endl;
         exit(1);
	  }
      SC_HANDLE svc=OpenService(scm,_this->GetName(),DELETE);
      if (!svc)
	  {
		cerr << "can't open service" << endl;
         exit(2);
	  }
      if (!DeleteService(svc))
	  {
		cerr << "can't delete service" << endl;
         exit(3);
	  }
	  cout << "service deleted." << endl;
      CloseServiceHandle(svc);
      CloseServiceHandle(scm);

      exit(0);
		
   }
   if (argc>1 && !stricmp(argv[1],"setup"))
   {
      char pname[1024];
      pname[0]='"';
      GetModuleFileName(NULL,pname+1,1023);
      strcat(pname,"\"");
      SC_HANDLE scm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE),svc;
      if (!scm) 
	  {
		cerr << "can't open SCM" << endl;
         exit(1);
	  }
      if (!(svc=CreateService(scm,_this->GetName(),_this->GetName(),
		 SERVICE_ALL_ACCESS,
         SERVICE_WIN32_OWN_PROCESS,SERVICE_DEMAND_START,
         SERVICE_ERROR_NORMAL,pname,NULL,NULL,NULL,NULL,NULL)))
	  {
		cerr << "service entry failed." << endl;
         exit(2);
	  }
	  cout << pname << " entry success." << endl;
      CloseServiceHandle(svc);
      CloseServiceHandle(scm);
      exit(0);
   }

   if (!StartServiceCtrlDispatcher( DispatchTable))     
   { 
      // error
      cerr << "StartServiceCtrlDispatcher failed." << endl;
		cerr << "[" <<		GetLastError() << "]" << endl;
/*
		LPVOID lpMsgBuf;
		FormatMessage(
			    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			    FORMAT_MESSAGE_FROM_SYSTEM |
			    FORMAT_MESSAGE_IGNORE_INSERTS,
			    NULL,
			    GetLastError(),
			    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Šù’è‚ÌŒ¾Œê
			    (LPTSTR) &lpMsgBuf,
			    0,
			    NULL
				);

		MessageBox(NULL, (LPCTSTR)lpMsgBuf, 
				"Error", MB_OK | MB_ICONINFORMATION);

		LocalFree(lpMsgBuf);
*/
   }
}

void main(int argc, char *argv[])
{
   WinServiceBase::_this->main(argc,argv);
}

