/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "includes.h"

#define LSA_PAM_CONFIG_FILE_PATH CONFIGDIR "/lsassd.conf"
#define LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE "Access denied"
#define LSA_PAM_ACTIVE_DIRECTORY_PASSWORD_PROMPT "Active Directory Password: "
#define LSA_PAM_LOCAL_PASSWORD_PROMPT "Password: "

typedef struct _LSA_CONFIG_READER_CONTEXT
{
    DWORD           dwSeenPamSection;
    PLSA_PAM_CONFIG pConfig;
} LSA_CONFIG_READER_CONTEXT, *PLSA_CONFIG_READER_CONTEXT;

typedef DWORD (*PFN_PAM_CONFIG_HANDLER)(
                    PLSA_PAM_CONFIG pConfig,
                    PCSTR          pszName,
                    PCSTR          pszValue
                    );

typedef struct __PAM_CONFIG_HANDLER
{
    PCSTR                  pszId;
    PFN_PAM_CONFIG_HANDLER pfnHandler;
} PAM_CONFIG_HANDLER, *PPAM_CONFIG_HANDLER;

static
DWORD
LsaPam_SetConfig_LogLevel(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LsaPam_SetConfig_DisplayMOTD(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LsaPam_SetConfig_UserNotAllowedError(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LsaPam_SetConfig_ActiveDirectoryPasswordPrompt(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LsaPam_SetConfig_LocalPasswordPrompt(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );


static PAM_CONFIG_HANDLER gConfigHandlers[] =
{
    { "log-level",              &LsaPam_SetConfig_LogLevel },
    { "display-motd",           &LsaPam_SetConfig_DisplayMOTD },
    { "user-not-allowed-error", &LsaPam_SetConfig_UserNotAllowedError },
    { "ActiveDirectory-Password-Prompt", &LsaPam_SetConfig_ActiveDirectoryPasswordPrompt },
    { "Local-Password-Prompt", &LsaPam_SetConfig_LocalPasswordPrompt },
};

DWORD
LsaPamInitializeConfig(
    PLSA_PAM_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PSTR  pszAccessDeniedMessage = NULL;
    PSTR  pszActiveDirectoryPasswordPrompt = NULL;
    PSTR  pszLocalPasswordPrompt = NULL;

    memset(pConfig, 0, sizeof(LSA_PAM_CONFIG));

    pConfig->bLsaPamDisplayMOTD = FALSE;

    dwError = LwAllocateString(
                    "error",
                    &pConfig->pszLogLevel);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(
                    LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE,
                    &pszAccessDeniedMessage);
    BAIL_ON_UP_ERROR(dwError);
       
    LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
    pConfig->pszAccessDeniedMessage = pszAccessDeniedMessage;        
    
    dwError = LwAllocateString(
                    LSA_PAM_ACTIVE_DIRECTORY_PASSWORD_PROMPT,
                    &pszActiveDirectoryPasswordPrompt);
    BAIL_ON_UP_ERROR(dwError);
    
    LW_SAFE_FREE_STRING(pConfig->pszActiveDirectoryPasswordPrompt);
    pConfig->pszActiveDirectoryPasswordPrompt = pszActiveDirectoryPasswordPrompt;
    
    dwError = LwAllocateString(
                    LSA_PAM_LOCAL_PASSWORD_PROMPT,
                    &pszActiveDirectoryPasswordPrompt);
    BAIL_ON_UP_ERROR(dwError);
    
    LW_SAFE_FREE_STRING(pConfig->pszLocalPasswordPrompt);
    pConfig->pszLocalPasswordPrompt = pszLocalPasswordPrompt;

error:

    return dwError;
}

VOID
LsaPamFreeConfig(
    PLSA_PAM_CONFIG pConfig
    )
{
    LsaPamFreeConfigContents(pConfig);
    LwFreeMemory(pConfig);
}

VOID
LsaPamFreeConfigContents(
    PLSA_PAM_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
    LW_SAFE_FREE_STRING(pConfig->pszLogLevel);
}

DWORD
LsaPamConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszName))
    {
        DWORD iHandler = 0;
        DWORD nHandlers = sizeof(gConfigHandlers)/sizeof(gConfigHandlers[0]);

        for (; iHandler < nHandlers; iHandler++)
        {
            if (!strcasecmp(gConfigHandlers[iHandler].pszId, pszName))
            {
                PLSA_PAM_CONFIG pConfig = (PLSA_PAM_CONFIG)pData;

                gConfigHandlers[iHandler].pfnHandler(
                                pConfig,
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
LsaPam_SetConfig_LogLevel(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    )
{
    DWORD dwError = 0;
    PSTR pszLogLevel = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        LW_SAFE_FREE_STRING(pConfig->pszLogLevel);
        pConfig->pszLogLevel = NULL;
    }
    else if (!strcasecmp(pszValue, "error")   ||
             !strcasecmp(pszValue, "warning") ||
             !strcasecmp(pszValue, "info")    ||
             !strcasecmp(pszValue, "verbose") ||
             !strcasecmp(pszValue, "debug"))
    {
        dwError = LwAllocateString(pszValue, &pszLogLevel);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszLogLevel);
        pConfig->pszLogLevel = pszLogLevel;
        pszLogLevel = NULL;
    }
    else
    {
        LW_SAFE_FREE_STRING(pConfig->pszLogLevel);
        pConfig->pszLogLevel = NULL;
    }

error:

    return 0;
}

static
DWORD
LsaPam_SetConfig_DisplayMOTD(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    )
{
    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
        (!strcasecmp(pszValue, "true") ||
         (*pszValue == 'Y') ||
         (*pszValue == 'y')))
    {
        pConfig->bLsaPamDisplayMOTD = TRUE;
    }
    else
    {
        pConfig->bLsaPamDisplayMOTD = FALSE;
    }

    return 0;
}

static
DWORD
LsaPam_SetConfig_UserNotAllowedError(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszMessage = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(
                        pszValue,
                        &pszMessage);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
        pConfig->pszAccessDeniedMessage = pszMessage;
    }

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszMessage);

    goto cleanup;
}

static
DWORD
LsaPam_SetConfig_ActiveDirectoryPasswordPrompt(
        PLSA_PAM_CONFIG pConfig,
        PCSTR           pszName,
        PCSTR           pszValue)
{
    DWORD dwError = 0;
    PSTR pszMessage = NULL;
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(
                        pszValue,
                        &pszMessage);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszActiveDirectoryPasswordPrompt);
        pConfig->pszActiveDirectoryPasswordPrompt = pszMessage;
    }

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszMessage);

    goto cleanup;
    
}

static
DWORD
LsaPam_SetConfig_LocalPasswordPrompt(
        PLSA_PAM_CONFIG pConfig,
        PCSTR           pszName,
        PCSTR           pszValue)
{
    DWORD dwError = 0;
    PSTR pszMessage = NULL;
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(
                        pszValue,
                        &pszMessage);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszLocalPasswordPrompt);
        pConfig->pszLocalPasswordPrompt = pszMessage;
    }

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszMessage);

    goto cleanup;
    
}

DWORD
PrintPamConfig(
    FILE *fp,
    PLSA_PAM_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters\\PAM]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "LogLevel", pConfig->pszLogLevel);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintBoolean(fp, "DisplayMotd", pConfig->bLsaPamDisplayMOTD);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "UserNotAllowedError", pConfig->pszAccessDeniedMessage);
    BAIL_ON_UP_ERROR(dwError);
    
    dwError = UpPrintString(fp, "ActiveDirectoryPasswordPrompt", pConfig->pszActiveDirectoryPasswordPrompt);
    BAIL_ON_UP_ERROR(dwError);
    
    dwError = UpPrintString(fp, "LocalPasswordPrompt", pConfig->pszLocalPasswordPrompt);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs("\n", fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

error:
    return dwError;
}

