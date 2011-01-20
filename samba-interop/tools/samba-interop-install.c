/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
#include <lwlogging.h>
#include <lwsecurityidentifier.h>
#include <lwtime.h>
#include <lsa/lsapstore-plugin.h>
#include <reg/regutil.h>

#include "samba-pstore-plugin.h"

#define WBCLIENT_FILENAME   "libwbclient.so.0"
#define LWICOMPAT_FILENAME  "lwicompat_v4.so"

#define BAIL_ON_LSA_ERROR(error)                                      \
    if (error) {                                                      \
        LW_LOG_DEBUG("Error in %s at %s:%d. Error code [%d]",          \
                      __FUNCTION__, __FILE__, __LINE__, error);       \
        goto cleanup;                                                     \
    }

static
VOID
LogLwMessageFunc(
    LwLogLevel level,
    PVOID pUserData,
    PCSTR pszMessage
    )
{
    printf("%s\n", pszMessage);
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

    *ppDir = NULL;

    error = FindFileInPath(
                    WBCLIENT_FILENAME,
                    "/usr/lib:/usr/lib64",
                    &pFoundPath);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        error = 0;
    }
    BAIL_ON_LSA_ERROR(error);

    if (pFoundPath)
    {
        pFoundPath[strlen(pFoundPath) - (sizeof(WBCLIENT_FILENAME) -1) - 1] = 0;
        *ppDir = pFoundPath;
        pFoundPath = NULL;
        goto cleanup;
    }

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

    error = ERROR_FILE_NOT_FOUND;
    BAIL_ON_LSA_ERROR(error);

cleanup:
    LW_SAFE_FREE_STRING(pFoundPath);
    return error;
}

