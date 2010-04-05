/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "ctbase.h"
#include "../Utilities.h"
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>

static const DWORD BUFSIZE = 4096;

static
long
GetEthernetAddress(
    PSTR pszAdapterName,
    PSTR * ppszENetAddress
    )
{
    long macError = eDSNoErr;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};

    if (!pszAdapterName || !ppszENetAddress) {
        macError = eParameterError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"ether \" | awk '{print $2}'",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
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
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        CTStripWhitespace(szBuf);
        macError = LWAllocateString(szBuf, ppszENetAddress);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = eDSAttributeDoesNotExist;
        goto cleanup;
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

    return macError;
}

static
long
GetIPAddress(
    PSTR pszAdapterName,
    PSTR * ppszIPAddress
    )
{
    long macError = eDSNoErr;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};

    if (!pszAdapterName || !ppszIPAddress) {
        macError = eParameterError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"inet \" | awk '{print $2}'",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
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
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        CTStripWhitespace(szBuf);
        macError = LWAllocateString(szBuf, ppszIPAddress);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        macError = eDSAttributeDoesNotExist;
        goto cleanup;
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

    return macError;
}

static
bool
GetIsUp(
    PSTR pszAdapterName
    )
{
    long macError = eDSNoErr;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};
    bool fIsUp = FALSE;

    if (!pszAdapterName) {
        macError = eParameterError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"UP\"",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
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
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
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

    if (macError)
       return false;

    return fIsUp;
}

static
bool
GetIsRunning(
    PSTR pszAdapterName
    )
{
    long macError = eDSNoErr;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};
    bool fIsRunning = FALSE;

    if (!pszAdapterName) {
        macError = eParameterError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig %s | grep \"RUNNING\"",
            pszAdapterName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
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
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
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

    if (macError)
        return FALSE;

    return fIsRunning;
}

long
CTGetNetAdapterList(
    bool fEnXOnly,
    PNETADAPTERINFO * ppNetAdapterList
    )
{
    long macError = eDSNoErr;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};
    PNETADAPTERINFO pAdapterList = NULL;
    PNETADAPTERINFO pPrev = NULL;
    PNETADAPTERINFO pNew = NULL;
    bool IsEthernet = FALSE;

    if (!ppNetAdapterList) {
        macError = eParameterError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    sprintf(szCommand,
            "/sbin/ifconfig -a | grep \"flags\" | awk '{print $1}'");

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
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
        macError = CTMapSystemError(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    while (!feof(pFile)) {
        IsEthernet = FALSE;
        
        if (NULL != fgets(szBuf, BUFSIZE, pFile)) {
            CTStripWhitespace(szBuf);
            
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
                macError = LWAllocateMemory(sizeof(NETADAPTERINFO), (PVOID*) &pNew);
                GOTO_CLEANUP_ON_MACERROR(macError);

                macError = LWAllocateString(szBuf, &pNew->pszName);
                GOTO_CLEANUP_ON_MACERROR(macError);
                
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
        CTFreeNetAdapterList(pAdapterList);
        
    if (pNew)
        CTFreeNetAdapterList(pNew);

    return macError;
}

void
CTFreeNetAdapterList(
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


