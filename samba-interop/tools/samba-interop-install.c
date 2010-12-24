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

#include <lw/swab.h>

#include <tdb.h>

#define WBCLIENT_FILENAME   "libwbclient.so.0"
#define LWICOMPAT_FILENAME  "lwicompat_v4.so"

#define BAIL_ON_LSA_ERROR(x)    if ((x) != 0) { goto cleanup; }

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
            if (*ppFoundPath != NULL)
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
        pFoundPath[strlen(pFoundPath) - (sizeof(WBCLIENT_FILENAME) -1)] = 0;
        *ppDir = pFoundPath;
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
                strlen(pVersionString) - (sizeof("Version ") - 1));
    }
    LwStripWhitespace(
            pVersionString,
            TRUE,
            TRUE);

    LW_LOG_INFO("Found smbd version %s", pVersionString);

    if (!strncmp(pVersionString, "3.2.", sizeof("3.2.") - 1))
    {
    }
    else if (!strncmp(pVersionString, "3.0.25.", sizeof("3.0.25.") - 1))
    {
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
    PCSTR pLikewiseWbClient = LIBDIR WBCLIENT_FILENAME;
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
        // Already configured
        goto cleanup;
    }

    error = LwAllocateStringPrintf(
            &pWbClientOriginal,
            "%s.lwidentity.orig",
            pWbClient
            );
    BAIL_ON_LSA_ERROR(error);

    if (rename(pWbClient, pWbClientOriginal) < 0)
    {
        if (errno != ENOENT)
        {
            error = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(error);   
        }
    }

    if (symlink(pLikewiseWbClient, pWbClient) < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
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
        "/bin/sh"
        "-c",
        NULL,
        NULL
    };
    PSTR pSambaLibdir = NULL;
    PSTR pDir = NULL;

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

    if (!strncmp(pSambaLibdir, "LIBDIR: ", sizeof("LIBDIR: ") -1))
    {
        memmove(
                pSambaLibdir,
                pSambaLibdir + (sizeof("LIBDIR: ") - 1),
                strlen(pSambaLibdir) - (sizeof("LIBDIR: ") - 1));
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
        "/bin/sh"
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
                strlen(pSambaPrivateDir) - (sizeof("PRIVATE_DIR: ") - 1));
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
    PCSTR pLikewiseLwiCompat = LIBDIR LWICOMPAT_FILENAME;

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

    if (symlink(pLikewiseLwiCompat, pLwiCompat) < 0)
    {
        error = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(error);   
    }

cleanup:
    LW_SAFE_FREE_STRING(pSambaDir);
    LW_SAFE_FREE_STRING(pLwiCompat);
    return error;
}

