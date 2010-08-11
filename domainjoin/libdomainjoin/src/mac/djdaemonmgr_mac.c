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
#include <libxml/xpath.h>
#include <libxml/parser.h>
#include <lwsm/lwsm.h>
#include <lwerror.h>
#include <lwstr.h>
#include <lwmem.h>
#include <lw/base.h>

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

// aka: DWORD_LICENSE_INCORRECT
// static DWORD GPAGENT_LICENSE_ERROR = 0x00002001;

// DWORD_LICENSE_EXPIRED
// static DWORD GPAGENT_LICENSE_EXPIRED_ERROR = 0x00002002;

// static PSTR pszAuthdStartPriority = "90";
// static PSTR pszAuthdStopPriority = "10";
// static PSTR pszGPAgentdStartPriority = "91";
// static PSTR pszGPAgentdStopPriority = "9";

// Runs an xpath expression on an xml file. If the resultant nodeset contains
// exactly one text node, it is returned through result, otherwise
// ERROR_NOT_FOUND or ERROR_INVALID_DATA is returned.
static DWORD GetXPathString(PCSTR file, PSTR *result, PCSTR expression)
{
    xmlDocPtr xmlDoc = NULL;
    xmlXPathContextPtr xpathCtx = NULL; 
    xmlXPathObjectPtr xpathObj = NULL; 
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bExists = FALSE;

    *result = NULL;

    GCE(ceError = CTCheckFileExists(file, &bExists));
    if (bExists == FALSE)
        GCE(ceError = ERROR_BAD_FORMAT);

    xmlDoc = xmlReadFile(file, NULL, XML_PARSE_NONET | XML_PARSE_NOERROR);
    if(xmlDoc == NULL)
        GCE(ceError = ERROR_BAD_FORMAT);

    xpathCtx = xmlXPathNewContext(xmlDoc);
    if(xpathCtx == NULL)
        GCE(ceError = ERROR_OUTOFMEMORY);

    xpathObj = xmlXPathEvalExpression((xmlChar*)expression, xpathCtx);

    if(xpathObj == NULL)
        GCE(ceError = ERROR_BAD_FORMAT);

    if(xpathObj->type != XPATH_NODESET)
        GCE(ceError = ERROR_INVALID_DATA);
    if(xpathObj->nodesetval == NULL)
        GCE(ceError = ERROR_NOT_FOUND);
    if(xpathObj->nodesetval->nodeNr < 1)
        GCE(ceError = ERROR_NOT_FOUND);
    if(xpathObj->nodesetval->nodeNr > 1 ||
            xpathObj->nodesetval->nodeTab[0]->type != XML_TEXT_NODE)
    {
        GCE(ceError = ERROR_INVALID_DATA);
    }
    GCE(ceError = CTStrdup((PCSTR)xpathObj->nodesetval->nodeTab[0]->content, result));

cleanup:
    if(xpathObj != NULL)
        xmlXPathFreeObject(xpathObj);
    if(xpathCtx != NULL)
        xmlXPathFreeContext(xpathCtx);
    if(xmlDoc != NULL)
        xmlFreeDoc(xmlDoc);
    return ceError;
}

static DWORD DJDaemonLabelToConfigFile(PSTR *configFile, PCSTR dirName, PCSTR label)
{
    DIR *dir = NULL;
    PSTR filePath = NULL;
    struct dirent *dirEntry = NULL;
    PSTR fileLabel = NULL;
    DWORD ceError = ERROR_SUCCESS;

    *configFile = NULL;

    if ((dir = opendir(dirName)) == NULL) {
        GCE(ceError = LwMapErrnoToLwError(errno));
    }

    while(1)
    {
        errno = 0;
        dirEntry = readdir(dir);
        if(dirEntry == NULL)
        {
            if(errno != 0)
                GCE(ceError = LwMapErrnoToLwError(errno));
            else
            {
                //No error here. We simply read the last entry
                break;
            }
        }
        if(dirEntry->d_name[0] == '.')
            continue;

        CT_SAFE_FREE_STRING(filePath);
        GCE(ceError = CTAllocateStringPrintf(&filePath, "%s/%s",
                    dirName, dirEntry->d_name));

        CT_SAFE_FREE_STRING(fileLabel);
        ceError = GetXPathString(filePath, &fileLabel,
            "/plist/dict/key[text()='Label']/following-sibling::string[position()=1]/text()");
        if(ceError)
        {
            DJ_LOG_INFO("Cannot read daemon label from '%s' file. Error code %X. Ignoring it.", filePath, ceError);
            ceError = ERROR_SUCCESS;
            continue;
        }
        if(!strcmp(fileLabel, label))
        {
            //This is a match
            *configFile = filePath;
            filePath = NULL;
            goto cleanup;
        }
    }

    GCE(ceError = ERROR_SERVICE_NOT_FOUND);

cleanup:
    CT_SAFE_FREE_STRING(fileLabel);
    CT_SAFE_FREE_STRING(filePath);
    if(dir != NULL)
    {
        closedir(dir);
    }
    return ceError;
}

