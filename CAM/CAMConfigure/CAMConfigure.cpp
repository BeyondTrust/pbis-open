// CAMConfigure.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define BAIL_ON_ERROR(dwError) if (dwError) goto error;

LPCWSTR CAMSERVICE_NAME = L"CamService";

DWORD
CamInstall(int argc, _TCHAR* argv[]);

DWORD
CamUninstall(int argc, _TCHAR* argv[]);


DWORD
CamStart(int argc, _TCHAR* argv[]);

int _tmain(int argc, _TCHAR* argv[])
{
	if (!argv[1])
	{
		printf("Usage:CAMConfigure [install|uninstall|start]\n");
		return 0;
	}
	
	if (!_wcsicmp(L"install", argv[1])) {
		CamInstall(argc, argv);
	}else if (!_wcsicmp(L"uninstall", argv[1])){
		CamUninstall(argc, argv);
	}
	 else if (!_wcsicmp(L"start", argv[1])){
		CamStart(argc, argv);
	}else {
		printf("Usage:CAMConfigure [install|uninstall|start]\n");
	}
	return 0;
}

DWORD
CamInstall(int argc, _TCHAR* argv[])
{
	SC_HANDLE hService = NULL;
	SC_HANDLE hScm = NULL;
	DWORD dwError = 0;

	hScm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!hScm) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}
	hService = CreateService(
				hScm,
				CAMSERVICE_NAME,
				L"Computer Account Manager Service",
				SERVICE_ALL_ACCESS,
				SERVICE_WIN32_OWN_PROCESS,
				SERVICE_DEMAND_START,
				SERVICE_ERROR_NORMAL,
				L"C:\\Documents and Settings\\wfu.CORP\\Desktop\\Work\\Sapphire\\src\\linux\\CAM\\debug\\CAM.exe",
				0,
				0,
				0,
				0,
				0);
	if (!hService) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}

error:
	if (hService) {
		CloseServiceHandle(hService);
	}
	if (hScm) {
		CloseServiceHandle(hScm);
	}

	return dwError;
}

DWORD
CamUninstall(int argc, _TCHAR* argv[])
{
	SC_HANDLE hService = NULL;
	SC_HANDLE hScm = NULL;
	DWORD dwError = 0;
	BOOL bRet = FALSE;
	SERVICE_STATUS serviceStatus = {0};

	hScm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!hScm) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}
	hService = OpenService(
				hScm,
				CAMSERVICE_NAME,
				SERVICE_ALL_ACCESS | DELETE
				);
	if (!hService) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}

	bRet = QueryServiceStatus(hService, &serviceStatus);
	if (!bRet) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}

	if (serviceStatus.dwCurrentState != SERVICE_STOPPED) {
		bRet = ControlService(hService, SERVICE_STOP, &serviceStatus);
	}

	bRet = DeleteService(hService);

error:
	if (hService) {
		CloseServiceHandle(hService);
	}
	if (hScm) {
		CloseServiceHandle(hScm);
	}

	return dwError;
}

DWORD
CamStart(int argc, _TCHAR* argv[])
{
    SERVICE_STATUS_PROCESS serviceStatus = {0}; 
    SC_HANDLE hService = NULL;
	SC_HANDLE hScm = NULL;

    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;

	DWORD dwError = 0;
	BOOL bRet = FALSE;

	hScm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!hScm) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}
	hService = OpenService(
				hScm,
				CAMSERVICE_NAME,
				SERVICE_ALL_ACCESS
				);
	if (!hService) {
		dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
	}  

    // Check the status in case the service is not stopped. 

    if (!QueryServiceStatusEx( 
            hService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE) &serviceStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // size needed if buffer is too small
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
    }

    // Check if the service is already running. It would be possible 
    // to stop the service here, but for simplicity this example just returns. 

    if(serviceStatus.dwCurrentState != SERVICE_STOPPED && serviceStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        printf("Cannot start the service because it is already running\n");
        dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = serviceStatus.dwCheckPoint;

    // Wait for the service to stop before attempting to start it.

    while (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 
 
        dwWaitTime = serviceStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status until the service is no longer stop pending. 
 
        if (!QueryServiceStatusEx( 
                hService,                     // handle to service 
                SC_STATUS_PROCESS_INFO,         // information level
                (LPBYTE) &serviceStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded ) )              // size needed if buffer is too small
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            dwError = GetLastError();
		    BAIL_ON_ERROR(dwError);
        }

        if ( serviceStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = serviceStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > serviceStatus.dwWaitHint)
            {
                printf("Timeout waiting for service to stop\n");
                dwError = GetLastError();
		        BAIL_ON_ERROR(dwError);
            }
        }
    }

    // Attempt to start the service.

    if (!StartService(
            hService,  // handle to service 
            0,           // number of arguments 
            NULL) )      // no arguments 
    {
        printf("StartService failed (%d)\n", GetLastError());
        dwError = GetLastError();
		BAIL_ON_ERROR(dwError); 
    }
    else printf("Service start pending...\n"); 

    // Check the status until the service is no longer start pending. 
 
    if (!QueryServiceStatusEx( 
            hService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // info level
            (LPBYTE) &serviceStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
    {
        printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
        dwError = GetLastError();
		BAIL_ON_ERROR(dwError);
    }
 
    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = serviceStatus.dwCheckPoint;

    while (serviceStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 
 
        dwWaitTime = serviceStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status again. 
 
        if (!QueryServiceStatusEx( 
            hService,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            (LPBYTE) &serviceStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded ) )              // if buffer too small
        {
            printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
            break; 
        }
 
        if ( serviceStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = serviceStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > serviceStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    } 

    // Determine whether the service is running.

    if (serviceStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        printf("Service started successfully.\n"); 
    }
    else 
    { 
        printf("Service not started. \n");
        printf("  Current State: %d\n", serviceStatus.dwCurrentState); 
        printf("  Exit Code: %d\n", serviceStatus.dwWin32ExitCode); 
        printf("  Check Point: %d\n", serviceStatus.dwCheckPoint); 
        printf("  Wait Hint: %d\n", serviceStatus.dwWaitHint); 
    } 

error:

    CloseServiceHandle(hService); 
    CloseServiceHandle(hScm);

	return dwError;


}

