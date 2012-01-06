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
#include "ctfileutils.h"

typedef enum
{
    CSPSearchPath = 0,
    NSPSearchPath
} SearchPolicyType;

#define LWDSPLUGIN_NAME         "/Likewise - Active Directory"
#define APPLEADDSPLUGIN_NAME    "^Active Directory"
#define PID_FILE_CONTENTS_SIZE   ((9 * 2) + 2)
#define CONFIGD_PID_FILE         "/var/run/configd.pid"

static
DWORD
DJSetSearchPath(
   SearchPolicyType searchPolicyType
   )
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    LONG status = 0;

    DJ_LOG_INFO("Setting search policy to %s", searchPolicyType == CSPSearchPath ? "Custom path" : "Automatic");

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)(PVOID)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/Search", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-create", ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/", ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("SearchPolicy", ppszArgs+4);
    BAIL_ON_CENTERIS_ERROR(ceError);

    switch (searchPolicyType)
    {
           case CSPSearchPath:
           {
                ceError = CTAllocateString("CSPSearchPath", ppszArgs+5);
                BAIL_ON_CENTERIS_ERROR(ceError);

                break;
           }
           case NSPSearchPath:
           {
                ceError = CTAllocateString("NSPSearchPath", ppszArgs+5);
                BAIL_ON_CENTERIS_ERROR(ceError);

                break;
           }
    }

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0)
    {
       ceError = ERROR_BAD_COMMAND;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (ppszArgs)
    {
       CTFreeStringArray(ppszArgs, nArgs);
    }

    if (pProcInfo)
    {
       FreeProcInfo(pProcInfo);
    }

    return ceError;
}

/* Use dscl to place the DSPlugin in the authenticator list */
static
DWORD
DJRegisterDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    LONG status = 0;
    DWORD retryCount = 3;

    DJ_LOG_INFO("Registering DSPlugin for Macintosh Directory Services Authentication");

    ceError = DJSetSearchPath(CSPSearchPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)(PVOID)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/Search", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-append", ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/", ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("CSPSearchPath", ppszArgs+4);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(LWDSPLUGIN_NAME, ppszArgs+5);
    BAIL_ON_CENTERIS_ERROR(ceError);

    while (retryCount)
    {
        ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = DJGetProcessStatus(pProcInfo, &status);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (status == 0)
        {
            goto error;
        }

        if (pProcInfo)
        {
            FreeProcInfo(pProcInfo);
            pProcInfo = NULL;
        }

        retryCount--;

        sleep(5);

        // Set last error
        ceError = ERROR_REGISTRY_IO_FAILED;
    }

error:

    if (ppszArgs)
    {
       CTFreeStringArray(ppszArgs, nArgs);
    }

    if (pProcInfo)
    {
       FreeProcInfo(pProcInfo);
    }

    return ceError;
}

/* Remove DSPlugin from the authenticator list */
static
DWORD
DJUnregisterDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    LONG status = 0;

    DJ_LOG_INFO("Unregistering DSPlugin from Open Directory Authentication");

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)(PVOID)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/Search", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-delete", ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/", ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("CSPSearchPath", ppszArgs+4);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(LWDSPLUGIN_NAME, ppszArgs+5);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0)
    {
       ceError = ERROR_REGISTRY_IO_FAILED;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = DJSetSearchPath(NSPSearchPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    if (ppszArgs)
    {
       CTFreeStringArray(ppszArgs, nArgs);
    }

    if (pProcInfo)
    {
       FreeProcInfo(pProcInfo);
    }

    return ceError;
}

static
DWORD
DJFlushCache(
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int i;
    const char* cacheUtils[] = {
        "/usr/sbin/lookupd", /* Before Mac OS X 10.5 */
        "/usr/bin/dscacheutil" /* On Mac OS X 10.5 */
    };

    DJ_LOG_INFO("Flushing dns cache");

    for (i = 0; i < (sizeof(cacheUtils) / sizeof(cacheUtils[0])); i++)
    {
        const char* command = cacheUtils[i];
        BOOLEAN exists;

        /* Sanity check */
        if (!command)
        {
            continue;
        }

        ceError = CTCheckFileExists(command, &exists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!exists)
        {
            continue;
        }

        ceError = CTShell("%command -flushcache",
                          CTSHELL_STRING(command, command));
        /* Bail regardless */
        goto error;
    }

    DJ_LOG_ERROR("Could not locate cache flush utility");
    ceError = ERROR_FILE_NOT_FOUND;

error:
    return ceError;
}


DWORD
DJConfigureDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;

    ceError = DJRegisterDSPlugin();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJFlushCache();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

DWORD
DJUnconfigureDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;

    ceError = DJUnregisterDSPlugin();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJFlushCache();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

DWORD
DJIsAppleADPluginInUse(BOOLEAN* pExists)
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR value = NULL;
    PSTR valueStart = NULL;
    BOOLEAN bInUse = FALSE;

    DJ_LOG_INFO("Testing to see if Apple AD plugin is already in use");

    ceError = CTCaptureOutput("/usr/bin/dscl localhost -list / | grep \"^Active Directory\"", &value);
    if (ceError)
    {
        ceError = ERROR_SUCCESS;
        goto error;
    }

    CTStripWhitespace(value);
    valueStart = strstr(value, "Active Directory");
    if (valueStart != NULL)
    {
        bInUse = TRUE;
    }

error:

    CT_SAFE_FREE_STRING(value);

    *pExists = bInUse;

    return ceError;
}

