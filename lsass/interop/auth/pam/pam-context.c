/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-context.c
 *
 * Abstract:
 * 
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module (PAM)
 * 
 *        BeyondTrust Context API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

DWORD
LsaPamGetContext(
    pam_handle_t* pamh, 
    int           flags, 
    int           argc, 
    const char**  argv,
    PPAMCONTEXT*  ppPamContext
    )
{
    DWORD       dwError = 0;
    PPAMCONTEXT pPamContext = NULL;
    BOOLEAN     bFreeContext = FALSE;
    int         iPamError = 0;
    
    LSA_LOG_PAM_DEBUG("LsaPamGetContext::begin");
    
    iPamError = pam_get_data(pamh, MODULE_NAME, (PAM_GET_DATA_TYPE)&pPamContext);
    dwError = LsaPamUnmapErrorCode(iPamError);
    if (dwError)
    {
        if (dwError == LsaPamUnmapErrorCode(PAM_NO_MODULE_DATA))
        {
                dwError = LwAllocateMemory(
                                sizeof(PAMCONTEXT),
                                (PVOID*)&pPamContext);
                BAIL_ON_LSA_ERROR(dwError);

                bFreeContext = TRUE;

                iPamError = pam_set_data(
                                pamh,
                                MODULE_NAME,
                                (PVOID)pPamContext,
                                &LsaPamCleanupContext);
                dwError = LsaPamUnmapErrorCode(iPamError);
                BAIL_ON_LSA_ERROR(dwError);

                bFreeContext = FALSE;
        }
        else
        {
                BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LsaPamGetLoginId(pamh, pPamContext, NULL, FALSE);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaPamGetOptions(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext->pamOptions);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPamContext->pamOptions.bDebug)
       LsaPamSetLogLevel(LSA_PAM_LOG_LEVEL_DEBUG);
    
    *ppPamContext = pPamContext;
    
cleanup:

    LSA_LOG_PAM_DEBUG("LsaPamGetContext::end");

    return dwError;

error:

    if (pPamContext && bFreeContext) {
       LsaPamFreeContext(pPamContext);
    }

    *ppPamContext = NULL;
    
    LSA_LOG_PAM_ERROR("LsaPamGetContext failed [error code: %u]", dwError);

    goto cleanup;
}

DWORD
LsaPamGetOptions(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char**  argv,
    PPAMOPTIONS   pPamOptions
    )
{
    DWORD dwError = 0;
    int iArg = 0;
    
    LSA_LOG_PAM_DEBUG("LsaPamGetOptions::begin");

    memset(pPamOptions, 0, sizeof(PAMOPTIONS));
    
    for (; iArg < argc; iArg++)
    {
        if (!strcasecmp(argv[iArg], "try_first_pass"))
        {
            pPamOptions->bTryFirstPass = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "use_first_pass"))
        {
            pPamOptions->bUseFirstPass = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "use_authtok"))
        {
            pPamOptions->bUseAuthTok = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "unknown_ok"))
        {
            pPamOptions->bUnknownOK = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "remember_chpass"))
        {
            pPamOptions->bRememberChPass = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "set_default_repository"))
        {
            pPamOptions->bSetDefaultRepository = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "lsass_users_only"))
        {
            pPamOptions->bLsassUsersOnly = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "smartcard_prompt"))
        {
            pPamOptions->bSmartCardPrompt = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "smartcard"))
        {
            pPamOptions->bSmartCardPrompt = TRUE;
            pPamOptions->bSmartCardAuth = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "disable_password_change"))
        {
            pPamOptions->bDisablePasswordChange = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "no_require_membership"))
        {
            pPamOptions->bNoRequireMembership = TRUE;
        }
        else if (!strcasecmp(argv[iArg], "debug"))
        {
            pPamOptions->bDebug = TRUE;
        }
    }
    
    LSA_LOG_PAM_DEBUG("LsaPamGetOptions::end");

    return dwError;
}

