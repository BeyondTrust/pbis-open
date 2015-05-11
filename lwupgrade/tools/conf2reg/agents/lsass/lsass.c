/*
 * Copyright Likewise Software    2004-2009
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

#include "includes.h"

DWORD
LsassConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

static
DWORD
LsassSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue)
{
    DWORD dwError = 0;

    PLSASS_CONFIG pConfig = (PLSASS_CONFIG) pData;

    if (!strcasecmp(pszSectionName, "global"))
    {
        LsassGlobalSectionHandler(bSectionStart, pszSectionName, &pConfig->LsaConfig, pbContinue);
    }
    else if (!strcasecmp(pszSectionName, "pam"))
    {
        ;
    }
    else if (!strcasecmp(pszSectionName, "auth provider:lsa-activedirectory-provider"))
    {
        UpAdSectionHandler(bSectionStart, pszSectionName, &pConfig->ADConfig, pbContinue);
    }
    else if (!strcasecmp(pszSectionName, "auth provider:lsa-local-provider"))
    {
        ;
    }
    else if (!strcasecmp(pszSectionName, "rpc server:lsarpc"))
    {
        ;
    }
    else if (!strcasecmp(pszSectionName, "rpc server:samr"))
    {
        ;
    }
    else if (!strcasecmp(pszSectionName, "rpc server:dssetup"))
    {
        ;
    }

    return dwError;
}

static
DWORD
LsassNameValueHandler(
    PCSTR   pszSectionName,
    PCSTR   pszName,
    PCSTR   pszValue,
    PVOID pData,
    BOOLEAN *pbContinue)
{
    DWORD dwError = 0;

    PLSASS_CONFIG pConfig = (PLSASS_CONFIG) pData;

    if (!strcasecmp(pszSectionName, "global"))
    {
        LsaSrvApiConfigNameValuePair(pszName, pszValue, &pConfig->LsaConfig, pbContinue);
    }
    else if (!strcasecmp(pszSectionName, "pam"))
    {
        LsaPamConfigNameValuePair(pszName, pszValue, &pConfig->PamConfig, pbContinue);
    }
    else if (!strcasecmp(pszSectionName, "auth provider:lsa-activedirectory-provider"))
    {
        pConfig->bSawProviderAD = TRUE;
        UpAdConfigNameValuePair(pszName, pszValue, &pConfig->ADConfig, pbContinue);
    }
    else if (!strcasecmp(pszSectionName, "auth provider:lsa-local-provider"))
    {
        pConfig->bSawProviderLocal = TRUE;
        UpLocalCfgNameValuePair(pszName, pszValue, &pConfig->LocalConfig, pbContinue);
    }
    else if (!strcasecmp(pszSectionName, "rpc server:lsarpc"))
    {
        ;
    }
    else if (!strcasecmp(pszSectionName, "rpc server:samr"))
    {
        ;
    }
    else if (!strcasecmp(pszSectionName, "rpc server:dssetup"))
    {
        ;
    }

    return dwError;
}

static
DWORD
PrintRegFile(
    FILE *fp,
    LSASS_CONFIG *pConfig
    )
{
    DWORD dwError = 0;

    dwError = PrintLsaConfig(fp, &pConfig->LsaConfig);
    BAIL_ON_UP_ERROR(dwError);

    dwError = PrintPamConfig(fp, &pConfig->PamConfig);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters\\Providers]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpAdPrintConfig(fp, &pConfig->ADConfig);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpLocalPrintConfig(fp, &pConfig->LocalConfig);
    BAIL_ON_UP_ERROR(dwError);

error:
    return dwError;
}

DWORD
LsassConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    )
{
    DWORD dwError = 0;
    FILE *fp = NULL;
    LSASS_CONFIG Config;

    memset(&Config, 0, sizeof(LSASS_CONFIG));

    fp = fopen(pszRegFile, "w");
    if (!fp)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = LsaSrvApiInitConfig(&Config.LsaConfig);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LsaPamInitializeConfig(&Config.PamConfig);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpAdInitializeConfig(&Config.ADConfig);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpLocalCfgInitialize(&Config.LocalConfig);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpParseConfigFile(
                pszConfFile,
                LsassSectionHandler,
                LsassNameValueHandler,
                &Config);
    BAIL_ON_UP_ERROR(dwError);


    dwError = PrintRegFile(fp, &Config);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

    LsaSrvApiFreeConfigContents(&Config.LsaConfig);
    LsaPamFreeConfigContents(&Config.PamConfig);
    UpAdFreeConfigContents(&Config.ADConfig);
    UpLocalCfgFreeContents(&Config.LocalConfig);

    return dwError;
error:
    goto cleanup;
}
