/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lpcfg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Wrappers for accessing global configuration variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
LocalCfgEnableEventLog(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetDefaultLoginShell(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetHomedirPrefix(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetHomedirTemplate(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetCreateHomedir(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetHomedirUmask(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetSkeletonDirs(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
BOOLEAN
LocalCfgGetBooleanValue(
    PCSTR pszValue
    );

typedef DWORD (*PFN_LOCAL_CFG_HANDLER)(
                    PLOCAL_CONFIG pConfig,
                    PCSTR         pszName,
                    PCSTR         pszValue
                    );

typedef struct __LOCAL_CFG_HANDLER
{
    PCSTR                 pszId;
    PFN_LOCAL_CFG_HANDLER pfnHandler;

} LOCAL_CFG_HANDLER, *PLOCAL_CFG_HANDLER;

static LOCAL_CFG_HANDLER gLocalCfgHandlers[] =
{
    {"enable-eventlog",              &LocalCfgEnableEventLog},
    {"login-shell-template",         &LocalCfgSetDefaultLoginShell},
    {"homedir-prefix",               &LocalCfgSetHomedirPrefix},
    {"homedir-template",             &LocalCfgSetHomedirTemplate},
    {"create-homedir",               &LocalCfgSetCreateHomedir},
    {"homedir-umask",                &LocalCfgSetHomedirUmask},
    {"skeleton-dirs",                &LocalCfgSetSkeletonDirs}
};


DWORD
UpLocalCfgInitialize(
    PLOCAL_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PCSTR pszDefaultLoginShell      = LOCAL_CFG_DEFAULT_LOGIN_SHELL;
    PCSTR pszDefaultHomedirPrefix   = LOCAL_CFG_DEFAULT_HOMEDIR_PREFIX;
    PCSTR pszDefaultHomedirTemplate = LOCAL_CFG_DEFAULT_HOMEDIR_TEMPLATE;
    PCSTR pszDefaultSkelDirs        = LOCAL_CFG_DEFAULT_SKELETON_DIRS;

    memset(pConfig, 0, sizeof(LOCAL_CONFIG));

    pConfig->bEnableEventLog = FALSE;
    pConfig->dwMaxGroupNestingLevel = LOCAL_CFG_MAX_GROUP_NESTING_LEVEL_DEFAULT;

    dwError = LwAllocateString(
                    pszDefaultLoginShell,
                    &pConfig->pszLoginShell);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(
                    pszDefaultHomedirPrefix,
                    &pConfig->pszHomedirPrefix);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(
                    pszDefaultHomedirTemplate,
                    &pConfig->pszHomedirTemplate);
    BAIL_ON_UP_ERROR(dwError);

    pConfig->bCreateHomedir = LOCAL_CFG_DEFAULT_CREATE_HOMEDIR;
    pConfig->pszUmask = LOCAL_CFG_DEFAULT_HOMEDIR_UMASK;

    dwError = LwAllocateString(
                    pszDefaultSkelDirs,
                    &pConfig->pszSkelDirs);
    BAIL_ON_UP_ERROR(dwError);

error:

    return dwError;
}

VOID
UpLocalCfgFreeContents(
    PLOCAL_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszLoginShell);
    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    LW_SAFE_FREE_STRING(pConfig->pszSkelDirs);
}

DWORD
UpLocalCfgNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    DWORD iHandler = 0;
    DWORD nHandlers = sizeof(gLocalCfgHandlers)/sizeof(gLocalCfgHandlers[0]);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszName))
    {
        for (; iHandler < nHandlers; iHandler++)
        {
            if (!strcasecmp(gLocalCfgHandlers[iHandler].pszId, pszName))
            {
                gLocalCfgHandlers[iHandler].pfnHandler(
                                                (PLOCAL_CONFIG)pData,
                                                pszName,
                                                pszValue);
                break;
            }
        }
    }

    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
LocalCfgEnableEventLog(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bEnableEventLog =  LocalCfgGetBooleanValue(pszValue);

    return 0;
}

static
DWORD
LocalCfgSetDefaultLoginShell(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszLoginShell = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    if (access(pszValue, X_OK) != 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    dwError = LwAllocateString(
                    pszValue,
                    &pszLoginShell);
    BAIL_ON_UP_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszLoginShell);

    pConfig->pszLoginShell = pszLoginShell;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszLoginShell);

    goto cleanup;
}

static
DWORD
LocalCfgSetHomedirPrefix(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR pszHomedirPrefix = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = LwAllocateString(
                pszValue,
                &pszHomedirPrefix);
    BAIL_ON_UP_ERROR(dwError);

    LwStripWhitespace(pszHomedirPrefix, TRUE, TRUE);

    BAIL_ON_INVALID_STRING(pszHomedirPrefix);

    if (*pszHomedirPrefix != '/')
    {
        LOG_ERROR("Invalid home directory prefix [%s]", pszHomedirPrefix);
        goto error;
    }

    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    pConfig->pszHomedirPrefix = pszHomedirPrefix;

cleanup:

    return 0;

error:

    LW_SAFE_FREE_STRING(pszHomedirPrefix);

    goto cleanup;
}

static
DWORD
LocalCfgSetHomedirTemplate(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszHomedirTemplate = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(
                      pszValue,
                      &pszHomedirTemplate);
        BAIL_ON_UP_ERROR(dwError);
    }

    LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);

    pConfig->pszHomedirTemplate = pszHomedirTemplate;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszHomedirTemplate);

    goto cleanup;
}

