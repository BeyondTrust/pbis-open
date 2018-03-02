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

static BOOLEAN IsLocalUser(PCSTR pszLoginId);

typedef struct _pam_sm_ctx_t
{
    PSTR pszLoginId;
    PSTR pszPassword;
    PSTR pszPIN;
    PSTR pszPINPrompt;
    PSTR pszSmartCardReader;
    LSA_AUTH_USER_PAM_PARAMS params;
} pam_sm_ctx_t;

#ifdef HAVE_STRUCT_PAM_REPOSITORY

static
DWORD
set_default_repository(
        pam_handle_t* pamh,
        PPAMCONTEXT pPamContext
        )
{
    DWORD dwError = 0;
    BOOLEAN bChangeRepository = FALSE;
    struct pam_repository *currentRepository = NULL;
    HANDLE hLsaConnection = (HANDLE) NULL;
    PSTR pszLoginId = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    pam_get_item(
            pamh,
            PAM_REPOSITORY,
            (PAM_GET_ITEM_TYPE) & currentRepository);

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
                        (PVOID*) & pUserInfo);
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
        int iPamError = 0;
        struct pam_repository files = {"files", NULL, 0};

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

cleanup:
    if (hLsaConnection != (HANDLE) NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID) pUserInfo);
    }

    LW_SAFE_FREE_STRING(pszLoginId);

    return dwError;

error:
    goto cleanup;
}
#endif

static char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789#-";

static void sm_lwscr_encode_str(char type, const char *str, char **prv)
{
    char *rv = *prv;

    if (str)
    {
        ssize_t ll = strlen(str);
        const char *pv = str;

        *rv++ = '@';
        *rv++ = type;

        while (ll > 0)
        {
            *rv++ = B64[(pv[0] >> 2) & 0x3F];
            *rv++ = B64[((pv[0] & 0x03) << 4) | ((pv[1] & 0xF0) >> 4)];
            ll--; // Fully encoded the first character
            if (ll > 0)
            {
                *rv++ = B64[((pv[1] & 0x0F) << 2) | ((pv[2] & 0xC0) >> 6)];
                ll--; // Fully encoded the second character
            }
            if (ll > 0)
            {
                *rv++ = B64[(pv[2] & 0x3F)];
                ll--; // Fully encoded the third character
            }
            
            if (ll > 0) pv += 3;
        }
    }

    *prv = rv;
}

static void sm_lwscr_encode_int(char type, unsigned int v32, char **prv)
{
    char str[12] = {0};

    snprintf(str, sizeof(str), "%u", v32);
    
    sm_lwscr_encode_str(type, str, prv);
}

#define encoded_len(l) (((l + 2) / 3) * 4)

static DWORD sm_lwscr_encode(const char *token, const char *host, const char *svc, char **prv)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t ll = 24; // Space for UIDs
    char *rv = NULL;

    if (token) ll += encoded_len(strlen(token)) + 2;
    if (host) ll += encoded_len(strlen(host)) + 2;
    if (svc) ll += encoded_len(strlen(svc)) + 2;

    dwError = LwAllocateMemory(ll + 1, (PVOID*)&rv);

    if (dwError == LW_ERROR_SUCCESS)
    {
        if (prv) *prv = rv;
        
        sm_lwscr_encode_str('T', token, &rv);
        sm_lwscr_encode_str('H', host, &rv);
        sm_lwscr_encode_str('S', svc, &rv);
        sm_lwscr_encode_int('R', getuid(), &rv);
        sm_lwscr_encode_int('E', geteuid(), &rv);
    }

    return dwError;
}

