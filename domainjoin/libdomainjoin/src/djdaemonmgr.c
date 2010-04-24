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
#include "djdaemonmgr.h"
#include "ctstrutils.h"
#include "djauthinfo.h"
#include "djdistroinfo.h"
#include <lsa/lsa.h>

// aka: CENTERROR_LICENSE_INCORRECT
static DWORD GPAGENT_LICENSE_ERROR = 0x00002001;

// CENTERROR_LICENSE_EXPIRED
static DWORD GPAGENT_LICENSE_EXPIRED_ERROR = 0x00002002;

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))
#define PWGRD "/etc/rc.config.d/pwgr"

static QueryResult QueryStopDaemons(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN running;
    QueryResult result = FullyConfigured;
    LWException *inner = NULL;

    /* Check for lwiauthd and likewise-open */

    DJGetDaemonStatus("gpagentd", &running, &inner);
    if (!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        /* The gpagentd may not be installed so ignore */
        LW_HANDLE(&inner);
        running = FALSE;
    }
    LW_CLEANUP(exc, inner);

    if(running)
        result = NotConfigured;
    
cleanup:
    LW_HANDLE(&inner);
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
    BOOLEAN running;
    QueryResult result = FullyConfigured;
    const ModuleState *stopState = DJGetModuleStateByName((JoinProcessOptions *)options, "stop");
    LWException *inner = NULL;

    if(!options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    DJGetDaemonStatus("gpagentd", &running, &inner);
    if (!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        /* The gpagentd may not be installed so ignore */
        LW_HANDLE(&inner);
        running = TRUE;
    }
    LW_CLEANUP(exc, inner);

    if(!running)
        result = NotConfigured;
    
    if(stopState != NULL && stopState->runModule)
        result = NotConfigured;

cleanup:
    LW_HANDLE(&inner);
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
    PSTR daemonsDescription = NULL;
    PSTR ret = NULL;

    LW_TRY(exc, DJManageDaemonsDescription(TRUE, &daemonsDescription,
        &LW_EXC));

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret,
        "rm /var/lib/lwidentity/*_cache.tdb\n%s", daemonsDescription));

cleanup:
    CT_SAFE_FREE_STRING(daemonsDescription);
    return ret;
}

const JoinModule DJDaemonStartModule = { TRUE, "start", "start daemons", QueryStartDaemons, StartDaemons, GetStartDescription };

void DJRestartIfRunning(PCSTR daemon, LWException **exc)
{
    BOOLEAN running;
    LWException *inner = NULL;

    DJGetDaemonStatus(daemon, &running, &inner);
    if(!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
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
    int daemonCount;
    int i;
    int j;
    StringBuffer buffer;
    PSTR daemonDescription = NULL;

    LW_CLEANUP_CTERR(exc, CTStringBufferConstruct(&buffer));

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(PWGRD, &bFileExists));
    if(bFileExists && bStart)
    {
        LW_CLEANUP_CTERR(exc, CTStringBufferAppend(&buffer, "Shutdown pwgrd because it only handles usernames up to 8 characters long. This is done by running '/sbin/init.d/pwgr stop' and setting PWGR=0 in "PWGRD"."));
    }

    //Figure out how many daemons there are
    for(daemonCount = 0; daemonList[daemonCount].primaryName != NULL; daemonCount++);

    if(bStart)
    {
        //Start the daemons in ascending order
        i = 0;
    }
    else
    {
        i = daemonCount - 1;
    }
    while(TRUE)
    {
        if(i >= daemonCount)
            break;
        if(i < 0)
            break;

        CT_SAFE_FREE_STRING(daemonDescription);


        DJManageDaemonDescription(daemonList[i].primaryName,
                         bStart,
                         daemonList[i].startPriority,
                         daemonList[i].stopPriority,
                         &daemonDescription,
                         &innerExc);

        //Try the alternate daemon name if there is one
        for(j = 0; !LW_IS_OK(innerExc) &&
                innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                daemonList[i].alternativeNames[j] != NULL; j++)
        {
            LW_HANDLE(&innerExc);
            DJManageDaemonDescription(daemonList[i].alternativeNames[j],
                             bStart,
                             daemonList[i].startPriority,
                             daemonList[i].stopPriority,
                             &daemonDescription,
                             &innerExc);
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
            {
                LW_HANDLE(&innerExc);
            }
            else
                break;
        }
        if (!LW_IS_OK(innerExc) &&
                innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                !daemonList[i].required)
        {
            LW_HANDLE(&innerExc);
        }
        LW_CLEANUP(exc, innerExc);

        if(daemonDescription != NULL)
        {
            LW_CLEANUP_CTERR(exc, CTStringBufferAppend(&buffer, daemonDescription));
        }

        if(bStart)
            i++;
        else
            i--;
    }

    *description = CTStringBufferFreeze(&buffer);

cleanup:
    CT_SAFE_FREE_STRING(daemonDescription);
    LW_HANDLE(&innerExc);
    CTStringBufferDestroy(&buffer);
}

