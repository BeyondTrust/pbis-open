// test_copyfile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BAIL_ON_ERROR(dwError) \
	if (dwError) goto error;
 
int
_tmain(
    int argc,
	TCHAR *argv[]
    ) 
{
	DWORD  dwError = 0;
    HANDLE hRemoteFile = (HANDLE)NULL;
	HANDLE hLocalFile = (HANDLE)NULL;
    LPTSTR lpszRemotePath = NULL;
	LPTSTR lpszLocalPath = NULL;

	if (argc < 3)
	{
		printf("Usage: npcopy <remote path> <local path>\n");
		dwError = 1;
		BAIL_ON_ERROR(dwError);
	}

	lpszRemotePath = argv[1];
	lpszLocalPath = argv[2];
 
    hRemoteFile = CreateFile( 
					lpszRemotePath,
					GENERIC_READ, 
					0,              // no sharing 
					NULL,           // default security attributes
					OPEN_EXISTING,  // opens existing pipe
					0,              // default attributes 
					NULL);          // no template file 

    if (hRemoteFile == INVALID_HANDLE_VALUE)
    {
		dwError = GetLastError();
		_tprintf(TEXT("Failed to open file %s. [error : %d]"), lpszRemotePath, dwError);
		BAIL_ON_ERROR(dwError);
    }

	hLocalFile = CreateFile( 
					lpszLocalPath,
					GENERIC_WRITE, 
					0,              // no sharing 
					NULL,           // default security attributes
					OPEN_ALWAYS,
					0,              // default attributes 
					NULL);          // no template file 

    if (hLocalFile == INVALID_HANDLE_VALUE)
    {
		dwError = GetLastError();
		_tprintf(TEXT("Failed to open file %s. [error : %d]"), lpszLocalPath, dwError);
		BAIL_ON_ERROR(dwError);
    }
 
	do 
	{
		BYTE  buffer[1024];
		BOOL  fWriteSuccess = FALSE;
		BOOL  fReadSuccess = FALSE;
		DWORD cbRead = 0;
		DWORD cbWritten = 0;

		fReadSuccess = ReadFile( 
						hRemoteFile,
						buffer, 
						sizeof(buffer),
						&cbRead,  // number of bytes read 
						NULL);    // not overlapped

		if (!fReadSuccess)
		{
			dwError = GetLastError();
			_tprintf(TEXT("Failed to read file %s. [error : %d]"), lpszRemotePath, dwError);
			BAIL_ON_ERROR(dwError);
		}
 
		if (!cbRead)
		{
			break;
		}

		fWriteSuccess = WriteFile(
							hLocalFile,
							buffer,
							cbRead,
							&cbWritten,
							NULL);
		if (!fWriteSuccess)
		{
			dwError = GetLastError();
			_tprintf(TEXT("Failed to write file %s. [error : %d]"), lpszLocalPath, dwError);
			BAIL_ON_ERROR(dwError);
		}

   } while (1);
 
cleanup:
	 
	if (hRemoteFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hRemoteFile);
	}

	if (hLocalFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hLocalFile);
	}

	return dwError;

error:

	goto cleanup;
}


