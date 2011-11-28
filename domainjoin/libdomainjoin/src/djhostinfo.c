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
#include "djauthinfo.h"
#include "djdistroinfo.h"
#include "lwmem.h"
#include "lwstr.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

DWORD
DJGetComputerName(
    PSTR* ppszComputerName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    CHAR szBuf[256+1];
    PSTR pszTmp = NULL;

    if (gethostname(szBuf, 256) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszTmp = szBuf;
    while (*pszTmp != '\0') {
        if (*pszTmp == '.') {
            *pszTmp = '\0';
            break;
        }
        pszTmp++;
    }

    if (IsNullOrEmptyString(szBuf)) {
        ceError = ERROR_INVALID_COMPUTERNAME;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateString(szBuf, ppszComputerName);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
DWORD
WriteHostnameToFiles(
    PSTR pszComputerName,
    PSTR* ppszHostfilePaths
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszFilePath = (ppszHostfilePaths ? *ppszHostfilePaths : NULL);
    BOOLEAN bFileExists = FALSE;
    FILE* fp = NULL;

    while (pszFilePath != NULL && *pszFilePath != '\0') {

        ceError = CTCheckFileExists(pszFilePath, &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            fp = fopen(pszFilePath, "w");
            if (fp == NULL) {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            if (fprintf(fp, "%s\n", pszComputerName) < 0)
            {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

            fclose(fp);
            fp = NULL;
        }

        pszFilePath = *(++ppszHostfilePaths);
    }

error:

    if (fp) {
        fclose(fp);
    }

    return ceError;
}

#define NETCONF "/etc/rc.config.d/netconf"
static DWORD SetHPUXHostname(PSTR pszComputerName)
{
  DWORD ceError = ERROR_SUCCESS;
  PPROCINFO pProcInfo = NULL;
  PSTR *ppszArgs = NULL;
  DWORD nArgs = 6;
  CHAR szBuf[512];
  LONG status = 0;

  DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

  ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("/bin/sh", ppszArgs);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("-c", ppszArgs+1);
  BAIL_ON_CENTERIS_ERROR(ceError);

  memset(szBuf, 0, sizeof(szBuf));
  snprintf(szBuf, sizeof(szBuf), "/usr/bin/sed s/HOSTNAME=\\\"[a-zA-Z0-9].*\\\"/HOSTNAME=\\\"%s\\\"/ %s > %s.new", pszComputerName, NETCONF, NETCONF);
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

  memset(szBuf, 0, sizeof(szBuf));
  snprintf(szBuf, sizeof(szBuf), "%s.new", NETCONF);

  ceError = CTMoveFile(szBuf, NETCONF);
  BAIL_ON_CENTERIS_ERROR(ceError);

  CTFreeStringArray(ppszArgs, nArgs);
  ppszArgs = NULL;
  FreeProcInfo(pProcInfo);
  pProcInfo = NULL;

  /* After updating the file, HP-UX wants us to "start" the hostname */
  ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("/sbin/init.d/hostname", ppszArgs);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("start", ppszArgs + 1);
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
  if(ppszArgs)
    CTFreeStringArray(ppszArgs, nArgs);

  if(pProcInfo)
    FreeProcInfo(pProcInfo);

  return ceError;
}

static
DWORD
SetAIXHostname(
    PSTR pszComputerName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 6;
    CHAR  szBuf[512];
    LONG  status = 0;

    DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("chdev", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-a", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "hostname=%s", pszComputerName);
    ceError = CTAllocateString(szBuf, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-l", ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("inet0", ppszArgs+4);
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

DWORD
SedEscapeLiteral(
    PCSTR pInput,
    PSTR* ppOutput
    )
{
    PSTR pOutput = NULL;
    DWORD outputIndex = 0;
    DWORD index = 0;
    DWORD dwError = 0;

    // Calculate the length first
    for (index = 0; pInput[index]; index++)
    {
        switch (pInput[index])
        {
            // See the \char section of gnu sed manual
            case '$':
            case '*':
            case '.':
            case '[':
            case '\\':
            case '^':
                outputIndex += 2;
                break;
            default:
                outputIndex += 1;
        }
    }

    dwError = LwAllocateMemory(
                    outputIndex + 1,
                    PPCAST(&pOutput));
    BAIL_ON_CENTERIS_ERROR(dwError);

    outputIndex = 0;
    for (index = 0; pInput[index]; index++)
    {
        switch (pInput[index])
        {
            // See the \char section of gnu sed manual
            case '$':
            case '*':
            case '.':
            case '[':
            case '\\':
            case '^':
                pOutput[outputIndex++] = '\\';
                break;
        }
        pOutput[outputIndex++] = pInput[index];
    }
    pOutput[outputIndex++] = 0;

error:
    *ppOutput = pOutput;
    return dwError;
}

static
DWORD
WriteHostnameToSunFiles(
    PCSTR pOldShortHostname,
    PCSTR pNewShortHostname,
    PCSTR pDnsDomainName,
    PCSTR pOldFqdnHostname,
    PCSTR pNewFqdnHostname
    )
{
    DWORD ceError = ERROR_SUCCESS;
    FILE* fp = NULL;
    PSTR* ppszHostfilePaths = NULL;
    DWORD nPaths = 0;
    DWORD iPath = 0;
    PSTR pRealPath = NULL;
    PSTR pTempPath = NULL;
    PSTR pOldEscapedShortHostname = NULL;
    PSTR pOldEscapedFqdnHostname = NULL;
    PSTR pOldSedExpression = NULL;
    PSTR pSedExpression = NULL;
    BOOLEAN isSame = FALSE;

    DJ_LOG_INFO("Setting hostname to [%s]", pNewShortHostname);

    ceError = CTGetFileTempPath(
                        "/etc/nodename",
                        &pRealPath,
                        &pTempPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTOpenFile(
            pTempPath,
            "w",
            &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (fprintf(fp, "%s\n", pNewShortHostname) < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTSafeCloseFile(&fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTFileContentsSame(
                    pTempPath,
                    pRealPath,
                    &isSame);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if (isSame)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTRemoveFile(pTempPath));
    }
    else
    {
        ceError = CTSafeReplaceFile(pRealPath, pTempPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    CT_SAFE_FREE_STRING(pRealPath);
    CT_SAFE_FREE_STRING(pTempPath);

    ceError = CTGetFileTempPath(
                        "/etc/defaultdomain",
                        &pRealPath,
                        &pTempPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTOpenFile(
            pTempPath,
            "w",
            &fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    fprintf(fp, "%s\n", pDnsDomainName);

    ceError = CTSafeCloseFile(&fp);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTFileContentsSame(
                    pTempPath,
                    pRealPath,
                    &isSame);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if (isSame)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTRemoveFile(pTempPath));
    }
    else
    {
        ceError = CTSafeReplaceFile(pRealPath, pTempPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* The first column of the /etc/hostname.<interface> files is the IP
     * address of the interface. It may either be specified directly in the
     * file, or the file may refer to an entry in /etc/hosts. This program is
     * editing /etc/hosts, any old entries in the hostname files need to be
     * fixed. */
    ceError = CTGetMatchingFilePathsInFolder("/etc",
                                             "hostname.*",
                                             &ppszHostfilePaths,
                                             &nPaths);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = SedEscapeLiteral(
                pOldShortHostname,
                &pOldEscapedShortHostname);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf(
                &pSedExpression,
                "s/^%s\\([ ].*\\)\\{0,1\\}$/%s\\1/",
                pOldEscapedShortHostname,
                pNewShortHostname);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pOldFqdnHostname != NULL)
    {
        ceError = SedEscapeLiteral(
                    pOldFqdnHostname,
                    &pOldEscapedFqdnHostname);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pOldSedExpression = pSedExpression;
        pSedExpression = NULL;

        ceError = LwAllocateStringPrintf(
                    &pSedExpression,
                    "%s;s/^%s\\([ ].*\\)\\{0,1\\}$/%s\\1/",
                    pOldSedExpression,
                    pOldEscapedFqdnHostname,
                    pNewFqdnHostname);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    for (iPath = 0; iPath < nPaths; iPath++)
    {
        if (!CTStrEndsWith(ppszHostfilePaths[iPath], ".lwidentity.bak") &&
            !CTStrEndsWith(ppszHostfilePaths[iPath], ".lwidentity.orig"))
        {
            ceError = CTRunSedOnFile(
                            ppszHostfilePaths[iPath],
                            ppszHostfilePaths[iPath],
                            FALSE,
                            pSedExpression);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

error:

    if (ppszHostfilePaths)
        CTFreeStringArray(ppszHostfilePaths, nPaths);

    if (fp) {
        fclose(fp);
    }

    CT_SAFE_FREE_STRING(pRealPath);
    CT_SAFE_FREE_STRING(pTempPath);
    CT_SAFE_FREE_STRING(pOldEscapedShortHostname);
    CT_SAFE_FREE_STRING(pOldEscapedFqdnHostname);
    CT_SAFE_FREE_STRING(pOldSedExpression);
    CT_SAFE_FREE_STRING(pSedExpression);

    return ceError;
}

static
DWORD
SetMacOsXHostName(
    PCSTR HostName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int EE = 0;
    char command[] = "scutil";
    /* ISSUE-2007/08/01-dalmeida -- Fix const-ness of arg array in procutils */
    PSTR args[5] = { command, "--set", "HostName", (char*)HostName };
    PPROCINFO procInfo = NULL;
    LONG status = 0;

    ceError = DJSpawnProcessSilent(command, args, &procInfo);
    GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);

    ceError = DJGetProcessStatus(procInfo, &status);
    GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);

    if (status != 0) {
        DJ_LOG_ERROR("%s failed [Status code: %d]", command, status);
        ceError = ERROR_BAD_COMMAND;
        GOTO_CLEANUP_ON_DWORD_EE(ceError, EE);
    }

cleanup:
    if (procInfo)
    {
        FreeProcInfo(procInfo);
    }

    DJ_LOG_VERBOSE("SetMacOsXHostName LEAVE -> 0x%08x (EE = %d)", ceError, EE);

    return ceError;
}

static
DWORD
DJCheckIfDHCPHost(
    PSTR pszPathifcfg,
    PBOOLEAN pbDHCPHost
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszFilter = "^[[:space:]]*BOOTPROTO.*dhcp.*$";
    BOOLEAN bDHCPHost = FALSE;

    DJ_LOG_INFO("Checking if DHCP Host...");

    // now that we have a file, we need to check out our BOOTPROTO,
    // if it's DHCP, we have to update the DHCP_HOSTNAME
    // ps: the expression should be BOOTPROTO='?dhcp'? because RH uses dhcp and SuSE 'dhcp'
    // sRun = "grep BOOTPROTO=\\'\\\\?dhcp\\'\\\\? " + sPathifcfg;
    //sRun = "grep BOOTPROTO=\\'*dhcp\\'* " + sPathifcfg;

    ceError = CTCheckFileHoldsPattern(pszPathifcfg, pszFilter, &bDHCPHost);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pbDHCPHost = bDHCPHost;

    return ceError;

error:

    *pbDHCPHost = FALSE;

    return ceError;
}

static
BOOLEAN
IsComment(
    PSTR pszLine
    )
{
    PSTR pszTmp = pszLine;

    if (IsNullOrEmptyString(pszLine))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int) *pszTmp))
        pszTmp++;

    return *pszTmp == '#' || *pszTmp == '\0';
}

static
DWORD
DJReplaceNameValuePair(
    PSTR pszFilePath,
    PSTR pszName,
    PSTR pszValue
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszTmpPath = NULL;
    PSTR pszFinalPath = NULL;
    FILE* fpSrc = NULL;
    FILE* fpDst = NULL;
    regex_t rx;
    CHAR szRegExp[256];
    CHAR szBuf[1024+1];
    BOOLEAN bRemoveFile = FALSE;

    memset(&rx, 0, sizeof(rx));

    ceError = CTGetFileTempPath(
                        pszFilePath,
                        &pszFinalPath,
                        &pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szRegExp, "^[[:space:]]*%s[[:space:]]*=.*$", pszName);

    if (regcomp(&rx, szRegExp, REG_EXTENDED) < 0) {
        ceError = LW_ERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((fpSrc = fopen(pszFinalPath, "r")) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((fpDst = fopen(pszTmpPath, "w")) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bRemoveFile = TRUE;

    while (1) {

        if (fgets(szBuf, 1024, fpSrc) == NULL) {
            if (feof(fpSrc)) {
                break;
            } else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        if (!IsComment(szBuf) &&
            !regexec(&rx, szBuf, (size_t)0, NULL, 0)) {

            if (fprintf(fpDst, "%s=%s\n", pszName, pszValue) < 0) {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        } else {

            if (fputs(szBuf, fpDst) == EOF) {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        }

    }

    fclose(fpSrc); fpSrc = NULL;
    fclose(fpDst); fpDst = NULL;

    ceError = CTSafeReplaceFile(pszFinalPath, pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

error:

    if (fpSrc)
        fclose(fpSrc);

    if (fpDst)
        fclose(fpDst);

    regfree(&rx);

    if (bRemoveFile)
        CTRemoveFile(pszTmpPath);

    CT_SAFE_FREE_STRING(pszTmpPath);
    CT_SAFE_FREE_STRING(pszFinalPath);

    return ceError;
}

static
DWORD
DJAppendNameValuePair(
    PSTR pszFilePath,
    PSTR pszName,
    PSTR pszValue
    )
{
    DWORD ceError = ERROR_SUCCESS;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "a")) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (fprintf(fp, "\n%s=%s\n", pszName, pszValue) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fclose(fp); fp = NULL;

error:

    if (fp)
        fclose(fp);

    return ceError;
}

DWORD
DJFixDHCPHost(
    PSTR pszPathifcfg,
    PSTR pszComputerName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bPatternExists = FALSE;

    ceError = CTCheckFileHoldsPattern(pszPathifcfg,
                                      "^[[:space:]]*DHCP_HOSTNAME[[:space:]]*=.*$",
                                      &bPatternExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bPatternExists) {

        ceError = DJReplaceNameValuePair(pszPathifcfg,
                                         "DHCP_HOSTNAME",
                                         pszComputerName);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        ceError = DJAppendNameValuePair(pszPathifcfg,
                                        "DHCP_HOSTNAME",
                                        pszComputerName);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

error:

    return ceError;
}

#if !defined(HAVE_SETHOSTNAME) || ! HAVE_DECL_SETHOSTNAME
static
DWORD
DJFixNetworkManagerOnlineTimeout(
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszFilePath = "/etc/sysconfig/network/config";
    DWORD dwTimeout = 60;
    int EE = 0;
    BOOLEAN bFileExists = FALSE;
    char *isEnabled = NULL;
    char *currentTimeout = NULL;
    char *sedExpression = NULL;
    long currentTimeoutLong;
    char *conversionEnd;

    ceError = CTCheckFileExists(pszFilePath, &bFileExists);
    CLEANUP_ON_DWORD_EE(ceError, EE);
    if(!bFileExists)
        goto cleanup;

    ceError = CTShell(". %pszFilePath; echo \"$NETWORKMANAGER\" >%enabled; echo \"$NM_ONLINE_TIMEOUT\" >%timeout",
            CTSHELL_STRING (pszFilePath, pszFilePath),
            CTSHELL_BUFFER (enabled, &isEnabled),
            CTSHELL_BUFFER (timeout, &currentTimeout));
    CLEANUP_ON_DWORD_EE(ceError, EE);
    CTStripTrailingWhitespace(isEnabled);
    CTStripTrailingWhitespace(currentTimeout);

    DJ_LOG_VERBOSE("Network manager enabled [%s] network manager timeout [%s]", isEnabled, currentTimeout);

    if(strcasecmp(isEnabled, "yes"))
    {
        DJ_LOG_INFO("Network manager is not enabled");
        goto cleanup;
    }

    currentTimeoutLong = strtol(currentTimeout, &conversionEnd, 10);
    if(*conversionEnd != '\0')
    {
        DJ_LOG_INFO("Unable to convert network manager timeout to long");
        currentTimeoutLong = 0;
    }

    if(currentTimeoutLong < dwTimeout)
    {
        DJ_LOG_INFO("Setting network manager timeout to %d", dwTimeout);
        ceError = CTAllocateStringPrintf(&sedExpression,
                "s/^\\([ \t]*NM_ONLINE_TIMEOUT[ \t]*=[ \t]*\\).*$/\\1%d/",
                dwTimeout);
        CLEANUP_ON_DWORD_EE(ceError, EE);
        ceError = CTRunSedOnFile(pszFilePath, pszFilePath, FALSE, sedExpression);
        CLEANUP_ON_DWORD_EE(ceError, EE);
    }

cleanup:
    DJ_LOG_VERBOSE("DJFixNetworkManagerOnlineTimeout LEAVE -> 0x%08x (EE = %d)", ceError, EE);

    CT_SAFE_FREE_STRING(isEnabled);
    CT_SAFE_FREE_STRING(currentTimeout);
    CT_SAFE_FREE_STRING(sedExpression);

    return ceError;
}
#endif

static
DWORD
DJConfigureDHCPService(
    PSTR pszComputerName
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int EE = 0;
    BOOLEAN bFileExists = FALSE;
    PSTR dhcpFilePath = "/etc/sysconfig/network/dhcp";
    PSTR  ppszArgs[] =
        { "/bin/sed",
          "s/^.*\\(DHCLIENT_SET_HOSTNAME\\).*=.*$/\\1=\\\"no\\\"/",
          dhcpFilePath,
          NULL
        };
#if !defined(HAVE_SETHOSTNAME) || !HAVE_DECL_SETHOSTNAME
    PSTR  ppszNetArgs[] =
        {
#if defined(_AIX)
            "/etc/rc.d/init.d/network",
#else
            "/etc/init.d/network",
#endif
            "restart",
            NULL
        };
#endif
    PPROCINFO pProcInfo = NULL;
    LONG status = 0;
    PSTR pszFinalPath = NULL;
    PSTR pszTmpPath = NULL;

    ceError = CTCheckFileExists(dhcpFilePath, &bFileExists);
    CLEANUP_ON_DWORD_EE(ceError, EE);

    if (bFileExists)
    {
        ceError = CTGetFileTempPath(
                            dhcpFilePath,
                            &pszFinalPath,
                            &pszTmpPath);
        CLEANUP_ON_DWORD_EE(ceError, EE);

        ppszArgs[2] = pszFinalPath;
        ceError = DJSpawnProcessOutputToFile(ppszArgs[0], ppszArgs, pszTmpPath, &pProcInfo);
        CLEANUP_ON_DWORD_EE(ceError, EE);

        ceError = DJGetProcessStatus(pProcInfo, &status);
        CLEANUP_ON_DWORD_EE(ceError, EE);

        if (status != 0) {
            ceError = ERROR_FAIL_RESTART;
            CLEANUP_ON_DWORD_EE(ceError, EE);
        }

        ceError = CTSafeReplaceFile(pszFinalPath, pszTmpPath);
        CLEANUP_ON_DWORD_EE(ceError, EE);
    }

    if (pProcInfo) {
        FreeProcInfo(pProcInfo);
        pProcInfo = NULL;
    }

#if defined(HAVE_SETHOSTNAME) && HAVE_DECL_SETHOSTNAME
    if (sethostname(pszComputerName, strlen(pszComputerName)) < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        CLEANUP_ON_DWORD_EE(ceError, EE);
    }
#else
    ceError = DJFixNetworkManagerOnlineTimeout();
    CLEANUP_ON_DWORD_EE(ceError, EE);

    /* Restart network */

    ceError = DJSpawnProcess(ppszNetArgs[0], ppszNetArgs, &pProcInfo);
    CLEANUP_ON_DWORD_EE(ceError, EE);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    CLEANUP_ON_DWORD_EE(ceError, EE);

    if (status != 0) {
        ceError = ERROR_BAD_COMMAND;
        CLEANUP_ON_DWORD_EE(ceError, EE);
    }
#endif

cleanup:

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    DJ_LOG_VERBOSE("DJRestartDHCPService LEAVE -> 0x%08x (EE = %d)", ceError, EE);

    CT_SAFE_FREE_STRING(pszFinalPath);
    CT_SAFE_FREE_STRING(pszTmpPath);

    return ceError;
}

static
void
FixNetworkInterfaces(
    PSTR pszComputerName,
    LWException **exc
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int EE = 0;
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bDirExists = FALSE;
    PSTR* ppszPaths = NULL;
    DWORD nPaths = 0;
    DWORD iPath = 0;
    CHAR szBuf[1024];
    PSTR pszPathifcfg = NULL;
    BOOLEAN bDHCPHost = FALSE;
    PSTR pszMachineSID = NULL;
    PCSTR networkConfigPath = "/etc/sysconfig/network";

    LW_CLEANUP_CTERR(exc, DJGetMachineSID(&pszMachineSID));

    /*
     * fixup HOSTNAME variable in /etc/sysconfig/network file if it exists
     * note that 'network' is a *directory* on some dists (ie SUSE),
     * is a *file* on others (ie Redhat). weird.
     */
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(networkConfigPath, &bFileExists));

    if (bFileExists) {
        sprintf(szBuf, "s/^.*\\(HOSTNAME\\).*=.*$/\\1=%s/", pszComputerName);
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(networkConfigPath, networkConfigPath,
                FALSE, szBuf));
    }

    LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists("/etc/sysconfig/network", &bDirExists));
    if (!bDirExists)
    {
        LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists("/etc/sysconfig/network-scripts", &bDirExists));
    }

    if (bDirExists) {

        struct
        {
            PCSTR dir;
            PCSTR glob;
        } const searchPaths[] = {
            {"/etc/sysconfig/network", "ifcfg-eth-id-[^.]*$"},
            {"/etc/sysconfig/network", "ifcfg-eth0[^.]*$"},
            {"/etc/sysconfig/network", "ifcfg-eth-bus[^.]*$"},
            //SLES 10.1 on zSeries uses one of:
            //  /etc/sysconfig/network/ifcfg-qeth-bus-ccw-0.0.0500
            //  /etc/sysconfig/network/ifcfg-ctc-bus-ccw-0.0.0004
            {"/etc/sysconfig/network", "ifcfg-qeth-bus.*\\.[0-9]\\+$"},
            {"/etc/sysconfig/network", "ifcfg-ctc-bus.*\\.[0-9]\\+$"},
            // Redhat uses /etc/sysconfig/network-scripts/ifcfg-eth<number>
            {"/etc/sysconfig/network-scripts", "ifcfg-eth[^.]*$"},
            // RHEL 6 uses this
            {"/etc/sysconfig/network-scripts", "ifcfg-Auto_eth[^.]*$"},
            // ESX 3.5 and 4.0 use
            // /etc/sysconfig/network-scripts/ifcfg-vswif<number>
            {"/etc/sysconfig/network-scripts", "ifcfg-vswif[^.]*$"},
            {NULL, NULL}
        };

        // Find the ifcfg file
        pszPathifcfg = NULL;

        for(iPath = 0; searchPaths[iPath].dir != NULL && pszPathifcfg == NULL; iPath++)
        {
            if (ppszPaths)
            {
                CTFreeStringArray(ppszPaths, nPaths);
                ppszPaths = NULL;
            }

            ceError = CTGetMatchingFilePathsInFolder(searchPaths[iPath].dir,
                                                         searchPaths[iPath].glob,
                                                         &ppszPaths,
                                                         &nPaths);
            if(ceError == ERROR_DIRECTORY)
            {
                ceError = ERROR_SUCCESS;
                continue;
            }
            LW_CLEANUP_CTERR(exc, ceError);

            if(nPaths > 0)
            {
                LW_CLEANUP_CTERR(exc, CTAllocateString(ppszPaths[0], &pszPathifcfg));
            }
        }

        if (IsNullOrEmptyString(pszPathifcfg)) {
            LW_CLEANUP_CTERR(exc, ERROR_FILE_NOT_FOUND);
        }

        DJ_LOG_INFO("Found ifcfg file at %s", pszPathifcfg);

        LW_CLEANUP_CTERR(exc, DJCheckIfDHCPHost(pszPathifcfg, &bDHCPHost));

        if (bDHCPHost) {
            LW_CLEANUP_CTERR(exc, DJFixDHCPHost(pszPathifcfg, pszComputerName));
        }
    }

    ceError = CTShell("/bin/hostname %hostname >/dev/null",
            CTSHELL_STRING(hostname, pszComputerName));

    LW_CLEANUP_CTERR(exc, ceError);

    // Only DHCP boxes need to restart their networks
    if (bDHCPHost) {
        LW_CLEANUP_CTERR(exc, DJConfigureDHCPService(pszComputerName));
    }

cleanup:

    // This ensures that we do not change the SID after a machine name
    // change.  The issue here is that Samba implements its SAM such that
    // a machine name change changes the seeding used for the machine SID.
    // Therefore, we must re-store the old SID with the new machine name
    // seed.
    if (pszMachineSID) {
        if (*pszMachineSID != '\0')
            DJSetMachineSID(pszMachineSID);
        CTFreeString(pszMachineSID);
    }

    if (ppszPaths)
        CTFreeStringArray(ppszPaths, nPaths);

    if (pszPathifcfg)
        CTFreeString(pszPathifcfg);

    DJ_LOG_VERBOSE("FixNetworkInterfaces LEAVE -> 0x%08x (EE = %d)", ceError, EE);
}

static QueryResult QueryDescriptionSetHostname(const JoinProcessOptions *options, PSTR *changeDescription, LWException **exc)
{
    PSTR required = NULL;
    PSTR optional = NULL;
    PSTR both = NULL;
    PSTR newFqdn = NULL;
    PSTR oldShortHostname = NULL;
    PSTR oldFqdnHostname = NULL;
    PSTR newValue;
    PHOSTSFILELINE pHostsFileLineList = NULL;
    BOOLEAN describedFqdn = FALSE;
    QueryResult result = CannotConfigure;
    BOOLEAN modified = FALSE;
    DWORD ceError = ERROR_SUCCESS;

    if(!options->joiningDomain || options->enableMultipleJoins)
    {
        if(changeDescription != NULL)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(changeDescription,
                "The hostname is fully set"));
        }
        result = NotApplicable;
        goto cleanup;
    }

    //Throws an exception if the hostname is not valid
    LW_TRY(exc, DJCheckValidComputerName(options->computerName, &LW_EXC));

    LW_CLEANUP_CTERR(exc, CTStrdup("", &required));
    LW_CLEANUP_CTERR(exc, CTStrdup("", &optional));
    LW_CLEANUP_CTERR(exc, CTStrdup("", &both));

    LW_CLEANUP_CTERR(exc, DJGetFQDN(&oldShortHostname, &oldFqdnHostname));
    CTStrToLower(oldShortHostname);
    CTStrToLower(oldFqdnHostname);
    if(strcasecmp(oldShortHostname, options->computerName))
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tSet the computer short hostname (currently %s) to the requested hostname (%s) by setting:\n"
"\t\t* The kernel hostname\n"
"\t\t* The hostname in files for statically configured computers\n"
"\t\t* The hostname in files for DHCP configured computers",
            required, oldShortHostname, options->computerName));
        CT_SAFE_FREE_STRING(required);
        required = newValue;
    }
    
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newFqdn,
                "%s.%s", options->computerName,
            options->domainName));
    CTStrToLower(newFqdn);
    if(oldFqdnHostname == NULL)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tGive the machine a fully-qualified domain name. If performed automatically, the fqdn will be set through /etc/hosts to '%s', but it is possible to use a different fqdn and/or set it through dns instead of /etc/hosts. However in all cases, the fqdn must follow standard DNS naming conventions, and have a period in the name. The following steps will be used if the fqdn is set automatically:\n"
"\t\t* Make sure local comes before bind in nsswitch\n"
"\t\t* Add a loopback entry in /etc/hosts and put the fqdn as the primary name\n",
            required, newFqdn));
        CT_SAFE_FREE_STRING(required);
        required = newValue;
        describedFqdn = TRUE;
    }
    else if(strcmp(newFqdn, oldFqdnHostname))
    {
        //The current fqdn does not match our ideal fqdn
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tChange the fqdn from '%s' to '%s'. This could be done via DNS, but this program will do it with the following steps:\n"
"\t\t* Making sure local comes before bind in nsswitch\n"
"\t\t* Adding the fqdn before all entries in /etc/hosts that contain the short hostname and removing the old fqdn if it appears on the line\n"
"\t\t* Restart nscd (if running) to flush the DNS cache",
            optional, oldFqdnHostname, newFqdn));
        CT_SAFE_FREE_STRING(optional);
        optional = newValue;
        describedFqdn = TRUE;
    }

    //Find out if the fqdn is stored in /etc/hosts
    if(oldFqdnHostname != NULL && !strcmp(oldFqdnHostname, "localhost"))
    {
        CT_SAFE_FREE_STRING(oldFqdnHostname);
    }
    if(oldShortHostname != NULL && !strcmp(oldShortHostname, "localhost"))
    {
        CT_SAFE_FREE_STRING(oldShortHostname);
    }

    LW_CLEANUP_CTERR(exc, DJParseHostsFile("/etc/hosts", &pHostsFileLineList));
    LW_CLEANUP_CTERR(exc, DJReplaceHostnameInMemory(
                pHostsFileLineList,
                oldShortHostname, oldFqdnHostname,
                options->computerName, options->domainName));
    modified |= DJHostsFileWasModified(pHostsFileLineList);
    if (pHostsFileLineList)
    {
        DJFreeHostsFileLineList(pHostsFileLineList);
        pHostsFileLineList = NULL;
    }
    ceError = DJParseHostsFile("/etc/inet/ipnodes", &pHostsFileLineList);
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        ceError = ERROR_SUCCESS;
    }
    else if(!ceError)
    {
        LW_CLEANUP_CTERR(exc, DJReplaceHostnameInMemory(
                    pHostsFileLineList,
                    oldShortHostname, oldFqdnHostname,
                    options->computerName, options->domainName));
        modified |= DJHostsFileWasModified(pHostsFileLineList);
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if (modified && !describedFqdn) {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tSet the fqdn in /etc/hosts and /etc/inet/ipnodes. This will preserve the fqdn even when the DNS server is unreachable. This will be performed with these steps:\n"
"\t\t* Making sure local comes before bind in nsswitch\n"
"\t\t* Adding the fqdn before all entries in /etc/hosts and /etc/inet/ipnodes that contain the short hostname and removing the old fqdn if it appears on the line\n"
"\t\t* Restart nscd (if running) to flush the DNS cache"
        , optional, oldFqdnHostname, newFqdn));
        CT_SAFE_FREE_STRING(optional);
        optional = newValue;
        describedFqdn = TRUE;
    }

    if(strlen(required) > 1)
    {
        if(strlen(optional) > 1)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The following step(s) are required:%s\n\nThe following step(s) are optional (but will performed automatically when this step is run):%s", required, optional));
        }
        else
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The following step(s) are required:%s", required));
        }
        result = NotConfigured;
    }
    else if(strlen(optional) > 1)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The following step(s) are optional:%s", optional));
        result = SufficientlyConfigured;
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The hostname is fully set"));
        result = FullyConfigured;
    }

    if(changeDescription != NULL)
    {
        *changeDescription = both;
        both = NULL;
    }

