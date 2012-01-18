/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samba-interop-install.c
 *
 * Abstract:
 *
 *        Install program for Likewise Samba interop pieces
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 */

#include "config.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <lsa/lsa.h>
#include <lsa/ad.h>
#include <lwstr.h>
#include <lwmem.h>
#include <lwfile.h>
#include <lwdef.h>
#include <lw/rtllog.h>
#include <lwsecurityidentifier.h>
#include <lwtime.h>
#include <lsa/lsapstore-plugin.h>
#include <reg/regutil.h>

#include "samba-pstore-plugin.h"

#define WBCLIENT_FILENAME   "libwbclient.so.0"
#define LWICOMPAT_FILENAME  "lwicompat_v4.so"

#define BAIL_ON_LSA_ERROR(error)                                      \
    if (error) {                                                      \
        LW_RTL_LOG_DEBUG("Error code %d", error); \
        goto cleanup;                                                     \
    }

#define PLUGIN_PATH (LIBDIR "/lw-pstore/samba" MOD_EXT)

static
VOID
LogCallback(
    IN OPTIONAL PVOID Context,
    IN LW_RTL_LOG_LEVEL Level,
    IN OPTIONAL PCSTR ComponentName,
    IN PCSTR FunctionName,
    IN PCSTR FileName,
    IN ULONG LineNumber,
    IN PCSTR Format,
    IN ...
    )
{
    DWORD dwError = 0;
    PSTR formattedMessage = NULL;
    LW_RTL_LOG_LEVEL maxLevel = LwRtlLogGetLevel();
    va_list argList;

    va_start(argList, Format);
    dwError = LwAllocateStringPrintfV(&formattedMessage, Format, argList);
    va_end(argList);

    if (!dwError)
    {
        size_t length = strlen(formattedMessage);
        if ((length > 0) && (Format[length-1] != '\n'))
        {
            if (maxLevel >= LW_RTL_LOG_LEVEL_DEBUG)
            {
                printf("[%s() %s:%d] %s\n", FunctionName, FileName, LineNumber,
                       formattedMessage);
            }
            else
            {
                printf("%s\n", formattedMessage);
            }
        }
        else
        {
            if (maxLevel >= LW_RTL_LOG_LEVEL_DEBUG)
            {
                printf("[%s() %s:%d] %s", FunctionName, FileName, LineNumber,
                       formattedMessage);
            }
            else
            {
                printf("%s", formattedMessage);
            }
        }
    }

    LW_SAFE_FREE_STRING(formattedMessage);
}

DWORD
FindFileInPath(
    PCSTR pFilename,
    PCSTR pSearchPath,
    PSTR* ppFoundPath
    )
{
    DWORD error = ERROR_SUCCESS;
    //Copy the search path so that strtok can be run on it
    PSTR pMySearchPath = NULL;
    PSTR pStrtokSavePtr = NULL;
    PSTR pCurrentDir = NULL;
    PSTR pTestPath = NULL;
    BOOLEAN exists = FALSE;

    if (ppFoundPath != NULL)
    {
        *ppFoundPath = NULL;
    }

    error = LwAllocateString(pSearchPath, &pMySearchPath);
    BAIL_ON_LSA_ERROR(error);

    pCurrentDir = strtok_r(pMySearchPath, ":", &pStrtokSavePtr);
    while (TRUE)
    {
        LW_SAFE_FREE_STRING(pTestPath);
        error = LwAllocateStringPrintf(
                &pTestPath,
                "%s/%s",
                pCurrentDir,
                pFilename);

        error = LwCheckFileTypeExists(
                    pTestPath,
                    LWFILE_REGULAR,
                    &exists);
        BAIL_ON_LSA_ERROR(error);
        
        if (!exists)
        {
            error = LwCheckFileTypeExists(
                        pTestPath,
                        LWFILE_SYMLINK,
                        &exists);
            BAIL_ON_LSA_ERROR(error);
        }

        if (!exists)
        {
            error = LwCheckFileTypeExists(
                        pTestPath,
                        LWFILE_DIRECTORY,
                        &exists);
            BAIL_ON_LSA_ERROR(error);
        }

        if (exists)
        {
            if (ppFoundPath != NULL)
            {
                *ppFoundPath = pTestPath;
                pTestPath = NULL;
            }
            break;
        }
        pCurrentDir = strtok_r(NULL, ":", &pStrtokSavePtr);
        if(pCurrentDir == NULL)
        {
            error = ERROR_FILE_NOT_FOUND;
            BAIL_ON_LSA_ERROR(error);
        }
    }

cleanup:
    LW_SAFE_FREE_STRING(pMySearchPath);
    LW_SAFE_FREE_STRING(pTestPath);
    return error;
}

