/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "domainjoin.h"
#include "djdaemonmgr.h"
#include "ctstrutils.h"
#include "djauthinfo.h"
#include "djdistroinfo.h"
#include "djservicemgr.h"
#include "djregistry.h"
#include <lsa/lsa.h>

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))
#define PWGRD "/etc/rc.config.d/pwgr"

static QueryResult QueryStopDaemons(const JoinProcessOptions *options, LWException **exc)
{
    DWORD dwError = 0;
    BOOLEAN running;
    QueryResult result = FullyConfigured;

    if (options->enableMultipleJoins)
    {
        result = NotApplicable;
        goto cleanup;
    }

    /* Check for gpagentd */

    dwError = DJGetServiceStatus("gpagent", &running);
    if (dwError == LW_ERROR_NO_SUCH_SERVICE)
    {
        /* The gpagentd may not be installed so ignore */
        running = FALSE;
        dwError = 0;
    }
    if (dwError)
    {
        LW_RAISE_EX(exc, dwError,
                "Received error while querying lwsmd.",
                "Received error while querying lwsmd.");
        goto cleanup;
    }

    if(running)
        result = NotConfigured;
    
cleanup:
    return result;
}

static void StopDaemons(JoinProcessOptions *options, LWException **exc)
{
    LW_TRY(exc, DJManageDaemons(FALSE, &LW_EXC));
cleanup:
    ;
}

void
DJManageDaemonsDescription(
    BOOLEAN bStart,
    PSTR *description,
    LWException **exc
    );

static PSTR GetStopDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR daemonsDescription = NULL;

    LW_TRY(exc, DJManageDaemonsDescription(FALSE, &daemonsDescription,
        &LW_EXC));

cleanup:
    return daemonsDescription;
}

const JoinModule DJDaemonStopModule = { TRUE, "stop", "stop daemons", QueryStopDaemons, StopDaemons, GetStopDescription };

static QueryResult QueryStartDaemons(const JoinProcessOptions *options, LWException **exc)
{
    DWORD dwError = 0;
    BOOLEAN running;
    QueryResult result = FullyConfigured;
    const ModuleState *stopState = DJGetModuleStateByName((JoinProcessOptions *)options, "stop");

    if(!options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    dwError = DJGetServiceStatus("gpagent", &running);
    if (dwError == LW_ERROR_NO_SUCH_SERVICE)
    {
        /* The gpagentd may not be installed so ignore */
        running = TRUE;
        dwError = 0;
    }
    if (dwError)
    {
        LW_RAISE_EX(exc, dwError,
                "Received error while querying lwsmd.",
                "Received error while querying lwsmd.");
        goto cleanup;
    }

    if(!running)
        result = NotConfigured;

    if(stopState != NULL && stopState->disposition == EnableModule)
        result = NotConfigured;

cleanup:
    return result;
}

static void StartDaemons(JoinProcessOptions *options, LWException **exc)
{
    LW_TRY(exc, DJManageDaemons(TRUE, &LW_EXC));
cleanup:
    ;
}

static PSTR GetStartDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;

    LW_TRY(exc, DJManageDaemonsDescription(TRUE, &ret,
        &LW_EXC));

cleanup:
    return ret;
}

const JoinModule DJDaemonStartModule = { TRUE, "start", "start daemons", QueryStartDaemons, StartDaemons, GetStartDescription };

void DJRestartIfRunning(PCSTR daemon, LWException **exc)
{
    BOOLEAN running;
    LWException *inner = NULL;

    DJGetDaemonStatus(daemon, &running, &inner);
    if(!LW_IS_OK(inner) && inner->code == ERROR_SERVICE_NOT_FOUND)
    {
        //The daemon isn't installed
        LW_HANDLE(&inner);
        running = FALSE;
    }
    LW_CLEANUP(exc, inner);
    if(!running)
        goto cleanup;

    DJ_LOG_INFO("Restarting '%s'", daemon);
    LW_TRY(exc, DJStartStopDaemon(daemon, FALSE, &LW_EXC));
    DJ_LOG_INFO("Starting '%s'", daemon);
    LW_TRY(exc, DJStartStopDaemon(daemon, TRUE, &LW_EXC));

cleanup:
    LW_HANDLE(&inner);
}