cleanup:
    CT_SAFE_FREE_STRING(required);
    CT_SAFE_FREE_STRING(optional);
    CT_SAFE_FREE_STRING(both);
    CT_SAFE_FREE_STRING(oldShortHostname);
    CT_SAFE_FREE_STRING(oldFqdnHostname);
    CT_SAFE_FREE_STRING(newFqdn);
    if (pHostsFileLineList)
        DJFreeHostsFileLineList(pHostsFileLineList);

    return result;
}

static QueryResult QuerySetHostname(const JoinProcessOptions *options, LWException **exc)
{
    return QueryDescriptionSetHostname(options, NULL, exc);
}

static void DoSetHostname(JoinProcessOptions *options, LWException **exc)
{
    LWException *inner = NULL;
    DWORD ceError;

    LW_TRY(exc,
        DJSetComputerName(options->computerName,
            options->domainName, &LW_EXC));

#ifndef ENABLE_MINIMAL
    ceError = DJConfigureHostsEntry(NULL);
    if(ceError == ERROR_FILE_NOT_FOUND)
    {
        ceError = ERROR_SUCCESS;
#if !defined(__LWI_MACOSX__)
        DJ_LOG_WARNING("Warning: Could not find nsswitch file");
#endif
    }
    LW_CLEANUP_CTERR(exc, ceError);
#endif

#ifndef ENABLE_MINIMAL
    DJRestartIfRunning("nscd", &inner);
    if(!LW_IS_OK(inner) && inner->code == ERROR_FILE_NOT_FOUND)
        LW_HANDLE(&inner);
    LW_CLEANUP(exc, inner);
#endif

cleanup:
    LW_HANDLE(&inner);
}