DWORD
LsaPamGetLoginId(
    pam_handle_t* pamh,
    PPAMCONTEXT   pPamContext,
    PSTR*         ppszLoginId,
    BOOLEAN       bAllowPrompt
    )
{
    DWORD dwError = 0;
    PSTR pszLoginId = NULL;
    PSTR pszPamId = NULL;
    int iPamError = 0;
    
    LSA_LOG_PAM_DEBUG("LsaPamGetLoginId::begin");

        
    iPamError = pam_get_item(
                    pamh,
                    PAM_USER,
                    (PAM_GET_ITEM_TYPE)&pszPamId);
    dwError = LsaPamUnmapErrorCode(iPamError);
#if defined(HAVE_DECL_PAM_BAD_ITEM) || defined(PAM_BAD_ITEM)
    if (dwError == LsaPamUnmapErrorCode(PAM_BAD_ITEM))
    {
        pszPamId = NULL;
        dwError = 0;
    }
#endif
    BAIL_ON_LSA_ERROR(dwError);
    if (LW_IS_NULL_OR_EMPTY_STR(pszPamId) && bAllowPrompt)
    {
        iPamError = pam_get_user(
                        pamh,
                        (PPCHAR_ARG_CAST)&pszPamId,
                        NULL);
        dwError = LsaPamUnmapErrorCode(iPamError);
        if (dwError)
        {
           dwError = (dwError == LsaPamUnmapErrorCode(PAM_CONV_AGAIN)) ?
               LsaPamUnmapErrorCode(PAM_INCOMPLETE) :
               LsaPamUnmapErrorCode(PAM_SERVICE_ERR);
           BAIL_ON_LSA_ERROR(dwError);
        }
	    if (LW_IS_NULL_OR_EMPTY_STR(pszPamId))
	    {
	       dwError = LsaPamUnmapErrorCode(PAM_SERVICE_ERR);
	       BAIL_ON_LSA_ERROR(dwError);
	    }
    }

    dwError = LwStrDupOrNull(
                pszPamId,
                &pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);
    
    LW_SAFE_FREE_STRING(pPamContext->pszLoginName);
    dwError = LwStrDupOrNull(pszPamId, &pPamContext->pszLoginName);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppszLoginId)
    {
        *ppszLoginId = pszLoginId;
    }
    else
    {
        LW_SAFE_FREE_STRING(pszLoginId);
    }

cleanup:

    LSA_LOG_PAM_DEBUG("LsaPamGetLoginId::end");

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszLoginId);
    
    if (ppszLoginId)
    {
       *ppszLoginId = NULL;
    }
    
    LSA_LOG_PAM_ERROR("LsaPamGetLoginId failed [error code: %u]", dwError);

    goto cleanup;
}

void
LsaPamCleanupContext(
    pam_handle_t* pamh,
    void*         pData,
    int           error_status
    )
{
    LSA_LOG_PAM_DEBUG("LsaPamCleanupContext::begin");
    
    if (pData) {
        LsaPamFreeContext((PPAMCONTEXT)pData);
    }

    // Syslog must be reset before this pam module is unloaded
    LsaPamCloseLog();
    
    LSA_LOG_PAM_DEBUG("LsaPamCleanupContext::end");
}

void
LsaPamFreeContext(
    PPAMCONTEXT pPamContext
    )
{
    LSA_LOG_PAM_DEBUG("LsaPamFreeContext::begin");
    
    LW_SAFE_FREE_STRING(pPamContext->pszLoginName);
    
    LwFreeMemory(pPamContext);
    
    LSA_LOG_PAM_DEBUG("LsaPamFreeContext::end");
}

static void
LsaPamCleanupDataString(
    pam_handle_t* pamh,
    void* data,
    int pam_end_status
    )
{
    PSTR pStr = data;

    LW_SECURE_FREE_STRING(pStr);
}

DWORD
LsaPamSetDataString(
    pam_handle_t* pamh,
    PCSTR pszKey,
    PCSTR pszStr
    )
{
    DWORD dwError = 0;
    PSTR pszStrCopy = NULL;
    int iPamError = 0;

    dwError = LwAllocateString(pszStr, &pszStrCopy);
    BAIL_ON_LSA_ERROR(dwError);
    
    iPamError = pam_set_data(pamh, pszKey, pszStrCopy, LsaPamCleanupDataString);
    dwError = LsaPamUnmapErrorCode(iPamError);
    BAIL_ON_LSA_ERROR(dwError);

error:
    
    return dwError;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
