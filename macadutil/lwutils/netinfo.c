/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

static const DWORD BUFSIZE = 4096;

static
DWORD
GetEthernetAddress(
    PSTR pszAdapterName,
    PSTR * ppszENetAddress
    );

static
DWORD
GetIPAddress(
    PSTR pszAdapterName,
    PSTR * ppszIPAddress
    );

static
BOOLEAN
GetIsRunning(
    PSTR pszAdapterName
    );

static
BOOLEAN
GetIsUp(
    PSTR pszAdapterName
    );

DWORD
LWGetNetAdapterList(
    BOOLEAN fEnXOnly,
    PNETADAPTERINFO * ppNetAdapterList
    )
{
    DWORD dwError = 0;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};
    PNETADAPTERINFO pAdapterList = NULL;
    PNETADAPTERINFO pPrev = NULL;
    PNETADAPTERINFO pNew = NULL;
    BOOLEAN IsEthernet = FALSE;

    if (!ppNetAdapterList) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig -a | grep \"flags\" | awk '{print $1}'");

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    if(child == 0)
    {
        /* The child */
        close(filedes[0]);
        dup2(filedes[1], 1);
        close(filedes[1]);
        execl("/bin/sh", "sh", "-c", szCommand, (char *)0);
        /* This exit call should not be reached */
        exit(-1);
    }

    /* The parent */
    close(filedes[1]);
    filedes[1] = -1;
    pFile = fdopen(filedes[0], "r");
    if (pFile == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    while (!feof(pFile)) {
        IsEthernet = FALSE;
        
        if (NULL != fgets(szBuf, BUFSIZE, pFile)) {
            LWStripWhitespace(szBuf);
            
            if (strlen(szBuf) > 2) {
                if (szBuf[strlen(szBuf) - 1] == ':')
                    szBuf[strlen(szBuf) - 1] = 0;
                
                if (szBuf[0] == 'e' && szBuf[1] == 'n')
                    IsEthernet = TRUE;
            } else {
                /* This adapter name does not seem proper, skip to next line of output */
                continue;
            }
            
            if ( (fEnXOnly && IsEthernet) || (!fEnXOnly) ) {
                dwError = LWAllocateMemory(sizeof(NETADAPTERINFO), (PVOID*) &pNew);
                BAIL_ON_MAC_ERROR(dwError);

                dwError = LWAllocateString(szBuf, &pNew->pszName);
                BAIL_ON_MAC_ERROR(dwError);
                
                GetEthernetAddress(pNew->pszName, &pNew->pszENetAddress);
                GetIPAddress(pNew->pszName, &pNew->pszIPAddress);
                pNew->IsUp = GetIsUp(pNew->pszName);
                pNew->IsRunning = GetIsRunning(pNew->pszName);
                   
                if (pAdapterList) {
                    pPrev->pNext = pNew;
                    pPrev = pNew;
                } else {
                    pAdapterList = pNew;
                    pPrev = pAdapterList;
                }
            
                pNew = NULL;
            }
        }
    }
    
    if (pAdapterList) {
        *ppNetAdapterList = pAdapterList;
        pAdapterList = NULL;
    }

cleanup:

    if (pFile != NULL) {
        pclose(pFile);
    }
    if (filedes[0] != -1)
        close(filedes[0]);
    if (filedes[1] != -1)
        close(filedes[1]);
    if (child != 0)
    {
        int status;
        waitpid(child, &status, 0);
    }
    
    if (pAdapterList)
        LWFreeNetAdapterList(pAdapterList);
        
    if (pNew)
        LWFreeNetAdapterList(pNew);

    return dwError;

error:

    goto cleanup;
}

void
LWFreeNetAdapterList(
    PNETADAPTERINFO pNetAdapterList
    )
{
    while (pNetAdapterList)
    {
        PNETADAPTERINFO pTemp = pNetAdapterList;

        pNetAdapterList = pNetAdapterList->pNext;

        if (pTemp->pszName)
            LWFreeString(pTemp->pszName);

        if (pTemp->pszENetAddress)
            LWFreeString(pTemp->pszENetAddress);

        if (pTemp->pszIPAddress)
            LWFreeString(pTemp->pszIPAddress);

        LWFreeMemory(pTemp);
    }
}