static PSTR GetSetHostnameDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    QueryDescriptionSetHostname(options, &ret, exc);
    return ret;
}

const JoinModule DJSetHostname = { TRUE, "hostname", "set computer hostname", QuerySetHostname, DoSetHostname, GetSetHostnameDescription };

DWORD
DJGetFQDN(
    PSTR *shortName,
    PSTR *fqdn
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR _shortName = NULL;
    PSTR _fqdn = NULL;
    size_t i;
    struct hostent* pHostent = NULL;

    if(shortName != NULL)
        *shortName = NULL;
    if(fqdn != NULL)
        *fqdn = NULL;

    ceError = DJGetComputerName(&_shortName);
    CLEANUP_ON_DWORD(ceError);

    //We have the short hostname that the hostname command returns, now we're
    //going to get the long hostname. This is the same as 'hostname -f' on
    //systems which support it.
    //Try to look it up upto 3 times
    for(i = 0; i < 3; i++)
    {
        PSTR foundFqdn = NULL;
        pHostent = gethostbyname(_shortName);
        if (pHostent == NULL) {
            if (h_errno == TRY_AGAIN) {
                sleep(1);
                continue;
            }
            break;
        }
        /*
         * We look for the first name that looks like an FQDN.  This is
         * the same heuristics used by other software such as Kerberos and
         * Samba.
         */
        if (strchr(pHostent->h_name, '.') != 0)
        {
            foundFqdn = pHostent->h_name;
        }
        else
        {
            for (i = 0; pHostent->h_aliases[i]; i++)
            {
                if (strchr(pHostent->h_aliases[i], '.') != 0)
                {
                    foundFqdn = pHostent->h_aliases[i];
                    break;
                }
            }
       }
        /* If we still have nothing, just return the first name */
        if (!foundFqdn)
        {
            foundFqdn = pHostent->h_name;
        }
        ceError = CTAllocateString(foundFqdn, &_fqdn);
        CLEANUP_ON_DWORD(ceError);
        break;
    }

    if(shortName != NULL)
    {
        *shortName = _shortName;
        _shortName = NULL;
    }
    if(fqdn != NULL)
    {
        *fqdn = _fqdn;
        _fqdn = NULL;
    }

cleanup:
    CT_SAFE_FREE_STRING(_fqdn);
    CT_SAFE_FREE_STRING(_shortName);
    return ceError;
}