#if 0
DWORD
DJIsAppleADPluginInUse(BOOLEAN* pExists)
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 8;
    LONG status = 0;
    BOOLEAN bInUse = FALSE;

    DJ_LOG_INFO("Testing to see if Apple AD plugin is already in use");

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)(PVOID)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("localhost", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-list", ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/", ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("|", ppszArgs+4);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("grep", ppszArgs+5);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(APPLEADDSPLUGIN_NAME, ppszArgs+6);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status == 0)
    {
        bInUse = TRUE;
    }

error:

    if (ppszArgs)
    {
       CTFreeStringArray(ppszArgs, nArgs);
    }

    if (pProcInfo)
    {
       FreeProcInfo(pProcInfo);
    }

    *pExists = bInUse;

    return ceError;
}
#endif

static QueryResult QueryDSPlugin(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN exists = FALSE;
    QueryResult result = NotConfigured;
    PSTR value = NULL;
    PSTR valueStart = NULL;
    BOOLEAN bLikewisePresent = FALSE;

    if (options->enableMultipleJoins)
    {
        result = NotApplicable;
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/usr/bin/dscl", &exists));
    if(!exists)
    {
        result = NotApplicable;
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, DJIsAppleADPluginInUse(&exists));
    if(exists)
    {
        result = ApplePluginInUse;
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTCaptureOutput("/usr/bin/dscl /Search -read / dsAttrTypeStandard:SearchPolicy", &value));
    CTStripWhitespace(value);
    valueStart = value;
    if(CTStrStartsWith(valueStart, "SearchPolicy:"))
        valueStart += strlen("SearchPolicy:");
    CTStripWhitespace(valueStart);
    if(CTStrStartsWith(valueStart, "dsAttrTypeStandard:"))
      valueStart += strlen("dsAttrTypeStandard:");
    CTStripWhitespace(valueStart);
    if(options->joiningDomain)
    {
        if(strcmp(valueStart, "CSPSearchPath"))
            goto cleanup;
    }
    else
    {
        if(strcmp(valueStart, "NSPSearchPath"))
            goto cleanup;
    }
    CT_SAFE_FREE_STRING(value);

    LW_CLEANUP_CTERR(exc, CTCaptureOutput("/usr/bin/dscl /Search -read / dsAttrTypeStandard:CSPSearchPath", &value));
    CTStripWhitespace(value);
    valueStart = strstr(value, LWDSPLUGIN_NAME);
    if (valueStart == NULL)
    {
        bLikewisePresent = FALSE;
    }
    else
    {
        switch (valueStart[strlen(LWDSPLUGIN_NAME)])
        {
            case 0:
            case '\r':
            case '\n':
                bLikewisePresent = TRUE;
                break;
            default:
                bLikewisePresent = FALSE;
        }
    }

    if( (bLikewisePresent != 0) != (options->joiningDomain != 0) )
        goto cleanup;
    CT_SAFE_FREE_STRING(value);

    result = FullyConfigured;

cleanup:
    CT_SAFE_FREE_STRING(value);
    return result;
}

static void DoDSPlugin(JoinProcessOptions *options, LWException **exc)
{
    if(options->joiningDomain)
        LW_CLEANUP_CTERR(exc, DJConfigureDSPlugin());
    else
        LW_CLEANUP_CTERR(exc, DJUnconfigureDSPlugin());
cleanup:
    ;
}

static PSTR GetDSPluginDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(
                    "The PowerBroker Identity Services Directory Services plugin will be enabled by adding it to the custom search path and switching the search policy to custom.",
                    &ret));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(
                    "The PowerBroker Identity Services Directory Services plugin will removed from the custom search path and and the search policy will be switched back to standard.",
                    &ret));
    }

cleanup:
    return ret;
}

const JoinModule DJDSPlugin = { TRUE, "dsplugin", "enable PBIS directory services plugin", QueryDSPlugin, DoDSPlugin, GetDSPluginDescription};