void
DJWaitDaemonStatus(
    PCSTR pszDaemonPath,
    BOOLEAN bStatus,
    LWException **exc
    )
{
    int count;
    int retry = 5;
    BOOLEAN bStarted = FALSE;

    for (count = 0; count < retry; count++) {

        LW_TRY(exc, DJGetDaemonStatus(pszDaemonPath, &bStarted, &LW_EXC));

        if (bStarted == bStatus) {
            break;
        }

        /* The Mac daemons may take a little longer to startup as they need to
           sequence a few conditions :
           1) netlogond needs to verify that the hostname is not set to localhost, not likely by
              by the time the user is logged on and running Domain Join tools.
           2) netlogon needs to wait about 5 seconds after testing hostname to wait for the resolver
              libraries to become usable. This is a fixed startup cost to the netlogon daemon, and it
              affects non-boot up time start operations also.
           4) netlogond then creates the PID file for itself.
           5) lsassd needs to stagger its start time to wait for netlogond to be online, it does this by
              similarly testing the hostname, and then waiting about 10 seconds before commencing.
           6) lsassd will then create its PID file and load the configured provider plugins. */

        sleep(5);
    }

    if (bStarted != bStatus) {

        if(bStatus)
        {
            LW_RAISE_EX(exc, ERROR_INVALID_STATE, "Unable to start daemon", "An attempt was made to start the '%s' daemon, but querying its status revealed that it did not start.", pszDaemonPath);
        }
        else
        {
            LW_RAISE_EX(exc, ERROR_INVALID_STATE, "Unable to stop daemon", "An attempt was made to stop the '%s' daemon, but querying its status revealed that it did not stop.", pszDaemonPath);
        }
        goto cleanup;
    }
cleanup:
    // NO-OP
    ;
}

static
DWORD
DJGetServiceStatus(
    PCSTR pszServiceName,
    PBOOLEAN pbStarted
    )
{
    DWORD dwError = 0;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_STATUS status = {0};
    PWSTR pwszServiceName = NULL;

    dwError = LwMbsToWc16s(pszServiceName, &pwszServiceName);
    BAIL_ON_CENTERIS_ERROR(dwError);

    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_CENTERIS_ERROR(dwError);

    dwError = LwSmQueryServiceStatus(hHandle, &status);
    BAIL_ON_CENTERIS_ERROR(dwError);

    *pbStarted = status.state == LW_SERVICE_STATE_RUNNING;

error:

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (pwszServiceName)
    {
        LwFreeMemory(pwszServiceName);
    }

    return dwError;
}