void
DJSetComputerName(
    PCSTR pszComputerName,
    PCSTR pszDnsDomainName,
    LWException **exc
    )
{
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bValidComputerName = FALSE;
    PSTR oldShortHostname = NULL;
    PSTR oldFqdnHostname = NULL;
    PSTR pszComputerName_lower = NULL;
    PSTR pNewFqdnHostname = NULL;
    PSTR ppszHostfilePaths[] = { "/etc/hostname", "/etc/HOSTNAME", NULL };
    LwDistroInfo distro;

    memset(&distro, 0, sizeof(distro));

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));

    LW_CLEANUP_CTERR(exc, DJIsValidComputerName(pszComputerName, &bValidComputerName));

    if (!bValidComputerName) {
        LW_CLEANUP_CTERR(exc, ERROR_INVALID_COMPUTERNAME);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateString(pszComputerName, &pszComputerName_lower));

    CTStrToLower(pszComputerName_lower);

    /* Start spelunking for various hostname holding things. Rather
       than trying to worry about what flavor of linux we are
       running, we look for various files and fix them up if they
       exist. That way we dont end up with a huge wad of repeated
       code for each linux flavor.

       change the repositories of the 'HOSTNAME' variable.
       it's a string in /etc/HOSTNAME for some dists, it's a variable in
       /etc/sysconfig/network for others

       fixup HOSTNAME file if it exists
       Ubuntu/Debian have /etc/hostname, so add that...
    */

    LW_CLEANUP_CTERR(exc, WriteHostnameToFiles(pszComputerName_lower,
                                   ppszHostfilePaths));

    // insert/correct the new hostname in /etc/hosts - note that this
    // has to be done *before* we update the running hostname because
    // we call hostname to get the current hostname so that we can
    // find it and replace it.
    LW_CLEANUP_CTERR(exc, DJGetFQDN(&oldShortHostname, &oldFqdnHostname));

    //Don't replace localhost in /etc/hosts, always add our new hostname instead
    if(oldFqdnHostname != NULL && !strcmp(oldFqdnHostname, "localhost"))
    {
        CTFreeString(oldFqdnHostname);
        oldFqdnHostname = NULL;
    }
    if(oldShortHostname != NULL && !strcmp(oldShortHostname, "localhost"))
    {
        CTFreeString(oldShortHostname);
        oldShortHostname = NULL;
    }

    if (pszDnsDomainName[0])
    {
        ceError = LwAllocateStringPrintf(
                    &pNewFqdnHostname,
                    "%s.%s",
                    pszComputerName,
                    pszDnsDomainName);
        LW_CLEANUP_CTERR(exc, ceError);
    }
    else
    {
        ceError = LwAllocateStringPrintf(
                    &pNewFqdnHostname,
                    "%s",
                    pszComputerName);
        LW_CLEANUP_CTERR(exc, ceError);
    }

    ceError = DJCopyMissingHostsEntry("/etc/inet/ipnodes", "/etc/hosts",
            pszComputerName_lower, oldShortHostname);
    if(ceError == ERROR_FILE_NOT_FOUND)
        ceError = ERROR_SUCCESS;
    LW_CLEANUP_CTERR(exc, ceError);

    LW_CLEANUP_CTERR(exc, DJReplaceNameInHostsFile("/etc/hosts",
            oldShortHostname, oldFqdnHostname,
            pszComputerName_lower, pszDnsDomainName));

    ceError = DJReplaceNameInHostsFile("/etc/inet/ipnodes",
            oldShortHostname, oldFqdnHostname,
            pszComputerName_lower, pszDnsDomainName);
    if(ceError == ERROR_FILE_NOT_FOUND)
        ceError = ERROR_SUCCESS;
    LW_CLEANUP_CTERR(exc, ceError);

    switch (distro.os)
    {
        case OS_SUNOS:
            LW_CLEANUP_CTERR(exc, WriteHostnameToSunFiles(
                        oldShortHostname,
                        pszComputerName_lower,
                        pszDnsDomainName,
                        oldFqdnHostname,
                        pNewFqdnHostname
                        ));
            break;
        case OS_AIX:
            LW_CLEANUP_CTERR(exc, SetAIXHostname(pszComputerName_lower));
            break;
        case OS_HPUX:
            LW_CLEANUP_CTERR(exc, SetHPUXHostname(pszComputerName_lower));
            break;
        case OS_DARWIN:
            LW_CLEANUP_CTERR(exc, SetMacOsXHostName(pszComputerName_lower));
            break;
        default:
            break;
    }

    LW_TRY(exc, FixNetworkInterfaces(pszComputerName_lower, &LW_EXC));

