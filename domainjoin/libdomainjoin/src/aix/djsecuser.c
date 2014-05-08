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
#include "djaixparser.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

static PCSTR USER_SECURITY_CONFIG_PATH = "/etc/security/user";

static
DWORD
GetAuthSystem(const DynamicArray *lines, PSTR *result)
{
    DWORD ceError = DJGetOptionValue(lines, "default", "SYSTEM", result);
    if(ceError == ERROR_NOT_FOUND)
    {
        //return the default
        ceError = CTStrdup("compat", result);
    }
    return ceError;
}

static
DWORD
SetAuthSystem(DynamicArray *lines, PCSTR value)
{
    return DJSetOptionValue(lines, "default", "SYSTEM", value);
}
DWORD
UnconfigureUserSecurity(
    PCSTR pszConfigFilePath
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PCSTR pszFilePath = NULL;
    PSTR pszFinalPath = NULL;
    PSTR pszTmpPath = NULL;
    BOOLEAN bFileExists = FALSE;
    FILE* fp = NULL;
    FILE* fp_new = NULL;
    DynamicArray lines;
    PSTR currentSystem = NULL;
    PSTR newSystem = NULL;
    PSTR beforeLwi;
    PSTR afterLwi;

    memset(&lines, 0, sizeof(lines));
    if (IsNullOrEmptyString(pszConfigFilePath))
        pszFilePath = USER_SECURITY_CONFIG_PATH;
    else
        pszFilePath = pszConfigFilePath;

    GCE(ceError = CTCheckFileExists(pszFilePath, &bFileExists));

    if (!bFileExists)
        goto cleanup;

    ceError = CTGetFileTempPath(
                        pszFilePath,
                        &pszFinalPath,
                        &pszTmpPath);
    GCE(ceError);

    GCE(ceError = CTOpenFile(pszFinalPath, "r", &fp));
    GCE(ceError = CTReadLines(fp, &lines));
    GCE(ceError = CTSafeCloseFile(&fp));

    GCE(ceError = GetAuthSystem(&lines, &currentSystem));
    beforeLwi = strstr(currentSystem, "LSASS");
    if(beforeLwi == NULL)
    {
        //Lwidentity is already not configured
        goto cleanup;
    }

    afterLwi = beforeLwi + strlen("LSASS");
    *beforeLwi = '\0';
    if(CTStrEndsWith(currentSystem, "or "))
        beforeLwi[-3] = '\0';
    else if(CTStrEndsWith(currentSystem, "and "))
        beforeLwi[-4] = '\0';

    GCE(ceError = CTAllocateStringPrintf(&newSystem, "%s%s", currentSystem, afterLwi));
    GCE(ceError = SetAuthSystem(&lines, newSystem));

    GCE(ceError = CTOpenFile(pszTmpPath, "w", &fp_new));
    GCE(ceError = CTWriteLines(fp_new, &lines));
    GCE(ceError = CTSafeCloseFile(&fp_new));

    GCE(ceError = CTSafeReplaceFile(pszFinalPath, pszTmpPath));

cleanup:
    CTSafeCloseFile(&fp);
    CTSafeCloseFile(&fp_new);

    CT_SAFE_FREE_STRING(pszTmpPath);
    CT_SAFE_FREE_STRING(pszFinalPath);
    CT_SAFE_FREE_STRING(currentSystem);
    CT_SAFE_FREE_STRING(newSystem);
    CTFreeLines(&lines);

    return ceError;
}