DWORD
TdbStore(
    TDB_CONTEXT *pTdb,
    PCSTR pKeyStart,
    PCSTR pKeyEnd,
    PVOID pData,
    DWORD DataLen
    )
{
    DWORD error = 0;
    int ret = 0;
    TDB_DATA tdbKey = { 0 };
    TDB_DATA tdbData = { 0 };
    PSTR pKey = NULL;

    error = LwAllocateStringPrintf(
                    &pKey,
                    "%s/%s",
                    pKeyStart,
                    pKeyEnd);
    BAIL_ON_LSA_ERROR(error);

    tdbKey.dptr = pKey;
    tdbKey.dsize = strlen(pKey);

    tdbData.dptr = pData;
    tdbData.dsize = DataLen;

    if ((ret = tdb_transaction_start(pTdb)) != 0) {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

    if ((ret = tdb_store(pTdb, tdbKey, tdbData, TDB_REPLACE)) != 0) {
        tdb_transaction_cancel(pTdb);
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

    if ((ret = tdb_transaction_commit(pTdb)) != 0) {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

cleanup:
    LW_SAFE_FREE_STRING(pKey);
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
    PLW_SECURITY_IDENTIFIER pSid = NULL;
    TDB_CONTEXT *pTdb = NULL;
    DWORD schannelType = 0;
    DWORD LCT = 0;

    error = GetSecretsPath(
        pSmbdPath,
        &pSecretsPath);
    BAIL_ON_LSA_ERROR(error);

    pTdb = tdb_open(
                    pSmbdPath,
                    0,
                    TDB_DEFAULT,
                    O_RDWR|O_CREAT,
                    0600);
    if (pTdb == NULL)
    {
        error = ERROR_INTERNAL_DB_ERROR;
        BAIL_ON_LSA_ERROR(error);
    }

    error = LsaOpenServer(
        &hLsa);
    BAIL_ON_LSA_ERROR(error);

    error = LsaAdGetMachinePasswordInfo(
        hLsa,
        NULL,
        &pPasswordInfo);
    BAIL_ON_LSA_ERROR(error);

    /* Machine Password */
    // The terminating null must be stored with the password
    error = TdbStore(
                    pTdb,
                    "SECRETS/MACHINE_PASSWORD",
                    pPasswordInfo->Account.NetbiosDomainName,
                    pPasswordInfo->Password,
                    strlen(pPasswordInfo->Password) + 1);
    BAIL_ON_LSA_ERROR(error);

    /* Domain SID */

    error = LwAllocSecurityIdentifierFromString(
                    pPasswordInfo->Account.DomainSid,
                    &pSid);
    BAIL_ON_LSA_ERROR(error);

    error = TdbStore(
                    pTdb,
                    "SECRETS/SID",
                    pPasswordInfo->Account.NetbiosDomainName,
                    (PBYTE)&pSid->pucSidBytes,
                    pSid->dwByteLength);
    BAIL_ON_LSA_ERROR(error);

    /* Schannel Type */

    switch(pPasswordInfo->Account.Type)
    {
        case LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION:
            schannelType = LW_HTOL32(2);
            break;
        case LSA_MACHINE_ACCOUNT_TYPE_DC:
            schannelType = LW_HTOL32(4);
            break;
        case LSA_MACHINE_ACCOUNT_TYPE_BDC:
            schannelType = LW_HTOL32(6);
            break;
        default:
            error = ERROR_INVALID_LOGON_TYPE;
            BAIL_ON_LSA_ERROR(error);
            break;
    }

    error = TdbStore(
                    pTdb,
                    "SECRETS/MACHINE_SEC_CHANNEL_TYPE",
                    pPasswordInfo->Account.NetbiosDomainName,
                    &schannelType,
                    sizeof(DWORD));
    BAIL_ON_LSA_ERROR(error);

    /* Last Change Time */

    LCT = LW_HTOL32(LwNtTimeToWinTime(
                pPasswordInfo->Account.LastChangeTime));

    error = TdbStore(
                    pTdb,
                    "SECRETS/MACHINE_LAST_CHANGE_TIME",
                    pPasswordInfo->Account.NetbiosDomainName,
                    &LCT,
                    sizeof(DWORD));
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
    if (pSid != NULL)
    {
        LwFreeSecurityIdentifier(pSid);
    }
    if (pTdb)
    {
        tdb_close(pTdb);
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
        SHOW_HELP,
        CHECK_VERSION,
        INSTALL,
        UNINSTALL
    } mode;
    PCSTR pSmbdPath = NULL;
    PSTR pFoundSmbdPath = NULL;
    DWORD error = 0;

    if (argc < 2 || argc > 3)
    {
        mode = SHOW_HELP;
    }
    else if(!strcmp(argv[1], "--check-version"))
    {
        mode = CHECK_VERSION;
    }
    else if(!strcmp(argv[1], "--install"))
    {
        mode = INSTALL;
    }
    else if(!strcmp(argv[1], "--uninstall"))
    {
        mode = UNINSTALL;
    }
    else
    {
        mode = SHOW_HELP;
    }

    if (argc == 3)
    {
        pSmbdPath = argv[2];
    }

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

    error = InstallWbclient(pSmbdPath);
    BAIL_ON_LSA_ERROR(error);

    error = InstallLwiCompat(pSmbdPath);
    BAIL_ON_LSA_ERROR(error);

    error = SynchronizePassword(
                pSmbdPath);
    BAIL_ON_LSA_ERROR(error);

cleanup:
    LW_SAFE_FREE_STRING(pFoundSmbdPath);
    return error;
}