DWORD
CaptureOutputWithStderr(
    PCSTR pCommand,
    PCSTR* ppArgs,
    PSTR* ppOutput,
    int *pExitCode
    )
{
    DWORD error = ERROR_SUCCESS;
    size_t bufferCapacity = 1024;
    ssize_t inBuffer = 0;
    ssize_t readCount = 0;
    int pipeFds[2] = { -1, -1 };
    pid_t pid = -1;
    int status = 0;
    PSTR pTempOutput = NULL;
    // Do not free
    PSTR pNewOutput = NULL;

    if (ppOutput != NULL)
    {
        *ppOutput = NULL;
    }
    
    if (pipe(pipeFds))
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
    }
    
    pid = fork();
    
    if (pid < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);     
    }
    else if (pid == 0)
    {
        // Child process
        if (dup2(pipeFds[1], STDOUT_FILENO) < 0)
        {
            abort();
        }
        if (dup2(pipeFds[1], STDERR_FILENO) < 0)
        {
            abort();
        }
        if (close(pipeFds[0]))
        {
            abort();
        }
        if (close(pipeFds[1]))
        {
            abort();
        }
        execvp(pCommand, (char **)ppArgs);
        abort();
    }
    
    if (close(pipeFds[1]))
    {
        pipeFds[1] = -1;
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);     
    }
    pipeFds[1] = -1;
    
    error = LwAllocateMemory(bufferCapacity, (PVOID*) &pTempOutput);
    BAIL_ON_LSA_ERROR(error);
    
    while ((readCount = read(pipeFds[0], pTempOutput + inBuffer, bufferCapacity - inBuffer)) > 0)
    {
        inBuffer += readCount;
        if (inBuffer == bufferCapacity)
        {
            bufferCapacity *= 2;
            error = LwReallocMemory(
                            pTempOutput,
                            (PVOID*)&pNewOutput,
                            bufferCapacity);
            BAIL_ON_LSA_ERROR(error);

            pTempOutput = pNewOutput;
        }
    }
    
    if (readCount < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error); 
    }
    
    if (close(pipeFds[0]) < 0)
    {
        pipeFds[0] = -1;
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error); 
    }
    pipeFds[0] = -1;
    
    if (waitpid(pid, &status, 0) != pid)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);
    }

    if (ppOutput != NULL)
    {
        *ppOutput = pTempOutput;
        pTempOutput = NULL;
    }
    
    if (pExitCode != NULL)
    {
        *pExitCode = WEXITSTATUS(status);
    }
    else if (status)
    {
        error = ERROR_BAD_COMMAND;
        BAIL_ON_LSA_ERROR(error); 
    }
    
cleanup:   
    if (pipeFds[0] >= 0)
    {
        close(pipeFds[0]);
    }
    if (pipeFds[1] >= 0)
    {
        close(pipeFds[1]);
    }
    LW_SAFE_FREE_STRING(pTempOutput);
    return error;
}

DWORD
GetWbclientDir(
    PCSTR pSmbdPath,
    PSTR* ppDir
    )
{
    PCSTR ppBackupPaths[] = {
        "/usr/lib",
        "/usr/lib64",
        NULL,
    };
    DWORD index = 0;
    DWORD error = 0;
    BOOLEAN exists = 0;
    PSTR pFoundPath = NULL;
    PSTR pCommandLine = NULL;
    PCSTR ppArgs[] = {
        "/bin/sh",
        "-c",
        NULL,
        NULL
    };
    PSTR pSambaLibdir = NULL;

    *ppDir = NULL;

    // First see if libwbclient.so.0 is in Samba's libdir. There may be two
    // copies of libwbclient.so.0 because of different architectures. This will
    // identify which one is the primary one.
    error = LwAllocateStringPrintf(
            &pCommandLine,
            "%s -b | grep LIBDIR:",
            pSmbdPath
            );
    BAIL_ON_LSA_ERROR(error);

    ppArgs[2] = pCommandLine;

    error = CaptureOutputWithStderr(
                "/bin/sh",
                ppArgs,
                &pSambaLibdir,
                NULL);
    BAIL_ON_LSA_ERROR(error);

    LwStripWhitespace(
            pSambaLibdir,
            TRUE,
            TRUE);

    if (strstr(pSambaLibdir, ": "))
    {
        char *pValueStart = strstr(pSambaLibdir, ": ") + 2;
        memmove(
                pSambaLibdir,
                pValueStart,
                strlen(pSambaLibdir) - (pValueStart - pSambaLibdir) + 1);
    }

    error = FindFileInPath(
                    WBCLIENT_FILENAME,
                    pSambaLibdir,
                    &pFoundPath);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        // Fall back to trying the two standard system paths
        error = FindFileInPath(
                        WBCLIENT_FILENAME,
                        "/usr/lib:/usr/lib64",
                        &pFoundPath);
        if (error == ERROR_FILE_NOT_FOUND)
        {
            error = 0;
        }
    }
    BAIL_ON_LSA_ERROR(error);

    if (pFoundPath)
    {
        pFoundPath[strlen(pFoundPath) - (sizeof(WBCLIENT_FILENAME) -1) - 1] = 0;
        *ppDir = pFoundPath;
        pFoundPath = NULL;
        goto cleanup;
    }

    // Could not find an existing libwbclient.so.0. This could be a Samba 3.0.x
    // build. Just stick the file in a system path.
    for (index = 0; ppBackupPaths[index]; index++)
    {
        error = LwCheckFileTypeExists(
                    ppBackupPaths[index],
                    LWFILE_DIRECTORY,
                    &exists);
        BAIL_ON_LSA_ERROR(error);

        if (exists)
        {
            error = LwAllocateString(ppBackupPaths[index], ppDir);
            BAIL_ON_LSA_ERROR(error);
            goto cleanup;
        }
    }

    // Could not find the system library paths.
    error = ERROR_FILE_NOT_FOUND;
    BAIL_ON_LSA_ERROR(error);

