/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "domainjoin.h"
#include "ctshell.h"
#include "djdistroinfo.h"
#include "djdaemonmgr.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

static PSTR pszChkConfigPath = "/sbin/chkconfig";
static PSTR pszUpdateRcDFilePath = "/usr/sbin/update-rc.d";

#define DAEMON_BINARY_SEARCH_PATH "/usr/local/sbin:/usr/local/bin:/usr/dt/bin:/opt/dce/sbin:/usr/sbin:/usr/bin:/sbin:/bin:" BINDIR ":" SBINDIR

DWORD
GetInitScriptDir(PSTR *store)
{
#if defined(_AIX)
    return CTStrdup("/etc/rc.d/init.d", store);
#elif defined(_HPUX_SOURCE)
    return CTStrdup("/sbin/init.d", store);
#elif defined(__LWI_FREEBSD__)
    return CTStrdup("/etc/rc.d", store);
#else
    return CTStrdup("/etc/init.d", store);
#endif
}

static void FindDaemonScript(PCSTR name, PSTR *path, LWException **exc);

void
DJGetDaemonStatus(
    PCSTR pszDaemonName,
    PBOOLEAN pbStarted,
    LWException **exc
    )
{
    PSTR statusBuffer = NULL;
    PSTR prefixedPath = NULL;
    PSTR initDir = NULL;
    PSTR daemonPath = NULL;
    PSTR altDaemonName = NULL;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN daemon_installed = FALSE;
    DWORD ceError;

    if(pszDaemonName[0] == '/')
        LW_CLEANUP_CTERR(exc, CTStrdup(pszDaemonName, &prefixedPath));
    else
    {
        LW_TRY(exc, FindDaemonScript(pszDaemonName, &prefixedPath, &LW_EXC));
    }

    DJ_LOG_INFO("Checking status of daemon [%s]", prefixedPath);

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(prefixedPath, &daemon_installed));

    if (!daemon_installed) {
        LW_CLEANUP_CTERR(exc, ERROR_SERVICE_NOT_FOUND);
    }

    /* AIX has /etc/rc.dt. When '/etc/rc.dt status' is run, it tries to
     * start XWindows. So to be safe, we won't call that.
     */
    if(!strcmp(prefixedPath, "/etc/rc.dt"))
    {
        status = 1;
    }
    else if(!strcmp(prefixedPath, "/sbin/init.d/Rpcd"))
    {
        /* HP-UX has this script. When it is run with the status option, it
         * prints out the usage info and leaves with exit code 0.
         */
        status = 1;
    }
#if defined(__LWI_FREEBSD__)
    /* On various versions of FreeBSD, running the script nscd w/ status
     * returns 0 (and prints nothing) even when nscd is not running.
     */
    else if (!strcmp(prefixedPath, "/etc/rc.d/nscd"))
    {
        status = 1;
    }
