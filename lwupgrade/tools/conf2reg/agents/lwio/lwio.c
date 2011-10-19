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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Service (Process Utilities)
 *
 */
#include "includes.h"

static
VOID
LwioConfigFreeContents(
    PLWIOINFO pConfig
    );

static
DWORD
LwioConfigPrint(
    FILE *fp,
    PLWIOINFO pConfig
    );

static
DWORD
LwioSetConfigDefaults(
    PLWIOINFO pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(LWIOINFO));

    pConfig->pszDriversLoad = NULL;

    pConfig->bSrvSupportSmb2 = TRUE;

    return dwError;
}

static
VOID
LwioConfigFreeContents(
    PLWIOINFO pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszDriversLoad);
    LW_SAFE_FREE_STRING(pConfig->pszPvfsPath);
    LW_SAFE_FREE_STRING(pConfig->pszNpfsPath);
    LW_SAFE_FREE_STRING(pConfig->pszRdrPath);
    LW_SAFE_FREE_STRING(pConfig->pszSrvPath);
    LW_SAFE_FREE_STRING(pConfig->pszIotestPath);
}

/* call back functions to get the values from config file */
DWORD
LwioConfigSectionHandler(
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
LwioDetermineLoad(
    PLWIOINFO pConfig
    )
{
    DWORD dwError = 0;
    PSTR pszLoad = NULL;
    DWORD len = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszPvfsPath))
        len += strlen("pvfs,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszNpfsPath))
        len += strlen("npfs,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszRdrPath))
        len += strlen("rdr,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszSrvPath))
        len += strlen("srv,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszIotestPath))
        len += strlen("iotest,");
    len += 1;

    dwError = LwAllocateMemory(len, (PVOID)&pszLoad);
    BAIL_ON_UP_ERROR(dwError);

    pszLoad[0] = '\0';
    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszPvfsPath))
        strcat(pszLoad, "pvfs,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszNpfsPath))
        strcat(pszLoad, "npfs,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszRdrPath))
        strcat(pszLoad, "rdr,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszSrvPath))
        strcat(pszLoad, "srv,");

    if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszIotestPath))
        strcat(pszLoad, "iotest,");

    if (len > 1 )
        pszLoad[len - 2] = '\0';

    pConfig->pszDriversLoad = pszLoad;
    pszLoad = NULL;

cleanup:
    LW_SAFE_FREE_STRING(pszLoad);
    return dwError;

error:
    goto cleanup;
}

DWORD
LwioConfigNameValuePair(
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLWIOINFO pConfig = (PLWIOINFO)pData;

    if (!strcmp(pszSectionName, "driver:pvfs"))
    {
        if (!strcmp(pszName, "path"))
        {
            LW_SAFE_FREE_STRING(pConfig->pszPvfsPath);
            dwError = LwAllocateString(pszValue, &pConfig->pszPvfsPath);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    if (!strcmp(pszSectionName, "driver:npfs"))
    {
        if (!strcmp(pszName, "path"))
        {
            LW_SAFE_FREE_STRING(pConfig->pszNpfsPath);
            dwError = LwAllocateString(pszValue, &pConfig->pszNpfsPath);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    if (!strcmp(pszSectionName, "driver:rdr"))
    {
        if (!strcmp(pszName, "path"))
        {
            LW_SAFE_FREE_STRING(pConfig->pszRdrPath);
            dwError = LwAllocateString(pszValue, &pConfig->pszRdrPath);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    if (!strcmp(pszSectionName, "driver:srv"))
    {
        if (!strcmp(pszName, "path"))
        {
            LW_SAFE_FREE_STRING(pConfig->pszSrvPath);
            dwError = LwAllocateString(pszValue, &pConfig->pszSrvPath);
            BAIL_ON_UP_ERROR(dwError);
        }
    }
    if (!strcmp(pszSectionName, "driver:iotest"))
    {
        if (!strcmp(pszName, "path"))
        {
            LW_SAFE_FREE_STRING(pConfig->pszIotestPath);
            dwError = LwAllocateString(pszValue, &pConfig->pszIotestPath);
            BAIL_ON_UP_ERROR(dwError);
        }
    }

    *pbContinue = TRUE;

cleanup:
    return dwError;

error:

    *pbContinue = FALSE;
    goto cleanup;
}


DWORD
LwioConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    )
{
    DWORD dwError = 0;
    LWIOINFO Config;
    FILE *fp = NULL;

    memset(&Config, 0, sizeof(LWIOINFO));

    fp = fopen(pszRegFile, "w");
    if (!fp)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwioSetConfigDefaults(&Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpParseConfigFile(
                pszConfFile,
                &LwioConfigSectionHandler,
                &LwioConfigNameValuePair,
                &Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwioDetermineLoad(&Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwioConfigPrint(fp, &Config);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
    LwioConfigFreeContents(&Config);

    return dwError;

error:

    goto cleanup;

}

static
DWORD
LwioConfigPrint(
    FILE *fp,
    PLWIOINFO pConfig
    )
{
    DWORD dwError = 0;

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lwio]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lwio\\Parameters]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lwio\\Drivers]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "Load", pConfig->pszDriversLoad);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs("\n", fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    if(!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszPvfsPath))
    {
        if (fputs("[HKEY_THIS_MACHINE\\Services\\lwio\\Drivers\\pvfs]\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);

        dwError = UpPrintString(fp, "Path", pConfig->pszPvfsPath);
        BAIL_ON_UP_ERROR(dwError);

        if (fputs("\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);
    }

    if(!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszNpfsPath))
    {
        if (fputs("[HKEY_THIS_MACHINE\\Services\\lwio\\Drivers\\npfs]\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);

        dwError = UpPrintString(fp, "Path", pConfig->pszNpfsPath);
        BAIL_ON_UP_ERROR(dwError);

        if (fputs("\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);
    }

    if(!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszRdrPath))
    {
        if (fputs("[HKEY_THIS_MACHINE\\Services\\lwio\\Drivers\\rdr]\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);

        dwError = UpPrintString(fp, "Path", pConfig->pszRdrPath);
        BAIL_ON_UP_ERROR(dwError);

        if (fputs("\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);
    }

    if(!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszSrvPath))
    {
        if (fputs("[HKEY_THIS_MACHINE\\Services\\lwio\\Drivers\\srv]\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);

        dwError = UpPrintString(fp, "Path", pConfig->pszSrvPath);
        BAIL_ON_UP_ERROR(dwError);

        dwError = UpPrintBoolean(fp, "SupportSmb2", pConfig->bSrvSupportSmb2);
        BAIL_ON_UP_ERROR(dwError);

        if (fputs("\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);
    }

    if(!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszIotestPath))
    {
        if (fputs("[HKEY_THIS_MACHINE\\Services\\lwio\\Drivers\\iotest]\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);

        dwError = UpPrintString(fp, "Path", pConfig->pszIotestPath);
        BAIL_ON_UP_ERROR(dwError);

        if (fputs("\n", fp) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        BAIL_ON_UP_ERROR(dwError);
    }

error:
    return dwError;
}

