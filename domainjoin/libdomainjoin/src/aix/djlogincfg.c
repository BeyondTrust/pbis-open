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
#include "djdistroinfo.h"
#include "djpamconf.h"
#include "djaixparser.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

static PCSTR LOGIN_CONFIG_PATH = "/etc/security/login.cfg";

static
DWORD
GetAuthType(const DynamicArray *lines, PSTR *result)
{
    DWORD ceError = DJGetOptionValue(lines, "usw", "auth_type", result);
    if(ceError == ERROR_NOT_FOUND)
    {
        //return the default
        ceError = CTStrdup("STD_AUTH", result);
    }
    return ceError;
}

static
DWORD
SetAuthType(DynamicArray *lines, PCSTR value)
{
    return DJSetOptionValue(lines, "usw", "auth_type", value);
}

DWORD
DJFixLoginConfigFile(
    PCSTR pszPath
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PCSTR pszFilePath = NULL;
    PSTR pszTmpPath = NULL;
    PSTR pszFinalPath = NULL;
    BOOLEAN bFileExists = FALSE;
    FILE* fp = NULL;
    FILE* fp_new = NULL;
    DynamicArray lines;
    PSTR currentSystem = NULL;

    memset(&lines, 0, sizeof(lines));
    if (IsNullOrEmptyString(pszPath))
        pszFilePath = LOGIN_CONFIG_PATH;
    else
        pszFilePath = pszPath;

    GCE(ceError = CTGetFileTempPath(
                        pszFilePath,
                        &pszFinalPath,
                        &pszTmpPath));

    GCE(ceError = CTCheckFileExists(pszFinalPath, &bFileExists));

    if (!bFileExists)
        goto cleanup;

    GCE(ceError = CTOpenFile(pszFinalPath, "r", &fp));
    GCE(ceError = CTReadLines(fp, &lines));
    GCE(ceError = CTSafeCloseFile(&fp));

    GCE(ceError = GetAuthType(&lines, &currentSystem));
    if(!strcmp(currentSystem, "PAM_AUTH"))
        goto cleanup;

    GCE(ceError = SetAuthType(&lines, "PAM_AUTH"));

    GCE(ceError = CTOpenFile(pszTmpPath, "w", &fp_new));
    GCE(ceError = CTWriteLines(fp_new, &lines));
    GCE(ceError = CTSafeCloseFile(&fp_new));

    GCE(ceError = CTSafeReplaceFile(pszFilePath, pszTmpPath));

cleanup:
    CTSafeCloseFile(&fp);
    CTSafeCloseFile(&fp_new);

    CT_SAFE_FREE_STRING(currentSystem);
    CT_SAFE_FREE_STRING(pszTmpPath);
    CT_SAFE_FREE_STRING(pszFinalPath);
    CTFreeLines(&lines);

    return ceError;
}

static QueryResult QueryPamMode(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN bFileExists = FALSE;
    LwDistroInfo distro;
    QueryResult result = NotApplicable;
    FILE* fp = NULL;
    DynamicArray lines;
    PCSTR pszFilePath = LOGIN_CONFIG_PATH;
    PSTR currentSystem = NULL;

    memset(&lines, 0, sizeof(lines));
    memset(&distro, 0, sizeof(distro));

    if(!options->joiningDomain || options->enableMultipleJoins)
        goto cleanup;

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));
    if(distro.os != OS_AIX || strcmp(distro.version, "5.3"))
        goto cleanup;

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszFilePath, &bFileExists));

    if (!bFileExists)
        goto cleanup;

    result = SufficientlyConfigured;

    LW_CLEANUP_CTERR(exc, CTOpenFile(pszFilePath, "r", &fp));
    LW_CLEANUP_CTERR(exc, CTReadLines(fp, &lines));
    LW_CLEANUP_CTERR(exc, CTSafeCloseFile(&fp));

    LW_CLEANUP_CTERR(exc, GetAuthType(&lines, &currentSystem));
    if(strcmp(currentSystem, "PAM_AUTH"))
        goto cleanup;

    result = FullyConfigured;

cleanup:
    CT_SAFE_FREE_STRING(currentSystem);
    DJFreeDistroInfo(&distro);
    CTSafeCloseFile(&fp);
    CTFreeLines(&lines);
    return result;
}

static void DoPamMode(JoinProcessOptions *options, LWException **exc)
{
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, DJAddMissingAIXServices(NULL));
        LW_CLEANUP_CTERR(exc, DJFixLoginConfigFile(NULL));
    }
cleanup:
    ;
}

static PSTR GetPamModeDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"By default AIX will use LAM to perform authentication requests, but AIX 5.3 may instead use PAM for authentication. PAM allows for richer text in prompts and error messages, so it is recommended that the system be switched into PAM mode. This will be done by setting 'auth_type = PAM_AUTH' in the 'usw' stanza in /etc/security/login.cfg.\n"
"\n"
"A few of the programs that ship with AIX are not enabled in the default pam.conf. Before switching the system into PAM mode, entries are made in pam.conf for these services:"
"\tsshd\n"
"\tsudo\n"
"\tdtsession"));

cleanup:
    return ret;
}

const JoinModule DJPamMode = { TRUE, "pam-mode", "switch authentication from LAM to PAM", QueryPamMode, DoPamMode, GetPamModeDescription };
