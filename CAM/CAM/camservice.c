#include "stdafx.h"

#define BAIL_ON_ERROR(dwError) if (dwError) goto error;

LPCWSTR CAMSERVICE_NAME = L"CamService";


SC_HANDLE hServiceHandle = NULL;
SERVICE_STATUS serviceStatus = {0};
HANDLE shutdownServiceEvent = NULL;
DWORD serviceCurrentStatus = 0;
SERVICE_STATUS_HANDLE hServiceStatus = NULL;
HANDLE threadHandle = NULL;


RPC_BINDING_VECTOR* pServerBinding = NULL;


VOID
CamServiceMain(
	DWORD argc,
	LPWSTR *argv
	);

SERVICE_TABLE_ENTRY serviceTable[] =
{
   { L"CamService", (LPSERVICE_MAIN_FUNCTION) CamServiceMain },
   { NULL, NULL }
};

VOID
ServiceCtrlHandler(
	DWORD dwControlCode
	);

BOOL
CamServiceUpdateStatus(
	DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwServiceSpecificExitCode,
	DWORD dwCheckPoint,
	DWORD dwWaitHint
	);

VOID
KillService(
    VOID
	);

// Main functionality delievered by CamService while its running
DWORD 
CamServiceExecutionThread(
    LPDWORD param
	);

// Start 'CamServiceExecutionThread'
BOOL 
StartServiceThread(
    VOID
	);

VOID
TerminateService(
	DWORD dwError
	);



void
main(int argc, char **argv)
{
	BOOL bRet = FALSE;
	DWORD dwError = 0;

	bRet = StartServiceCtrlDispatcher(
				serviceTable
				);
	if (!bRet) 
	{
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}

error:
	return;
}

VOID
CamServiceMain(
	DWORD argc,
	LPWSTR *argv
	)
{
	BOOL bRet = FALSE;
	DWORD temp = 0;


	hServiceStatus = RegisterServiceCtrlHandler(
							CAMSERVICE_NAME,
							(LPHANDLER_FUNCTION)ServiceCtrlHandler
							);
	if (!hServiceStatus) 
	{
		TerminateService(GetLastError());
		return;
	}

	bRet = CamServiceUpdateStatus(
				SERVICE_START_PENDING,
				NO_ERROR,
				0, 1, 5000
				);
	if (!bRet) 
	{
		TerminateService(GetLastError());
		return;
	}

	shutdownServiceEvent = CreateEvent(
								0,
								TRUE, FALSE,0
								);
	if (!shutdownServiceEvent) 
	{
		TerminateService(GetLastError());
		return;
	}

	// Notify SCM the service progress
    bRet = CamServiceUpdateStatus(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
    if (!bRet)
    {    
       TerminateService(GetLastError());
		return;
    }

#if 0
	// Todo:Process any parameter that might be passed in
    

	// Notify SCM the service progress
    bRet = CamServiceUpdateStatus(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
    if (!bRet)
    {    
       TerminateService(GetLastError());
		return;
    }
#endif

   // Start the service execution thread by calling our StartServiceThread function...
   bRet = StartServiceThread();   
   if (!bRet)
   {
      TerminateService(GetLastError());
      return;
   }
   // The service is now running.  Notify SCM the service is running
   serviceCurrentStatus = SERVICE_RUNNING;
   bRet = CamServiceUpdateStatus(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
   if (!bRet)
   {
      TerminateService(GetLastError());
      return;
   }

	WaitForSingleObject(shutdownServiceEvent, INFINITE);
	TerminateService(0);

	return;
}

BOOL
CamServiceUpdateStatus(
	DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwServiceSpecificExitCode,
	DWORD dwCheckPoint,
	DWORD dwWaitHint
	)
{
	BOOL bRet = FALSE;
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = dwCurrentState;

	if (dwCurrentState == SERVICE_START_PENDING) 
	{
		serviceStatus.dwControlsAccepted = 0;
	}else 
	{
		serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	}

	if (dwServiceSpecificExitCode == 0) 
	{
		serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
	}else 
	{
		serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	}

	serviceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
	serviceStatus.dwCheckPoint = dwCheckPoint;
	serviceStatus.dwWaitHint = dwWaitHint;

	bRet = SetServiceStatus(hServiceStatus, &serviceStatus);
	if (!bRet)
   {
      KillService();
   }

	return bRet;

} 

VOID
ServiceCtrlHandler(
	DWORD dwControlCode
	)
{
	BOOL bRet = FALSE;

	switch (dwControlCode) 
	{
		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
         serviceCurrentStatus = SERVICE_STOP_PENDING;
         bRet = CamServiceUpdateStatus(SERVICE_STOP_PENDING, NO_ERROR, 0, 1, 5000);
         KillService();

   	     serviceCurrentStatus = SERVICE_STOPPED;
         bRet = CamServiceUpdateStatus(SERVICE_STOPPED, NO_ERROR, 0, 1, 5000);
         return;

      default:
          break;
   }

   CamServiceUpdateStatus(serviceCurrentStatus, NO_ERROR, 0, 0, 0);
}

void
KillService()
{ 
    SetEvent(shutdownServiceEvent);
}

DWORD 
CamServiceExecutionThread(
    LPDWORD param
	)
{   	
	printf("start camserver service....\n");	
	
	return CamServerMain(&pServerBinding);
}

// StartService -
//   This starts the service by creating its execution thread.
BOOL StartServiceThread()
{
   DWORD id;
 
   threadHandle = CreateThread(0,0,
	                          (LPTHREAD_START_ROUTINE) CamServiceExecutionThread,
	                           0, 0, &id);
   if (!threadHandle)
   {
      return FALSE;
   }
   else
   {     
      return TRUE;
   }
}

VOID
TerminateService(
    DWORD dwError
	)
{
	// Cleanup Service
	CamServerCleanup(pServerBinding);
	return;
}