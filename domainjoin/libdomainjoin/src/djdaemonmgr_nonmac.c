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
        if(!strcmp(daemonBaseName, "likewise-open"))
            daemonBaseName = "likewise-winbindd";

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

void
DJConfigureForDaemonRestart(
    PCSTR pszDaemonName,
    BOOLEAN bStatus,
    int startPriority,
    int stopPriority,
    LWException **exc
    )
{
    BOOLEAN bFileExists = FALSE;
    LwDistroInfo distro;
    PSTR symlinkTarget = NULL;
    PSTR symlinkName = NULL;
    long status = 0;
    PSTR statusBuffer = NULL;
    PSTR outputBuffer = NULL;

    memset(&distro, 0, sizeof(distro));

    DJ_LOG_VERBOSE("Looking for '%s'", pszChkConfigPath);
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszChkConfigPath, &bFileExists));

    if (bFileExists) {
        DWORD ceError;

        DJ_LOG_VERBOSE("Found '%s'", pszChkConfigPath);
        ceError = DJDoChkConfig(pszDaemonName, bStatus);
        if(ceError == ERROR_BAD_COMMAND)
        {
            LW_RAISE_EX(exc, ERROR_BAD_COMMAND, "chkconfig failed", "An error occurred while using chkconfig to process the '%s' daemon. This daemon was being %s the list of processes to start on reboot.", pszDaemonName, bStatus? "added to" : "removed from");
            goto cleanup;
        }
        LW_CLEANUP_CTERR(exc, ceError);

        goto done;
    }

    DJ_LOG_VERBOSE("Looking for '%s'", pszUpdateRcDFilePath);
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszUpdateRcDFilePath, &bFileExists));

    if (bFileExists) {

        DJ_LOG_VERBOSE("Found '%s'", pszUpdateRcDFilePath);
        LW_TRY(exc, DJDoUpdateRcD(pszDaemonName, bStatus, startPriority, stopPriority, &LW_EXC));

        goto done;
    }

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo("", &distro));

    if(distro.os == OS_AIX)
    {
        /* Programs on AIX may store their init scripts in /etc/rc.d/init.d .
         * The symlinks for specific runlevels are stored in /etc/rc.d/rc2.d,
         * /etc/rc.d/rc3.d, upto /etc/rc9.d . Only runlevels 0-2 are used.
         * Init scripts can only be started in 0 or 1 by editing inittab. Run
         * levels 3 and higher are left for the user to define.
         *
         * During startup, the /etc/rc.d/rc script runs all of the kill
         * scripts in /etc/rc.d/rc2.d followed by all of the start scripts in
         * that directory. Since the system goes to runlevel 2 at start up,
         * only scripts in that directory are run.
         *
         * During shutdown, the /sbin/shutdown program directly (not through
         * init or rc) runs all of the kill scripts in /etc/rc.d/rc*.d .
         *
         * So in order for a daemon to correctly be started once when the
         * machine boots, and only be shutdown once when the machine shuts
         * down, it should only create start and kill symlink in
         * /etc/rc.d/rc2.d.
         */
        if(bStatus)
        {
            LW_TRY(exc, FindDaemonScript(pszDaemonName,
                        &symlinkTarget, &LW_EXC));
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                    "/etc/rc.d/rc2.d/K%02d%s", stopPriority, pszDaemonName));
            LW_TRY(exc, DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
            CT_SAFE_FREE_STRING(symlinkName);

            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                    "/etc/rc.d/rc2.d/S%02d%s", startPriority, pszDaemonName));
            LW_TRY(exc, DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
        }
        else
        {
            PCSTR directories[] = {
                "/etc/rc.d/rc2.d",
                "/etc/rc.d/rc3.d",
                "/etc/rc.d/rc4.d",
                "/etc/rc.d/rc5.d",
                "/etc/rc.d/rc6.d",
                "/etc/rc.d/rc7.d",
                "/etc/rc.d/rc8.d",
                "/etc/rc.d/rc9.d",
                NULL };
            LW_TRY(exc, DJRemoveDaemonLinksInDirectories(
                        directories, pszDaemonName, &LW_EXC));
        }
    }
    else if(distro.os == OS_HPUX)
    {
        /* On HP-UX, in order for a daemon to start in multi-user mode,
         * a start symlink must be put in /sbin/rc2.d, and a stop symlink
         * must be put in /sbin/rc1.d
         */
        if(bStatus)
        {
            PCSTR relativeName = strrchr(pszDaemonName, '/');

            if (relativeName == NULL)
            {
                relativeName = pszDaemonName;
            }
            else
            {
                relativeName++;
            }

            LW_TRY(exc, FindDaemonScript(pszDaemonName,
                        &symlinkTarget, &LW_EXC));
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                    "/sbin/rc1.d/K%03d%s", stopPriority, relativeName));
            LW_TRY(exc,
                    DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
            CT_SAFE_FREE_STRING(symlinkName);

            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                    "/sbin/rc2.d/S%03d%s", startPriority, relativeName));
            LW_TRY(exc,
                    DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
        }
        else
        {
            /* Remove any symlinks to the daemon if they exist */
            PCSTR directories[] = {
                "/sbin/rc0.d",
                "/sbin/rc1.d",
                "/sbin/rc2.d",
                "/sbin/rc3.d",
                "/sbin/rc4.d",
                NULL };
            LW_TRY(exc, DJRemoveDaemonLinksInDirectories(
                        directories, pszDaemonName, &LW_EXC));
        }
    }
    else if(distro.os == OS_SUNOS)
    {
        BOOLEAN bUsesSVCS = FALSE;

        LW_CLEANUP_CTERR(exc, CTCheckFileExists("/usr/sbin/svcadm", &bUsesSVCS));
        if (bUsesSVCS)
        {
            /* On newer Solaris system, in order for a daemon to start in multi-user mode,
             * we need to register our daemons with the service manager (svcs).
             *
             * To add:
             * svccfg import /etc/likewise/svcs-solaris/<daemon name>.xml
             * svcadm enable <daemon name>
             *
             * To remove:
             * svcadm disable <daemon name>
             * svccfg delete  <daemon name>
             */
            if(bStatus)
            {
                LW_CLEANUP_CTERR(exc, CTShell("/usr/sbin/svccfg import /etc/likewise/svcs-solaris/%daemonName.xml >%outputBuffer 2>&1; echo $? >%statusBuffer",
                            CTSHELL_STRING(daemonName, pszDaemonName),
                            CTSHELL_BUFFER(outputBuffer, &outputBuffer),
                            CTSHELL_BUFFER(statusBuffer, &statusBuffer)));

                status = atol(statusBuffer);

                CT_SAFE_FREE_STRING(statusBuffer);
                statusBuffer = NULL;

                DJ_LOG_INFO("Daemon [%s]: svccfg import /etc/likewise/svcs-solaris/%s.xml status [%d]", pszDaemonName, pszDaemonName, status);
                if (outputBuffer[0] != '\0')
                {
                    DJ_LOG_VERBOSE("svccfg command output: %s", outputBuffer);
                }

                CT_SAFE_FREE_STRING(outputBuffer);
                outputBuffer = NULL;

                if (status) {
                    LW_RAISE_EX(exc, ERROR_INVALID_STATE, "svccfg import failed", "An error occurred while using svccfg to process the '%s' daemon. This daemon was being added to the list of processes to start on reboot.", pszDaemonName);
                    goto cleanup;
                }

                LW_CLEANUP_CTERR(exc, CTShell("/usr/sbin/svcadm enable %daemonName >%outputBuffer 2>&1; echo $? >%statusBuffer",
                            CTSHELL_STRING(daemonName, pszDaemonName),
                            CTSHELL_BUFFER(outputBuffer, &outputBuffer),
                            CTSHELL_BUFFER(statusBuffer, &statusBuffer)));

                status = atol(statusBuffer);

                DJ_LOG_INFO("Daemon [%s]: svcadm enable %s status [%d]", pszDaemonName, pszDaemonName, status);
                if (outputBuffer[0] != '\0')
                {
                    DJ_LOG_VERBOSE("svcadm command output: %s", outputBuffer);
                }

                CT_SAFE_FREE_STRING(outputBuffer);
                outputBuffer = NULL;

                if (status) {
                    LW_RAISE_EX(exc, ERROR_INVALID_STATE, "svcadm enable failed", "An error occurred while using svcadm to process the '%s' daemon. This daemon was being added to the list of processes to start on reboot.", pszDaemonName);
                    goto cleanup;
                }
            }
            else
            {
                LW_CLEANUP_CTERR(exc, CTShell("/usr/sbin/svcadm disable %daemonName >%outputBuffer 2>&1; echo $? >%statusBuffer",
                            CTSHELL_STRING(daemonName, pszDaemonName),
                            CTSHELL_BUFFER(outputBuffer, &outputBuffer),
                            CTSHELL_BUFFER(statusBuffer, &statusBuffer)));

                status = atol(statusBuffer);

                CT_SAFE_FREE_STRING(statusBuffer);
                statusBuffer = NULL;

                DJ_LOG_INFO("Daemon [%s]: svcadm disable %s status [%d]", pszDaemonName, pszDaemonName, status);
                if (outputBuffer[0] != '\0')
                {
                    DJ_LOG_VERBOSE("svcadm command output: %s", outputBuffer);
                }

                CT_SAFE_FREE_STRING(outputBuffer);
                outputBuffer = NULL;

                LW_CLEANUP_CTERR(exc, CTShell("/usr/sbin/svccfg delete %daemonName >%outputBuffer 2>&1; echo $? >%statusBuffer",
                            CTSHELL_STRING(daemonName, pszDaemonName),
                            CTSHELL_BUFFER(outputBuffer, &outputBuffer),
                            CTSHELL_BUFFER(statusBuffer, &statusBuffer)));

                status = atol(statusBuffer);

                DJ_LOG_INFO("Daemon [%s]: svccfg delete %s status [%d]", pszDaemonName, pszDaemonName, status);
                if (outputBuffer[0] != '\0')
                {
                    DJ_LOG_VERBOSE("svccfg command output: %s", outputBuffer);
                }

                CT_SAFE_FREE_STRING(outputBuffer);
                outputBuffer = NULL;
            }
        }
        else
        {
            /* On older Solaris system, in order for a daemon to start in multi-user mode,
             * a start symlink must be put in /etc/rc2.d, and a stop symlink
             * must be put in /etc/rc0.d, /etc/rc1.d, /etc/rcS.d.
             */
            if(bStatus)
            {
                PCSTR relativeName = strrchr(pszDaemonName, '/');
    
                if (relativeName == NULL)
                {
                    relativeName = pszDaemonName;
                }
                else
                {
                    relativeName++;
                }
    
                LW_TRY(exc, FindDaemonScript(pszDaemonName,
                            &symlinkTarget, &LW_EXC));
                LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                        "/etc/rc0.d/K%02d%s", stopPriority, relativeName));
                LW_TRY(exc,
                        DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
                CT_SAFE_FREE_STRING(symlinkName);
    
                LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                        "/etc/rc1.d/K%02d%s", stopPriority, relativeName));
                LW_TRY(exc,
                        DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
                CT_SAFE_FREE_STRING(symlinkName);
    
                LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                        "/etc/rcS.d/K%02d%s", stopPriority, relativeName));
                LW_TRY(exc,
                        DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
                CT_SAFE_FREE_STRING(symlinkName);
    
                LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&symlinkName,
                        "/etc/rc2.d/S%02d%s", startPriority, relativeName));
                LW_TRY(exc,
                        DJOverwriteSymlink(symlinkTarget, symlinkName, &LW_EXC));
            }
            else
            {
                /* Remove any symlinks to the daemon if they exist */
                PCSTR directories[] = {
                    "/etc/rc0.d",
                    "/etc/rc1.d",
                    "/etc/rc2.d",
                    "/etc/rc3.d",
                    "/etc/rcS.d",
                    NULL };
                LW_TRY(exc, DJRemoveDaemonLinksInDirectories(
                            directories, pszDaemonName, &LW_EXC));
            }
        }
    }