void
DJGetDaemonStatus(
    PCSTR pszDaemonPath,
    PBOOLEAN pbStarted,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    LONG status = 0;
    PPROCINFO pProcInfo = NULL;
    CHAR  szBuf[1024+1];
    FILE* fp = NULL;
    DWORD ceError;
    PSTR configFile = NULL;
    PSTR command = NULL;
    PSTR pszServiceName = NULL;
    int argNum = 0;
    PSTR whitePos = NULL;

    /* Translate the Unix daemon names into the mac daemon names */
    if(!strcmp(pszDaemonPath, "lsassd"))
        pszDaemonPath = "com.likewisesoftware.lsassd";
    else if(!strcmp(pszDaemonPath, "gpagentd"))
        pszDaemonPath = "com.likewisesoftware.gpagentd";
    else if (!strcmp(pszDaemonPath, "lwmgmtd"))
        pszDaemonPath = "com.likewisesoftware.lwmgmtd";
    else if (!strcmp(pszDaemonPath, "eventfwdd"))
        pszDaemonPath = "com.likewisesoftware.eventfwdd";
    else if (!strcmp(pszDaemonPath, "reapsysld"))
        pszDaemonPath = "com.likewisesoftware.reapsysld";
    else if (!strcmp(pszDaemonPath, "lwregd"))
        pszDaemonPath = "com.likewisesoftware.lwregd";
    else if (!strcmp(pszDaemonPath, "lwsmd"))
        pszDaemonPath = "com.likewisesoftware.lwsmd";

    /* Find the .plist file for the daemon */
    ceError = DJDaemonLabelToConfigFile(&configFile, "/Library/LaunchDaemons", pszDaemonPath);
    if(ceError == ERROR_SERVICE_NOT_FOUND)
        ceError = DJDaemonLabelToConfigFile(&configFile, SYSCONFDIR "/LaunchDaemons", pszDaemonPath);
    if(ceError == ERROR_SERVICE_NOT_FOUND)
    {
        DJ_LOG_ERROR("Checking status of daemon [%s] failed: Missing", pszDaemonPath);
        LW_RAISE_EX(exc, ERROR_SERVICE_NOT_FOUND, "Unable to find daemon plist file", "The plist file for the '%s' daemon could not be found in /Library/LaunchDaemons or " SYSCONFDIR "/LaunchDaemons .", pszDaemonPath);
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    DJ_LOG_INFO("Found config file [%s] for daemon [%s]", configFile, pszDaemonPath);

    /* Figure out the daemon binary by reading the command from the plist file
     */
    LW_CLEANUP_CTERR(exc, GetXPathString(configFile, &command,
        "/plist/dict/key[text()='ProgramArguments']/following-sibling::array[position()=1]/string[position()=1]/text()"));

    DJ_LOG_INFO("Found daemon binary [%s] for daemon [%s]", command, pszDaemonPath);

    if (strstr(command, "/bin/lwsm") != NULL)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateString(strrchr(pszDaemonPath, '.') + 1, &pszServiceName));

        if (pszServiceName[strlen(pszServiceName) - 1] == 'd')
        {
            pszServiceName[strlen(pszServiceName) - 1] = '\0';
        }

        LW_CLEANUP_CTERR(exc, DJGetServiceStatus(pszServiceName, pbStarted));
    }
    else
    {
        DJ_LOG_INFO("Checking status of daemon [%s]", pszDaemonPath);

        LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));
        
        LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/ps", ppszArgs + argNum++));
        
        LW_CLEANUP_CTERR(exc, CTAllocateString("-U", ppszArgs + argNum++));
        
        LW_CLEANUP_CTERR(exc, CTAllocateString("root", ppszArgs + argNum++));
        
        LW_CLEANUP_CTERR(exc, CTAllocateString("-o", ppszArgs + argNum++));
        
        LW_CLEANUP_CTERR(exc, CTAllocateString("command=", ppszArgs + argNum++));
        
        LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
        
        LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));
        
        *pbStarted = FALSE;
        if (!status) {
            
            fp = fdopen(pProcInfo->fdout, "r");
            if (!fp) {
                LW_CLEANUP_CTERR(exc, LwMapErrnoToLwError(errno));
            }
            
            while (1) {
                
                if (fgets(szBuf, 1024, fp) == NULL) {
                    if (!feof(fp)) {
                        LW_CLEANUP_CTERR(exc, LwMapErrnoToLwError(errno));
                    }
                    else
                        break;
                }
                
                CTStripWhitespace(szBuf);
                
                if (IsNullOrEmptyString(szBuf))
                    continue;
                
                whitePos = strchr(szBuf, ' ');
                if(whitePos != NULL)
                    *whitePos = '\0';
                
                if (!strcmp(szBuf, command)) {
                    *pbStarted = TRUE;
                    break;
                }
            }
        }
    }

cleanup:

    if (fp)
        fclose(fp);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(configFile);
    CT_SAFE_FREE_STRING(command);
    CT_SAFE_FREE_STRING(pszServiceName);
}

