// test_np_client_echo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUFSIZE 512

#define BAIL_ON_ERROR(dwError) \
	if (dwError) goto error;
 
int
_tmain(
    int    argc,
	TCHAR* argv[]
    ) 
{
	DWORD dwError = 0;
    HANDLE hPipe = (HANDLE)NULL; 
    LPTSTR lpvMessage=TEXT("The quick brown fox jumped over the lazy dog.\n"); 
    TCHAR chBuf[BUFSIZE]; 
    BOOL fSuccess; 
    DWORD cbRead, cbWritten, dwMode; 
    LPTSTR lpszPipename = NULL;

   if (argc < 2)
   {
	   _tprintf(TEXT("Usage: test_np_client_echo <server pipe>"));
		dwError = 1;
		BAIL_ON_ERROR(dwError);
   }

   lpszPipename = argv[1]; 
 
   while (1) 
   { 
      hPipe = CreateFile( 
					lpszPipename, 
					GENERIC_READ | GENERIC_WRITE, 
					0,              // no sharing 
					NULL,           // default security attributes
					OPEN_EXISTING,  // opens existing pipe 
					0,              // default attributes 
					NULL);          // no template file 
 
      if (hPipe != INVALID_HANDLE_VALUE)
	  {
         break;
	  }
 
      // Exit if an error other than ERROR_PIPE_BUSY occurs. 
 
      if (GetLastError() != ERROR_PIPE_BUSY) 
      {
		  dwError = GetLastError();
		  _tprintf(TEXT("Failed to open pipe. [error code: %d)"), dwError); 
		  BAIL_ON_ERROR(dwError);
      }
 
      // All pipe instances are busy, so wait for 20 seconds. 
 
      if (!WaitNamedPipe(lpszPipename, 20000)) 
      {
		  dwError = GetLastError();
		  _tprintf(TEXT("Failed waiting to open pipe [error code: %d]"), dwError);
		  BAIL_ON_ERROR(dwError);
      } 
   }
 
   // The pipe connected; change to message-read mode. 
   dwMode = PIPE_READMODE_MESSAGE; 
   fSuccess = SetNamedPipeHandleState( 
					hPipe,    // pipe handle 
					&dwMode,  // new pipe mode 
					NULL,     // don't set maximum bytes 
					NULL);    // don't set maximum time 
   if (!fSuccess)
   {
	   dwError = GetLastError();
	   _tprintf(TEXT("SetNamedPipeHandleState failed. [error code: %d]"), dwError); 
       BAIL_ON_ERROR(dwError);
   } 
 
   fSuccess = WriteFile( 
				hPipe,                  // pipe handle 
				lpvMessage,             // message 
				(lstrlen(lpvMessage)+1)*sizeof(TCHAR), // message length 
				&cbWritten,             // bytes written 
				NULL);                  // not overlapped 
   if (!fSuccess) 
   {
	   dwError = GetLastError();
	   _tprintf(TEXT("WriteFile failed. [Error code: %d]"), dwError); 
       BAIL_ON_ERROR(dwError);
   }
 
   do 
   {
      fSuccess = ReadFile( 
				    hPipe,    // pipe handle 
					chBuf,    // buffer to receive reply 
					BUFSIZE*sizeof(TCHAR),  // size of buffer 
					&cbRead,  // number of bytes read 
					NULL);    // not overlapped 
 
      if (!fSuccess)
	  {
		  dwError = GetLastError();

		  if (dwError == ERROR_MORE_DATA)
		  {
			  continue;
		  }

		  _tprintf(TEXT("Failed ReadFile. [error code : %d]"), dwError);

		  BAIL_ON_ERROR(dwError);
	  }

	  if (!_tcsnccmp(lpvMessage, chBuf, _tcslen(lpvMessage)))
	  {
		  _tprintf(TEXT("Echo from server [%s]", lpvMessage));
		  break;
	  }
	  else
	  {
		  dwError = 1;
		  _tprintf(TEXT("Echo from server does not match\n"));
		  BAIL_ON_ERROR(dwError);
	  }

   } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA

cleanup:

    if (hPipe != INVALID_HANDLE_VALUE)
    {
 		CloseHandle(hPipe); 
    }

	return dwError;

error:

	goto cleanup;
}


