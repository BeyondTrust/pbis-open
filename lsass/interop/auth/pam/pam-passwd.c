/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        pam-passwd.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 *        Password API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

#ifndef PAM_BAD_ITEM
#define PAM_BAD_ITEM PAM_SERVICE_ERR
#endif

int
pam_sm_chauthtok(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char**  argv
    )
{
    DWORD dwError = 0;
    PPAMCONTEXT pPamContext = NULL;
    PLSA_PAM_CONFIG pConfig = NULL;
    int iPamError = 0;

    LSA_LOG_PAM_DEBUG("pam_sm_chauthtok::begin");

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);


    if (!(flags & PAM_UPDATE_AUTHTOK) &&
        !(flags & PAM_PRELIM_CHECK))
    {
        dwError = LsaPamUnmapErrorCode(PAM_AUTHTOK_ERR);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPamGetContext(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext);
    BAIL_ON_LSA_ERROR(dwError);

    if (flags & PAM_PRELIM_CHECK)
    {
        dwError = LsaPamCheckCurrentPassword(
                            pamh,
                            pPamContext);
        if ( dwError == LW_ERROR_PASSWORD_EXPIRED )
        {
            /*
             * We must return success, so that PAM will
             * prompt us for the next stage where the password
             * is actually changed
             */
            dwError = 0;
        }
    }
    else if (flags & PAM_UPDATE_AUTHTOK)
    {

        pPamContext->pamOptions.bUseFirstPass = TRUE;

        dwError = LsaPamUpdatePassword(
                        pamh,
                        pPamContext);
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    LSA_LOG_PAM_DEBUG("pam_sm_chauthtok::end");

#ifdef __LWI_AIX__
     /* If PAM_SUCCESS is returned on AIX 5.3 TL4, AIX will not call back
        with PAM_UPDATE_AUTHTOK; AIX will think the password change has
        finished and was successful. It seems like any error code other
        than PAM_SUCCESS will convince AIX to call back.
 
        On AIX 5.3 TL6, PAM_SUCCESS can be returned, however most other
        error codes (including PAM_NEW_AUTHTOK_REQD) will cause AIX to
        think an unrecoverable error occurred.

        PAM_IGNORE is the magic error code that works on both AIX 5.3
        TL4 and AIX 5.3 TL6.
      */
    if ( !dwError && (flags & PAM_PRELIM_CHECK) )
    {
        iPamError = PAM_IGNORE;
    }
    else
    {
        iPamError = LsaPamOpenPamFilterChauthtok(
                                    LsaPamMapErrorCode(dwError, pPamContext));
    }
#else
    iPamError = LsaPamOpenPamFilterChauthtok(
                                LsaPamMapErrorCode(dwError, pPamContext));
#endif

    LSA_LOG_PAM_DEBUG("pam_sm_chauthtok::returning pam error code %d", iPamError);
    return iPamError;

error:

    if (dwError ==  LW_ERROR_NO_SUCH_USER)
    {
        LSA_LOG_PAM_DEBUG("pam_sm_chauthtok failed since the user could not be found [error code: %u]", dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("pam_sm_chauthtok failed [error code: %u]", dwError);
    }

    goto cleanup;
}

DWORD
LsaPamCheckCurrentPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT pPamContext
    )
{
    DWORD   dwError = 0;
    HANDLE  hLsaConnection = (HANDLE)NULL;
    BOOLEAN bCheckOldPassword = TRUE;
    PSTR    pszOldPassword = NULL;
    PSTR    pszLoginId = NULL;
    PSTR pszMessage = NULL;

    LSA_LOG_PAM_DEBUG("LsaPamCheckCurrentPassword::begin");

    dwError = LsaPamGetLoginId(
                    pamh,
                    pPamContext,
                    &pszLoginId,
                    TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPamMustCheckCurrentPassword(
                     hLsaConnection,
                     pszLoginId,
                     &bCheckOldPassword);
    BAIL_ON_LSA_ERROR(dwError);

    if (bCheckOldPassword)
    {
        dwError = LsaPamGetOldPassword(
                          pamh,
                          pPamContext,
                          &pszOldPassword);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAuthenticateUser(
                          hLsaConnection,
                          pszLoginId,
                          pszOldPassword,
                          &pszMessage);
        if (pszMessage)
        {
            LsaPamConverse(pamh,
                           pszMessage,
                           PAM_TEXT_INFO,
                           NULL);
        }
        BAIL_ON_LSA_ERROR(dwError);
     }

cleanup:

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszLoginId);
    LW_SECURE_FREE_STRING(pszOldPassword);
    LW_SAFE_FREE_STRING(pszMessage);

    LSA_LOG_PAM_DEBUG("LsaPamCheckCurrentPassword::end");

    return dwError;

error:

    LSA_LOG_PAM_ERROR("LsaPamCheckCurrentPassword failed [login:%s][error code: %u]",
                      LSA_SAFE_LOG_STRING(pszLoginId),
                      dwError);

    goto cleanup;
}

DWORD
LsaPamMustCheckCurrentPassword(
    HANDLE   hLsaConnection,
    PCSTR    pszLoginId,
    PBOOLEAN pbCheckOldPassword)
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;
    BOOLEAN bCheckOldPassword = FALSE;

    LSA_LOG_PAM_DEBUG("LsaPamMustCheckCurrentPassword::begin");

    dwError = LsaFindUserByName(
                     hLsaConnection,
                     pszLoginId,
                     dwUserInfoLevel,
                     &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (((PLSA_USER_INFO_1)pUserInfo)->bIsLocalUser)
    {
        // Local root user does not have to
        // provider a user's old password.
        bCheckOldPassword = (getuid() != 0);
    }
    else
    {
        // TODO: Handle domain admins
        // For now, prompt for old password of AD Users
        bCheckOldPassword = TRUE;
    }

    *pbCheckOldPassword = bCheckOldPassword;

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    LSA_LOG_PAM_DEBUG("LsaPamMustCheckCurrentPassword::end");

    return dwError;

error:

    *pbCheckOldPassword = TRUE;

    if (dwError == LW_ERROR_NO_SUCH_USER)
    {
        LSA_LOG_PAM_DEBUG("LsaPamMustCheckCurrentPassword failed since the user could not be found [login:%s][error code: %u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("LsaPamMustCheckCurrentPassword failed [login:%s][error code: %u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }

    goto cleanup;
}

DWORD
LsaPamUpdatePassword(
    pam_handle_t* pamh,
    PPAMCONTEXT   pPamContext
    )
{
    DWORD  dwError = 0;
    PSTR   pszOldPassword = NULL;
    PSTR   pszPassword = NULL;
    PSTR   pszLoginId = NULL;
    PCSTR  pszConstLoginId = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_QUERY_LIST query = { NULL };
    PLSA_SECURITY_OBJECT* ppUser = NULL;

    LSA_LOG_PAM_DEBUG("LsaPamUpdatePassword::begin");

    if (pPamContext->bPasswordChangeFailed)
    {
        LSA_LOG_PAM_DEBUG("Password change already failed");
        dwError = LW_ERROR_USER_CANNOT_CHANGE_PASSWD;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (pPamContext->bPasswordChangeSuceeded)
    {
        LSA_LOG_PAM_DEBUG("Password change already suceeded");
        goto cleanup;
    }

    dwError = LsaPamGetLoginId(
                    pamh,
                    pPamContext,
                    &pszLoginId,
                    TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    // Verify the user is known to lsass
    pszConstLoginId = pszLoginId;
    query.ppszStrings = &pszConstLoginId;
    dwError = LsaFindObjects(
                    hLsaConnection,
                    NULL,
                    LSA_FIND_FLAGS_NSS,
                    LSA_OBJECT_TYPE_USER,
                    LSA_QUERY_TYPE_BY_NAME,
                    1,
                    query,
                    &ppUser);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ppUser[0])
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPamGetOldPassword(
                   pamh,
                   pPamContext,
                   &pszOldPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPamGetNewPassword(
                   pamh,
                   pPamContext,
                   &pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangePassword(
                   hLsaConnection,
                   pszLoginId,
                   pszPassword,
                   pszOldPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SECURE_FREE_STRING(pszPassword);
    LW_SECURE_FREE_STRING(pszOldPassword);
    LW_SAFE_FREE_STRING(pszLoginId);
    if (ppUser)
    {
        LsaFreeSecurityObjectList(
            1,
            ppUser);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pPamContext->pamOptions.bRememberChPass)
    {
        if (dwError == LW_ERROR_SUCCESS)
        {
            pPamContext->bPasswordChangeSuceeded = TRUE;
        }
        else
        {
            pPamContext->bPasswordChangeFailed = TRUE;
        }
    }

    LSA_LOG_PAM_DEBUG("LsaPamUpdatePassword::end");

    return dwError;

error:

    if ( dwError == LW_ERROR_PASSWORD_RESTRICTION )
    {
        LsaPamConverse(
            pamh,
            "Password does not meet requirements",
            PAM_ERROR_MSG,
            NULL);
    }

    if (dwError == LW_ERROR_NO_SUCH_USER)
    {
        LSA_LOG_PAM_DEBUG("LsaPamUpdatePassword failed since the user could not be found [login:%s][error code: %u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("LsaPamUpdatePassword failed [login:%s][error code: %u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }

    goto cleanup;
}

DWORD
LsaPamGetCurrentPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT   pPamContext,
    PCSTR pcszPasswordPrompt,
    PSTR*         ppszPassword
    )
{
    DWORD dwError = 0;
    PSTR pszPassword = NULL;
    BOOLEAN bPrompt = TRUE;
    PPAMOPTIONS pPamOptions = &pPamContext->pamOptions;
    int iPamError = 0;

    LSA_LOG_PAM_DEBUG("LsaPamGetCurrentPassword::begin");

    if (pPamOptions->bTryFirstPass ||
        pPamOptions->bUseFirstPass) {
        PCSTR pszItem = NULL;

        iPamError = pam_get_item(
                        pamh,
                        PAM_AUTHTOK,
                        (PAM_GET_ITEM_TYPE)&pszItem);
        dwError = LsaPamUnmapErrorCode(iPamError);
        if (dwError)
        {
            if (dwError == LsaPamUnmapErrorCode(PAM_BAD_ITEM))
            {
                if (pPamOptions->bUseFirstPass)
                {
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    dwError = 0;
                }
            }
            else
            {
               BAIL_ON_LSA_ERROR(dwError);
            }

        }
        else if (!LW_IS_NULL_OR_EMPTY_STR(pszItem))
        {
            dwError = LwAllocateString(pszItem, &pszPassword);
            BAIL_ON_LSA_ERROR(dwError);
            bPrompt = FALSE;
        }
        else if (pPamOptions->bUseFirstPass)
        {
            dwError = LsaPamUnmapErrorCode(PAM_BAD_ITEM);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (bPrompt) {

       dwError = LsaPamConverse(
                        pamh,
                        pcszPasswordPrompt,
                        PAM_PROMPT_ECHO_OFF,
                        &pszPassword);
       BAIL_ON_LSA_ERROR(dwError);

       iPamError = pam_set_item(
                       pamh,
                       PAM_AUTHTOK,
                       (const void*) pszPassword);
       dwError = LsaPamUnmapErrorCode(iPamError);
       BAIL_ON_LSA_ERROR(dwError);

    }

    *ppszPassword = pszPassword;

cleanup:

    LSA_LOG_PAM_DEBUG("LsaPamGetCurrentPassword::end");

    return dwError;

error:

    LW_SECURE_FREE_STRING(pszPassword);

    *ppszPassword = NULL;

    LSA_LOG_PAM_ERROR("LsaPamGetCurrentPassword failed [error code: %u]", dwError);

    goto cleanup;
}

DWORD
LsaPamGetOldPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT pPamContext,
    PSTR* ppszPassword
    )
{
    DWORD dwError = 0;
    PSTR pszPassword = NULL;
    BOOLEAN bPrompt = TRUE;
    PPAMOPTIONS pPamOptions = &pPamContext->pamOptions;
    int iPamError = 0;

    LSA_LOG_PAM_DEBUG("LsaPamGetOldPassword::begin");

    if (pPamOptions->bTryFirstPass ||
        pPamOptions->bUseFirstPass)
    {
        PCSTR pszItem = NULL;

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__)
        /* Solaris doesn't use PAM_OLDAUTHTOK, but we previously saved
           the password as PAM_LSASS_OLDAUTHTOK during pam_sm_authenticate,
           so we'll grab it from there */

        /* HP-UX likes to clear PAM_OLDAUTHTOK between the two phases
           of chauthtok, so we also grab our saved version in this case */
        iPamError = pam_get_data(
            pamh,
            PAM_LSASS_OLDAUTHTOK,
            (PAM_GET_DATA_TYPE)&pszItem);
        dwError = LsaPamUnmapErrorCode(iPamError);
#else
        iPamError = pam_get_item(
            pamh,
            PAM_OLDAUTHTOK,
            (PAM_GET_ITEM_TYPE)&pszItem);
        dwError = LsaPamUnmapErrorCode(iPamError);
#endif
        if (dwError == LsaPamUnmapErrorCode(PAM_BAD_ITEM) ||
            pszItem == NULL)
        {
            if (pPamOptions->bUseFirstPass)
            {
                bPrompt = FALSE;
                dwError = LW_ERROR_INVALID_PASSWORD;
                BAIL_ON_LSA_ERROR(dwError);
            }
            else if (pPamOptions->bTryFirstPass)
            {
                dwError = 0;
            }
        }
        else if (dwError)
        {
           BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LwAllocateString(pszItem, &pszPassword);
            BAIL_ON_LSA_ERROR(dwError);

            bPrompt = FALSE;
        }
    }

    if (bPrompt) {

        LSA_LOG_PAM_DEBUG("LsaPamGetOldPassword::prompting for current password");
       dwError = LsaPamConverse(
                            pamh,
                            "Current password: ",
                            PAM_PROMPT_ECHO_OFF,
                            &pszPassword);
       BAIL_ON_LSA_ERROR(dwError);

       iPamError = pam_set_item(
                       pamh,
                       PAM_OLDAUTHTOK,
                       (const void*) pszPassword);
       dwError = LsaPamUnmapErrorCode(iPamError);
       BAIL_ON_LSA_ERROR(dwError);

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__)
       /* HP-UX clears PAM_OLDAUTHTOK between the two phases of chauthtok, so
          save a copy of the old password where we can find it later */

       /* For Solaris, we read PAM_LSASS_OLDAUTHTOK instead of 
          PAM_OLDAUTHTOK. */
       dwError = LsaPamSetDataString(pamh, PAM_LSASS_OLDAUTHTOK, pszPassword);
       BAIL_ON_LSA_ERROR(dwError);
#endif
    }

    *ppszPassword = pszPassword;

cleanup:

    LSA_LOG_PAM_DEBUG("LsaPamGetOldPassword::end");

    return dwError;

error:

    LW_SECURE_FREE_STRING(pszPassword);

    *ppszPassword = NULL;

    LSA_LOG_PAM_ERROR("LsaPamGetOldPassword failed [error code: %u]", dwError);

    goto cleanup;
}

DWORD
LsaPamGetNewPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT   pPamContext,
    PSTR*         ppszPassword
    )
{
    DWORD dwError = 0;
    PSTR pszPassword_1 = NULL;
    PSTR pszPassword_2 = NULL;
    DWORD dwLen_1 = 0;
    DWORD dwLen_2 = 0;
    BOOLEAN bPrompt = TRUE;
    PPAMOPTIONS pPamOptions = &pPamContext->pamOptions;
    int iPamError = 0;

    LSA_LOG_PAM_DEBUG("LsaPamGetNewPassword::begin");

    if (pPamOptions->bUseAuthTok) {

        PCSTR pszItem = NULL;

        iPamError = pam_get_item(
                    pamh,
                    PAM_AUTHTOK,
                    (PAM_GET_ITEM_TYPE)&pszItem);
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);

        if (!LW_IS_NULL_OR_EMPTY_STR(pszItem)) {
            dwError = LwAllocateString(pszItem, &pszPassword_1);
            BAIL_ON_LSA_ERROR(dwError);

            bPrompt = FALSE;
        }
    }

    while (bPrompt)
    {
        LSA_LOG_PAM_DEBUG("LsaPamGetOldPassword::prompting for new password");
       dwError = LsaPamConverse(
                pamh,
                "New password: ",
                PAM_PROMPT_ECHO_OFF,
                &pszPassword_1);
       BAIL_ON_LSA_ERROR(dwError);

        LSA_LOG_PAM_DEBUG("LsaPamGetOldPassword::prompting for re-enter password");
       dwError = LsaPamConverse(
                pamh,
                "Re-enter password: ",
                PAM_PROMPT_ECHO_OFF,
                &pszPassword_2);
       BAIL_ON_LSA_ERROR(dwError);

       // Will never get NULL on success.
       // It could be an empty string though.
       dwLen_1 = strlen(pszPassword_1);
       dwLen_2 = strlen(pszPassword_2);

       if ((dwLen_1 != dwLen_2) ||
           (strcmp(pszPassword_1, pszPassword_2) != 0))
       {
           LsaPamConverse(pamh, "Passwords do not match", PAM_ERROR_MSG, NULL);
           dwError = 0;

           LW_SECURE_FREE_STRING(pszPassword_1);
           LW_SECURE_FREE_STRING(pszPassword_2);

       }
       else
       {
           iPamError = pam_set_item(
                       pamh,
                       PAM_AUTHTOK,
                       (const void*) pszPassword_1);
           dwError = LsaPamUnmapErrorCode(iPamError);
           BAIL_ON_LSA_ERROR(dwError);

           bPrompt = FALSE;
       }
    }

    *ppszPassword = pszPassword_1;

cleanup:

    LW_SECURE_FREE_STRING(pszPassword_2);

    LSA_LOG_PAM_DEBUG("LsaPamGetNewPassword::end");

    return dwError;

error:

    LW_SECURE_FREE_STRING(pszPassword_1);

    *ppszPassword = NULL;

    LSA_LOG_PAM_ERROR("LsaPamGetNewPassword failed [error code: %u]", dwError);

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
