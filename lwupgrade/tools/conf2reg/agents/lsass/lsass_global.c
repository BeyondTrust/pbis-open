/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "includes.h"

DWORD
LsaSrvApiConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_CONFIG pConfig = (PLSA_SRV_API_CONFIG)pData;
    PSTR pszLogLevel = NULL;

    BAIL_ON_INVALID_POINTER(pConfig);
    BAIL_ON_INVALID_STRING(pszName);

    if (!strcasecmp(pszName, "enable-eventlog"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
            (!strcasecmp(pszValue, "true") ||
             !strcasecmp(pszValue, "1") ||
             (*pszValue == 'y') ||
             (*pszValue == 'Y')))
        {
            pConfig->bEnableEventLog = TRUE;
        }
        else
        {
            pConfig->bEnableEventLog = FALSE;
        }
    }
    else if (!strcasecmp(pszName, "log-network-connection-events"))
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
            (!strcasecmp(pszValue, "false") ||
             !strcasecmp(pszValue, "0") ||
             (*pszValue == 'n') ||
             (*pszValue == 'N')))
        {
            pConfig->bLogNetworkConnectionEvents = FALSE;
        }
        else
        {
            pConfig->bLogNetworkConnectionEvents = TRUE;
        }
    }
    else if (!strcasecmp(pszName, "log-level"))
    {
        if (!strcasecmp(pszValue, "error") ||
            !strcasecmp(pszValue, "warning") ||
            !strcasecmp(pszValue, "info") ||
            !strcasecmp(pszValue, "verbose") ||
            !strcasecmp(pszValue, "debug") ||
            !strcasecmp(pszValue, "trace"))
        {
            dwError = LwAllocateString(pszValue, &pszLogLevel);
            BAIL_ON_UP_ERROR(dwError);

            LW_SAFE_FREE_STRING(pConfig->pszLogLevel);
            pConfig->pszLogLevel = pszLogLevel;
            pszLogLevel = NULL;
        }
    }

    *pbContinue = TRUE;

cleanup:

    LW_SAFE_FREE_STRING(pszLogLevel);

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LsaSrvApiInitConfig(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    LsaSrvApiFreeConfigContents(pConfig);

    pConfig->bSawProviderAD = FALSE;
    pConfig->bSawProviderLocal = FALSE;

    pConfig->bEnableEventLog = FALSE;
    pConfig->bLogNetworkConnectionEvents = TRUE;

    pConfig->pszLogLevel = NULL;
    dwError = LwAllocateString("error", &pConfig->pszLogLevel);
    BAIL_ON_UP_ERROR(dwError);

cleanup:
    return dwError;

error:

    LW_SAFE_FREE_STRING(pConfig->pszLogLevel);
    goto cleanup;
}

VOID
LsaSrvApiFreeConfigContents(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszLogLevel);
    memset(pConfig, 0, sizeof(*pConfig));
}

DWORD
LsassGlobalSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue)
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
PrintLsaConfig(
    FILE *fp,
    PLSA_SRV_API_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    if (fputs(
            "[HKEY_THIS_MACHINE\\Services]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lsass]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "EnableEventlog", pConfig->bEnableEventLog);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "LogNetworkConnectionEvents", pConfig->bLogNetworkConnectionEvents);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "LogLevel", pConfig->pszLogLevel);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs("\n",fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

error:
    return dwError;
}
 
