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
 *        pam-auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

#ifndef PAM_BAD_ITEM
#define PAM_BAD_ITEM PAM_SERVICE_ERR
#endif

int
pam_sm_authenticate(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char**  argv
    )
{
    DWORD       dwError = 0;
    PPAMCONTEXT pPamContext = NULL;
    PSTR        pszPassword = NULL;
    HANDLE      hLsaConnection = (HANDLE)NULL;
    PSTR        pszLoginId = NULL;
    PLSA_PAM_CONFIG pConfig = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    int iPamError = 0;
    LSA_AUTH_USER_PAM_PARAMS params = { 0 };
    PLSA_AUTH_USER_PAM_INFO pInfo = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSTR pszSmartCardReader = NULL;
    PSTR pszPINPrompt = NULL;

    LSA_LOG_PAM_DEBUG("pam_sm_authenticate::begin");

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);

    dwError = LsaPamGetContext(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext);
    BAIL_ON_LSA_ERROR(dwError);

    /* If we are just overriding the default repository
       (Solaris), only do that */
    if (pPamContext->pamOptions.bSetDefaultRepository)
    {
#ifdef HAVE_STRUCT_PAM_REPOSITORY
        BOOLEAN bChangeRepository = FALSE;
        struct pam_repository *currentRepository = NULL;
        pam_get_item(
                pamh,
                PAM_REPOSITORY,
                (PAM_GET_ITEM_TYPE)&currentRepository);
        if (currentRepository == NULL)
        {
            if (!pPamContext->pamOptions.bLsassUsersOnly)
            {
                bChangeRepository = TRUE;
            }
            else
            {
                dwError = LsaPamGetLoginId(
                                pamh,
                                pPamContext,
                                &pszLoginId,
                                TRUE);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaOpenServer(&hLsaConnection);

                if (dwError == ERROR_SUCCESS)
                {
                    dwError = LsaFindUserByName(
                                    hLsaConnection,
                                    pszLoginId,
                                    dwUserInfoLevel,
                                    (PVOID*)&pUserInfo);
                    if (dwError == ERROR_SUCCESS)
                    {
                        bChangeRepository = FALSE;
                    }
                    if (dwError == LW_ERROR_NO_SUCH_USER ||
                            dwError == LW_ERROR_NOT_HANDLED)
                    {
                        LSA_LOG_PAM_INFO("Not setting pam repository for unknown user %s", LSA_SAFE_LOG_STRING(pszLoginId));
                        dwError = 0;
                    }
                    else
                    {
                        LSA_LOG_PAM_INFO("Error %d looking up user %s", dwError, LSA_SAFE_LOG_STRING(pszLoginId));
                        dwError = 0;
                    }
                }
                else
                {
                    dwError = 0;
                }
            }
        }
        if (bChangeRepository)
        {
            struct pam_repository files = { "files", NULL, 0 };
            iPamError = pam_set_item(pamh, PAM_REPOSITORY, &files);
            dwError = LsaPamUnmapErrorCode(iPamError);
            if (dwError)
            {
                LSA_LOG_PAM_WARNING("pam_sm_authenticate: warning unable to set pam repository [error code: %u]. This will cause password changes on login to fail, and it may cause password changes in general to fail.", dwError);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        /* This gets mapped to PAM_IGNORE */
        dwError = LW_ERROR_NOT_HANDLED;
#else
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
#endif
    }
    else if (pPamContext->pamOptions.bSmartCardPrompt)
    {
        /*
         * Prompt for the smartcard PIN, or the password if no smartcard
         * is found.  This entry is added early in the PAM configuration,
         * before other modules that are configured to expect it to
         * always prompt for the password (try_first_pass isn't supported
         * on some platforms, and doesn't work properly on others).
         * Therefore, it takes a "prompt or bust" attitude, only
         * bailing out for catastrophic errors that indicate that
         * the whole authentication is likely to fail (such as memory
         * allocation failures).  This entry is marked "requisite", so
         * that if it fails the whole authentication sequence will fail.
         */
        DWORD i;
        int bCheckForSmartCard = 0;

        /*
         * Clear any previous SMART_CARD_PIN and SMART_CARD_READER values.
         * Failure probably indicates memory allocation failed.
         */
        iPamError = pam_set_data(
            pamh,
            PAM_LSASS_SMART_CARD_PIN,
            NULL,
            NULL);
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);

        iPamError = pam_set_data(
            pamh,
            PAM_LSASS_SMART_CARD_READER,
            NULL,
            NULL);
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);

        iPamError = pam_get_item(
                        pamh,
                        PAM_SERVICE,
                        (PAM_GET_ITEM_TYPE)&params.pszPamSource);
        if (iPamError == PAM_BAD_ITEM)
        {
            iPamError = 0;
            params.pszPamSource = NULL;
        }
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < pConfig->dwNumSmartCardServices; ++i)
        {
            if (!strcmp(
                    pConfig->ppszSmartCardServices[i],
                    params.pszPamSource))
            {
                bCheckForSmartCard = 1;
                break;
            }
        }

        if (bCheckForSmartCard)
        {
            LSA_LOG_PAM_DEBUG("Checking for smartcard");

            dwError = LsaOpenServer(&hLsaConnection);

            if (dwError == LW_ERROR_SUCCESS)
            {
                dwError = LsaGetSmartCardUserObject(
                                hLsaConnection,
                                &pObject,
                                &pszSmartCardReader);
                if (dwError == LW_ERROR_SUCCESS)
                {
                    LSA_LOG_PAM_DEBUG(
                        "Found smartcard for user '%s' in reader '%s'",
                        pObject->userInfo.pszUnixName,
                        pszSmartCardReader);

                    if (pPamContext->pszLoginName && *pPamContext->pszLoginName)
                    {
                        /*
                         * Verify that the passed-in username is the same as
                         * the smartcard user.
                         */
                        if (strcmp(
                                pPamContext->pszLoginName,
                                pObject->userInfo.pszUnixName) != 0)
                        {
                            LSA_LOG_PAM_DEBUG(
                                "Pam user '%s' does not match smartcard user",
                                pPamContext->pszLoginName);
                            LsaFreeSecurityObject(
                                pObject);
                            pObject = NULL;
                        }
                    }
                }
                else
                {
                    LSA_LOG_PAM_DEBUG("No smartcard user found");
                }
            }
        }
        else
        {
            LSA_LOG_PAM_DEBUG(
                "Service '%s' is not on the SmartCardServices list; "
                "not checking for smartcard.",
                params.pszPamSource);
        }

        if (pObject != NULL)
        {
            /* SmartCard user found. */
            int bPromptGecos = 0;

            for (i = 0; i < pConfig->dwNumSmartCardPromptGecos; ++i)
            {
                if (!strcmp(
                        pConfig->ppszSmartCardPromptGecos[i],
                        params.pszPamSource))
                {
                    bPromptGecos = 1;
                    break;
                }
            }

            dwError = LwAllocateStringPrintf(
                &pszPINPrompt,
                "Enter PIN for %s: ",
                pObject->userInfo.pszUnixName);
            BAIL_ON_LSA_ERROR(dwError);

            if (bPromptGecos && pObject->userInfo.pszGecos)
            {
                LSA_PAM_CONVERSE_MESSAGE messages[2];

                messages[0].messageStyle = PAM_TEXT_INFO;
                messages[0].pszMessage = pObject->userInfo.pszGecos;
                messages[0].ppszResponse = NULL;
                messages[1].messageStyle = PAM_PROMPT_ECHO_OFF;
                messages[1].pszMessage = pszPINPrompt;
                messages[1].ppszResponse = &pszPassword;

                dwError = LsaPamConverseMulti(
                    pamh,
                    2,
                    messages);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LsaPamConverse(
                    pamh,
                    pszPINPrompt,
                    PAM_PROMPT_ECHO_OFF,
                    &pszPassword);
                BAIL_ON_LSA_ERROR(dwError);
            }

            iPamError = pam_set_item(
                pamh,
                PAM_USER,
                pObject->userInfo.pszUnixName);
            dwError = LsaPamUnmapErrorCode(iPamError);
            BAIL_ON_LSA_ERROR(dwError);

            /*
             * Set a bogus password, so that later modules like
             * pam_unix will be guaranteed to fail, even if there's
             * a local user with the same name and the card's PIN
             * as their password.  Put the PIN into a separate data
             * item.
             */
            iPamError = pam_set_item(
                       pamh,
                       PAM_AUTHTOK,
                       "\x01\xFFPIN ENTERED");
            dwError = LsaPamUnmapErrorCode(iPamError);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaPamSetDataString(
                pamh,
                PAM_LSASS_SMART_CARD_PIN,
                pszPassword);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaPamSetDataString(
                pamh,
                PAM_LSASS_SMART_CARD_READER,
                pszSmartCardReader);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            /*
             * SmartCard user not found (or not even looked for).
             * Prompt for username and password as usual.
             * This is done here because some modules
             * don't work properly with try_first_pass
             * or use_first_pass if no earlier module
             * prompted for the password.
             */
            dwError = LsaPamGetLoginId(
                pamh,
                pPamContext,
                &pszLoginId,
                TRUE);
            BAIL_ON_LSA_ERROR(dwError);
            dwError = LsaOpenServer(&hLsaConnection);
            BAIL_ON_LSA_ERROR(dwError);
        
            /* avoid the extra Lsa call if all the prompts are the same, just choose
            the first one
             */
            if(!(strcmp(pConfig->pszActiveDirectoryPasswordPrompt, 
                    pConfig->pszLocalPasswordPrompt) == 0 && 
                strcmp(pConfig->pszLocalPasswordPrompt, 
                    pConfig->pszOtherPasswordPrompt) == 0))
            {
                dwError = LsaFindUserByName(
                        hLsaConnection,
                        pszLoginId,
                        dwUserInfoLevel,
                        (PVOID*)&pUserInfo);     
            }
            
            if(dwError == 0)
            {
                dwError = LsaPamGetCurrentPassword(
                pamh,
                pPamContext,
                pConfig->pszActiveDirectoryPasswordPrompt,
                &pszPassword);
            }
            else if(getpwnam(pszLoginId) != NULL)
            {
                dwError = LsaPamGetCurrentPassword(
                pamh,
                pPamContext,
                pConfig->pszLocalPasswordPrompt,
                &pszPassword);
            }
            else
            {
                dwError = LsaPamGetCurrentPassword(
                pamh,
                pPamContext,
                pConfig->pszOtherPasswordPrompt,
                &pszPassword);
            }

            
            BAIL_ON_LSA_ERROR(dwError);

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__)
            /* On Solaris, we must save the user's password in
               a custom location so that we can pull it out later
               for password changes */
            dwError = LsaPamSetDataString(
                pamh,
                PAM_LSASS_OLDAUTHTOK,
                pszPassword);
            BAIL_ON_LSA_ERROR(dwError);