#endif

    if (!status)
    {
        LW_CLEANUP_CTERR(exc, CTShell("%script status >/dev/null 2>&1; echo $? >%statusBuffer",
                    CTSHELL_STRING(script, prefixedPath),
                    CTSHELL_BUFFER(statusBuffer, &statusBuffer)));

        status = atol(statusBuffer);

        // see
        // http://forgeftp.novell.com/library/SUSE%20Package%20Conventions/spc_init_scripts.html
        // and http://www.linuxbase.org/~gk4/wip-sys-init.html
        //
        // note further that some other error codes might be thrown, so
        // we choose to only pay attention to the ones that lsb says are
        // valid return codes for status that indicate that a progam
        // isnt running, otherwise, we are gonna throw it.

        DJ_LOG_INFO("Daemon [%s]: status [%d]", prefixedPath, status);
    }

    if (!status) {
        *pbStarted = TRUE;
    }
    else if (status == 2 || status == 3 || status == 4)
        *pbStarted = FALSE;
    else if (status == 1)
    {
        // An unknown error occurred. Most likely the init script doesn't
        // support the query option. We'll have to query it with ps.

        pid_t daemonPid;
        const char *daemonBaseName = strrchr(pszDaemonName, '/');
        if(daemonBaseName == NULL)
        {
            daemonBaseName = pszDaemonName;
        }
        else
        {
            daemonBaseName++;
        }

        DJ_LOG_VERBOSE("Looking for %s", daemonBaseName);

        ceError = CTFindFileInPath(daemonBaseName,
                DAEMON_BINARY_SEARCH_PATH, &daemonPath);
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            CT_SAFE_FREE_STRING(altDaemonName);
            LW_CLEANUP_CTERR(exc, CTStrdup(daemonBaseName, &altDaemonName));
            CTStrToLower(altDaemonName);
            ceError = CTFindFileInPath(altDaemonName,
                    DAEMON_BINARY_SEARCH_PATH, &daemonPath);
        }
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            CT_SAFE_FREE_STRING(altDaemonName);
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&altDaemonName,
                    "%sd", daemonBaseName));
            ceError = CTFindFileInPath(altDaemonName,
                    DAEMON_BINARY_SEARCH_PATH, &daemonPath);
        }
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            LW_RAISE_EX(exc, ERROR_FILE_NOT_FOUND, "Cannot find daemon binary", "Querying the daemon init script with '%s status' returned code 1. This either means the init script does not support the status option, or the daemon died leaving a pid file. This program is instead attempting to see if the program is running using the ps command, however the daemon binary (%s) that goes along with the init script cannot be found.", prefixedPath, daemonBaseName);
            goto cleanup;
        }
        LW_CLEANUP_CTERR(exc, ceError);

        DJ_LOG_VERBOSE("Found %s", daemonPath);
        ceError = CTGetPidOfCmdLine(NULL, daemonPath, NULL, 0, &daemonPid, NULL);
        if(ceError == ERROR_PROC_NOT_FOUND || ceError == ERROR_NOT_SUPPORTED)
        {
            //Nope, couldn't find the daemon running
            *pbStarted = FALSE;
        }
        else
        {
            LW_CLEANUP_CTERR(exc, ceError);
            DJ_LOG_INFO("Even though '%s status' exited with code 1, '%s' is running with pid %d.", prefixedPath, daemonPath, daemonPid);
            *pbStarted = TRUE;
        }
    }
    else {
        LW_RAISE_EX(exc, ERROR_INVALID_STATE, "Non-standard return code from init script", "According to http://forgeftp.novell.com/library/SUSE%20Package%20Conventions/spc_init_scripts.html, init scripts should return 0, 1, 2, 3, or 4. However, '%s status' returned %d.", status, prefixedPath);
        goto cleanup;
    }

cleanup:
    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(prefixedPath);
    CT_SAFE_FREE_STRING(initDir);
    CT_SAFE_FREE_STRING(daemonPath);
    CT_SAFE_FREE_STRING(altDaemonName);
    CT_SAFE_FREE_STRING(statusBuffer);
}

static void FindDaemonScript(PCSTR name, PSTR *path, LWException **exc)
{
    PSTR altName = NULL;
    DWORD ceError;
    const char *searchPath = "/etc/init.d:/etc/rc.d/init.d:/sbin/init.d:/etc/rc.d";
    BOOLEAN fileExists = FALSE;

    *path = NULL;

    if (name[0] == '/')
    {
        LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(name, &fileExists));
        if (!fileExists)
        {
            LW_RAISE_EX(exc, ERROR_SERVICE_NOT_FOUND,
                    "Unable to find daemon",
                    "The '%s' daemon could not be found.",
                    name);
        }
        LW_CLEANUP_CTERR(exc, CTAllocateString(name, path));
    }
    else
    {
        ceError = CTFindFileInPath(name, searchPath, path);
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            CT_SAFE_FREE_STRING(altName);
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&altName,
                        "%s.rc", name));
            ceError = CTFindFileInPath(altName, searchPath, path);
        }
        if(ceError == ERROR_FILE_NOT_FOUND && !strcmp(name, "dtlogin"))
        {
            CT_SAFE_FREE_STRING(altName);
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&altName,
                        "rc.dt"));
            ceError = CTFindFileInPath(altName, searchPath, path);
        }
        if(ceError == ERROR_FILE_NOT_FOUND)
        {
            LW_RAISE_EX(exc, ERROR_SERVICE_NOT_FOUND,
                    "Unable to find daemon",
                    "The '%s' daemon could not be found in the search path '%s'. It could not be found with the alternative name '%s' either.",
                    name, searchPath, altName);
            goto cleanup;
        }
        LW_CLEANUP_CTERR(exc, ceError);
    }

