/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
LocalCfgSetDefaultLoginShell(
    PLOCAL_CONFIG   pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LocalCfgSetHomedirPrefix(
    PLOCAL_CONFIG   pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LocalCfgSetHomedirUmask(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

DWORD
LocalCfgInitialize(
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
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    pszDefaultHomedirPrefix,
                    &pConfig->pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    pszDefaultHomedirTemplate,
                    &pConfig->pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->bCreateHomedir = LOCAL_CFG_DEFAULT_CREATE_HOMEDIR;
    pConfig->bAcceptNTLMv1 = LOCAL_CFG_DEFAULT_ACCEPT_NTLMV1;
    pConfig->dwHomedirUMask = LOCAL_CFG_DEFAULT_HOMEDIR_UMASK;
    pConfig->EnableUnixIds = LOCAL_CFG_DEFAULT_ENABLE_UNIX_IDS;

    dwError = LwAllocateString(
                    pszDefaultSkelDirs,
                    &pConfig->pszSkelDirs);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalCfgTransferContents(
    PLOCAL_CONFIG pSrcConfig,
    PLOCAL_CONFIG pDstConfig
    )
{
    LocalCfgFreeContents(pDstConfig);

    *pDstConfig = *pSrcConfig;

    memset(pSrcConfig, 0, sizeof(LOCAL_CONFIG));

    return 0;
}

DWORD
LocalCfgGetMinPasswordAge(
    PLONG64 pllMinPwdAge
    )
{
    DWORD  dwError = 0;
    LONG64 llMinPwdAge = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_RDLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    llMinPwdAge = gLPGlobals.llMinPwdAge;

    LOCAL_UNLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    *pllMinPwdAge = llMinPwdAge;

    return dwError;
}

DWORD
LocalCfgGetMaxPasswordAge(
    PLONG64 pllMaxPwdAge
    )
{
    DWORD  dwError = 0;
    LONG64 llMaxPwdAge = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_RDLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    llMaxPwdAge = gLPGlobals.llMaxPwdAge;

    LOCAL_UNLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    *pllMaxPwdAge = llMaxPwdAge;

    return dwError;
}

DWORD
LocalCfgGetMinPwdLength(
    PDWORD pdwMinPwdLength
    )
{
    DWORD  dwError = 0;
    DWORD dwMinPwdLength = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_RDLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    dwMinPwdLength = gLPGlobals.dwMinPwdLength;

    LOCAL_UNLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    *pdwMinPwdLength = dwMinPwdLength;

    return dwError;
}

DWORD
LocalCfgGetPasswordChangeWarningTime(
    PLONG64 pllPasswdChangeWarningTime
    )
{
    DWORD dwError = 0;
    LONG64 llPasswdChangeWarningTime = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_RDLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    llPasswdChangeWarningTime = gLPGlobals.llPwdChangeTime;

    LOCAL_UNLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    *pllPasswdChangeWarningTime = llPasswdChangeWarningTime;

    return dwError;
}

DWORD
LocalCfgIsEventlogEnabled(
    PBOOLEAN pbValue
    )
{
    DWORD dwError = 0;
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LOCAL_RDLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    bResult = gLPGlobals.cfg.bEnableEventLog;

    LOCAL_UNLOCK_RWLOCK(bInLock, &gLPGlobals.rwlock);

    *pbValue = bResult;

    return dwError;
}

DWORD
LocalCfgGetMaxGroupNestingLevel(
    PDWORD pdwNestingLevel
    )
{
    DWORD dwError = 0;
    DWORD dwMaxGroupNestingLevel = LOCAL_CFG_MAX_GROUP_NESTING_LEVEL_DEFAULT;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    dwMaxGroupNestingLevel = gLPGlobals.cfg.dwMaxGroupNestingLevel;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    *pdwNestingLevel = dwMaxGroupNestingLevel;

    return dwError;
}

DWORD
LocalCfgGetDefaultShell(
    PSTR* ppszLoginShell
    )
{
    DWORD dwError = 0;
    PSTR  pszLoginShell = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    dwError = LwAllocateString(
                    gLPGlobals.cfg.pszLoginShell,
                    &pszLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszLoginShell = pszLoginShell;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return dwError;

error:

    *ppszLoginShell = NULL;

    LW_SAFE_FREE_STRING(pszLoginShell);

    goto cleanup;
}

DWORD
LocalCfgGetHomedirPrefix(
    PSTR* ppszHomedirPrefix
    )
{
    DWORD dwError = 0;
    PSTR  pszHomedirPrefix = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    dwError = LwAllocateString(
                    gLPGlobals.cfg.pszHomedirPrefix,
                    &pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHomedirPrefix = pszHomedirPrefix;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return dwError;

error:

    *ppszHomedirPrefix = NULL;

    LW_SAFE_FREE_STRING(pszHomedirPrefix);

    goto cleanup;
}

DWORD
LocalCfgGetHomedirTemplate(
    PSTR* ppszHomedirTemplate
    )
{
    DWORD dwError = 0;
    PSTR  pszHomedirTemplate = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    dwError = LwAllocateString(
                    gLPGlobals.cfg.pszHomedirTemplate,
                    &pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHomedirTemplate = pszHomedirTemplate;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return dwError;

error:

    *ppszHomedirTemplate = NULL;

    LW_SAFE_FREE_STRING(pszHomedirTemplate);

    goto cleanup;
}

DWORD
LocalCfgGetHomedirUmask(
    mode_t* pUmask
    )
{
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    *pUmask = gLPGlobals.cfg.dwHomedirUMask;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return 0;

}

DWORD
LocalCfgMustCreateHomedir(
    PBOOLEAN pbCreateHomedir
    )
{
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    *pbCreateHomedir = gLPGlobals.cfg.bCreateHomedir;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return 0;
}

DWORD
LocalCfgAcceptNTLMv1(
    PBOOLEAN pbResult
    )
{
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    *pbResult = gLPGlobals.cfg.bAcceptNTLMv1;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return 0;
}

DWORD
LocalCfgGetSkeletonDirs(
    PSTR* ppszSkelDirs
    )
{
    DWORD dwError = 0;
    PSTR  pszSkelDirs = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    dwError = LwAllocateString(
                    gLPGlobals.cfg.pszSkelDirs,
                    &pszSkelDirs);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSkelDirs = pszSkelDirs;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return dwError;

error:

    *ppszSkelDirs = NULL;

    LW_SAFE_FREE_STRING(pszSkelDirs);

    goto cleanup;
}

DWORD
LocalCfgGetEnableUnixIds(
    PBOOLEAN pbResult
    )
{
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    *pbResult = gLPGlobals.cfg.EnableUnixIds;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.cfgMutex);

    return 0;
}

VOID
LocalCfgFree(
    PLOCAL_CONFIG pConfig
    )
{
    LocalCfgFreeContents(pConfig);
    LwFreeMemory(pConfig);
}

VOID
LocalCfgFreeContents(
    PLOCAL_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszLoginShell);
    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    LW_SAFE_FREE_STRING(pConfig->pszSkelDirs);
}


DWORD
LocalCfgReadRegistry(
    PLOCAL_CONFIG pConfig
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszLoginShell = NULL;
    PSTR pszHomedirPrefix = NULL;
    PSTR pszUmask = NULL;
    LOCAL_CONFIG StagingConfig;

    LWREG_CONFIG_ITEM LpConfigDescription[] =
    {
        {
            "LoginShellTemplate",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &pszLoginShell,
            NULL
        },
        {
            "HomeDirPrefix",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &pszHomedirPrefix,
            NULL
        },
        {
            "HomeDirUmask",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &pszUmask,
            NULL
        },
        {
            "HomeDirTemplate",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszHomedirTemplate,
            NULL
        },
        {
            "CreateHomeDir",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bCreateHomedir,
            NULL
        },
        {
            "SkeletonDirs",
            TRUE,
            LwRegTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszSkelDirs,
            NULL
        },
        {
            "AcceptNTLMv1",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bAcceptNTLMv1,
            NULL
        },
        {
            "EnableUnixIds",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.EnableUnixIds,
            NULL
        },
    };

    LWREG_CONFIG_ITEM LsaConfigDescription[] =
    {
        {
            "EnableEventlog",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bEnableEventLog,
            NULL
        }
    };


    dwError = LocalCfgInitialize(&StagingConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\Providers\\Local",
                "Policy\\Services\\lsass\\Parameters\\Providers\\Local",
                LpConfigDescription,
                sizeof(LpConfigDescription)/sizeof(LpConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters",
                "Policy\\Services\\lsass\\Parameters",
                LsaConfigDescription,
                sizeof(LsaConfigDescription)/sizeof(LsaConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgSetDefaultLoginShell(
                &StagingConfig,
                "LoginShellTemplate",
                pszLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgSetHomedirPrefix(
                &StagingConfig,
                "HomeDirPrefix",
                pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgSetHomedirUmask(
                &StagingConfig,
                "HomeDirUmask",
                pszUmask);
    BAIL_ON_LSA_ERROR(dwError);

    LocalCfgTransferContents(&StagingConfig, pConfig);

cleanup:
    LW_SAFE_FREE_STRING(pszLoginShell);
    LW_SAFE_FREE_STRING(pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pszUmask);

    LocalCfgFreeContents(&StagingConfig);
    return dwError;

error:
    LocalCfgFreeContents(pConfig);
    goto cleanup;
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

    PSTR pszLoginShell = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto error;
    }

    if (access(pszValue, X_OK) != 0)
    {
        LSA_LOG_ERROR("Invalid login shell [%s]", pszValue);
        goto error;
    }

    dwError = LwAllocateString(pszValue, &pszLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszLoginShell);

    pConfig->pszLoginShell = pszLoginShell;
    pszLoginShell = NULL;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszLoginShell);

    goto cleanup;
}

static
DWORD
LocalCfgSetHomedirPrefix(
    PLOCAL_CONFIG   pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    )
{
    DWORD dwError = 0;
    PSTR pszHomedirPrefix = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto error;
    }

    dwError = LwAllocateString(pszValue, &pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    LwStripWhitespace(pszHomedirPrefix, TRUE, TRUE);

    if (LW_IS_NULL_OR_EMPTY_STR(pszHomedirPrefix))
    {
        goto error;
    }

    if (*pszHomedirPrefix != '/')
    {
        LSA_LOG_ERROR("Invalid home directory prefix [%s]", pszHomedirPrefix);
        goto error;
    }

    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);

    pConfig->pszHomedirPrefix = pszHomedirPrefix;
    pszHomedirPrefix = NULL;

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszHomedirPrefix);

    goto cleanup;
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

    // Convert the umask octal string to a decimal number
    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto error;
    }

    cp2[1] = 0;

    for ( cp = pszValue, dwCnt = 0 ; isdigit((int)*cp) ; cp++, dwCnt++ )
    {
        dwOct *= 8;

        cp2[0] = *cp;
        dwVal = atoi(cp2);

        if ( dwVal > 7 )
        {
            LSA_LOG_ERROR("Invalid Umask [%s]", pszValue);
            goto error;
        }

        dwOct += dwVal;
    }

    if ( dwCnt > 4 )
    {
        LSA_LOG_ERROR("Invalid Umask [%s]", pszValue);
        goto error;
    }

    // Disallow 07xx since the user should always have
    // access to his home directory.
    if ( (dwOct & 0700) == 0700 )
    {
        LSA_LOG_ERROR("Invalid Umask [%s]. User cannot access home directory.",
                pszValue);
        goto error;
    }
    else
    {
        pConfig->dwHomedirUMask = dwOct;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}
