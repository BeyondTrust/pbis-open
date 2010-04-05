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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-context.c
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module (PAM)
 * 
 *        Likewise Context API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

int
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
    
    LSA_LOG_PAM_DEBUG("LsaPamGetContext::begin");
    
    dwError = pam_get_data(pamh, MODULE_NAME, (PAM_GET_DATA_TYPE)&pPamContext);
    if (PAM_SUCCESS != dwError)
    {
        switch(dwError)
        {
            case PAM_NO_MODULE_DATA:
                
                dwError = LwAllocateMemory(
                                sizeof(PAMCONTEXT),
                                (PVOID*)&pPamContext);
                BAIL_ON_LSA_ERROR(dwError);

                bFreeContext = TRUE;

                dwError = pam_set_data(
                                pamh,
                                MODULE_NAME,
                                (PVOID)pPamContext,
                                &LsaPamCleanupContext);
                BAIL_ON_LSA_ERROR(dwError);

                bFreeContext = FALSE;
                
                break;
                
            default:

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
    
    *ppPamContext = pPamContext;
    
cleanup:

    LSA_LOG_PAM_DEBUG("LsaPamGetContext::end");

    return LsaPamMapErrorCode(dwError, pPamContext);

error:

    if (pPamContext && bFreeContext) {
       LsaPamFreeContext(pPamContext);
    }

    *ppPamContext = NULL;
    
    LSA_LOG_PAM_ERROR("LsaPamGetContext failed [error code: %d]", dwError);

    goto cleanup;
}

int
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
    }
    
    LSA_LOG_PAM_DEBUG("LsaPamGetOptions::end");

    return LsaPamMapErrorCode(dwError, NULL);
}

int
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
    
    LSA_LOG_PAM_DEBUG("LsaPamGetLoginId::begin");

        
    dwError = pam_get_item(
                    pamh,
                    PAM_USER,
                    (PAM_GET_ITEM_TYPE)&pszPamId);
#if HAVE_DECL_PAM_BAD_ITEM
    if (dwError == PAM_BAD_ITEM)
    {
        pszPamId = NULL;
        dwError = 0;
    }
#endif
    BAIL_ON_LSA_ERROR(dwError);
    if (LW_IS_NULL_OR_EMPTY_STR(pszPamId) && bAllowPrompt)
    {
        dwError = pam_get_user(
                        pamh,
                        (PPCHAR_ARG_CAST)&pszPamId,
                        NULL);
        if (dwError != PAM_SUCCESS)
        {
           dwError = (dwError == PAM_CONV_AGAIN) ?
               PAM_INCOMPLETE : PAM_SERVICE_ERR;
           BAIL_ON_LSA_ERROR(dwError);
        }
	    if (LW_IS_NULL_OR_EMPTY_STR(pszPamId))
	    {
	       dwError = PAM_SERVICE_ERR;
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

    return LsaPamMapErrorCode(dwError, pPamContext);

error:

    LW_SAFE_FREE_STRING(pszLoginId);
    
    if (ppszLoginId)
    {
       *ppszLoginId = NULL;
    }
    
    LSA_LOG_PAM_ERROR("LsaPamGetLoginId failed [error code: %d]", dwError);

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
    if (data)
    {
        LwFreeString((PSTR) data);
    }
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

    dwError = LwAllocateString(pszStr, &pszStrCopy);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = pam_set_data(pamh, pszKey, pszStrCopy, LsaPamCleanupDataString);
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