cleanup:
    CT_SAFE_FREE_STRING(altName);
}

void
DJStartStopDaemon(
    PCSTR pszDaemonName,
    BOOLEAN bStatus,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN bStarted = FALSE;
    PSTR pszDaemonPath = NULL;
    int retry;

    LW_TRY(exc, FindDaemonScript(pszDaemonName, &pszDaemonPath, &LW_EXC));

    if (bStatus) {
        DJ_LOG_INFO("Starting daemon [%s]", pszDaemonPath);
    } else {
        DJ_LOG_INFO("Stopping daemon [%s]", pszDaemonPath);
    }

    if (!strcmp(pszDaemonPath, "/etc/rc.dt") && !bStatus)
    {
        // The dtlogin init script is broken on AIX. It always starts the
        // process.

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
        LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/sh", ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString("-c", ppszArgs+1));

        LW_CLEANUP_CTERR(exc, CTAllocateString("kill `cat /var/dt/Xpid`", ppszArgs+2));
    }
    else {

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs)));
        LW_CLEANUP_CTERR(exc, CTAllocateString(pszDaemonPath, ppszArgs));
        LW_CLEANUP_CTERR(exc, CTAllocateString((bStatus ? "start" : "stop"), ppszArgs+1));
    }

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    for(retry = 0; retry < 20; retry++)
    {
        LW_TRY(exc, DJGetDaemonStatus(pszDaemonName, &bStarted, &LW_EXC));
        if (bStarted == bStatus)
            break;
        sleep(1);
    }

    if (bStarted != bStatus) {

        if(bStatus)
        {
            if (!strcmp(pszDaemonPath, "/sbin/init.d/dtlogin.rc"))
            {
                // dtlogin on HP-UX does not have a status option
                LW_RAISE_EX(exc, ERROR_INVALID_STATE, "Unable to start daemon", "An attempt was made to start the '%s' daemon, but querying its status revealed that it did not start. Try running '%s start; ps -ef | grep dtlogin' to diagnose the issue", pszDaemonPath, pszDaemonPath);
            }
            else
            {
                LW_RAISE_EX(exc, ERROR_INVALID_STATE, "Unable to start daemon", "An attempt was made to start the '%s' daemon, but querying its status revealed that it did not start. Try running '%s start; %s status' to diagnose the issue", pszDaemonPath, pszDaemonPath, pszDaemonPath);
            }
        }
        else
        {
            if (!strcmp(pszDaemonPath, "/sbin/init.d/dtlogin.rc"))
            {
                // dtlogin on HP-UX does not have a status option
                LW_RAISE_EX(exc, ERROR_INVALID_STATE, "Unable to stop daemon", "An attempt was made to stop the '%s' daemon, but querying its status revealed that it did not stop. Try running '%s stop; ps -ef | grep dtlogin' to diagnose the issue", pszDaemonPath, pszDaemonPath);
            }
            else
            {
                LW_RAISE_EX(exc, ERROR_INVALID_STATE, "Unable to stop daemon", "An attempt was made to stop the '%s' daemon, but querying its status revealed that it did not stop. Try running '%s stop; %s status' to diagnose the issue", pszDaemonPath, pszDaemonPath, pszDaemonPath);
            }
        }
        goto cleanup;
    }

cleanup:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(pszDaemonPath);
}

void
DJDoUpdateRcD(
    PCSTR pszDaemonName,
    BOOLEAN bStatus,
    int startPriority,
    int stopPriority,
    LWException **exc
    )
{
    PSTR command = NULL;

    if (!bStatus) {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&command,
                    "%s -f %s remove", pszUpdateRcDFilePath,
                    pszDaemonName));
    }
    else
    {
        if (startPriority == 0)
            startPriority = 20;
        if (stopPriority == 0)
            stopPriority = 20;

        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&command,
                    "%s %s defaults %d %d", pszUpdateRcDFilePath,
                    pszDaemonName, startPriority, stopPriority));
    }

    CTCaptureOutputToExc(command, exc);

cleanup:
    CT_SAFE_FREE_STRING(command);
}

DWORD
DJDoChkConfig(
    PCSTR pszDaemonName,
    BOOLEAN bStatus
    )
{
    DWORD ceError = ERROR_SUCCESS;
    CHAR szDaemonPath[PATH_MAX+1];
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG status = 0;
    PPROCINFO pProcInfo = NULL;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];