cleanup:
    CT_SAFE_FREE_STRING(oldShortHostname);
    CT_SAFE_FREE_STRING(oldFqdnHostname);
    CT_SAFE_FREE_STRING(pszComputerName_lower);
    CT_SAFE_FREE_STRING(pNewFqdnHostname);
    DJFreeDistroInfo(&distro);
}

void DJCheckValidComputerName(
    PCSTR pszComputerName,
    LWException **exc)
{
    size_t dwLen;
    size_t i;

    if (IsNullOrEmptyString(pszComputerName))
    {
        LW_RAISE_EX(exc, ERROR_INVALID_COMPUTERNAME, "Invalid hostname", "Hostname is empty");
        goto cleanup;
    }

    dwLen = strlen(pszComputerName);

    //Zero length hostnames are already handled above
    if (dwLen > 63)
    {
        LW_RAISE_EX(exc, ERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The name '%s' is %d characters long. Hostnames may only be up to 63 characters long.", pszComputerName, dwLen);
        goto cleanup;
    }

    if (!strcasecmp(pszComputerName, "linux") ||
        !strcasecmp(pszComputerName, "localhost"))
    {
        LW_RAISE_EX(exc, ERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The hostname may not be 'linux' or 'localhost'.");
        goto cleanup;
    }

    if (pszComputerName[0] == '-' || pszComputerName[dwLen - 1] == '-')
    {
        LW_RAISE_EX(exc, ERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The hostname may not start or end with a hyphen.");
        goto cleanup;
    }

    for(i = 0; i < dwLen; i++)
    {
        char c = pszComputerName[i];
        if (!(c == '-' ||
              (c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9'))) {
            LW_RAISE_EX(exc, ERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The given hostname, '%s', contains a '%c'. Valid hostnames may only contain hyphens, letters, and digits.", pszComputerName, c);
            goto cleanup;
        }
    }

cleanup:
    ;
}

DWORD
DJIsValidComputerName(
    PCSTR pszComputerName,
    PBOOLEAN pbIsValid
    )
{
    DWORD ceError = ERROR_SUCCESS;
    LWException *exc = NULL;

    *pbIsValid = FALSE;
    DJCheckValidComputerName(pszComputerName, &exc);
    if(LW_IS_OK(exc))
    {
        *pbIsValid = TRUE;
    }
    else
    {
        ceError = exc->code;
        LWHandle(&exc);
    }

    if (ceError == ERROR_INVALID_COMPUTERNAME || 
            ceError == ERROR_INVALID_COMPUTERNAME)
    {
        ceError = ERROR_SUCCESS;
    }
    return ceError;
}

DWORD
DJIsDomainNameResolvable(
    PCSTR pszDomainName,
    PBOOLEAN pbIsResolvable
    )
{
    DWORD ceError = ERROR_SUCCESS;
    struct hostent* pHostent = NULL;
    int i = 0;

    *pbIsResolvable = FALSE;

    if (IsNullOrEmptyString(pszDomainName)) {
        ceError = ERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    for(i = 0; i < 3; i++){
        pHostent = gethostbyname(pszDomainName);
        if (pHostent == NULL) {
            if (h_errno == TRY_AGAIN) {
                continue;
            } else {
                *pbIsResolvable = FALSE;
                break;
            }
        } else {
            *pbIsResolvable = !IsNullOrEmptyString(pHostent->h_name);
            break;
        }
    }

    return ceError;

error:

    *pbIsResolvable = FALSE;

    return ceError;
}

DWORD
DJGetFinalFqdn(
    const JoinProcessOptions *options,
    PSTR *fqdn
    )
{
    DWORD ceError = ERROR_SUCCESS;
    const ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "hostname");

    *fqdn = NULL;

    if(state != NULL && state->runModule)
    {
        //The fqdn will be set
        ceError = CTAllocateStringPrintf(fqdn, "%s.%s", options->computerName, options->domainName);
        GCE(ceError);
    }
    else
    {
        //The fqdn will not be set
        GCE(ceError = DJGetFQDN(NULL, fqdn));
    }
cleanup:
    return ceError;
}
