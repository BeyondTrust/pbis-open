/*--------------------------------------------------------------------------
MODULE:   service.c

PURPOSE:  Implements the body of the service.
          The behavior is to set server defaults, create DB, initialize logging and register RPC
          and listen for RPC to accept connections.

FUNCTIONS:
          ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
          ServiceStop( );

COMMENTS: 
--------------------------------------------------------------------------*/


#include "service.h"

// this event is signalled when the
// service should end
//
HANDLE gServerStopEvent = NULL;

RPC_BINDING_VECTOR* pServerBinding = NULL;


//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//     PURPOSE:  Implements the body of the service.
//               The behavior is to set server defaults, create DB, initialize logging and register RPC
//                and listen for RPC to accept connections.
//
VOID
ServiceStart(
    DWORD dwArgc,
    LPTSTR * ppszArgv
    )
{

    DWORD dwError = 0;
    
    //int i = 1;
    
    CLTR_LOG_VERBOSE("In Service Start\n");

    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, // service state
                              NO_ERROR,              // exit code
                              3000))                 // wait hint
        BAIL_ON_CLTR_ERROR(dwError);
    
    // create the event object. The control handler function signal
    // this event when it receives the "stop" control code.
    gServerStopEvent = CreateEvent ( NULL,
                                     TRUE,
                                     FALSE,
                                     NULL);
    
    if (!gServerStopEvent) {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }

    // Set server defaults, created DB, and initialize logging 
    dwError = CltrServerMain( dwArgc,
                             (PSTR*)ppszArgv);
    BAIL_ON_CLTR_ERROR(dwError);

    //Register for RPC
    CLTR_LOG_INFO("Registering RPC...");

    dwError = RpcServerRegisterAuthInfo(
                    L"unused",
                    RPC_C_AUTHN_GSS_NEGOTIATE,
                    NULL,
                    NULL);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrRegisterForRPC(TEXT("Likewise Eventlog Service"),
                                &pServerBinding);
    BAIL_ON_CLTR_ERROR(dwError);

    if (!ReportStatusToSCMgr ( SERVICE_RUNNING,    // service state
                                NO_ERROR,    // exit code
                                0))    // wait hint
        goto error;

    dwError = CltrListenForRPC();
    BAIL_ON_CLTR_ERROR(dwError);
     
cleanup:
    if (gServerStopEvent)
        CloseHandle (gServerStopEvent);

    if (pServerBinding) {
        CltrUnregisterForRPC(pServerBinding);
    }

    CltrCloseGlobalLog();

    return;
    

error:
    goto cleanup;
}


//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//
VOID
ServiceStop ()
{
    if (pServerBinding) {
        CltrUnregisterForRPC(pServerBinding);
    }

    if (gServerStopEvent){
        CLTR_LOG_VERBOSE("Stopping the service.....\n");
        SetEvent (gServerStopEvent);
    }

    //Adding 
    RpcMgmtStopServerListening(NULL);
    
    //close the log
    CltrCloseGlobalLog();
}