static
DWORD
LocalCfgSetCreateHomedir(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bCreateHomedir = LocalCfgGetBooleanValue(pszValue);

    return 0;
}

static
DWORD
LocalCfgSetHomedirUmask(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PCSTR cp = NULL;
    DWORD dwOct = 0;
    DWORD dwVal = 0;
    DWORD dwCnt = 0;
    char  cp2[2];
    PSTR pszUmask = NULL;

    // Convert the umask octal string to a decimal number

    cp2[1] = 0;

    for (cp = pszValue, dwCnt = 0; isdigit((int)*cp); cp++, dwCnt++)
    {
        dwOct *= 8;

        cp2[0] = *cp;
        dwVal = atoi(cp2);

        if (dwVal > 7)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_UP_ERROR(dwError);

        dwOct += dwVal;
    }

    if (dwCnt > 4)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_UP_ERROR(dwError);

    // Disallow 07xx since the user should always have
    // access to his home directory.
    if ((dwOct & 0700) == 0700)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(pszValue, &pszUmask);
    BAIL_ON_UP_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszUmask);
    pConfig->pszUmask = pszUmask;
    pszUmask = NULL;

cleanup:

    LW_SAFE_FREE_STRING(pszUmask);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LocalCfgSetSkeletonDirs(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszSkelDirs = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(
                      pszValue,
                      &pszSkelDirs);
        BAIL_ON_UP_ERROR(dwError);
    }

    LW_SAFE_FREE_STRING(pConfig->pszSkelDirs);

    pConfig->pszSkelDirs = pszSkelDirs;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszSkelDirs);

    goto cleanup;
}

static
BOOLEAN
LocalCfgGetBooleanValue(
    PCSTR pszValue
    )
{
    BOOLEAN bResult = FALSE;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
        (!strcasecmp(pszValue, "true") ||
         !strcasecmp(pszValue, "1") ||
         (*pszValue == 'y') ||
         (*pszValue == 'Y')))
    {
        bResult = TRUE;
    }

    return bResult;
}

DWORD
UpLocalPrintConfig(
    FILE *fp,
    PLOCAL_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters\\Providers\\Local]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    //dwError = UpPrintBoolean(fp, "EnableEventlog", pConfig->bEnableEventLog);
    //BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "LoginShellTemplate", pConfig->pszLoginShell);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "HomeDirPrefix", pConfig->pszHomedirPrefix);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "HomeDirTemplate", pConfig->pszHomedirTemplate);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "CreateHomeDir", pConfig->bCreateHomedir);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "HomeDirUmask", pConfig->pszUmask);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "SkeletonDirs", pConfig->pszSkelDirs);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs("\n", fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

error:
    return dwError;
}