#endif
        }
    }
    /* Otherwise, proceed with usual authentication */
    else
    {
        dwError = LsaPamGetLoginId(
            pamh,
            pPamContext,
            &pszLoginId,
            TRUE);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (LsaShouldIgnoreUser(pszLoginId))
        {
            LSA_LOG_PAM_DEBUG("By passing lsassd for local account");
            dwError = LW_ERROR_NOT_HANDLED;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        dwError = LsaFindUserByName(
                        hLsaConnection,
                        pszLoginId,
                        dwUserInfoLevel,
                        (PVOID*)&pUserInfo);
                              
        if(dwError == 0)
        {
            dwError = LsaPamGetCurrentPassword(
            pamh,
            pPamContext,
            pConfig->pszActiveDirectoryPasswordPrompt,
            &pszPassword);
        }
        else if(getpwnam(pszLoginId) != NULL)
        {
            dwError = LsaPamGetCurrentPassword(
            pamh,
            pPamContext,
            pConfig->pszLocalPasswordPrompt,
            &pszPassword);
        }
        else
        {
            dwError = LsaPamGetCurrentPassword(
            pamh,
            pPamContext,
            pConfig->pszOtherPasswordPrompt,
            &pszPassword);
        }
        
        BAIL_ON_LSA_ERROR(dwError);
                
        iPamError = pam_get_item(
                        pamh,
                        PAM_SERVICE,
                        (PAM_GET_ITEM_TYPE)&params.pszPamSource);
        if (iPamError == PAM_BAD_ITEM)
        {
            iPamError = 0;
            params.pszPamSource = NULL;
        }
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);

        params.dwFlags = LSA_AUTH_USER_PAM_FLAG_RETURN_MESSAGE;
        params.pszLoginName = pszLoginId;

        iPamError = pam_get_data(
            pamh,
            PAM_LSASS_SMART_CARD_PIN,
            (PAM_GET_DATA_TYPE)&params.pszPassword);
        if (iPamError == PAM_SUCCESS && params.pszPassword != NULL)
        {
            params.dwFlags |= LSA_AUTH_USER_PAM_FLAG_SMART_CARD;
        }
        else
        {
            params.pszPassword = pszPassword;
        }

        dwError = LsaAuthenticateUserPam(
            hLsaConnection,
            &params,
            &pInfo);
        if (pInfo && pInfo->pszMessage)
        {
            LsaPamConverse(pamh,
                           pInfo->pszMessage,
                           PAM_TEXT_INFO,
                           NULL);
        }
        if (dwError == LW_ERROR_PASSWORD_EXPIRED)
        {
            if (pPamContext->pamOptions.bDisablePasswordChange)
            {
                LsaPamConverse(
                    pamh,
                    "Your password has expired",
                    PAM_ERROR_MSG,
                    NULL);
            }
            else
            {
                // deal with this error in the
                // next call to pam_sm_acct_mgmt
                pPamContext->bPasswordExpired = TRUE;
                dwError = 0;
            }
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pInfo && !pInfo->bOnlineLogon)
        {
            pPamContext->bOnlineLogon = FALSE;
        }
        else
        {
            // Assume online if no pInfo returned
            pPamContext->bOnlineLogon = TRUE;
        }

        if (!pPamContext->pamOptions.bNoRequireMembership)
        {
            dwError = LsaCheckUserInList(
                            hLsaConnection,
                            pszLoginId,
                            NULL);
            if (dwError)
            {
                LSA_LOG_PAM_ERROR("User %s is denied access because they are not in the 'require membership of' list",
                                  LSA_SAFE_LOG_STRING(pszLoginId));
                if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszAccessDeniedMessage))
                {
                    LsaPamConverse(pamh,
                                   pConfig->pszAccessDeniedMessage,
                                   PAM_TEXT_INFO,
                                   NULL);
                }
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        /* On Centos, the pam_console module will grab the username from pam
         * and create a file based off that name. The filename must match the
         * user's canonical name. To fix this, pam_lsass will canonicalize
         * the username before pam_console gets it.
         *
         * We know that the username can be found in AD, and that
         * their password matches the AD user's password. At this point, it
         * is very unlikely that we will mangle a local username. 
         */       
        if (strcmp(pszLoginId, pUserInfo->pszName))
        {
            LSA_LOG_PAM_INFO("Canonicalizing pam username from '%s' to '%s'\n",
                    pszLoginId, pUserInfo->pszName);
            iPamError = pam_set_item(
                    pamh,
                    PAM_USER,
                    pUserInfo->pszName);
            dwError = LsaPamUnmapErrorCode(iPamError);
            BAIL_ON_LSA_ERROR(dwError);
        }

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__)
        /*
         * DTLOGIN tests for home directory existence before
         * pam_sm_open_session() is called, and puts up a failure dialog
         * to create the home directory. Unfortunately, afterwards, the user is
         * logged into a fail-safe session. These LsaOpenSession() /
         * LsaCloseSession() calls force the creation of the home directory after
         * successful authentication, making DTLOGIN happy.
         */
        dwError = LsaOpenSession(hLsaConnection,
                                 pszLoginId);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LsaCloseSession(hLsaConnection,
                                  pszLoginId);
        BAIL_ON_LSA_ERROR(dwError);

        /* On Solaris, we must save the user's password in
           a custom location so that we can pull it out later
           for password changes */
        dwError = LsaPamSetDataString(
            pamh,
            PAM_LSASS_OLDAUTHTOK,
            pszPassword);
        BAIL_ON_LSA_ERROR(dwError);
