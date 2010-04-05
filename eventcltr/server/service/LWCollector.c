#include "service.h"

// internal variables
SERVICE_STATUS          gServiceStatus;       // current status of the service
SERVICE_STATUS_HANDLE   gStatusHandle;

// internal function prototypes
VOID 
WINAPI service_ctrl(
    DWORD dwCtrlCode
    );

VOID 
WINAPI service_main(
    DWORD dwArgc,
    LPTSTR *lpszArgv
    );

VOID 
CmdDebugService(
    INT nArgc,
    WCHAR **ppszArgv
    );

BOOL 
WINAPI ControlHandler(
    DWORD dwCtrlType
    );


DWORD
CliMain(
    INT nArgc,
    PWSTR *ppszArgv
    )
{
    DWORD dwError = 0;
    BOOLEAN bShowHelp = FALSE;
    BOOLEAN bSetRecordsPerPeriod = FALSE;
    BOOLEAN bSetRecordsPerBatch = FALSE;
    BOOLEAN bSetPeriodSeconds = FALSE;
    BOOLEAN bSetRemoteAccess = FALSE;
    BOOLEAN bShowSettings = FALSE;
    BOOLEAN bSetLogLevel = FALSE;

    DWORD dwRecordsPerPeriod = 0;
    DWORD dwRecordsPerBatch = 0;
    DWORD dwPeriodSeconds = 0;
    PCWSTR pwszRemoteAccess = NULL;
    PSECURITY_DESCRIPTOR pDescriptor = NULL;
    PWSTR pwszRemoteAccessLocal = NULL;
    PWSTR pwszDatabasePath = NULL;
    CltrLogLevel dwLogLevel = CLTR_LOG_LEVEL_ERROR;
    HKEY hKey = NULL;
    PCSTR pszLogLevel = NULL;

    // Skip the program name
    nArgc--;
    ppszArgv++;

    if (nArgc < 1)
    {
        bShowHelp = TRUE;
    }

    while (nArgc > 0)
    {
        if ((ppszArgv[0][0] != '/' && ppszArgv[0][0] != '-') ||
            wcslen(ppszArgv[0]) != 2)
        {
            bShowHelp = TRUE;
            break;
        }

        switch(ppszArgv[0][1])
        {
        case 'p':
            nArgc--;
            ppszArgv++;
            if (nArgc < 1)
            {
                bShowHelp = TRUE;
            }
            else
            {
                dwRecordsPerPeriod = wcstoul(ppszArgv[0], NULL, 10);
                bSetRecordsPerPeriod = TRUE;
            }
            break;

        case 'b':
            nArgc--;
            ppszArgv++;
            if (nArgc < 1)
            {
                bShowHelp = TRUE;
            }
            else
            {
                dwRecordsPerBatch = wcstoul(ppszArgv[0], NULL, 10);
                bSetRecordsPerBatch = TRUE;
            }
            break;

        case 't':
            nArgc--;
            ppszArgv++;
            if (nArgc < 1)
            {
                bShowHelp = TRUE;
            }
            else
            {
                dwPeriodSeconds = wcstoul(ppszArgv[0], NULL, 10);
                bSetPeriodSeconds = TRUE;
            }
            break;

        case 'a':
            nArgc--;
            ppszArgv++;
            if (nArgc < 1)
            {
                bShowHelp = TRUE;
            }
            else
            {
                pwszRemoteAccess = ppszArgv[0];
                bSetRemoteAccess = TRUE;
            }
            break;

        case 'l':
            nArgc--;
            ppszArgv++;
            if (nArgc < 1)
            {
                bShowHelp = TRUE;
            }
            else if (!CltrWC16StringCompareWCNString(ppszArgv[0], L"error", FALSE))
            {
                dwLogLevel = CLTR_LOG_LEVEL_ERROR;
                bSetLogLevel = TRUE;
            }
            else if (!CltrWC16StringCompareWCNString(ppszArgv[0], L"warning", FALSE))
            {
                dwLogLevel = CLTR_LOG_LEVEL_WARNING;
                bSetLogLevel = TRUE;
            }
            else if (!CltrWC16StringCompareWCNString(ppszArgv[0], L"info", FALSE))
            {
                dwLogLevel = CLTR_LOG_LEVEL_INFO;
                bSetLogLevel = TRUE;
            }
            else if (!CltrWC16StringCompareWCNString(ppszArgv[0], L"verbose", FALSE))
            {
                dwLogLevel = CLTR_LOG_LEVEL_VERBOSE;
                bSetLogLevel = TRUE;
            }
            else if (!CltrWC16StringCompareWCNString(ppszArgv[0], L"debug", FALSE))
            {
                dwLogLevel = CLTR_LOG_LEVEL_DEBUG;
                bSetLogLevel = TRUE;
            }
            else
            {
                bShowHelp = TRUE;
            }

        case 's':
            bShowSettings = TRUE;
            break;

        default:
        case 'h':
            bShowHelp = TRUE;
            break;
        }
        nArgc--;
        ppszArgv++;
    }

    if (bShowHelp)
    {
        printf("%s",
"/h\n"
"Show this help message\n"
"\n"
"/p <integer>\n"
"Set the records that the eventforwarder can send per period. A period consists of sending multiple batches and then sleeping until the period is over. So this number is the maximum number of events that can be sent before the event forwarder sleeps.\n"
"\n"
"/b <integer>\n"
"Set the records that the eventforwarder can send per batch. A batch is sent with a single RPC call, so setting this too high delays adding any records in the batch until the entire batch is sent.\n"
"\n"
"/t <integer>\n"
"Set the number of seconds in a period. If an event forwarder finishes sending its events before this length of time is up, it will sleep to finish the period.\n"
"\n"
"/a <string>\n"
"Set the remote access security descriptor in SDDL syntax. The default value is \"O:LSG:BAD:PAR(A;;CCDCRP;;;BA)(A;;CCDCRP;;;DA)(A;;CC;;;DC)\".\n"
"\n"
"/l <level>\n"
"Set the log level to error, warning, info, verbose, or debug\".\n"
"\n"
"/s\n"
"Show current settings\n"
        );

        goto cleanup;
    }

    dwError = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Services\\LWCollector"),
                0,
                KEY_WRITE,
                &hKey);
    BAIL_ON_CLTR_ERROR(dwError);

    if (bSetRecordsPerPeriod)
    {
        dwError = RegSetValueEx(
                    hKey,
                    TEXT("RecordsPerPeriod"),
                    0,
                    REG_DWORD,
                    (const BYTE *)&dwRecordsPerPeriod,
                    sizeof(dwRecordsPerPeriod));
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bSetRecordsPerBatch)
    {
        dwError = RegSetValueEx(
                    hKey,
                    TEXT("RecordsPerBatch"),
                    0,
                    REG_DWORD,
                    (const BYTE *)&dwRecordsPerBatch,
                    sizeof(dwRecordsPerBatch));
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bSetPeriodSeconds)
    {
        dwError = RegSetValueEx(
                    hKey,
                    TEXT("PeriodSeconds"),
                    0,
                    REG_DWORD,
                    (const BYTE *)&dwPeriodSeconds,
                    sizeof(dwPeriodSeconds));
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bSetRemoteAccess)
    {
        dwError = RegSetValueEx(
                    hKey,
                    TEXT("RemoteAccess"),
                    0,
                    REG_SZ,
                    (const BYTE *)pwszRemoteAccess,
                    sizeof(WCHAR) * (DWORD)(wcslen(pwszRemoteAccess) + 1));
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bSetLogLevel)
    {
        dwError = RegSetValueEx(
                    hKey,
                    TEXT("LogLevel"),
                    0,
                    REG_DWORD,
                    (const BYTE *)&dwLogLevel,
                    sizeof(dwLogLevel));
        BAIL_ON_CLTR_ERROR(dwError);
    }
    if (bShowSettings)
    {
        dwError = CltrGetRecordsPerPeriod(&dwRecordsPerPeriod);
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = CltrGetRecordsPerBatch(&dwRecordsPerBatch);
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = CltrGetPeriodSeconds(&dwPeriodSeconds);
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = CltrGetDatabasePath(&pwszDatabasePath);
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = CltrGetRemoteSecurityDescriptor(&pDescriptor);
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = CltrGetLogLevel(&dwLogLevel);
        BAIL_ON_CLTR_ERROR(dwError);

        switch(dwLogLevel)
        {
        case CLTR_LOG_LEVEL_ALWAYS:
            pszLogLevel= "always";
            break;
        case CLTR_LOG_LEVEL_ERROR:
            pszLogLevel= "error";
            break;
        case CLTR_LOG_LEVEL_WARNING:
            pszLogLevel= "warning";
            break;
        case CLTR_LOG_LEVEL_INFO:
            pszLogLevel= "info";
            break;
        case CLTR_LOG_LEVEL_VERBOSE:
            pszLogLevel= "verbose";
            break;
        case CLTR_LOG_LEVEL_DEBUG:
            pszLogLevel= "debug";
            break;
        default:
            pszLogLevel= "unknown";
        }

        if (!ConvertSecurityDescriptorToStringSecurityDescriptor(
            pDescriptor,
            SDDL_REVISION_1,
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            &pwszRemoteAccessLocal,
            NULL))
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }

        printf("Current settings:\n"
            "Records per period                %d\n"
            "Records per batch                 %d\n"
            "Seconds in a period               %d\n"
            "Database location                 %ws\n"
            "Remote access security descriptor %ws\n"
            "Log level                         %hhs\n",
            dwRecordsPerPeriod,
            dwRecordsPerBatch,
            dwPeriodSeconds,
            pwszDatabasePath,
            pwszRemoteAccessLocal,
            pszLogLevel);
    }

