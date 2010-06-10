/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test Program for stress testing AD Provider
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

typedef void* (*PFN_LADS_THR_ROUTINE)(void* pData);

#define _POSIX_PTHREAD_SEMANTICS 1

#include "includes.h"

static
void
LADSInterruptHandler(
    int Signal
    )
{
    if (Signal == SIGINT)
    {
        raise(SIGTERM);
    }
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;
    LsaLogLevel maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;
    
    dwError = LADSInitGlobals();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LADSParseArgs(
                  argc,
                  argv,
                  &pszConfigFilePath,
                  &maxAllowedLogLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaInitLogging_r(
                    LADSGetProgramName(argv[0]),
                    LSA_LOG_TARGET_CONSOLE,
                    maxAllowedLogLevel,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LADSParseConfig(
                    pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LW_IS_NULL_OR_EMPTY_STR(gpszProviderLibPath))
    {
        fprintf(stderr, "Error: The stress config file does not have a valid provider library path\n");
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (LW_IS_NULL_OR_EMPTY_STR(gpszProviderConfigFilePath))
    {
        fprintf(stderr, "Error: The stress config file does not have a valid provider config file path\n");
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    gpAuthProvider->pszProviderLibpath = gpszProviderLibPath;
    
    dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LADSBlockSelectedSignals();
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LADSInitAuthProvider(
                    gpszProviderConfigFilePath,
                    gpAuthProvider);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LADSStartWorkers();
    BAIL_ON_LSA_ERROR(dwError);
    
    // Handle signals, blocking until we are supposed to exit.
    dwError = LADSHandleSignals();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LADSStopWorkers();

    LADSShutdownAuthProvider(gpAuthProvider);
    
    LsaShutdownLogging_r();

    LADSShutdownGlobals();

    LW_SAFE_FREE_STRING(pszConfigFilePath);

    return (dwError);

error:

    LSA_LOG_ERROR("Failed in stress-testing ad-provider [error code: %u]", dwError);

    goto cleanup;
}

DWORD
LADSInitGlobals(
    VOID
    )
{
    DWORD dwError = 0;
    
    memset(&gLADSStressData[0], 0, sizeof(gLADSStressData));
    
    return dwError;
}

PSTR
LADSGetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

DWORD
LADSShutdownGlobals(
    VOID
    )
{
    DWORD dwError = 0;
    
    DWORD iType = 0;
    
    for (iType = 0; iType < LADS_SENTINEL; iType++)
    {
        switch (iType)
        {
            case LADS_FIND_USER_BY_NAME:
            case LADS_FIND_GROUP_BY_NAME:
                
                if (gLADSStressData[iType].data.ppszNames)
                {
                    DWORD i = 0;
                    for (i = 0; i < gLADSStressData[iType].dwNumItems; i++)
                    {
                        LW_SAFE_FREE_STRING(gLADSStressData[iType].data.ppszNames[i]);
                    }
                    LwFreeMemory(gLADSStressData[iType].data.ppszNames);
                }
                break;
                
            case LADS_FIND_USER_BY_ID:
                
                LW_SAFE_FREE_MEMORY(gLADSStressData[iType].data.pUidArray);

            case LADS_FIND_GROUP_BY_ID:
                
                LW_SAFE_FREE_MEMORY(gLADSStressData[iType].data.pGidArray);

            default:
                break;
        }
    }
    
    LW_SAFE_FREE_STRING(gpszProviderLibPath);
    LW_SAFE_FREE_STRING(gpszProviderConfigFilePath);
    
    return dwError;
}

DWORD
LADSParseArgs(
    int          argc,
    char*        argv[],
    PSTR*        ppszConfigFilePath,
    LsaLogLevel* pMaxAllowedLogLevel
    )
{
    typedef enum {
      OpenMode = 0,
      ConfigFile,
      LogLevel,
      Done
    } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parse_mode = OpenMode;
    PSTR pszConfigFilePath = NULL;
    LsaLogLevel maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parse_mode)
        {
           case OpenMode:
           {
                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                   ShowUsage();
                   exit(0);
                }
                else if (!strcasecmp(pszArg, "--config"))
                {
                    parse_mode = ConfigFile;
                }
                else if (!strcmp(pszArg, "--loglevel"))
                {
                    parse_mode = LogLevel;
                }
                else
                {
                   parse_mode = Done;
                }

                break;
           }
           case ConfigFile:
               
               dwError = LwAllocateString(
                               pszArg,
                               &pszConfigFilePath);
               BAIL_ON_LSA_ERROR(dwError);
               
               parse_mode = OpenMode;
               break;
               
           case LogLevel:
               
               if (!strcasecmp(pszArg, "error")) {
                 
                 maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;
                 
               } else if (!strcasecmp(pszArg, "warning")) {
                 
                 maxAllowedLogLevel = LSA_LOG_LEVEL_WARNING;
                 
               } else if (!strcasecmp(pszArg, "info")) {
                 
                 maxAllowedLogLevel = LSA_LOG_LEVEL_INFO;
                 
               } else if (!strcasecmp(pszArg, "verbose")) {
                 
                 maxAllowedLogLevel = LSA_LOG_LEVEL_VERBOSE;
                 
               } else if (!strcasecmp(pszArg, "debug")) {

                 maxAllowedLogLevel = LSA_LOG_LEVEL_DEBUG;
                 
               } else {
                 
                 fprintf(stderr, "Error: Invalid log level [%s]", pszArg);
                 ShowUsage();
                 exit(1);
                 
               }
               
               break;
               
           case Done:
               break;
        }
    } while ((parse_mode != Done) && (iArg < argc));
    
    if (LW_IS_NULL_OR_EMPTY_STR(pszConfigFilePath))
    {
        ShowUsage();
        exit(1);
    }
    
    *ppszConfigFilePath = pszConfigFilePath;
    *pMaxAllowedLogLevel = maxAllowedLogLevel;

cleanup:

    return dwError;

error:

    *ppszConfigFilePath = NULL;

    LW_SAFE_FREE_STRING(pszConfigFilePath);
    
    goto cleanup;
}

void
ShowUsage(
    VOID
    )
{
    printf("Usage: test_stress_ad_provider --config <path>\n");
}

DWORD
LADSBlockSelectedSignals(
    VOID
    )
{
    DWORD dwError = 0;    
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    sigaddset(&default_signal_mask, SIGINT);
    sigaddset(&default_signal_mask, SIGTERM);
    sigaddset(&default_signal_mask, SIGHUP);
    sigaddset(&default_signal_mask, SIGQUIT);
    sigaddset(&default_signal_mask, SIGPIPE);

    dwError = pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
LADSHandleSignals(
    VOID
    )
{
    DWORD dwError = 0;
    struct sigaction action;
    sigset_t catch_signal_mask;
    sigset_t old_signal_mask;
    int which_signal = 0;
    BOOLEAN bWaitForSignals = TRUE;
    int sysRet = 0;

    // After starting up threads, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = LADSInterruptHandler;

    sysRet = sigaction(SIGINT, &action, NULL);
    dwError = (sysRet != 0) ? LwMapErrnoToLwError(errno) : 0;
    BAIL_ON_LSA_ERROR(dwError);

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    dwError = pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    // These should already be blocked...
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGTERM);
    sigaddset(&catch_signal_mask, SIGHUP);
    sigaddset(&catch_signal_mask, SIGQUIT);
    sigaddset(&catch_signal_mask, SIGPIPE);

    dwError = pthread_sigmask(SIG_BLOCK, &catch_signal_mask, &old_signal_mask);
    BAIL_ON_LSA_ERROR(dwError);

    while (bWaitForSignals)
    {
      
      /* wait for a signal to arrive */
      sigwait(&catch_signal_mask, &which_signal);

      LSA_LOG_WARNING("Received signal [%d]", which_signal);
      
      switch (which_signal)
        {
          
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
          
          {
            LADSStopProcess();

            bWaitForSignals = FALSE;
            
            break;
          }
      
        case SIGPIPE:

          {
             LSA_LOG_DEBUG("Handled SIGPIPE");

             break;
          }

        }
    }

error:
    return dwError;
}

DWORD
LADSInitAuthProvider(
    PCSTR pszConfigFilePath,
    PTEST_AUTH_PROVIDER pProvider
    )
{
    DWORD dwError = 0;
    PFNINITIALIZEPROVIDER pfnInitProvider = NULL;
    PCSTR pszError = NULL;
    PSTR pszProviderLibpath = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pProvider->pszProviderLibpath))
    {
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pszProviderLibpath = pProvider->pszProviderLibpath;
    
    dlerror();
    pProvider->pLibHandle = dlopen(pszProviderLibpath, RTLD_NOW | RTLD_GLOBAL);
    if (!pProvider->pLibHandle)
    {
        LSA_LOG_ERROR("Failed to open auth provider at path '%s'", pszProviderLibpath);
    
        pszError = dlerror();
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
        {
            LSA_LOG_ERROR("%s", pszError);
        }
    
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dlerror();
    pfnInitProvider = (PFNINITIALIZEPROVIDER)dlsym(
        pProvider->pLibHandle,
        LSA_SYMBOL_NAME_INITIALIZE_PROVIDER);
    if (!pfnInitProvider)
    {
        LSA_LOG_ERROR("Ignoring invalid auth provider at path '%s'", pszProviderLibpath);
    
        pszError = dlerror();
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
        {
            LSA_LOG_ERROR("%s", pszError);
        }
    
        dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_LSA_ERROR(dwError);
    }
        
    dwError = pfnInitProvider(
                    &pProvider->pszName,
                    &pProvider->pFnTable);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LADSValidateProvider(pProvider);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LADSValidateProvider(
    PTEST_AUTH_PROVIDER pProvider
    )
{
    if (!pProvider ||
        !pProvider->pFnTable ||
        !pProvider->pFnTable->pfnShutdownProvider ||
        !pProvider->pFnTable->pfnOpenHandle ||
        !pProvider->pFnTable->pfnCloseHandle ||
        !pProvider->pFnTable->pfnServicesDomain ||
        !pProvider->pFnTable->pfnAuthenticateUserPam ||
        !pProvider->pFnTable->pfnValidateUser ||
        !pProvider->pFnTable->pfnLookupUserByName ||
        !pProvider->pFnTable->pfnLookupUserById ||
        !pProvider->pFnTable->pfnBeginEnumUsers ||
        !pProvider->pFnTable->pfnEnumUsers ||
        !pProvider->pFnTable->pfnEndEnumUsers ||
        !pProvider->pFnTable->pfnLookupGroupByName ||
        !pProvider->pFnTable->pfnLookupGroupById ||
        !pProvider->pFnTable->pfnGetGroupsForUser ||
        !pProvider->pFnTable->pfnBeginEnumGroups ||
        !pProvider->pFnTable->pfnEnumGroups ||
        !pProvider->pFnTable->pfnEndEnumGroups ||
        !pProvider->pFnTable->pfnChangePassword ||
        !pProvider->pFnTable->pfnAddUser ||
        !pProvider->pFnTable->pfnModifyUser ||
        !pProvider->pFnTable->pfnDeleteUser ||
        !pProvider->pFnTable->pfnAddGroup ||
        !pProvider->pFnTable->pfnDeleteGroup ||
        !pProvider->pFnTable->pfnOpenSession ||
        !pProvider->pFnTable->pfnCloseSession ||
        !pProvider->pFnTable->pfnGetNamesBySidList ||
        !pProvider->pFnTable->pfnBeginEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnEndEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnGetStatus ||
        !pProvider->pFnTable->pfnFreeStatus
        )
    {
        return LW_ERROR_INVALID_AUTH_PROVIDER;
    }
    
    return 0;
}

DWORD
LADSStartWorkers(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD iType = 0;

    for (iType = LADS_FIND_USER_BY_NAME;
         iType < LADS_SENTINEL;
         iType++)
    {
        DWORD iThread = 0;
        PFN_LADS_THR_ROUTINE pFnThrRoutine = NULL;

        if (!gLADSStressData[iType].dwNumThreads)
        {
           continue;
        }

        dwError = LwAllocateMemory(
                      sizeof(pthread_t) * gLADSStressData[iType].dwNumThreads,
                      (PVOID*)&gLADSStressData[iType].pThreadArray);
        BAIL_ON_LSA_ERROR(dwError);

        switch (gLADSStressData[iType].type)
        {
            case LADS_FIND_USER_BY_NAME:
                 pFnThrRoutine = &LADSFindUserByName;
                 break;

            case LADS_FIND_USER_BY_ID:
                 pFnThrRoutine = &LADSFindUserById;
                 break;
 
            case LADS_ENUM_USERS:
                 pFnThrRoutine = &LADSEnumUsers;
                 break;

            case LADS_FIND_GROUP_BY_ID:
                 pFnThrRoutine = &LADSFindGroupById;
                 break;

            case LADS_FIND_GROUP_BY_NAME:
                 pFnThrRoutine = &LADSFindGroupByName;
                 break;

            case LADS_ENUM_GROUPS:
                 pFnThrRoutine = &LADSEnumGroups;
                 break;

            default:

                 break;
        }

        for (iThread = 0;
             iThread < gLADSStressData[iType].dwNumThreads;
             iThread++)
        {

            dwError = pthread_create(
                       &gLADSStressData[iType].pThreadArray[iThread],
                       NULL,
                       pFnThrRoutine,
                       NULL);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LADSStopWorkers(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD iType = 0;
    
    LADSStopProcess();

    for (iType = LADS_FIND_USER_BY_NAME;
         iType < LADS_SENTINEL;
         iType++)
    {
        DWORD iThread = 0;

        if (!gLADSStressData[iType].dwNumThreads)
        {
           continue;
        }

        for (iThread = 0;
             iThread < gLADSStressData[iType].dwNumThreads;
             iThread++)
        {
            PVOID pThreadResult = NULL;

            if (gLADSStressData[iType].pThreadArray[iThread])
            {
                pthread_join(gLADSStressData[iType].pThreadArray[iThread],
                             &pThreadResult);
            }
        }

        LwFreeMemory(gLADSStressData[iType].pThreadArray);
    }

    return dwError;
}

DWORD
LADSShutdownAuthProvider(
    PTEST_AUTH_PROVIDER pProvider
    )
{
    if (pProvider)
    {
        if (pProvider->pFnTable && pProvider->pFnTable->pfnShutdownProvider)
        {
            pProvider->pFnTable->pfnShutdownProvider();
        }

        if (pProvider->pLibHandle)
        {
            dlclose(pProvider->pLibHandle);
        }
    }

    return 0;
}

VOID
LADSStopProcess(
    VOID
    )
{
    pthread_mutex_lock(&gExitLock);
    
    gbExit = TRUE;
    
    pthread_mutex_unlock(&gExitLock);
}

BOOLEAN
LADSProcessShouldStop(
    VOID
    )
{
    BOOLEAN bValue = FALSE;
    
    pthread_mutex_lock(&gExitLock);
    
    bValue = gbExit;
    
    pthread_mutex_unlock(&gExitLock);
    
    return bValue;
}