cleanup:
    LW_SAFE_FREE_STRING(pFoundPath);
    LW_SAFE_FREE_STRING(pCommandLine);
    LW_SAFE_FREE_STRING(pSambaLibdir);
    return error;
}

DWORD
CheckSambaVersion(
    PCSTR pSmbdPath,
    PSTR *ppVersion
    )
{
    DWORD error = 0;
    PCSTR ppArgs[] = {
        pSmbdPath,
        "-V",
        0
    };
    PSTR pVersionString = NULL;

    error = CaptureOutputWithStderr(
                pSmbdPath,
                ppArgs,
                &pVersionString,
                NULL);
    BAIL_ON_LSA_ERROR(error);

    if (!strncmp(pVersionString, "Version ", sizeof("Version ") -1))
    {
        memmove(
                pVersionString,
                pVersionString + (sizeof("Version ") - 1),
                strlen(pVersionString) - (sizeof("Version ") - 1) + 1);
    }
    LwStripWhitespace(
            pVersionString,
            TRUE,
            TRUE);

    LW_RTL_LOG_ERROR("Found smbd version %s", pVersionString);

    if (!strncmp(pVersionString, "3.2.", sizeof("3.2.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.4.", sizeof("3.4.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.5.", sizeof("3.5.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.6.", sizeof("3.6.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.0.", sizeof("3.0.") - 1))
    {
        int build = 0;
        sscanf(pVersionString, "3.0.%d.", &build);

        if (build < 25)
        {
            LW_RTL_LOG_ERROR("Unsupported smbd version %s", pVersionString);
            error = ERROR_PRODUCT_VERSION;
            BAIL_ON_LSA_ERROR(error);
        }
    }
    else
    {
        LW_RTL_LOG_ERROR("Unsupported smbd version %s", pVersionString);
        error = ERROR_PRODUCT_VERSION;
        BAIL_ON_LSA_ERROR(error);
    }


cleanup:
    if (error)
    {
        LW_SAFE_FREE_STRING(pVersionString);
    }
    *ppVersion = pVersionString;
    return error;
}

DWORD
InstallWbclient(
    PCSTR pSmbdPath
    )
{
    DWORD error = 0;
    PSTR pSambaDir = NULL;
    PSTR pWbClient = NULL;
    PSTR pWbClientOriginal = NULL;
    PCSTR pLikewiseWbClient = LIBDIR "/" WBCLIENT_FILENAME;
    char pBuffer[1024] = { 0 };

    error = GetWbclientDir(
                pSmbdPath,
                &pSambaDir);
    BAIL_ON_LSA_ERROR(error);

    error = LwAllocateStringPrintf(
            &pWbClient,
            "%s/%s",
            pSambaDir,
            WBCLIENT_FILENAME
            );
    BAIL_ON_LSA_ERROR(error);

    if (readlink(pWbClient, pBuffer, sizeof(pBuffer)) < 0)
    {
        switch(errno)
        {
            // File does not exist
            case ENOENT:
            // Not a symbolic link
            case EINVAL:
                pBuffer[0] = 0;
                break;
            default:
                error = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(error);
        }
    }
    pBuffer[sizeof(pBuffer) - 1] = 0;

    if (!strcmp(pBuffer, pLikewiseWbClient))
    {
        LW_RTL_LOG_INFO("Link %s already points to %s", pWbClient, pBuffer);
        // Already configured
        goto cleanup;
    }

    error = LwAllocateStringPrintf(
            &pWbClientOriginal,
            "%s.lwidentity.orig",
            pWbClient
            );
    BAIL_ON_LSA_ERROR(error);

    if (!strcmp(pBuffer, pWbClientOriginal))
    {
        if (unlink(pWbClient) < 0)
        {
            error = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(error);   
        }
    }
    else
    {
        if (rename(pWbClient, pWbClientOriginal) < 0)
        {
            if (errno != ENOENT)
            {
                error = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(error);   
            }
        }
    }

    if (symlink(pLikewiseWbClient, pWbClient) < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
    }

    LW_RTL_LOG_INFO("Linked %s to %s", pWbClient, pLikewiseWbClient);

cleanup:
    LW_SAFE_FREE_STRING(pSambaDir);
    LW_SAFE_FREE_STRING(pWbClient);
    LW_SAFE_FREE_STRING(pWbClientOriginal);
    return error;
}

DWORD
UninstallWbclient(
    PCSTR pSmbdPath
    )
{
    DWORD error = 0;
    PSTR pSambaDir = NULL;
    PSTR pWbClient = NULL;
    PSTR pWbClientOriginal = NULL;
    PCSTR pLikewiseWbClient = LIBDIR "/" WBCLIENT_FILENAME;
    char pBuffer[1024] = { 0 };
    struct stat statBuf = { 0 };

    error = GetWbclientDir(
                pSmbdPath,
                &pSambaDir);
    BAIL_ON_LSA_ERROR(error);

    error = LwAllocateStringPrintf(
            &pWbClient,
            "%s/%s",
            pSambaDir,
            WBCLIENT_FILENAME
            );
    BAIL_ON_LSA_ERROR(error);

    if (readlink(pWbClient, pBuffer, sizeof(pBuffer)) < 0)
    {
        switch(errno)
        {
            // File does not exist
            case ENOENT:
            // Not a symbolic link
            case EINVAL:
                pBuffer[0] = 0;
                break;
            default:
                error = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(error);
        }
    }
    pBuffer[sizeof(pBuffer) - 1] = 0;

    if (strcmp(pBuffer, pLikewiseWbClient))
    {
        LW_RTL_LOG_INFO("Path %s is not a symbolic link or does not point to %s", pWbClient, pLikewiseWbClient);
        // Already configured
        goto cleanup;
    }

    error = LwAllocateStringPrintf(
            &pWbClientOriginal,
            "%s.lwidentity.orig",
            pWbClient
            );
    BAIL_ON_LSA_ERROR(error);

    if (unlink(pWbClient) < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
    }

    if (stat(pWbClientOriginal, &statBuf) < 0)
    {
        if (errno == ENOENT)
        {
            // This is probably Samba 3.0.x, and it did not have an original
            // libwbclient.so.
        }
        else
        {
            LW_RTL_LOG_ERROR("Cannot find original wbclient library at %s",
                    pWbClientOriginal);
            error = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(error);   
        }
    }
    else
    {
        if (symlink(pWbClientOriginal, pWbClient) < 0)
        {
            error = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(error);   
        }

        LW_RTL_LOG_INFO("Linked %s to %s", pWbClient, pLikewiseWbClient);
    }

cleanup:
    LW_SAFE_FREE_STRING(pSambaDir);
    LW_SAFE_FREE_STRING(pWbClient);
    LW_SAFE_FREE_STRING(pWbClientOriginal);
    return error;
}

DWORD
GetIdmapDir(
    PCSTR pSmbdPath,
    PSTR* ppDir
    )
{
    DWORD error = 0;
    PSTR pCommandLine = NULL;
    PCSTR ppArgs[] = {
        "/bin/sh",
        "-c",
        NULL,
        NULL
    };
    PSTR pSambaLibdir = NULL;
    PSTR pDir = NULL;

    error = LwAllocateStringPrintf(
            &pCommandLine,
            "%s -b | grep MODULESDIR:",
            pSmbdPath
            );
    BAIL_ON_LSA_ERROR(error);

    ppArgs[2] = pCommandLine;

    error = CaptureOutputWithStderr(
                "/bin/sh",
                ppArgs,
                &pSambaLibdir,
                NULL);
    if (error == ERROR_BAD_COMMAND)
    {
        // This version of smbd is older than 3.4. Try looking for the LIBDIR
        // instead.
        LW_SAFE_FREE_STRING(pCommandLine);

        error = LwAllocateStringPrintf(
                &pCommandLine,
                "%s -b | grep LIBDIR:",
                pSmbdPath
                );
        BAIL_ON_LSA_ERROR(error);

        ppArgs[2] = pCommandLine;

        error = CaptureOutputWithStderr(
                    "/bin/sh",
                    ppArgs,
                    &pSambaLibdir,
                    NULL);
    }
    BAIL_ON_LSA_ERROR(error);

    LwStripWhitespace(
            pSambaLibdir,
            TRUE,
            TRUE);

    if (strstr(pSambaLibdir, ": "))
    {
        char *pValueStart = strstr(pSambaLibdir, ": ") + 2;
        memmove(
                pSambaLibdir,
                pValueStart,
                strlen(pSambaLibdir) - (pValueStart - pSambaLibdir) + 1);
    }

    error = LwAllocateStringPrintf(
            &pDir,
            "%s/idmap",
            pSambaLibdir
            );
    BAIL_ON_LSA_ERROR(error);

cleanup:
    *ppDir = pDir;
    LW_SAFE_FREE_STRING(pCommandLine);
    LW_SAFE_FREE_STRING(pSambaLibdir);
    return error;
}

DWORD
GetSecretsPath(
    PCSTR pSmbdPath,
    PSTR* ppPath
    )
{
    DWORD error = 0;
    PSTR pCommandLine = NULL;
    PCSTR ppArgs[] = {
        "/bin/sh",
        "-c",
        NULL,
        NULL
    };
    PSTR pSambaPrivateDir = NULL;
    PSTR pPath = NULL;
    struct stat statBuf = { 0 };

    // Look for secrets.tdb in the statedir (Ubuntu 10.10 is like this)
    error = LwAllocateStringPrintf(
            &pCommandLine,
            "%s -b | grep STATEDIR:",
            pSmbdPath
            );
    BAIL_ON_LSA_ERROR(error);

    ppArgs[2] = pCommandLine;

    error = CaptureOutputWithStderr(
                "/bin/sh",
                ppArgs,
                &pSambaPrivateDir,
                NULL);
    if (error == ERROR_BAD_COMMAND)
    {
        pSambaPrivateDir = NULL;
        error = ERROR_BAD_COMMAND;
    }
    else
    {
        if (strstr(pSambaPrivateDir, ": "))
        {
            char *pValueStart = strstr(pSambaPrivateDir, ": ") + 2;
            memmove(
                    pSambaPrivateDir,
                    pValueStart,
                    strlen(pSambaPrivateDir) -
                        (pValueStart - pSambaPrivateDir) + 1);
        }

        LwStripWhitespace(
                pSambaPrivateDir,
                TRUE,
                TRUE);

        error = LwAllocateStringPrintf(
                &pPath,
                "%s/secrets.tdb",
                pSambaPrivateDir
                );
        BAIL_ON_LSA_ERROR(error);
        
        // Verify the path exists
        if (stat(pPath, &statBuf) < 0)
        {
            if (errno == ENOENT)
            {
                // Try the private dir instead
                LW_SAFE_FREE_STRING(pSambaPrivateDir);
                LW_SAFE_FREE_STRING(pPath);
            }
            else
            {
                LW_RTL_LOG_ERROR("Cannot find secrets.tdb at %s",
                        pPath);
                error = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(error);   
            }
        }
    }

    if (pPath == NULL)
    {
        // This version of smbd is older than 3.5, or the distro vendor decided
        // to put the file in the private dir (Fedora 14 is like that).
        LW_SAFE_FREE_STRING(pCommandLine);

        error = LwAllocateStringPrintf(
                &pCommandLine,
                "%s -b | grep PRIVATE_DIR:",
                pSmbdPath
                );
        BAIL_ON_LSA_ERROR(error);

        ppArgs[2] = pCommandLine;

        error = CaptureOutputWithStderr(
                    "/bin/sh",
                    ppArgs,
                    &pSambaPrivateDir,
                    NULL);
        BAIL_ON_LSA_ERROR(error);

        LwStripWhitespace(
                pSambaPrivateDir,
                TRUE,
                TRUE);

        if (strstr(pSambaPrivateDir, ": "))
        {
            char *pValueStart = strstr(pSambaPrivateDir, ": ") + 2;
            memmove(
                    pSambaPrivateDir,
                    pValueStart,
                    strlen(pSambaPrivateDir) -
                        (pValueStart - pSambaPrivateDir) + 1);
        }

        error = LwAllocateStringPrintf(
                &pPath,
                "%s/secrets.tdb",
                pSambaPrivateDir
                );
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    *ppPath = pPath;
    LW_SAFE_FREE_STRING(pCommandLine);
    LW_SAFE_FREE_STRING(pSambaPrivateDir);
    return error;
}

DWORD
InstallLwiCompat(
    PCSTR pSmbdPath
    )
{
    DWORD error = 0;
    PSTR pSambaDir = NULL;
    PSTR pLwiCompat = NULL;
    PCSTR pLikewiseLwiCompat = LIBDIR "/" LWICOMPAT_FILENAME;

    error = GetIdmapDir(
                pSmbdPath,
                &pSambaDir);
    BAIL_ON_LSA_ERROR(error);

    error = LwAllocateStringPrintf(
            &pLwiCompat,
            "%s/%s",
            pSambaDir,
            LWICOMPAT_FILENAME
            );
    BAIL_ON_LSA_ERROR(error);

    if (unlink(pLwiCompat) < 0)
    {
        if (errno != ENOENT)
        {
            error = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(error);   
        }
    }

    if (symlink(pLikewiseLwiCompat, pLwiCompat) < 0)
    {
        error = LwMapErrnoToLwError(errno);
        if (error == ERROR_FILE_NOT_FOUND)
        {
            LW_RTL_LOG_ERROR("Cannot access idmap directory %s. Please ensure you have winbind installed", pSambaDir);
        }
        BAIL_ON_LSA_ERROR(error);   
    }

    LW_RTL_LOG_INFO("Linked idmapper %s to %s", pLwiCompat, pLikewiseLwiCompat);

cleanup:
    LW_SAFE_FREE_STRING(pSambaDir);
    LW_SAFE_FREE_STRING(pLwiCompat);
    return error;
}

DWORD
UninstallLwiCompat(
    PCSTR pSmbdPath
    )
{
    DWORD error = 0;
    PSTR pSambaDir = NULL;
    PSTR pLwiCompat = NULL;

    error = GetIdmapDir(
                pSmbdPath,
                &pSambaDir);
    BAIL_ON_LSA_ERROR(error);

    error = LwAllocateStringPrintf(
            &pLwiCompat,
            "%s/%s",
            pSambaDir,
            LWICOMPAT_FILENAME
            );
    BAIL_ON_LSA_ERROR(error);

    if (unlink(pLwiCompat) < 0)
    {
        if (errno != ENOENT)
        {
            error = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(error);   
        }
    }

    LW_RTL_LOG_INFO("Unlinked idmapper %s", pLwiCompat);

cleanup:
    LW_SAFE_FREE_STRING(pSambaDir);
    LW_SAFE_FREE_STRING(pLwiCompat);
    return error;
}

static
DWORD
AddSambaLoadPath(
    IN HANDLE hReg
    )
{
    DWORD type = 0;
    HKEY hKey = NULL;
    DWORD error = 0;
    DWORD loadOrderSize = 0;
    PSTR pLoadOrder = NULL;
    DWORD newLoadOrderSize = 0;
    PSTR pNewLoadOrder = NULL;
    PCSTR pPos = NULL;

    error = LwRegOpenKeyExA(
                hReg,
                NULL,
                LSA_PSTORE_REG_KEY_PATH_PLUGINS,
                0,
                KEY_WRITE,
                &hKey);
    BAIL_ON_LSA_ERROR(error);

    error = LwRegGetValueA(
                hReg,
                hKey,
                NULL,
                "LoadOrder",
                RRF_RT_REG_MULTI_SZ,
                &type,
                NULL,
                &loadOrderSize);
    if (error == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        loadOrderSize = 1;
        error = LwAllocateMemory(loadOrderSize, (PVOID*) &pLoadOrder);
        BAIL_ON_LSA_ERROR(error);
        // pLoadOrder is already memset to 0

        error = 0;
    }
    else
    {
        BAIL_ON_LSA_ERROR(error);

        error = LwAllocateMemory(loadOrderSize, (PVOID*) &pLoadOrder);
        BAIL_ON_LSA_ERROR(error);

        error = LwRegGetValueA(
                    hReg,
                    hKey,
                    NULL,
                    "LoadOrder",
                    RRF_RT_REG_MULTI_SZ,
                    &type,
                    pLoadOrder,
                    &loadOrderSize);
        BAIL_ON_LSA_ERROR(error);
    }

    pPos = pLoadOrder;
    while (pPos[0])
    {
        if (!strcmp(pPos, PLUGIN_NAME))
        {
            LW_RTL_LOG_INFO("Samba is already in the load order");
            goto cleanup;
        }
        pPos += strlen(pPos) + 1;
    }

    newLoadOrderSize = loadOrderSize + strlen(PLUGIN_NAME) + 1;
    error = LwAllocateMemory(newLoadOrderSize, (PVOID*) &pNewLoadOrder);
    BAIL_ON_LSA_ERROR(error);

    memcpy(pNewLoadOrder, PLUGIN_NAME, strlen(PLUGIN_NAME) + 1);
    memcpy(pNewLoadOrder + strlen(PLUGIN_NAME) + 1, pLoadOrder, loadOrderSize);

    error = LwRegSetValueExA(
        hReg,
        hKey,
        "LoadOrder",
        0,
        REG_MULTI_SZ,
        (const BYTE*)pNewLoadOrder,
        newLoadOrderSize);
    BAIL_ON_LSA_ERROR(error);

cleanup:
    if (hKey != NULL)
    {
        LwRegCloseKey(
                hReg,
                hKey);
    }
    LW_SAFE_FREE_STRING(pLoadOrder);
    LW_SAFE_FREE_STRING(pNewLoadOrder);

    return error;
}

static
DWORD
RemoveSambaLoadPath(
    IN HANDLE hReg
    )
{
    DWORD type = 0;
    HKEY hKey = NULL;
    DWORD error = 0;
    DWORD loadOrderSize = 0;
    PSTR pLoadOrder = NULL;
    // Do not free
    PSTR pPos = NULL;
    BOOLEAN removedSamba = FALSE;

    error = LwRegOpenKeyExA(
                hReg,
                NULL,
                LSA_PSTORE_REG_KEY_PATH_PLUGINS,
                0,
                KEY_WRITE,
                &hKey);
    BAIL_ON_LSA_ERROR(error);

    error = LwRegGetValueA(
                hReg,
                hKey,
                NULL,
                "LoadOrder",
                RRF_RT_REG_MULTI_SZ,
                &type,
                NULL,
                &loadOrderSize);
    if (error == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        LW_RTL_LOG_INFO("LoadOrder key not present");
        error = 0;
        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(error);

    error = LwAllocateMemory(loadOrderSize, (PVOID*) &pLoadOrder);
    BAIL_ON_LSA_ERROR(error);

    error = LwRegGetValueA(
                hReg,
                hKey,
                NULL,
                "LoadOrder",
                RRF_RT_REG_MULTI_SZ,
                &type,
                pLoadOrder,
                &loadOrderSize);
    BAIL_ON_LSA_ERROR(error);

    pPos = pLoadOrder;
    while (pPos[0])
    {
        DWORD valueLen = strlen(pPos) + 1;

        if (!strcmp(pPos, PLUGIN_NAME))
        {
            loadOrderSize -= valueLen;
            memmove(
                    pPos,
                    pPos + valueLen,
                    valueLen);
            removedSamba = TRUE;
        }
        else
        {
            pPos += valueLen;
        }
    }

    if (removedSamba)
    {
        LW_RTL_LOG_INFO("Removed Samba from load order");
        error = LwRegSetValueExA(
            hReg,
            hKey,
            "LoadOrder",
            0,
            REG_MULTI_SZ,
            (const BYTE*)pLoadOrder,
            loadOrderSize);
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    if (hKey != NULL)
    {
        LwRegCloseKey(
                hReg,
                hKey);
    }
    LW_SAFE_FREE_STRING(pLoadOrder);

    return error;
}

DWORD
SynchronizePassword(
    PCSTR pSmbdPath
    )
{
    DWORD error = 0;
    PSTR pSecretsPath = NULL;
    LW_HANDLE hLsa = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;
    PLSA_PSTORE_PLUGIN_DISPATCH pDispatch = NULL;
    PLSA_PSTORE_PLUGIN_CONTEXT pContext = NULL;
    HANDLE hReg = NULL;

    error = LwRegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(error);

    error = GetSecretsPath(
        pSmbdPath,
        &pSecretsPath);
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilAddKey(
                hReg,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\" PLUGIN_NAME);
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilSetValue(
                hReg,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\" PLUGIN_NAME,
                "SecretsPath",
                REG_SZ,
                pSecretsPath,
                strlen(pSecretsPath));
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilSetValue(
                hReg,
                HKEY_THIS_MACHINE,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\" PLUGIN_NAME,
                "Path",
                REG_SZ,
                PLUGIN_PATH,
                strlen(PLUGIN_PATH));
    BAIL_ON_LSA_ERROR(error);

    error = AddSambaLoadPath(hReg);
    BAIL_ON_LSA_ERROR(error);

    error = LsaOpenServer(
        &hLsa);
    if (error)
    {
        LW_RTL_LOG_ERROR("Unable to contact lsassd");
    }
    BAIL_ON_LSA_ERROR(error);

    error = LsaAdGetMachinePasswordInfo(
        hLsa,
        NULL,
        &pPasswordInfo);
    if (error == NERR_SetupNotJoined)
    {
        LW_RTL_LOG_ERROR("Unable to write machine password in secrets.tdb because PowerBroker Identity Services is not joined. The password will be written to secrets.tdb on the next successful join attempt");
        error = 0;
    }
    else
    {
        BAIL_ON_LSA_ERROR(error);

        error = LsaPstorePluginInitializeContext(
                    LSA_PSTORE_PLUGIN_VERSION,
                    PLUGIN_NAME,
                    &pDispatch,
                    &pContext);
        BAIL_ON_LSA_ERROR(error);

        error = pDispatch->SetPasswordInfoA(
                    pContext,
                    pPasswordInfo);
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    LW_SAFE_FREE_STRING(pSecretsPath);
    if (hLsa != NULL)
    {
        LsaCloseServer(hLsa);
    }
    if (hReg != NULL)
    {
        LwRegCloseServer(hReg);
    }
    if (pPasswordInfo != NULL)
    {
        LsaAdFreeMachinePasswordInfo(pPasswordInfo);
    }
    if (pContext)
    {
        pDispatch->Cleanup(pContext);
    }
    return error;
}

DWORD
DeletePassword(
    PCSTR pSmbdPath
    )
{
    DWORD error = 0;
    PLSA_PSTORE_PLUGIN_DISPATCH pDispatch = NULL;
    PLSA_PSTORE_PLUGIN_CONTEXT pContext = NULL;
    PSTR pSecretsPath = NULL;
    LW_HANDLE hLsa = NULL;
    PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo = NULL;
    HANDLE hReg = NULL;

    error = LwRegOpenServer(&hReg);
    BAIL_ON_LSA_ERROR(error);

    // Even though this was set during the install process, we'll try setting
    // it again. This way if the user calls uninstall without calling install
    // first, they won't get an error.
    error = GetSecretsPath(
        pSmbdPath,
        &pSecretsPath);
    BAIL_ON_LSA_ERROR(error);

    error = LsaOpenServer(
        &hLsa);
    if (error)
    {
        LW_RTL_LOG_ERROR("Unable to contact lsassd");
    }
    BAIL_ON_LSA_ERROR(error);

    error = LsaAdGetMachineAccountInfo(
        hLsa,
        NULL,
        &pAccountInfo);
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilAddKey(
                hReg,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\" PLUGIN_NAME);
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilSetValue(
                hReg,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\" PLUGIN_NAME,
                "SecretsPath",
                REG_SZ,
                pSecretsPath,
                strlen(pSecretsPath));
    BAIL_ON_LSA_ERROR(error);

    error = RemoveSambaLoadPath(hReg);
    BAIL_ON_LSA_ERROR(error);

    error = LsaPstorePluginInitializeContext(
                LSA_PSTORE_PLUGIN_VERSION,
                PLUGIN_NAME,
                &pDispatch,
                &pContext);
    BAIL_ON_LSA_ERROR(error);

    error = pDispatch->DeletePasswordInfoA(
                pContext,
                pAccountInfo);
    BAIL_ON_LSA_ERROR(error);

cleanup:
    if (pContext)
    {
        pDispatch->Cleanup(pContext);
    }
    if (hReg != NULL)
    {
        LwRegCloseServer(hReg);
    }
    if (hLsa != NULL)
    {
        LsaCloseServer(hLsa);
    }
    if (pAccountInfo != NULL)
    {
        LsaAdFreeMachineAccountInfo(pAccountInfo);
    }
    return error;
}

void
ShowUsage(
    PCSTR pProgramName
    )
{
    fprintf(stderr, "Usage: %s {options} [smbd path]\n", pProgramName);
    fprintf(stderr, "\n");
    fputs(
"Installs interop libraries into directories used by Samba and copies the\n"
"machine password from the PowerBroker Identity Services' database to Samba's.\n",
    stderr);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options are:\n");
    fprintf(stderr, "    --help               Show this help message\n");
    fprintf(stderr, "    --install            Configure smbd to use interop libraries\n");
    fprintf(stderr, "    --uninstall          Deconfigure smbd's use of interop libraries\n");
    fprintf(stderr, "    --check-version      Ensure the version of smbd is supported\n");
    fprintf(stderr, "    --loglevel {level}   Set the logging to error (default), warning, info,\n");
    fprintf(stderr, "                         verbose, or debug\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "One of the options, --install, --uninstall, or --check-version must be passed.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "The last argument is the path to smbd. If not specified, it will be\n");
    fprintf(stderr, "automatically detected.\n");
}

int
main(
    int argc,
    char *argv[]
    )
{
    enum
    {
        UNSET,
        SHOW_HELP,
        CHECK_VERSION,
        INSTALL,
        UNINSTALL
    } mode = UNSET;
    PCSTR pSmbdPath = NULL;
    PSTR pFoundSmbdPath = NULL;
    DWORD error = 0;
    DWORD argIndex = 0;
    LW_RTL_LOG_LEVEL logLevel = LW_RTL_LOG_LEVEL_ERROR;
    PCSTR pErrorSymbol = NULL;
    PSTR pVersion = NULL;
    BOOLEAN smbdExists = FALSE;

    for (argIndex = 1; argIndex < argc; argIndex++)
    {
        if (!strcmp(argv[argIndex], "--check-version"))
        {
            if (mode == UNSET)
            {
                mode = CHECK_VERSION;
            }
            else
            {
                mode = SHOW_HELP;
            }
        }
        else if (!strcmp(argv[argIndex], "--install"))
        {
            if (mode == UNSET)
            {
                mode = INSTALL;
            }
            else
            {
                mode = SHOW_HELP;
            }
        }
        else if (!strcmp(argv[argIndex], "--uninstall"))
        {
            if (mode == UNSET)
            {
                mode = UNINSTALL;
            }
            else
            {
                mode = SHOW_HELP;
            }
        }
        else if (!strcmp(argv[argIndex], "--loglevel"))
        {
            argIndex++;
            if (argIndex >= argc)
            {
                error = ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(error);
            }
            if (!strcmp(argv[argIndex], "error"))
            {
                logLevel = LW_RTL_LOG_LEVEL_ERROR;
            }
            else if (!strcmp(argv[argIndex], "warning"))
            {
                logLevel = LW_RTL_LOG_LEVEL_WARNING;
            }
            else if (!strcmp(argv[argIndex], "info"))
            {
                logLevel = LW_RTL_LOG_LEVEL_INFO;
            }
            else if (!strcmp(argv[argIndex], "verbose"))
            {
                logLevel = LW_RTL_LOG_LEVEL_VERBOSE;
            }
            else if (!strcmp(argv[argIndex], "debug"))
            {
                logLevel = LW_RTL_LOG_LEVEL_DEBUG;
            }
            else
            {
                error = ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(error);
            }
        }
        else if (argIndex == argc - 1)
        {
            pSmbdPath = argv[argIndex];
        }
        else
        {
            mode = SHOW_HELP;
        }
    }

    if (mode == UNSET || mode == SHOW_HELP)
    {
        ShowUsage(argv[0]);
        goto cleanup;
    }

    LwRtlLogSetCallback(LogCallback, NULL);
    LwRtlLogSetLevel(logLevel);

    if (pSmbdPath == NULL)
    {
        PCSTR pSearchPath = "/usr/sbin:/usr/local/sbin:/usr/local/samba/sbin:/opt/csw/samba/sbin:/opt/sfw/samba/sbin:/opt/csw/bin:/usr/local/bin";
        error = FindFileInPath(
                        "smbd",
                        pSearchPath,
                        &pFoundSmbdPath);
        if (error == ERROR_FILE_NOT_FOUND)
        {
            LW_RTL_LOG_ERROR("The smbd file could not be automatically found on your system. The search path was '%s'. Pass the correct location as the last argument to this program.", pSearchPath);
        }
        BAIL_ON_LSA_ERROR(error);
        pSmbdPath = pFoundSmbdPath;
    }

    error = LwCheckFileTypeExists(
                pSmbdPath,
                LWFILE_REGULAR,
                &smbdExists);
    BAIL_ON_LSA_ERROR(error);
    
    if (!smbdExists)
    {
        error = LwCheckFileTypeExists(
                    pSmbdPath,
                    LWFILE_SYMLINK,
                    &smbdExists);
        BAIL_ON_LSA_ERROR(error);
    }

    if (!smbdExists)
    {
        LW_RTL_LOG_ERROR("Smbd file not found at path '%s'", pSmbdPath);
    }

    error = CheckSambaVersion(pSmbdPath, &pVersion);
    BAIL_ON_LSA_ERROR(error);

    if (mode == CHECK_VERSION)
    {
        fprintf(stderr, "Samba version supported\n");
    }
    else if (mode == INSTALL)
    {
        error = InstallWbclient(pSmbdPath);
        BAIL_ON_LSA_ERROR(error);

        if (!strncmp(pVersion, "3.0.", sizeof("3.0.") - 1))
        {
            // Only Samba 3.0.x needs this
            error = InstallLwiCompat(pSmbdPath);
            BAIL_ON_LSA_ERROR(error);
        }

        error = SynchronizePassword(
                    pSmbdPath);
        BAIL_ON_LSA_ERROR(error);

        fprintf(stderr, "Install successful\n");
    }
    else if (mode == UNINSTALL)
    {
        error = UninstallWbclient(pSmbdPath);
        BAIL_ON_LSA_ERROR(error);

        error = UninstallLwiCompat(pSmbdPath);
        BAIL_ON_LSA_ERROR(error);

        error = DeletePassword(
                    pSmbdPath);
        BAIL_ON_LSA_ERROR(error);

        fprintf(stderr, "Uninstall successful\n");
    }
    else
    {
        fprintf(stderr, "Uninstall mode not implemented\n");
        error = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    LW_SAFE_FREE_STRING(pFoundSmbdPath);
    LW_SAFE_FREE_STRING(pVersion);

    if (error)
    {
        pErrorSymbol = LwWin32ErrorToName(error);
        if (pErrorSymbol != NULL)
        {
            fprintf(stderr, "Error: %s\n", pErrorSymbol);
        }
        else
        {
            fprintf(stderr, "Unknown error\n");
        }
    }
    return error;
}
