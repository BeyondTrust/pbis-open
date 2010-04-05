#ifndef __SERVICE_H__
#define __SERVICE_H__

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501    // Change this to the appropriate value to target other versions of Windows.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "server.h"

#ifdef __cplusplus
extern "C" {
#endif

// name of the executable
#define SZAPPNAME            "LWCollector"
// internal name of the service
#define SZSERVICENAME        "LWCollectorService"
// displayed name of the service
#define SZSERVICEDISPLAYNAME "LWCollector Service"
// list of service dependencies - "dep1\0dep2\0\0"
#define SZDEPENDENCIES       ""
    
   VOID
   ServiceStart(
        DWORD dwArgc,
        LPTSTR *lpszArgv
        );

   VOID
   ServiceStop();


   BOOL
   ReportStatusToSCMgr(
        DWORD dwCurrentState,
        DWORD dwWin32ExitCode,
        DWORD dwWaitHint
        );

#ifdef __cplusplus
}
#endif

#endif