DWORD sm_prompt(pam_handle_t* pamh,
        PLSA_PAM_CONFIG pConfig,
        PPAMCONTEXT pPamContext,
        HANDLE hLsaConnection,
        pam_sm_ctx_t *ctx,
        PLSA_SECURITY_OBJECT* ppObject)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PLSA_USER_INFO_1 pTokenUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;
    int iPamError = 0;
    DWORD i;
    int bCheckForLocalSmartCard = 0;
    int bCheckForRemoteSmartCard = 0;

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

    iPamError = pam_get_item(pamh, PAM_SERVICE, (PAM_GET_ITEM_TYPE) & ctx->params.pszPamSource);
    if (iPamError == PAM_BAD_ITEM)
    {
        iPamError = 0;
        ctx->params.pszPamSource = NULL;
    }

    LSA_LOG_PAM_DEBUG("PAM Service: %s", LSA_SAFE_LOG_STRING(ctx->params.pszPamSource));

    iPamError = pam_get_item(pamh, PAM_RHOST, (PAM_GET_ITEM_TYPE) & ctx->params.pszRemoteHost);
    if (iPamError == PAM_BAD_ITEM)
    {
        iPamError = 0;
        ctx->params.pszRemoteHost = NULL;
    }

    LSA_LOG_PAM_DEBUG("PAM Remote Host: %s", LSA_SAFE_LOG_STRING(ctx->params.pszRemoteHost));

    LSA_LOG_PAM_DEBUG("PAM UID effective: %lu  real: %lu", (unsigned long)geteuid(), (unsigned long)getuid());

    dwError = LsaPamUnmapErrorCode(iPamError);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < pConfig->dwNumSmartCardServices; ++i)
    {
        if (!strcmp(
                pConfig->ppszSmartCardServices[i],
                ctx->params.pszPamSource))
        {
            bCheckForLocalSmartCard = 1;
            break;
        }
    }

    for (i = 0; i < pConfig->dwNumSmartCardRemoteServices; ++i)
    {
        if (!strcmp(
                pConfig->ppszSmartCardRemoteServices[i],
                ctx->params.pszPamSource))
        {
            bCheckForRemoteSmartCard = 1;
            break;
        }
    }

    if (bCheckForLocalSmartCard || bCheckForRemoteSmartCard)
    {
        LSA_LOG_PAM_DEBUG("Checking for smart card");

        dwError = LsaOpenServer(&hLsaConnection);

        if (dwError == LW_ERROR_SUCCESS)
        {
            dwError = LsaFindUserByName(
                    hLsaConnection,
                    ctx->pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*) & pTokenUserInfo);
        }

        if (dwError == LW_ERROR_SUCCESS)
        {
            if (pTokenUserInfo && pTokenUserInfo->pszUPN)
            {
                dwError = LsaGetSmartCardUserObject(
                        hLsaConnection,
                        pTokenUserInfo->pszUPN,
                        &pObject,
                        &ctx->pszSmartCardReader);
                if (dwError == LW_ERROR_SUCCESS)
                {
                    LSA_LOG_PAM_DEBUG(
                            "Found smart card for user '%s' in reader '%s'",
                            pObject->userInfo.pszUnixName,
                            ctx->pszSmartCardReader);

                    LSA_LOG_PAM_DEBUG(
                            "Comparing Pam user '%s' with smart card user %s",
                            pTokenUserInfo->pszUPN,
                            pObject->userInfo.pszUPN);
                    /*
                     * Verify that the passed-in username is the same as
                     * the smart card user.
                     */
                    if (strcmp(pTokenUserInfo->pszUPN, pObject->userInfo.pszUPN) != 0)
                    {
                        LSA_LOG_PAM_DEBUG(
                                "Pam user '%s' does not match smart card user %s",
                                pTokenUserInfo->pszUPN,
                                pObject->userInfo.pszUPN);

                        LsaFreeSecurityObject(pObject);
                        pObject = NULL;
                    }

                    if (strstr(ctx->pszSmartCardReader, LSA_SMARTCARD_REDIRECTOR_TAG))
                    {
                        if (!bCheckForRemoteSmartCard)
                        {
                            LSA_LOG_PAM_DEBUG(
                                    "User found using remote Smart Card but Pam service '%s' is not a remote service",
                                    ctx->params.pszPamSource);

                            LsaFreeSecurityObject(pObject);
                            pObject = NULL;
                        }
                    }
                    else
                    {
                        if (!bCheckForLocalSmartCard)
                        {
                            LSA_LOG_PAM_DEBUG(
                                    "User found using local Smart Card but Pam service '%s' is not a local service",
                                    ctx->params.pszPamSource);

                            LsaFreeSecurityObject(pObject);
                            pObject = NULL;
                        }
                    }
                }
                else
                {
                    LSA_LOG_PAM_DEBUG("User %s not found on smart card", pTokenUserInfo->pszUPN);
                }
            }
            else
            {
                LSA_LOG_PAM_DEBUG("Missing UPN for user %s", ctx->pszLoginId);
            }
        }
        else
        {
            LSA_LOG_PAM_DEBUG("No AD account found for user %s", ctx->pszLoginId);
        }
    }
    else
    {
        LSA_LOG_PAM_DEBUG(
                "Service '%s' is not on the Smart CardServices list; "
                "not checking for smart card.",
                ctx->params.pszPamSource);
    }

    if (pObject != NULL)
    {
        /* Smart Card user found. */
        int bPromptGecos = 0;
        int bSmartcardRedirector = strstr(ctx->pszSmartCardReader, LSA_SMARTCARD_REDIRECTOR_TAG) != NULL;

        for (i = 0; i < pConfig->dwNumSmartCardPromptGecos; ++i)
        {
            if (!strcmp(
                    pConfig->ppszSmartCardPromptGecos[i],
                    ctx->params.pszPamSource))
            {
                bPromptGecos = 1;
                break;
            }
        }

        dwError = LwAllocateStringPrintf(
                &ctx->pszPINPrompt,
                "Enter PIN for %s: ",
                pObject->userInfo.pszUnixName);
        BAIL_ON_LSA_ERROR(dwError);

        if (bPromptGecos && pObject->userInfo.pszGecos)
        {
            LSA_PAM_CONVERSE_MESSAGE messages[2];

            messages[0].messageStyle = PAM_TEXT_INFO;
            messages[0].pszMessage = pObject->userInfo.pszGecos;
            messages[0].ppszResponse = NULL;

            if (bSmartcardRedirector)
            {
                if (pTokenUserInfo->uid != getuid())
                {
                    char *token = NULL;

                    LSA_LOG_PAM_DEBUG("GeCos Prompting for LWSCR token");
                    messages[1].messageStyle = PAM_PROMPT_ECHO_OFF;
                    messages[1].pszMessage = LSA_SMARTCARD_REDIRECTOR_TAG" Secure Password:";
                    messages[1].ppszResponse = &token;

                    dwError = sm_lwscr_encode(token, ctx->params.pszRemoteHost, ctx->params.pszPamSource, &ctx->pszPassword);
                    
                    if (token) free(token);

                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    LSA_LOG_PAM_DEBUG("GeCos Prompting remotely for smart card PIN");
                    messages[1].messageStyle = PAM_TEXT_INFO;
                    messages[1].pszMessage = "Please provide your PIN when prompted:";
                    messages[1].ppszResponse = NULL;

                    dwError = sm_lwscr_encode(NULL, ctx->params.pszRemoteHost, ctx->params.pszPamSource, &ctx->pszPassword);
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            else
            {
                messages[1].messageStyle = PAM_PROMPT_ECHO_OFF;
                messages[1].pszMessage = ctx->pszPINPrompt;
                messages[1].ppszResponse = &ctx->pszPassword;
            }

            dwError = LsaPamConverseMulti(pamh, 2, messages);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            if (bSmartcardRedirector)
            {
                if (pTokenUserInfo->uid != getuid())
                {
                    char *token = NULL;

                    LSA_LOG_PAM_DEBUG("Prompting for LWSCR token");
                    dwError = LsaPamConverse(pamh, LSA_SMARTCARD_REDIRECTOR_TAG" Secure Password:", PAM_PROMPT_ECHO_OFF, &token);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = sm_lwscr_encode(token, ctx->params.pszRemoteHost, ctx->params.pszPamSource, &ctx->pszPassword);
                    
                    if (token) free(token);

                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    LSA_LOG_PAM_DEBUG("Prompting remotely for smart card PIN");
                    dwError = LsaPamConverse(pamh, "Please provide your PIN when prompted:", PAM_TEXT_INFO, NULL);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = sm_lwscr_encode(NULL, ctx->params.pszRemoteHost, ctx->params.pszPamSource, &ctx->pszPassword);
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            else
            {
                dwError = LsaPamConverse(
                        pamh,
                        ctx->pszPINPrompt,
                        PAM_PROMPT_ECHO_OFF,
                        &ctx->pszPassword);
                BAIL_ON_LSA_ERROR(dwError);
            }
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

        // Authenticating with smart card do not prompt for password
        pPamContext->pamOptions.bUseFirstPass = pPamContext->pamOptions.bSmartCardAuth;

        dwError = LsaPamSetDataString(
                pamh,
                PAM_LSASS_SMART_CARD_PIN,
                ctx->pszPassword);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaPamSetDataString(
                pamh,
                PAM_LSASS_SMART_CARD_READER,
                ctx->pszSmartCardReader);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;
    pObject = NULL;

cleanup:
    if (pObject) LsaFreeSecurityObject(pObject);
    if (pTokenUserInfo) LsaFreeUserInfo(dwUserInfoLevel, (PVOID) pTokenUserInfo);

    return dwError;

error:
    goto cleanup;
}

int
pam_sm_authenticate(
        pam_handle_t* pamh,
        int flags,
        int argc,
        const char** argv
        )
{
    DWORD dwError = 0;
    PPAMCONTEXT pPamContext = NULL;
    HANDLE hLsaConnection = (HANDLE) NULL;
    PLSA_PAM_CONFIG pConfig = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;
    int iPamError = 0;
    LSA_AUTH_USER_PAM_PARAMS params = {0};
    PLSA_AUTH_USER_PAM_INFO pInfo = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    pam_sm_ctx_t ctx = {0};
    BOOLEAN bUseRegularAuthentication = TRUE;
    BOOLEAN bIsLocalUser = FALSE;

    LSA_LOG_PAM_DEBUG("pam_sm_authenticate::begin");

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
            &ctx.pszLoginId,
            TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaShouldIgnoreUser(ctx.pszLoginId))
    {
        LSA_LOG_PAM_WARNING("By passing lsass for ignore user %s", ctx.pszLoginId);

        /* If we are just overriding the default repository
       (Solaris), only do that */
        if (pPamContext->pamOptions.bSetDefaultRepository)
        {
#ifdef HAVE_STRUCT_PAM_REPOSITORY
            dwError = set_default_repository(pamh, pPamContext);
#else
            dwError = LW_ERROR_INTERNAL;
#endif
        }
        else if (pPamContext->pamOptions.bSmartCardPrompt)
        {
            dwError = LW_ERROR_IGNORE_THIS_USER;
        }
        else
        {
            // Get the password and store in pam context, for the 
            // next module in the pam stack using try_first_pass or
            // use_first_pass 
            dwError = LsaPamGetCurrentPassword(
                    pamh,
                    pPamContext,
                    "",
                    &ctx.pszPassword);

            dwError = LW_ERROR_IGNORE_THIS_USER;
        }

        BAIL_ON_LSA_ERROR(dwError);
    }


    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);

    /* If we are just overriding the default repository
       (Solaris), only do that */
    if (pPamContext->pamOptions.bSetDefaultRepository)
    {
#ifdef HAVE_STRUCT_PAM_REPOSITORY
        dwError = set_default_repository(pamh, pPamContext);
#else
        dwError = LW_ERROR_INTERNAL;
#endif
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pPamContext->pamOptions.bSmartCardPrompt)
    {
        dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_LSA_ERROR(dwError);

        /*
         * Prompt for the smart card PIN, or the password if no smart card
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
        dwError = sm_prompt(pamh, pConfig, pPamContext, hLsaConnection, &ctx, &pObject);

        if (dwError == LW_ERROR_SUCCESS && pObject != NULL)
        {
            // If we are not just prompting we should continue to perform authentication
            bUseRegularAuthentication = pPamContext->pamOptions.bSmartCardAuth;
        }
        else
        {
            /*
             * Smart Card user not found (or not even looked for).
             * Prompt for username and password as usual.
             * This is done here because some modules
             * don't work properly with try_first_pass
             * or use_first_pass if no earlier module
             * prompted for the password.
             */

            bUseRegularAuthentication = TRUE;
            dwError = LW_ERROR_SUCCESS;
        }
    }

    /* Otherwise, proceed with usual authentication */
    if (bUseRegularAuthentication)
    {
        if (hLsaConnection == NULL)
        {
            dwError = LsaOpenServer(&hLsaConnection);
        }

        if (dwError)
        {
            // Lsass is unavailable. Get the password and return PAM_IGNORE
            //  so that pam_unix or pam_ldap can authenicate the password.
            dwError = LsaPamGetCurrentPassword(
                    pamh,
                    pPamContext,
                    "",
                    &ctx.pszPassword);
            LSA_LOG_PAM_DEBUG("Lsass unavailable. Bypassing lsass.");
            dwError = LW_ERROR_IGNORE_THIS_USER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Lsass is up. Get the appropriate password prompt if 
        // group policy is configured.
        dwError = LsaFindUserByName(
                hLsaConnection,
                ctx.pszLoginId,
                dwUserInfoLevel,
                (PVOID*) & pUserInfo);

        if (dwError == 0)
        {
            dwError = LsaPamGetCurrentPassword(
                    pamh,
                    pPamContext,
                    pConfig->pszActiveDirectoryPasswordPrompt,
                    &ctx.pszPassword);
        }
        else if (IsLocalUser(ctx.pszLoginId))
        {
            dwError = LsaPamGetCurrentPassword(
                    pamh,
                    pPamContext,
                    pConfig->pszLocalPasswordPrompt,
                    &ctx.pszPassword);
            bIsLocalUser = TRUE;
        }
        else
        {
            dwError = LsaPamGetCurrentPassword(
                    pamh,
                    pPamContext,
                    pConfig->pszOtherPasswordPrompt,
                    &ctx.pszPassword);
        }

        BAIL_ON_LSA_ERROR(dwError);

        iPamError = pam_get_item(
                pamh,
                PAM_SERVICE,
                (PAM_GET_ITEM_TYPE) & params.pszPamSource);
        if (iPamError == PAM_BAD_ITEM)
        {
            iPamError = 0;
            params.pszPamSource = NULL;
        }
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);

        params.dwFlags = LSA_AUTH_USER_PAM_FLAG_RETURN_MESSAGE;
        params.pszLoginName = ctx.pszLoginId;

        iPamError = pam_get_data(
                pamh,
                PAM_LSASS_SMART_CARD_PIN,
                (PAM_GET_DATA_TYPE) & params.pszPassword);
        if (iPamError == PAM_SUCCESS && params.pszPassword != NULL)
        {
            params.dwFlags |= LSA_AUTH_USER_PAM_FLAG_SMART_CARD;
        }
        else
        {
            params.pszPassword = ctx.pszPassword;
        }

        if (bIsLocalUser)
        {
            // Lsass does not handle local user. Return PAM_IGNORE
            // and let pam_unix or pam_ldap handle it.
            dwError = LW_ERROR_IGNORE_THIS_USER;
            BAIL_ON_LSA_ERROR(dwError);
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

        if (dwError == LW_ERROR_NOT_HANDLED)
        {
            // Lsass did not handle this user. Return PAM_IGNORE
            // and let pam_unix or pam_ldap handle it.
            dwError = LW_ERROR_IGNORE_THIS_USER;
        }

        BAIL_ON_LSA_ERROR(dwError);

        if (!pPamContext->pamOptions.bNoRequireMembership)
        {
            dwError = LsaCheckUserInList(
                    hLsaConnection,
                    ctx.pszLoginId,
                    NULL);
            if (dwError)
            {
                LSA_LOG_PAM_ERROR("User %s is denied access because they are not in the 'require membership of' list",
                        LSA_SAFE_LOG_STRING(ctx.pszLoginId));
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
        if (strcmp(ctx.pszLoginId, pUserInfo->pszName))
        {
            LSA_LOG_PAM_INFO("Canonicalizing pam username from '%s' to '%s'\n",
                    ctx.pszLoginId, pUserInfo->pszName);
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
                ctx.pszLoginId);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LsaCloseSession(hLsaConnection,
                ctx.pszLoginId);
        BAIL_ON_LSA_ERROR(dwError);

        /* On Solaris, we must save the user's password in
           a custom location so that we can pull it out later
           for password changes */
        dwError = LsaPamSetDataString(
                pamh,
                PAM_LSASS_OLDAUTHTOK,
                ctx.pszPassword);
        BAIL_ON_LSA_ERROR(dwError);
#endif

    }

cleanup:

    if (hLsaConnection != (HANDLE) NULL) LsaCloseServer(hLsaConnection);

    if (pConfig) LsaPamFreeConfig(pConfig);

    if (pUserInfo) LsaFreeUserInfo(dwUserInfoLevel, (PVOID) pUserInfo);

    if (pInfo) LsaFreeAuthUserPamInfo(pInfo);

    if (pObject) LsaFreeSecurityObject(pObject);

    if (ctx.pszPassword != ctx.pszPINPrompt) LW_SECURE_FREE_STRING(ctx.pszPassword);

    LW_SAFE_FREE_STRING(ctx.pszLoginId);
    LW_SAFE_FREE_STRING(ctx.pszSmartCardReader);
    LW_SAFE_FREE_STRING(ctx.pszPINPrompt);

    LSA_LOG_PAM_DEBUG("pam_sm_authenticate::end");

    return LsaPamOpenPamFilterAuthenticate(LsaPamMapErrorCode(dwError, pPamContext));

error:

    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED ||
            dwError == LW_ERROR_IGNORE_THIS_USER)
    {
        LSA_LOG_PAM_WARNING("pam_sm_authenticate error [login:%s][error code:%u]",
                LSA_SAFE_LOG_STRING(ctx.pszLoginId),
                dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("pam_sm_authenticate error [login:%s][error code:%u]",
                LSA_SAFE_LOG_STRING(ctx.pszLoginId),
                dwError);
    }

    goto cleanup;
}

int
pam_sm_setcred(
        pam_handle_t* pamh,
        int flags,
        int argc,
        const char* argv[]
        )
{
    HANDLE hLsaConnection = (HANDLE) NULL;
    DWORD dwError = 0;
    PLSA_PAM_CONFIG pConfig = NULL;
    PSTR pszLoginId = NULL;
    PPAMCONTEXT pPamContext = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    int iPamError = 0;

    LSA_LOG_PAM_DEBUG("pam_sm_setcred::begin");

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

    if (LsaShouldIgnoreUser(pszLoginId))
    {
        LSA_LOG_PAM_WARNING("Bypassing lsass for ignore user %s", pszLoginId);
        dwError = LW_ERROR_IGNORE_THIS_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);


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

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
            hLsaConnection,
            pszLoginId,
            dwUserInfoLevel,
            (PVOID*) & pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    if (hLsaConnection != (HANDLE) NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszLoginId);

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID) pUserInfo);
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
    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED ||
            dwError == LW_ERROR_IGNORE_THIS_USER)
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


#if !defined(__LWI_DARWIN__)

static
BOOLEAN IsLocalUser(PCSTR pszLoginId)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bFound = FALSE;
    struct stat statbuf;
    char buffer[1024];
    PSTR pUsername = NULL;
    FILE *file = NULL;

    if (!pszLoginId)
        BAIL_ON_LSA_ERROR(LW_ERROR_INTERNAL);


    // Append : delimiter since the first field in passwd file is "username:"
    dwError = LwAllocateStringPrintf(&pUsername, "%s:", pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    if (stat("/etc/passwd", &statbuf) < 0)
    {
        dwError = ERROR_FILE_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!S_ISREG(statbuf.st_mode))
    {
        // File is not a regular file.
        dwError = ERROR_FILE_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    file = fopen("/etc/passwd", "r");
    if (!file)
    {
        dwError = ERROR_FILE_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    while (!bFound)
    {
        memset(buffer, 0, 1024);
        if (fgets(buffer, 1024, file) == NULL)
            break;

        if (strncmp(buffer, pUsername, strlen(pUsername)) == 0)
            bFound = TRUE;
    }

cleanup:
    LW_SAFE_FREE_STRING(pUsername);

    fclose(file);

    return bFound;

error:
    goto cleanup;
}
#else

static
BOOLEAN IsLocalUser(PCSTR pszLoginId)
{
    // Temporary. Scheduled for a later release. On Mac systems, need to
    // interface with OpenDirectory Services. The limitation is that 
    // local users and other users with both show the local user password
    // prompt if the group policy is provisioned.
    return TRUE;
}
#endif