#endif

    }

cleanup:

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    LW_SECURE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszLoginId);
    LW_SAFE_FREE_STRING(pszSmartCardReader);
    LW_SAFE_FREE_STRING(pszPINPrompt);

    if (pInfo)
    {
        LsaFreeAuthUserPamInfo(pInfo);
    }

    if (pObject)
    {
        LsaFreeSecurityObject(pObject);
    }

    LSA_LOG_PAM_DEBUG("pam_sm_authenticate::end");

    return LsaPamOpenPamFilterAuthenticate(
                            LsaPamMapErrorCode(dwError, pPamContext));

error:

    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED)
    {
        LSA_LOG_PAM_WARNING("pam_sm_authenticate error [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("pam_sm_authenticate error [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }

    goto cleanup;
}

int
pam_sm_setcred(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char*   argv[]
    )
{
    HANDLE      hLsaConnection = (HANDLE)NULL;
    DWORD       dwError = 0;
    PLSA_PAM_CONFIG pConfig = NULL;
    PSTR        pszLoginId = NULL;
    PPAMCONTEXT pPamContext = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    int iPamError = 0;

    LSA_LOG_PAM_DEBUG("pam_sm_setcred::begin");

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);

    dwError = LsaPamGetContext(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPamGetLoginId(
        pamh,
        pPamContext,
        &pszLoginId,
        TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPamContext->pamOptions.bSetDefaultRepository)
    {
#ifdef HAVE_STRUCT_PAM_REPOSITORY
        /* This gets mapped to PAM_IGNORE */
        dwError = LW_ERROR_NOT_HANDLED;
#else
        dwError = LW_ERROR_INTERNAL;
#endif
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pPamContext->pamOptions.bSmartCardPrompt)
    {
        dwError = 0;
        goto cleanup;
    }

    if (LsaShouldIgnoreUser(pszLoginId))
    {
        LSA_LOG_PAM_DEBUG("By passing lsassd for local account");
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszLoginId);

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    LSA_LOG_PAM_DEBUG("pam_sm_setcred::end");

    iPamError = LsaPamOpenPamFilterSetCred(
                                LsaPamMapErrorCode(dwError, pPamContext));
#ifdef __LWI_SOLARIS__
    if (iPamError == PAM_SUCCESS)
    {
        /* Typically on Solaris a pam module would need to call setproject
         * here. It is rather complicated to determine what to set the project
         * to. Instead, if PAM_IGNORE is returned, pam_unix_cred will set the
         * project.
        */
        iPamError = PAM_IGNORE;
    }
#endif
    return iPamError;

error:
    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED)
    {
        LSA_LOG_PAM_WARNING("pam_sm_setcred error [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("pam_sm_setcred error [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    goto cleanup;
}