DWORD
ConfigureUserSecurity(
    PCSTR pszConfigFilePath
    )
{
    DWORD ceError = ERROR_SUCCESS;
    PCSTR pszFilePath = NULL;
    PSTR pszTmpPath = NULL;
    BOOLEAN bFileExists = FALSE;
    FILE* fp = NULL;
    FILE* fp_new = NULL;
    DynamicArray lines;
    PSTR currentSystem = NULL;
    PSTR newSystem = NULL;
    PSTR pszFinalPath = NULL;

    memset(&lines, 0, sizeof(lines));
    if (IsNullOrEmptyString(pszConfigFilePath))
        pszFilePath = USER_SECURITY_CONFIG_PATH;
    else
        pszFilePath = pszConfigFilePath;

    GCE(ceError = CTCheckFileExists(pszFilePath, &bFileExists));

    if (!bFileExists)
        goto cleanup;

    ceError = CTGetFileTempPath(
                        pszFilePath,
                        &pszFinalPath,
                        &pszTmpPath);
    GCE(ceError);

    GCE(ceError = CTOpenFile(pszFilePath, "r", &fp));
    GCE(ceError = CTReadLines(fp, &lines));
    GCE(ceError = CTSafeCloseFile(&fp));

    GCE(ceError = GetAuthSystem(&lines, &currentSystem));
    if(strstr(currentSystem, "LSASS") != NULL)
    {
        //Lwidentity is already configured
        goto cleanup;
    }

    GCE(ceError = CTAllocateStringPrintf(&newSystem, "%s or LSASS", currentSystem));
    GCE(ceError = SetAuthSystem(&lines, newSystem));

    GCE(ceError = CTAllocateStringPrintf(&pszTmpPath, "%s.new", pszFilePath));

    GCE(ceError = CTOpenFile(pszTmpPath, "w", &fp_new));
    GCE(ceError = CTWriteLines(fp_new, &lines));
    GCE(ceError = CTSafeCloseFile(&fp_new));

    GCE(ceError = CTSafeReplaceFile(pszFilePath, pszTmpPath));

cleanup:
    CTSafeCloseFile(&fp);
    CTSafeCloseFile(&fp_new);

    CT_SAFE_FREE_STRING(pszFinalPath);
    CT_SAFE_FREE_STRING(pszTmpPath);
    CT_SAFE_FREE_STRING(currentSystem);
    CT_SAFE_FREE_STRING(newSystem);
    CTFreeLines(&lines);

    return ceError;
}

static QueryResult QueryLamAuth(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN bFileExists = FALSE;
    QueryResult result = NotApplicable;
    FILE* fp = NULL;
    DynamicArray lines;
    PCSTR pszFilePath = USER_SECURITY_CONFIG_PATH;
    PSTR currentSystem = NULL;

    memset(&lines, 0, sizeof(lines));

    if (options->enableMultipleJoins)
        goto cleanup;

    LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszFilePath, &bFileExists));

    if (!bFileExists)
        goto cleanup;

    result = NotConfigured;

    LW_CLEANUP_CTERR(exc, CTOpenFile(pszFilePath, "r", &fp));
    LW_CLEANUP_CTERR(exc, CTReadLines(fp, &lines));
    LW_CLEANUP_CTERR(exc, CTSafeCloseFile(&fp));

    LW_CLEANUP_CTERR(exc, GetAuthSystem(&lines, &currentSystem));
    if(options->joiningDomain)
    {
        if(strstr(currentSystem, "LSASS") == NULL)
            goto cleanup;
    }
    else
    {
        if(strstr(currentSystem, "LSASS") != NULL)
            goto cleanup;
    }

    result = FullyConfigured;

cleanup:
    CTSafeCloseFile(&fp);
    CTFreeLines(&lines);
    CT_SAFE_FREE_STRING(currentSystem);
    return result;
}

static void DoLamAuth(JoinProcessOptions *options, LWException **exc)
{
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, ConfigureUserSecurity(NULL));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, UnconfigureUserSecurity(NULL));
    }
cleanup:
    ;
}

static PSTR GetLamAuthDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    if(options->joiningDomain)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"Include lwidentity to the list of authentication modules by adding 'OR LSASS' to the SYSTEM value inside the default stanza of /etc/security/user"));
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"Remove lwidentity to the list of authentication modules in the default stanza of /etc/security/user"));
    }

cleanup:
    return ret;
}

const JoinModule DJLamAuth = { TRUE, "lam-auth", "configure LAM for AD authentication", QueryLamAuth, DoLamAuth, GetLamAuthDescription };