cleanup:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    if (pDescriptor)
    {
        LocalFree(pDescriptor);
    }
    if (pwszRemoteAccessLocal)
    {
        LocalFree(pwszRemoteAccessLocal);
    }
    CLTR_SAFE_FREE_STRING(pwszDatabasePath);

    return dwError;

error:
    goto cleanup;
}

//
//  FUNCTION: main
//
//  PURPOSE: entrypoint for service
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    main() either performs the command line task, or
//    call StartServiceCtrlDispatcher to register the
//    main service thread.  When the this call returns,
//    the service has stopped, so exit.
//
INT
__cdecl wmain(
    INT nArgc,
    PWSTR *ppszArgv
    )
{
    DWORD dwError = 0;
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main},
        { NULL, NULL}
    };

    if (nArgc == 1)
    {
        if (!StartServiceCtrlDispatcher(dispatchTable))
        {
            dwError = GetLastError();
        }
    }
    else
    {
        dwError = ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }
    if (dwError == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
    {
        dwError = CliMain(nArgc, ppszArgv);
    }
    else
    {
        CLTR_LOG_VERBOSE("StartServiceCtrlDispatcher failed Error = %x\n", dwError);
    }
    
    return dwError;
}



//
//  FUNCTION: service_main
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
VOID
WINAPI service_main(
    DWORD dwArgc, 
    LPTSTR *lppszArgv
    )
{
    // register our service control handler:
    gStatusHandle = RegisterServiceCtrlHandler( TEXT(SZSERVICENAME),
                                                service_ctrl);
    
    if (!gStatusHandle)
        goto cleanup;

    
    // SERVICE_STATUS members that don't change in example
    gServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gServiceStatus.dwServiceSpecificExitCode = 0;

    // report the status to the service control manager.
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, // service state
                              NO_ERROR,              // exit code
                              3000))                 // wait hint
        goto cleanup;

    //Start the service
    ServiceStart( dwArgc,
                  lppszArgv);