DWORD
CheckSambaVersion(
    PCSTR pSmbdPath
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

    LW_LOG_INFO("Found smbd version %s", pVersionString);

    if (!strncmp(pVersionString, "3.2.", sizeof("3.2.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.4.", sizeof("3.4.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.5.", sizeof("3.5.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.0.", sizeof("3.0.") - 1))
    {
        int build = 0;
        sscanf(pVersionString, "3.0.%d.", &build);

        if (build < 25)
        {
            LW_LOG_ERROR("Unsupported smbd version %s", pVersionString);
            error = ERROR_PRODUCT_VERSION;
            BAIL_ON_LSA_ERROR(error);
        }
    }
    else
    {
        LW_LOG_ERROR("Unsupported smbd version %s", pVersionString);
        error = ERROR_PRODUCT_VERSION;
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    LW_SAFE_FREE_STRING(pVersionString);
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
        LW_LOG_INFO("Link %s already points to %s", pWbClient, pBuffer);
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

    LW_LOG_INFO("Linked %s to %s", pWbClient, pLikewiseWbClient);

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
        LW_LOG_INFO("Path %s is not a symbolic link or does not point to %s", pWbClient, pLikewiseWbClient);
        // Already configured
        goto cleanup;
    }

    error = LwAllocateStringPrintf(
            &pWbClientOriginal,
            "%s.lwidentity.orig",
            pWbClient
            );
    BAIL_ON_LSA_ERROR(error);

    if (stat(pWbClientOriginal, &statBuf) < 0)
    {
        LW_LOG_ERROR("Cannot find original wbclient library at %s", pWbClientOriginal);
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
    }

    if (unlink(pWbClient) < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
    }

    if (symlink(pWbClientOriginal, pWbClient) < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
    }

    LW_LOG_INFO("Linked %s to %s", pWbClient, pLikewiseWbClient);

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

    if (!strncmp(pSambaPrivateDir, "PRIVATE_DIR: ", sizeof("PRIVATE_DIR: ") -1))
    {
        memmove(
                pSambaPrivateDir,
                pSambaPrivateDir + (sizeof("PRIVATE_DIR: ") - 1),
                strlen(pSambaPrivateDir) - (sizeof("PRIVATE_DIR: ") - 1) + 1);
    }

    error = LwAllocateStringPrintf(
            &pPath,
            "%s/secrets.tdb",
            pSambaPrivateDir
            );
    BAIL_ON_LSA_ERROR(error);

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
        BAIL_ON_LSA_ERROR(error);   
    }

    LW_LOG_INFO("Linked idmapper %s to %s", pLwiCompat, pLikewiseLwiCompat);

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

    LW_LOG_INFO("Unlinked idmapper %s", pLwiCompat);

cleanup:
    LW_SAFE_FREE_STRING(pSambaDir);
    LW_SAFE_FREE_STRING(pLwiCompat);
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

    error = GetSecretsPath(
        pSmbdPath,
        &pSecretsPath);
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilAddKey(
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\Samba");
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilSetValue(
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\Samba",
                "SecretsPath",
                REG_SZ,
                pSecretsPath,
                strlen(pSecretsPath));
    BAIL_ON_LSA_ERROR(error);

    error = LsaOpenServer(
        &hLsa);
    if (error)
    {
        LW_LOG_ERROR("Unable to contact lsassd");
    }
    BAIL_ON_LSA_ERROR(error);

    error = LsaAdGetMachinePasswordInfo(
        hLsa,
        NULL,
        &pPasswordInfo);
    BAIL_ON_LSA_ERROR(error);

    error = LsaPstorePluginInitializeContext(
                LSA_PSTORE_PLUGIN_VERSION,
                &pDispatch,
                &pContext);
    BAIL_ON_LSA_ERROR(error);

    error = pDispatch->SetPasswordInfoA(
                pContext,
                pPasswordInfo);
    BAIL_ON_LSA_ERROR(error);

cleanup:
    LW_SAFE_FREE_STRING(pSecretsPath);
    if (hLsa != NULL)
    {
        LsaCloseServer(hLsa);
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
        LW_LOG_ERROR("Unable to contact lsassd");
    }
    BAIL_ON_LSA_ERROR(error);

    error = LsaAdGetMachineAccountInfo(
        hLsa,
        NULL,
        &pAccountInfo);
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilAddKey(
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\Samba");
    BAIL_ON_LSA_ERROR(error);

    error = RegUtilSetValue(
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_PATH,
                NULL,
                LSA_PSTORE_REG_ROOT_KEY_RELATIVE_PATH_PLUGINS "\\Samba",
                "SecretsPath",
                REG_SZ,
                pSecretsPath,
                strlen(pSecretsPath));
    BAIL_ON_LSA_ERROR(error);

    error = LsaPstorePluginInitializeContext(
                LSA_PSTORE_PLUGIN_VERSION,
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
    fprintf(stderr, "Usage: %s --help | --check-version | --install | \n",
            pProgramName);
    fprintf(stderr, "         --uninstall [smbd path]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Installs the Likewise-Samba interop libraries into the\n"
            "directories used by Samba, and copies over the machine password\n"
            "from Likewise's database to Samba's.\n");
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
    DWORD logLevel = LW_LOG_LEVEL_ERROR;
    PCSTR pErrorSymbol = NULL;

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
                logLevel = LW_LOG_LEVEL_ERROR;
            }
            else if (!strcmp(argv[argIndex], "warning"))
            {
                logLevel = LW_LOG_LEVEL_WARNING;
            }
            else if (!strcmp(argv[argIndex], "info"))
            {
                logLevel = LW_LOG_LEVEL_INFO;
            }
            else if (!strcmp(argv[argIndex], "verbose"))
            {
                logLevel = LW_LOG_LEVEL_VERBOSE;
            }
            else if (!strcmp(argv[argIndex], "debug"))
            {
                logLevel = LW_LOG_LEVEL_DEBUG;
            }
            else
            {
                error = ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(error);
            }
        }
        else if (argIndex == argc - 1)
        {
            pSmbdPath = argv[2];
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

    LwSetLogFunction(logLevel, LogLwMessageFunc, NULL);

    if (pSmbdPath == NULL)
    {
        error = FindFileInPath(
                        "smbd",
                        "/usr/sbin",
                        &pFoundSmbdPath);
        BAIL_ON_LSA_ERROR(error);
        pSmbdPath = pFoundSmbdPath;
    }

    error = CheckSambaVersion(pSmbdPath);
    BAIL_ON_LSA_ERROR(error);

    if (mode == CHECK_VERSION)
    {
        fprintf(stderr, "Samba version supported\n");
    }
    else if (mode == INSTALL)
    {
        error = InstallWbclient(pSmbdPath);
        BAIL_ON_LSA_ERROR(error);

        error = InstallLwiCompat(pSmbdPath);
        BAIL_ON_LSA_ERROR(error);

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