done:
cleanup:
    CT_SAFE_FREE_STRING(symlinkTarget);
    CT_SAFE_FREE_STRING(symlinkName);
    CT_SAFE_FREE_STRING(statusBuffer);
    CT_SAFE_FREE_STRING(outputBuffer);
}

void
DJManageDaemonDescription(
    PCSTR pszName,
    BOOLEAN bStatus,
    int startPriority,
    int stopPriority,
    PSTR *description,
    LWException **exc
    )
{
    PSTR daemonPath = NULL;

    *description = NULL;

    CT_SAFE_FREE_STRING(daemonPath);
    LW_TRY(exc, FindDaemonScript(pszName, &daemonPath, &LW_EXC));
    if(bStatus)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(description,
                "Start %s by running '%s start'.\n"
                "Create symlinks for %s so that it starts at reboot.\n",
                pszName, daemonPath, pszName));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(description,
                "Stop %s by running '%s stop'.\n"
                "Remove symlinks for %s so that it no longer starts at reboot.\n",
                pszName, daemonPath, pszName));
    }

cleanup:
    CT_SAFE_FREE_STRING(daemonPath);
}

void
DJManageDaemon(
    PCSTR pszName,
    BOOLEAN bStatus,
    int startPriority,
    int stopPriority,
    LWException **exc
    )
{
    BOOLEAN bStarted = FALSE;

    // Check for the existence of the daemon prior to doing anything. 
    // notice that we are using the private version so that if we fail,
    // our inner exception will be the one that was tossed due to the failure.
    LW_TRY(exc, DJGetDaemonStatus(pszName, &bStarted, &LW_EXC));

    // Verify that daemon is configured as needed for running after restart,
    // this also sets up any service control manager settings. On newer
    // Solaris systems, the service control manager will start/stop the
    // daemon as a result of the service configuration. It is okay to have
    // this function called more than once for a given daemon, if it is already
    // running or stopped it will remain the same with the same service pid.
    LW_TRY(exc, DJConfigureForDaemonRestart(pszName, bStatus, startPriority, stopPriority, &LW_EXC));

    // Reverify the daemon status now, it may be started or stopped as needed by the step above.
    LW_TRY(exc, DJGetDaemonStatus(pszName, &bStarted, &LW_EXC));

    // If we are already in the desired state, do nothing.
    if (bStarted != bStatus) {
        LW_TRY(exc, DJStartStopDaemon(pszName, bStatus, &LW_EXC));
    }

cleanup:
    ;
}

