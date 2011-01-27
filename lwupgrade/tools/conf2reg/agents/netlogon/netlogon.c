/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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
 *        lwnet-server-cfg.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
NetlogonConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

static
DWORD
LWNetSrvCfgNameValuePair(
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    );

static
DWORD
LWNetSrvCfgPrint(
    FILE *fp,
    PLWNET_SERVER_CONFIG pConfig
    );

static
DWORD
LWNetSrvCfgSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue
    );

static
DWORD
LWNetSrvCfgSetDefaults(
    PLWNET_SERVER_CONFIG pConfig
    );

static
VOID
LWNetSrvCfgFreeContents(
    PLWNET_SERVER_CONFIG pConfig
    );


DWORD
NetlogonConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    )
{
    DWORD dwError = 0;
    LWNET_SERVER_CONFIG Config;
    FILE *fp = NULL;

    memset(&Config, 0, sizeof(Config));

    fp = fopen(pszRegFile, "w");
    if (!fp)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = LWNetSrvCfgSetDefaults(&Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpParseConfigFile(
                pszConfFile,
                &LWNetSrvCfgSectionHandler,
                &LWNetSrvCfgNameValuePair,
                &Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LWNetSrvCfgPrint(fp, &Config);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
    LWNetSrvCfgFreeContents(&Config);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LWNetSrvCfgSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
LWNetSrvCfgNameValuePair(
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLWNET_SERVER_CONFIG pConfig = (PLWNET_SERVER_CONFIG)pData;

    if (!strcmp(pszName, "plugin-path"))
    {
        LW_SAFE_FREE_STRING(pConfig->pszPluginPath);
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
        {
            dwError = LwAllocateString(pszValue, &pConfig->pszPluginPath);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "ping-again-timeout"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
        {
            dwError = UpParseDateString(pszValue, &pConfig->dwPingAgainTimeoutSeconds);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "negative-cache-timeout"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
        {
            dwError = UpParseDateString(pszValue, &pConfig->dwNegativeCacheTimeoutSeconds);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "writable-rediscovery-timeout"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
        {
            dwError = UpParseDateString(pszValue, &pConfig->dwWritableRediscoveryTimeoutSeconds);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    else if (!strcmp(pszName, "writable-timestamp-minimum-change"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
        {
            dwError = UpParseDateString(pszValue, &pConfig->dwWritableTimestampMinimumChangeSeconds);
            BAIL_ON_UP_ERROR(dwError);
        }
    }

cleanup:
    *pbContinue = TRUE;

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LWNetSrvCfgPrint(
    FILE *fp,
    PLWNET_SERVER_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\netlogon]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\netlogon\\Parameters]\n\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    if (pConfig->pszPluginPath)
    {
        dwError = UpPrintString(fp, "PluginPath", pConfig->pszPluginPath);
        BAIL_ON_UP_ERROR(dwError);
    }

    dwError = UpPrintDword(fp, "PingAgainTimeout", pConfig->dwPingAgainTimeoutSeconds);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "NegativeCacheTimeout", pConfig->dwNegativeCacheTimeoutSeconds);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "WritableRediscoveryTimeout", pConfig->dwWritableRediscoveryTimeoutSeconds);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "WritableTimestampMinimumChange", pConfig->dwWritableTimestampMinimumChangeSeconds);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs("\n", fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
LWNetSrvCfgSetDefaults(
    PLWNET_SERVER_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(*pConfig));
        
    pConfig->pszPluginPath = NULL;
    pConfig->dwPingAgainTimeoutSeconds = LWNET_PING_AGAIN_TIMEOUT_SECONDS;
    pConfig->dwNegativeCacheTimeoutSeconds = LWNET_NEGATIVE_CACHE_TIMEOUT_SECONDS;
    pConfig->dwWritableRediscoveryTimeoutSeconds = LWNET_WRITABLE_REDISCOVERY_TIMEOUT_SECONDS;
    pConfig->dwWritableTimestampMinimumChangeSeconds = LWNET_WRITABLE_TIMESTAMP_MINIMUM_CHANGE_SECONDS;

    return dwError;
};

static
VOID
LWNetSrvCfgFreeContents(
    PLWNET_SERVER_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszPluginPath);

    memset(pConfig, 0, sizeof(LWNET_SERVER_CONFIG));
};