cleanup:

    // try to report the stopped status to the service control manager.
    if (gStatusHandle)
        (VOID)ReportStatusToSCMgr( SERVICE_STOPPED,
                                   NO_ERROR,
                                   0);

    return;
}



//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID 
WINAPI service_ctrl(
    DWORD dwCtrlCode
    )
{

    // Handle the requested control code.
    switch (dwCtrlCode)
    {
        // Stop the service.
        
        // SERVICE_STOP_PENDING should be reported before
        // setting the Stop Event - hServerStopEvent - in
        // ServiceStop().  This avoids a race condition
        // which may result in a 1053 - The Service did not respond...
        // error.
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN: //Added
            ReportStatusToSCMgr( SERVICE_STOP_PENDING,
                                 NO_ERROR,
                                 0);
            ServiceStop();
            return;

        // Update the service status.
        case SERVICE_CONTROL_INTERROGATE:
            break;
      
        default:
            break;

    }

    ReportStatusToSCMgr( gServiceStatus.dwCurrentState,
                         NO_ERROR,
                         0);
}



//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL
ReportStatusToSCMgr(
    DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHint
    )
{

    static DWORD dwCheckPoint = 1;
    BOOL bResult = TRUE;

    if (dwCurrentState == SERVICE_START_PENDING)
        gServiceStatus.dwControlsAccepted = 0;
    else
        gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
   
    gServiceStatus.dwCurrentState = dwCurrentState;
    gServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    gServiceStatus.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) ||
           ( dwCurrentState == SERVICE_STOPPED ) )
        gServiceStatus.dwCheckPoint = 0;
    else
        gServiceStatus.dwCheckPoint = dwCheckPoint++;


    // Report the status of the service to the service control manager.
    if (!(bResult = SetServiceStatus( gStatusHandle,
                                     &gServiceStatus)))
        CLTR_LOG_VERBOSE("SetServiceStatus");
      
    return bResult;
}