static
DWORD
GetEthernetAddress(
    PSTR pszAdapterName,
    PSTR * ppszENetAddress
    )
{
    DWORD dwError = 0;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};

    if (!pszAdapterName || !ppszENetAddress) {
        dwError = eParameterError;
        BAIL_ON_MAC_ERROR(dwError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"ether \" | awk '{print $2}'",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    if(child == 0)
    {
        /* The child */
        close(filedes[0]);
        dup2(filedes[1], 1);
        close(filedes[1]);
        execl("/bin/sh", "sh", "-c", szCommand, (char *)0);
        /* This exit call should not be reached */
        exit(-1);
    }

    /* The parent */
    close(filedes[1]);
    filedes[1] = -1;
    pFile = fdopen(filedes[0], "r");
    if (pFile == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        LWStripWhitespace(szBuf);
        dwError = LWAllocateString(szBuf, ppszENetAddress);
        BAIL_ON_MAC_ERROR(dwError);
    }
    else
    {
        dwError = MAC_AD_ERROR_NO_SUCH_ATTRIBUTE;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    if (pFile != NULL) {
        pclose(pFile);
    }
    if (filedes[0] != -1)
        close(filedes[0]);
    if (filedes[1] != -1)
        close(filedes[1]);
    if (child != 0)
    {
        int status;
        waitpid(child, &status, 0);
    }

    return dwError;
    
error:

    goto cleanup;
}

static
DWORD
GetIPAddress(
    PSTR pszAdapterName,
    PSTR * ppszIPAddress
    )
{
    DWORD dwError = 0;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};

    if (!pszAdapterName || !ppszIPAddress) {
        dwError = eParameterError;
        BAIL_ON_MAC_ERROR(dwError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"inet \" | awk '{print $2}'",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    if(child == 0)
    {
        /* The child */
        close(filedes[0]);
        dup2(filedes[1], 1);
        close(filedes[1]);
        execl("/bin/sh", "sh", "-c", szCommand, (char *)0);
        /* This exit call should not be reached */
        exit(-1);
    }

    /* The parent */
    close(filedes[1]);
    filedes[1] = -1;
    pFile = fdopen(filedes[0], "r");
    if (pFile == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        LWStripWhitespace(szBuf);
        dwError = LWAllocateString(szBuf, ppszIPAddress);
        BAIL_ON_MAC_ERROR(dwError);
    }
    else
    {
        dwError = MAC_AD_ERROR_NO_SUCH_ATTRIBUTE;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    if (pFile != NULL) {
        pclose(pFile);
    }
    if (filedes[0] != -1)
        close(filedes[0]);
    if (filedes[1] != -1)
        close(filedes[1]);
    if (child != 0)
    {
        int status;
        waitpid(child, &status, 0);
    }

    return dwError;
    
error:

    goto cleanup;
}

static
BOOLEAN
GetIsUp(
    PSTR pszAdapterName
    )
{
    DWORD dwError = 0;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};
    BOOLEAN fIsUp = FALSE;

    if (!pszAdapterName) {
        dwError = eParameterError;
        BAIL_ON_MAC_ERROR(dwError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"UP\"",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    if(child == 0)
    {
        /* The child */
        close(filedes[0]);
        dup2(filedes[1], 1);
        close(filedes[1]);
        execl("/bin/sh", "sh", "-c", szCommand, (char *)0);
        /* This exit call should not be reached */
        exit(-1);
    }

    /* The parent */
    close(filedes[1]);
    filedes[1] = -1;
    pFile = fdopen(filedes[0], "r");
    if (pFile == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        fIsUp = TRUE;
    }

cleanup:

    if (pFile != NULL) {
        pclose(pFile);
    }
    if (filedes[0] != -1)
        close(filedes[0]);
    if (filedes[1] != -1)
        close(filedes[1]);
    if (child != 0)
    {
        int status;
        waitpid(child, &status, 0);
    }

    return fIsUp;
    
error:

    fIsUp = FALSE;
    
    goto cleanup;
}

static
BOOLEAN
GetIsRunning(
    PSTR pszAdapterName
    )
{
    DWORD dwError = 0;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};
    BOOLEAN fIsRunning = FALSE;

    if (!pszAdapterName) {
        dwError = eParameterError;
        BAIL_ON_MAC_ERROR(dwError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"RUNNING\"",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    if(child == 0)
    {
        /* The child */
        close(filedes[0]);
        dup2(filedes[1], 1);
        close(filedes[1]);
        execl("/bin/sh", "sh", "-c", szCommand, (char *)0);
        /* This exit call should not be reached */
        exit(-1);
    }

    /* The parent */
    close(filedes[1]);
    filedes[1] = -1;
    pFile = fdopen(filedes[0], "r");
    if (pFile == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        fIsRunning = TRUE;
    }

cleanup:

    if (pFile != NULL) {
        pclose(pFile);
    }
    if (filedes[0] != -1)
        close(filedes[0]);
    if (filedes[1] != -1)
        close(filedes[1]);
    if (child != 0)
    {
        int status;
        waitpid(child, &status, 0);
    }

    return fIsRunning;
    
error:

    fIsRunning = FALSE;

    goto cleanup;
}



