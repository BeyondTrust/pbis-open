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

#define LWIDSPLUGIN_SYMLINK_PATH "/System/Library/Frameworks/DirectoryService.framework/Versions/Current/Resources/Plugins/LWIDSPlugin.dsplug"
#define LWEDSPLUGIN_SYMLINK_PATH "/System/Library/Frameworks/DirectoryService.framework/Versions/Current/Resources/Plugins/LWEDSPlugin.dsplug"
#define LWIDSPLUGIN_INSTALL_PATH LIBDIR "/LWIDSPlugin.dsplug"
#define LWEDSPLUGIN_INSTALL_PATH LIBDIR "/LWEDSPlugin.dsplug"
#define LWDSPLUGIN_NAME         "/Likewise - Active Directory"
#define APPLEADDSPLUGIN_NAME    "^Active Directory"
#define PID_FILE_CONTENTS_SIZE   ((9 * 2) + 2)
#define CONFIGD_PID_FILE         "/var/run/configd.pid"

#if 0
static
DWORD
DJGetConfigDPID(
    pid_t* ppid
    )
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    char contents[PID_FILE_CONTENTS_SIZE];
    int  fd = -1;
    int  len = 0;

    ceError = CTCheckFileExists(CONFIGD_PID_FILE, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists)
    {
       *ppid = 0;
       goto error;
    }

    fd = open(CONFIGD_PID_FILE, O_RDONLY, 0644);
    if (fd < 0)
    {
       ceError = LwMapErrnoToLwError(errno);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((len = read(fd, contents, sizeof(contents)-1)) < 0)
    {
       ceError = LwMapErrnoToLwError(errno);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else if (len == 0)
    {
       *ppid = 0;
       goto error;
    }
    contents[len-1] = 0;

    *ppid = atoi(contents);

error:

    if (fd >= 0)
    {
       close(fd);
    }

    return ceError;
}
#endif /* 0 */

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

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
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

/* Use dscl to place the LWIDSPlugin in the authenticator list */
static
DWORD
DJRegisterLWIDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    LONG status = 0;
    DWORD retryCount = 3;

    DJ_LOG_INFO("Registering LWIDSPlugin for Macintosh Directory Services Authentication");

    ceError = DJSetSearchPath(CSPSearchPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
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

/* Remove LWIDSPlugin from the authenticator list */
static
DWORD
DJUnregisterLWIDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    LONG status = 0;

    DJ_LOG_INFO("Unregistering LWIDSPlugin from Open Directory Authentication");

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
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

/*
   The LWIDSPlugin is saved to /opt/likewise/lib/LWIDSPlugin.dsplug upon installation

   In order to participate in Open Directory Services, we need to create a symbolic link

   from

   /System/Library/Frameworks/DirectoryService.framework/Versions/Current/Resources/Plugins/LWIDSPlugin.dsplug

   to

   /opt/likewise/lib/LWIDSPlugin.dsplug
*/
#if 0
static
DWORD
DJEngageLWIDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bLinkExists = FALSE;
    BOOLEAN bCreateSymlink = TRUE;
    PSTR    pszTargetPath = NULL;

    ceError = CTCheckDirectoryExists(LWIDSPLUGIN_INSTALL_PATH, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bDirExists)
    {
       DJ_LOG_ERROR("LWIDSPlugin folder [%s] does not exist", LWIDSPLUGIN_INSTALL_PATH);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTCheckLinkExists(LWIDSPLUGIN_SYMLINK_PATH, &bLinkExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bLinkExists)
    {
       ceError = CTGetSymLinkTarget(LWIDSPLUGIN_SYMLINK_PATH, &pszTargetPath);
       BAIL_ON_CENTERIS_ERROR(ceError);

       if (strcmp(pszTargetPath, LWIDSPLUGIN_INSTALL_PATH))
       {
          DJ_LOG_INFO("Removing symbolic link at [%s]", LWIDSPLUGIN_SYMLINK_PATH);
          ceError = CTRemoveFile(LWIDSPLUGIN_SYMLINK_PATH);
          BAIL_ON_CENTERIS_ERROR(ceError);
       }
       else
       {
          bCreateSymlink = FALSE;
       }
    }

    if (bCreateSymlink)
    {
       ceError = CTCreateSymLink(LWIDSPLUGIN_SYMLINK_PATH, LWIDSPLUGIN_INSTALL_PATH);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pszTargetPath)
    {
       CTFreeString(pszTargetPath);
    }

    return ceError;
}

static
DWORD
DJDisengageLWIDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bLinkExists = FALSE;
    BOOLEAN bDirExists = FALSE;

    ceError = CTCheckLinkExists(LWIDSPLUGIN_SYMLINK_PATH, &bLinkExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bLinkExists)
    {
       DJ_LOG_INFO("Removing symbolic link at [%s]", LWIDSPLUGIN_SYMLINK_PATH);

       ceError = CTRemoveFile(LWIDSPLUGIN_SYMLINK_PATH);
       BAIL_ON_CENTERIS_ERROR(ceError);

       goto done;
    }

    /* If a directory exists in the place of the symlink, remove that instead */
    ceError = CTCheckDirectoryExists(LWIDSPLUGIN_SYMLINK_PATH, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists)
    {
       ceError = CTRemoveDirectory(LWIDSPLUGIN_SYMLINK_PATH);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

done:
error:

    return ceError;
}
#endif


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
DJConfigureLWIDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;

    ceError = DJRegisterLWIDSPlugin();
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJFlushCache();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

DWORD
DJUnconfigureLWIDSPlugin()
{
    DWORD ceError = ERROR_SUCCESS;

    ceError = DJUnregisterLWIDSPlugin();
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
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    LONG status = 0;
    BOOLEAN bInUse = FALSE;

    DJ_LOG_INFO("Testing to see if Apple AD plugin is already in use");

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/usr/bin/dscl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("localhost", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-list", ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("|", ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("grep", ppszArgs+4);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(APPLEADDSPLUGIN_NAME, ppszArgs+5);
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

static QueryResult QueryDSPlugin(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN exists = FALSE;
    QueryResult result = NotConfigured;
    PSTR value = NULL;
    PSTR valueStart = NULL;

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
    if( (strstr(value, LWDSPLUGIN_NAME) == NULL) == options->joiningDomain )
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
        LW_CLEANUP_CTERR(exc, DJConfigureLWIDSPlugin());
    else
        LW_CLEANUP_CTERR(exc, DJUnconfigureLWIDSPlugin());
cleanup:
    ;
}

static PSTR GetDSPluginDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(
                    "The Likewise Directory Services plugin will be enabled by adding it to the custom search path and switching the search policy to custom.",
                    &ret));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTStrdup(
                    "The Likewise Directory Services plugin will removed from the custom search path and and the search policy will be switched back to standard.",
                    &ret));
    }

cleanup:
    return ret;
}

const JoinModule DJDSPlugin = { TRUE, "dsplugin", "enable likewise directory services plugin", QueryDSPlugin, DoDSPlugin, GetDSPluginDescription};