void
DJStartStopDaemon(
    PCSTR pszDaemonPath,
    BOOLEAN bStatus,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG status = 0;
    PPROCINFO pProcInfo = NULL;

    if (bStatus) {
        DJ_LOG_INFO("Starting daemon [%s]", pszDaemonPath);
    } else {
        DJ_LOG_INFO("Stopping daemon [%s]", pszDaemonPath);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));
    LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/launchctl", ppszArgs));

    if (bStatus)
    {
       LW_CLEANUP_CTERR(exc, CTAllocateString("start", ppszArgs+1));
    }
    else
    {
       LW_CLEANUP_CTERR(exc, CTAllocateString("stop", ppszArgs+1));
    }

    LW_CLEANUP_CTERR(exc, CTAllocateString(pszDaemonPath, ppszArgs+2));
    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    LW_TRY(exc, DJWaitDaemonStatus(pszDaemonPath, bStatus, &LW_EXC));

cleanup:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);
}

static
DWORD
DJExistsInLaunchCTL(
    PCSTR pszName,
    PBOOLEAN pbExists
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    LONG  status = 0;
    DWORD nArgs = 3;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];
    BOOLEAN bExists = FALSE;

    ceError = CTAllocateMemory(nArgs*sizeof(PSTR), (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/launchctl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("list", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = ERROR_BAD_COMMAND;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fp = fdopen(pProcInfo->fdout, "r");
    if (fp == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    for(;;)
    {
        if (fgets(szBuf, 1024, fp) == NULL)
        {
            if (feof(fp))
            {
                break;
            }
            else
            {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        CTStripWhitespace(szBuf);

        if (!strcmp(szBuf, pszName))
        {
            bExists = TRUE;
            break;
        }
    }

error:

    *pbExists = bExists;

    if (fp)
        fclose(fp);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    return ceError;
}

static
DWORD
DJPrepareServiceLaunchScript(
    PCSTR pszName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    LONG  status = 0;
    DWORD nArgs = 5;
    CHAR szBuf[1024+1];
    BOOLEAN bFileExists = FALSE;

    ceError = CTAllocateMemory(nArgs*sizeof(PSTR), (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/cp", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-f", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, SYSCONFDIR "/LaunchDaemons/%s.plist", pszName);
    ceError = CTCheckFileExists(szBuf, &bFileExists);  
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists)
	BAIL_ON_CENTERIS_ERROR(ceError = ERROR_SERVICE_NOT_FOUND);

    ceError = CTAllocateString(szBuf, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "/Library/LaunchDaemons/%s.plist", pszName);
    ceError = CTAllocateString(szBuf, ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = ERROR_BAD_COMMAND;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

static
DWORD
DJRemoveFromLaunchCTL(
    PCSTR pszName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bExistsInLaunchCTL = FALSE;
    CHAR    szBuf[1024+1];
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG  status = 0;
    LWException* innerException = NULL;

    ceError = DJExistsInLaunchCTL(pszName, &bExistsInLaunchCTL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExistsInLaunchCTL) {

        sprintf(szBuf, "/Library/LaunchDaemons/%s.plist", pszName);

        ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("/bin/launchctl", ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("unload", ppszArgs+1);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(szBuf, ppszArgs+2);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = DJGetProcessStatus(pProcInfo, &status);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (status != 0) {
            ceError = ERROR_BAD_COMMAND;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        DJWaitDaemonStatus(pszName, FALSE, &innerException);
        if (innerException)
        {
            ceError = innerException->code;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        sprintf(szBuf, "/Library/LaunchDaemons/%s.plist", pszName);
        ceError = CTRemoveFile(szBuf);
        if (ceError) {
            DJ_LOG_WARNING("Failed to remove file [%s]", szBuf);
            ceError = ERROR_SUCCESS;
        }
    }

error:
    LW_HANDLE(&innerException);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

static
DWORD
DJAddToLaunchCTL(
    PCSTR pszName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bExistsInLaunchCTL = FALSE;
    BOOLEAN bFileExists = FALSE;
    CHAR    szBuf[1024+1];
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG  status = 0;

    memset(szBuf, 0, sizeof(szBuf));
    sprintf(szBuf, "/Library/LaunchDaemons/%s.plist", pszName);

    ceError = CTCheckFileExists(szBuf, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {

        ceError = DJPrepareServiceLaunchScript(pszName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTCheckFileExists(szBuf, &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (!bFileExists) {
            ceError = ERROR_SERVICE_NOT_FOUND;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    ceError = DJExistsInLaunchCTL(pszName, &bExistsInLaunchCTL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bExistsInLaunchCTL)
    {
        sprintf(szBuf, "/Library/LaunchDaemons/%s.plist", pszName);

        ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("/bin/launchctl", ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("load", ppszArgs+1);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(szBuf, ppszArgs+2);
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

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
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
    const char* pszDaemonPath = pszName;

    DJ_LOG_INFO("Processing call to DJManageDaemon('%s',%s)", pszDaemonPath, bStatus ? "start" : "stop");

    /* Translate the Unix daemon names into the mac daemon names */
    if(!strcmp(pszName, "likewise-open"))
        pszDaemonPath = "com.likewise.open";
    else if(!strcmp(pszName, "gpagentd"))
        pszDaemonPath = "com.likewisesoftware.gpagentd";
    else if(!strcmp(pszName, "lsassd"))
        pszDaemonPath = "com.likewisesoftware.lsassd";
    else if(!strcmp(pszName, "netlogond"))
        pszDaemonPath = "com.likewisesoftware.netlogond";
    else if(!strcmp(pszName, "lwiod"))
        pszDaemonPath = "com.likewisesoftware.lwiod";
    else if(!strcmp(pszName, "eventlogd"))
        pszDaemonPath = "com.likewisesoftware.eventlogd";
    else if(!strcmp(pszName, "dcerpcd"))
        pszDaemonPath = "com.likewisesoftware.dcerpcd";
    else if (!strcmp(pszName, "lwmgmtd"))
        pszDaemonPath = "com.likewisesoftware.lwmgmtd";
    else if (!strcmp(pszName, "srvsvcd"))
        pszDaemonPath = "com.likewisesoftware.srvsvcd";
    else if (!strcmp(pszName, "eventfwdd"))
        pszDaemonPath = "com.likewisesoftware.eventfwdd";
    else if (!strcmp(pszName, "reapsysld"))
        pszDaemonPath = "com.likewisesoftware.reapsysld";
    else if (!strcmp(pszName, "lwregd"))
        pszDaemonPath = "com.likewisesoftware.lwregd";
    else if (!strcmp(pszName, "lwsmd"))
        pszDaemonPath = "com.likewisesoftware.lwsmd";
    else
        pszDaemonPath = pszName;

    DJ_LOG_INFO("DJManageDaemon('%s',%s) after name fixup", pszDaemonPath, bStatus ? "start" : "stop");

    if (bStatus)
    {
        LW_CLEANUP_CTERR(exc, DJAddToLaunchCTL(pszDaemonPath));
    }

    // check our current state prior to doing anything.  notice that
    // we are using the private version so that if we fail, our inner
    // exception will be the one that was tossed due to the failure.
    LW_TRY(exc, DJGetDaemonStatus(pszDaemonPath, &bStarted, &LW_EXC));

    // if we got this far, we have validated the existence of the
    // daemon and we have figured out if its started or stopped

    // if we are already in the desired state, do nothing.
    if (bStarted != bStatus) {

        LW_TRY(exc, DJStartStopDaemon(pszDaemonPath, bStatus, &LW_EXC));

    }
    else
    {
        DJ_LOG_INFO("daemon '%s' is already %s", pszDaemonPath, bStarted ? "started" : "stopped");
    }

    if (!bStatus)
    {
        // Daemon is no longer needed, we are leaving the domain.
        LW_CLEANUP_CTERR(exc, DJRemoveFromLaunchCTL(pszDaemonPath));
    }

cleanup:
    ;
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
    BOOLEAN bStarted = FALSE;

    *description = NULL;

    LW_TRY(exc, DJGetDaemonStatus(pszName, &bStarted, &LW_EXC));

    // if we got this far, we have validated the existence of the
    // daemon and we have figured out if its started or stopped

    // if we are already in the desired state, do nothing.
    if (bStarted != bStatus) {

        if(bStatus)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(description,
                    "Start %s by running '/bin/launchctl start /Library/LaunchDaemons/%s.plist'.\n", pszName, pszName));
        }
        else
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(description,
                    "Stop %s by running '/bin/launchctl unload /Library/LaunchDaemons/%s.plist'.\n", pszName, pszName));
        }
    }

cleanup:
    ;
}

struct _DaemonList daemonList[] = {
    { "com.likewisesoftware.gpagentd", {NULL}, FALSE, 22, 9 },
    //{ "com.likewisesoftware.eventfwdd", {NULL}, FALSE, 21, 9 },
    //{ "com.likewisesoftware.reapsysld", {NULL}, FALSE, 12, 9 },
    { NULL, {NULL}, FALSE, 0, 0 },
};