#if defined(_AIX)
    sprintf(szDaemonPath, "/etc/rc.d/init.d/%s", pszDaemonName);
#elif defined(_HPUX_SOURCE)
    sprintf(szDaemonPath, "/sbin/init.d/%s", pszDaemonName);
#elif defined(__LWI_FREEBSD__)
    sprintf(szDaemonPath, "/etc/rc.d/%s", pszDaemonName);
#else
    sprintf(szDaemonPath, "/etc/init.d/%s", pszDaemonName);
#endif

    // if we got this far, we have set the daemon to the running state
    // that the caller has requested.  now we need to set it's init
    // state (whether or not it get's ran on startup) to what the
    // caller has requested. we can do this unconditionally because
    // chkconfig wont complain if we do an --add to something that is
    // already in the --add state.

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszChkConfigPath, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString((bStatus ? "--add" : "--del"), ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszDaemonName, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = ERROR_BAD_COMMAND;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    // this step is needed in some circumstances, but not all. i
    // *think* that this step is needed for LSB-1.2 only. furthermore
    // this step appears to turn off LSB-2.0 daemons, so we have to
    // make sure that we dont try to run it on them. so we check for
    // BEGIN INIT INFO. It's slower than i would like due to the need
    // to read the file in and do stringops on it. shelling out and
    // using grep *might* be faster, but it might not be, and it has
    // all the complexity associated with running native apps.

    fp = fopen(szDaemonPath, "r");
    if (fp == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while (1) {
        if (fgets(szBuf, 1024, fp) == NULL) {

            if (feof(fp))
                break;
            else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            if (strstr(szBuf, "chkconfig:")) {

                CTFreeString(*(ppszArgs+1));

                if (pProcInfo) {
                    FreeProcInfo(pProcInfo);
                    pProcInfo = NULL;
                }

                ceError = CTAllocateString((bStatus ? "on" : "off"), ppszArgs+1);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
                BAIL_ON_CENTERIS_ERROR(ceError);

                ceError = DJGetProcessStatus(pProcInfo, &status);
                BAIL_ON_CENTERIS_ERROR(ceError);

                if (status != 0) {
                    ceError = ERROR_BAD_COMMAND;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

            }

            if (strstr(szBuf, "BEGIN INIT INFO"))
                break;

        }
    }

error:

    if (fp)
        fclose(fp);

    if (pProcInfo) {
        FreeProcInfo(pProcInfo);
        pProcInfo = NULL;
    }

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    return ceError;
}

void
DJOverwriteSymlink(
    PCSTR symlinkTarget,
    PCSTR symlinkName,
    LWException **exc
    )
{
    BOOLEAN bFileExists = FALSE;

    DJ_LOG_INFO("Creating symlink [%s] -> [%s]", symlinkName, symlinkTarget);
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(symlinkName, &bFileExists));
    if (bFileExists)
    {
        LW_CLEANUP_CTERR(exc, CTRemoveFile(symlinkName));
    }
    LW_CLEANUP_CTERR(exc, CTCreateSymLink(symlinkTarget,
                symlinkName));
cleanup:
    ;
}

void
DJRemoveDaemonLinksInDirectories(
    PCSTR *directories,
    PCSTR daemonName,
    LWException **exc
    )
{
    PSTR searchExpression = NULL;
    PSTR *matchingPaths = NULL;
    int i, j;
    DWORD matchCount = 0;

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&searchExpression,
            "^.*%s$", daemonName));

    for(i = 0; directories[i] != NULL; i++)
    {
        LW_CLEANUP_CTERR(exc, CTGetMatchingFilePathsInFolder(
            directories[i], searchExpression, &matchingPaths,
            &matchCount));
        for(j = 0; j < matchCount; j++)
        {
            DJ_LOG_INFO("Removing init script symlink [%s]",
                    matchingPaths[j]);
            LW_CLEANUP_CTERR(exc, CTRemoveFile(matchingPaths[j]));
        }
        //CTFreeStringArray will ignore matchingPaths if it is NULL
        CTFreeStringArray(matchingPaths, matchCount);
        matchingPaths = NULL;
    }

cleanup:
    CTFreeStringArray(matchingPaths, matchCount);
    CT_SAFE_FREE_STRING(searchExpression);
}