///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//


//
//  FUNCTION: CmdDebugService(int argc, WCHAR ** argv)
//
//  PURPOSE: Runs the service as a console application
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID
CmdDebugService(
    INT nArgc,
    PWCHAR * ppszArgv
    )
{

    DWORD dwArgc = 0;
    LPTSTR *lppszArgv = NULL;

#ifdef UNICODE
    lppszArgv = CommandLineToArgvW( GetCommandLineW(),
                                   &(dwArgc));
    if (NULL == lppszArgv){
        // CommandLineToArvW failed!!
        CLTR_LOG_ERROR("CmdDebugService CommandLineToArgvW returned NULL\n");
        return;
    }
#else
    dwArgc   = (DWORD) argc;
    lppszArgv = ppszArgv;
#endif

    CLTR_LOG_VERBOSE("Debugging %s.", TEXT(SZSERVICEDISPLAYNAME));

    SetConsoleCtrlHandler( ControlHandler,
                           TRUE);

    ServiceStart( dwArgc,
                  lppszArgv);

#ifdef UNICODE
    // Must free memory allocated for arguments
    GlobalFree(lppszArgv);
#endif // UNICODE

}


//
//  FUNCTION: ControlHandler ( DWORD dwCtrlType )
//
//  PURPOSE: Handled console control events
//
//  PARAMETERS:
//    dwCtrlType - type of control event
//
//  RETURN VALUE:
//    True - handled
//    False - unhandled
//
//  COMMENTS:
//
BOOL 
WINAPI ControlHandler(
    DWORD dwCtrlType
    )
{

    switch ( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            CLTR_LOG_VERBOSE("Received Ctrl+C or Ctrl+Break, hence stopping the service");
            ServiceStop();
            return TRUE;
            break;

    }
    return FALSE;
}