void
DJManageDaemonsDescription(
    BOOLEAN bStart,
    PSTR *description,
    LWException **exc
    )
{
    BOOLEAN bFileExists = TRUE;
    LWException *innerExc = NULL;
    StringBuffer buffer;
    PSTR daemonDescription = NULL;

    LW_CLEANUP_CTERR(exc, CTStringBufferConstruct(&buffer));

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(PWGRD, &bFileExists));
    if(bFileExists && bStart)
    {
        LW_CLEANUP_CTERR(exc, CTStringBufferAppend(&buffer, "Shutdown pwgrd because it only handles usernames up to 8 characters long. This is done by running '/sbin/init.d/pwgr stop' and setting PWGR=0 in "PWGRD"."));
    }

    *description = CTStringBufferFreeze(&buffer);

cleanup:
    CT_SAFE_FREE_STRING(daemonDescription);
    LW_HANDLE(&innerExc);
    CTStringBufferDestroy(&buffer);
}

DWORD
DJGetBaseDaemonPriorities(
    int *startPriority,
    int *stopPriority,
    int *stopLaterOffset
    )
{
    LwDistroInfo distro;
    DWORD ceError = ERROR_SUCCESS;

    memset(&distro, 0, sizeof(distro));

    ceError = DJGetDistroInfo(NULL, &distro);
    GOTO_CLEANUP_ON_DWORD(ceError);

    if (distro.os == OS_HPUX)
    {
        // Start after S590Rpcd
        *startPriority = 591;
        // Stop before K410Rpcd
        *stopPriority = 401;

        // On HP-UX, the kill scripts are run in numerical order. So adding 1
        // the priority causes it to stop later in the shutdown.
        *stopLaterOffset = 1;
    }
    else
    {
        *startPriority = 19;
        *stopPriority = 9;

        // On Linux, the stop scrips are run in descending order
        *stopLaterOffset = -1;
    }

cleanup:
    DJFreeDistroInfo(&distro);
    return ceError;
}

void
DJManageDaemons(
    BOOLEAN bStart,
    LWException **exc
    )
{
    BOOLEAN bFileExists = TRUE;
    FILE* fp = NULL;
    LWException *innerExc = NULL;
    int i;
    BOOLEAN bLsassContacted = FALSE;
    DWORD dwError = 0;
    LW_HANDLE hLsa = NULL;

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(PWGRD, &bFileExists));
    if(bFileExists)
    {
        //Shutdown pwgr (a nscd-like daemon) on HP-UX because it only handles
        //usernames up to 8 characters in length.
        LW_TRY(exc, DJStartStopDaemon("pwgr", FALSE, &LW_EXC));
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(PWGRD, PWGRD, FALSE, "s/=1/=0/"));
    }

    if(bStart)
    {
        // Set registry value for gpagentd to autostart.
        dwError = SetBooleanRegistryValue("Services\\gpagent", "Autostart", TRUE);
        // Trigger gpagentd start
        dwError = DJStartService("gpagent");

        // Make sure lsass is responding
        bLsassContacted = FALSE;
        for (i = 0; !bLsassContacted && i < 30; i++)
        {
            DJ_LOG_INFO("Trying to contact lsassd");
            if (hLsa)
            {
                LsaCloseServer(hLsa);
                hLsa = NULL;
            }
            dwError = LsaOpenServer(&hLsa);
            if (dwError == ERROR_FILE_NOT_FOUND ||
                    dwError == LW_ERROR_ERRNO_ECONNREFUSED)
            {
                DJ_LOG_INFO("Failed with %d", dwError);
                dwError = 0;
                sleep(1);
                continue;
            }
            LW_CLEANUP_CTERR(exc, dwError);
            bLsassContacted = TRUE;
        }
        if (!bLsassContacted)
        {
            LW_RAISE_EX(exc, ERROR_SERVICE_NOT_ACTIVE, "Unable to reach lsassd", "The lsass daemon could not be reached for 30 seconds after trying to start it. Please verify it is running.");
            goto cleanup;
        }
    }
    else
    {
        dwError = SetBooleanRegistryValue("Services\\gpagent", "Autostart", FALSE);
        dwError = DJStopService("gpagent");

        dwError = SetBooleanRegistryValue("Services\\autoenroll", "Autostart", FALSE);
        dwError = DJStopService("autoenroll");

        dwError = SetBooleanRegistryValue("Services\\lwsc", "Autostart", FALSE);
        dwError = DJStopService("lwsc");

        dwError = SetBooleanRegistryValue("Services\\lwpkcs11", "Autostart", FALSE);
        dwError = DJStopService("lwpkcs11");
    }

cleanup:
    CTSafeCloseFile(&fp);

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    LW_HANDLE(&innerExc);
}
