/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

#include "cltr-base.h"

#ifndef WIN32

#define _stat64i32 stat

FILE *
_wfopen(
    PCWSTR pwszPath,
    PCWSTR pwszMode
    )
{
    FILE *result = NULL;
    PSTR pszPath = NULL;
    PSTR pszMode = NULL;

    pszPath = awc16stombs(pwszPath);
    if (pszPath == NULL)
    {
        goto cleanup;
    }

    pszMode = awc16stombs(pwszMode);
    if (pszMode == NULL)
    {
        goto cleanup;
    }

    result = fopen(pszPath, pszMode);

cleanup:
    if (pszPath != NULL)
    {
        free(pszPath);
    }
    if (pszMode != NULL)
    {
        free(pszMode);
    }
    return result;
}

int _wstat(PCWSTR pwszPath, struct stat *pBuf)
{
    int result = 0;
    PSTR pszPath = awc16stombs(pwszPath);

    if (pszPath == NULL)
    {
        result = -1;
        goto cleanup;
    }

    result = stat(pszPath, pBuf);

cleanup:
    if (pszPath != NULL)
    {
        free(pszPath);
    }
    return result;
}
#endif

DWORD
CltrCheckFileExists(
    PWSTR pszPath,
    PBOOLEAN pbFileExists
    )
{
    DWORD dwError = 0;
    struct _stat64i32 statbuf;

    memset(&statbuf, 0, sizeof(struct _stat64i32));

    dwError = _wstat( pszPath, &statbuf );
    
    if( dwError != 0 ) {
        CLTR_LOG_INFO( "Specified file %ws not found.\n", pszPath);
        *pbFileExists = FALSE;
        dwError = 0;
        BAIL_ON_CLTR_ERROR(dwError);
        
    } else {
        //Found the file. Set the value to TRUE
        *pbFileExists = TRUE;
    }

error:
    return dwError;
}

DWORD
CltrRemoveFile(
    PWSTR pwszPath
    )
{
    DWORD dwError = 0;
#ifdef _WIN32
    if (DeleteFile(pwszPath))
    {
        dwError = 0;
    }
    else
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }
#else
    PSTR pszPath = awc16stombs(pwszPath);

    if (pszPath == NULL)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    if (remove(pszPath) < 0)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }
#endif

error:
#ifndef _WIN32
    if (pszPath)
    {
        free(pszPath);
    }
#endif
    return dwError;
}

DWORD
CltrCheckDirectoryExists(
    PWSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    DWORD dwError = 0;
    struct _stat64i32 statbuf;

    memset(&statbuf, 0, sizeof(struct _stat64i32));

    dwError = _wstat( pszPath, &statbuf );
    
    if( dwError != 0 ) {

        CLTR_LOG_INFO( "Specified Directory %ws not found.\n", pszPath);
        *pbDirExists = FALSE;
        dwError = 0;
        BAIL_ON_CLTR_ERROR(dwError);
    } else {
        //Found the directory. Set the value to TRUE
        *pbDirExists = TRUE;
    }

error:

    return dwError;
}

DWORD
CltrCreateDirectory(PCWSTR pwszPath)
{
    DWORD dwError = 0;

#ifdef _WIN32
    if(CreateDirectory(pwszPath,NULL))
        dwError = 0;
    else
    {
        dwError = 1;
        BAIL_ON_CLTR_ERROR(dwError);
    }
#else
    PSTR pszPath = awc16stombs(pwszPath);

    if (pszPath == NULL)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    if (mkdir(pszPath, S_IRUSR|S_IWUSR|S_IXUSR) < 0)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }
#endif
    
error:
#ifndef _WIN32
    if (pszPath)
    {
        free(pszPath);
    }
#endif
    return dwError;
}