CENTERROR
DJGetBaseDaemonPriorities(
    int *startPriority,
    int *stopPriority,
    int *stopLaterOffset
    )
{
    DistroInfo distro;
    CENTERROR ceError = CENTERROR_SUCCESS;

    memset(&distro, 0, sizeof(distro));

    ceError = DJGetDistroInfo(NULL, &distro);
    GOTO_CLEANUP_ON_CENTERROR(ceError);

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
    PSTR pszErrFilePath = "/var/cache/likewise/grouppolicy/gpagentd.err";
    CHAR szBuf[256+1];
    DWORD dwGPErrCode = 0;
    LWException *innerExc = NULL;
    int daemonCount;
    int i;
    int j;
    PLSA_LOG_INFO pLogInfo = NULL;
    BOOLEAN bLsassContacted = FALSE;
    DWORD dwError = 0;
    LW_HANDLE hLsa = NULL;
    int firstStart = 0;
    int firstStop = 0;
    int stopLaterOffset = 0;

    LW_TRY(exc, DJGetBaseDaemonPriorities(
                &firstStart,
                &firstStop,
                &stopLaterOffset));

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(PWGRD, &bFileExists));
    if(bFileExists)
    {
        //Shutdown pwgr (a nscd-like daemon) on HP-UX because it only handles
        //usernames up to 8 characters in length.
        LW_TRY(exc, DJStartStopDaemon("pwgr", FALSE, &LW_EXC));
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(PWGRD, PWGRD, FALSE, "s/=1/=0/"));
    }

    //Figure out how many daemons there are
    for(daemonCount = 0; daemonList[daemonCount].primaryName != NULL; daemonCount++);

    if(bStart)
    {
        //Start the daemons in ascending order
        for(i = 0; i < daemonCount; i++)
        {
 
            DJManageDaemon(daemonList[i].primaryName,
                             bStart,
                             firstStart + daemonList[i].startPriority,
                             firstStop +
                                 stopLaterOffset * daemonList[i].stopPriority,
                             &innerExc);

            //Try the alternate daemon name if there is one
            for(j = 0; !LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    daemonList[i].alternativeNames[j] != NULL; j++)
            {
                LW_HANDLE(&innerExc);
                DJManageDaemon(daemonList[i].alternativeNames[j],
                                 bStart,
                                 firstStart + daemonList[i].startPriority,
                                 firstStop + stopLaterOffset *
                                     daemonList[i].stopPriority,
                                 &innerExc);
                if (!LW_IS_OK(innerExc) &&
                        innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
                {
                    LW_HANDLE(&innerExc);
                }
                else
                    break;
            }
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    !daemonList[i].required)
            {
                LW_HANDLE(&innerExc);
            }
            if (LW_IS_OK(innerExc) && !strcmp(daemonList[i].primaryName, "gpagentd"))
            {
                LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszErrFilePath, &bFileExists));

                if (bFileExists) {

                    LW_HANDLE(&innerExc);
                    fp = fopen(pszErrFilePath, "r");
                    if (fp != NULL) {

                        if (fgets(szBuf, 256, fp) != NULL) {

                            CTStripWhitespace(szBuf);

                            dwGPErrCode = atoi(szBuf);

                            if (dwGPErrCode == GPAGENT_LICENSE_ERROR ||
                                dwGPErrCode == GPAGENT_LICENSE_EXPIRED_ERROR) {

                                LW_RAISE(exc, CENTERROR_DOMAINJOIN_LICENSE_ERROR);
                                goto cleanup;

                            }
                        }

                    } else {

                        DJ_LOG_ERROR("Failed to open file [%s]", pszErrFilePath);

                    }
                }
            }
            LW_CLEANUP(exc, innerExc);
        }

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
            LW_CLEANUP_LSERR(exc, dwError);
            LW_CLEANUP_LSERR(exc, LsaGetLogInfo(hLsa, &pLogInfo));
            bLsassContacted = TRUE;
        }
        if (!bLsassContacted)
        {
            LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INCORRECT_STATUS, "Unable to reach lsassd", "The lsass daemon could not be reached for 30 seconds after trying to start it. Please verify it is running.");
            goto cleanup;
        }
    }
    else
    {
        //Stop the daemons in descending order
        for(i = daemonCount - 1; i >= 0; i--)
        {
            DJManageDaemon(daemonList[i].primaryName,
                             bStart,
                             firstStart + daemonList[i].startPriority,
                             firstStop +
                                 stopLaterOffset * daemonList[i].stopPriority,
                             &innerExc);

            //Try the alternate daemon name if there is one
            for(j = 0; !LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    daemonList[i].alternativeNames[j] != NULL; j++)
            {
                LW_HANDLE(&innerExc);
                DJManageDaemon(daemonList[i].alternativeNames[j],
                                 bStart,
                                 firstStart + daemonList[i].startPriority,
                                 firstStop + stopLaterOffset *
                                     daemonList[i].stopPriority,
                                 &innerExc);
                if (!LW_IS_OK(innerExc) &&
                        innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
                {
                    LW_HANDLE(&innerExc);
                }
                else
                    break;
            }
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    !daemonList[i].required)
            {
                LW_HANDLE(&innerExc);
            }
            LW_CLEANUP(exc, innerExc);
        }
    }

cleanup:
    CTSafeCloseFile(&fp);
    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }
    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    LW_HANDLE(&innerExc);
}
