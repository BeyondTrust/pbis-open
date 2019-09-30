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
 *        pam-error.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 *        BeyondTrust Errors to PAM Error mapping API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

int
LsaPamMapErrorCode(
    DWORD       dwError,
    PPAMCONTEXT pPamContext
    )
{
    int ret = PAM_SUCCESS;

    if (!dwError) {
       goto cleanup;
    }

    if ((_LW_ERROR_PAM_BASE <= dwError) && (dwError <= _LW_ERROR_PAM_MAX))
    {
        ret = dwError - _LW_ERROR_PAM_BASE;
        goto cleanup;
    }

    switch(dwError)
    {
        case LW_ERROR_IGNORE_THIS_USER:
            ret =  PAM_IGNORE;
            break;
        case LW_ERROR_NOT_HANDLED:
        case LW_ERROR_NO_SUCH_USER:
            if (pPamContext && (pPamContext->pamOptions.bUnknownOK ||
                                pPamContext->pamOptions.bSetDefaultRepository))
                ret = PAM_IGNORE;
            else
                ret = PAM_USER_UNKNOWN;
            break;
        case LW_ERROR_ACCOUNT_EXPIRED:
            ret = PAM_ACCT_EXPIRED;
            break;
        case LW_ERROR_ACCOUNT_DISABLED:
            ret = PAM_PERM_DENIED;
            break;
        case LW_ERROR_ACCOUNT_LOCKED:
            ret = PAM_PERM_DENIED;
            break;
        case LW_ERROR_PASSWORD_EXPIRED:
            if (pPamContext && pPamContext->pamOptions.bDisablePasswordChange)
            {
                ret = PAM_PERM_DENIED;
            }
            else
            {
                ret = PAM_NEW_AUTHTOK_REQD;
            }
            break;
        case LW_ERROR_USER_CANNOT_CHANGE_PASSWD:
            ret = PAM_PERM_DENIED;
            break;
        case LW_ERROR_INTERNAL:
            ret = PAM_SERVICE_ERR;
            break;
        case LW_ERROR_ERRNO_ECONNREFUSED:
        case ERROR_FILE_NOT_FOUND:
            if (pPamContext && pPamContext->pamOptions.bUnknownOK)
                ret = PAM_IGNORE;
            else
                ret = PAM_AUTH_ERR;
            break;
        case ERROR_ACCESS_DENIED:
        case LW_ERROR_ACCESS_DENIED:
            ret = PAM_PERM_DENIED;
            break;
        default:
            ret = PAM_AUTH_ERR;
            break;
    }

cleanup:

    return ret;
}

int
LsaPamOpenPamFilterAuthenticate(
    int ret
    )
{
#ifdef __LWI_HAS_OPENPAM__
    LSA_PAM_OPENPAM_FILTER_COMMON(ret);

    if (ret == PAM_AUTH_ERR ||
        ret == PAM_CRED_INSUFFICIENT ||
        ret == PAM_AUTHINFO_UNAVAIL ||
        ret == PAM_USER_UNKNOWN ||
        ret == PAM_MAXTRIES)
        goto cleanup;

    ret = PAM_SERVICE_ERR;

cleanup:
#endif

    return ret;
}

int
LsaPamOpenPamFilterSetCred(
    int ret
    )
{
#ifdef __LWI_HAS_OPENPAM__
    LSA_PAM_OPENPAM_FILTER_COMMON(ret);

    if (ret == PAM_CRED_UNAVAIL ||
        ret == PAM_CRED_EXPIRED ||
        ret == PAM_USER_UNKNOWN ||
        ret == PAM_CRED_ERR)
        goto cleanup;

    ret = PAM_SERVICE_ERR;

cleanup:
#endif

    return ret;
}

int
LsaPamOpenPamFilterAcctMgmt(
    int ret
    )
{
#ifdef __LWI_HAS_OPENPAM__
    LSA_PAM_OPENPAM_FILTER_COMMON(ret);

    if (ret == PAM_USER_UNKNOWN ||
        ret == PAM_AUTH_ERR ||
        ret == PAM_NEW_AUTHTOK_REQD ||
        ret == PAM_ACCT_EXPIRED)
        goto cleanup;

    ret = PAM_SERVICE_ERR;

cleanup:
#endif

    return ret;
}

int
LsaPamOpenPamFilterOpenSession(
    int ret
    )
{
#ifdef __LWI_HAS_OPENPAM__
    LSA_PAM_OPENPAM_FILTER_COMMON(ret);

    if (ret == PAM_SESSION_ERR)
        goto cleanup;

    ret = PAM_SERVICE_ERR;

cleanup:
#endif

    return ret;
}

int
LsaPamOpenPamFilterCloseSession(
    int ret
    )
{
#ifdef __LWI_HAS_OPENPAM__
    LSA_PAM_OPENPAM_FILTER_COMMON(ret);

    if (ret == PAM_SESSION_ERR)
        goto cleanup;

    ret = PAM_SERVICE_ERR;

cleanup:
#endif

    return ret;
}

int
LsaPamOpenPamFilterChauthtok(
    int ret
    )
{
#ifdef __LWI_HAS_OPENPAM__
    LSA_PAM_OPENPAM_FILTER_COMMON(ret);

    if (ret == PAM_PERM_DENIED ||
        ret == PAM_AUTHTOK_ERR ||
        ret == PAM_AUTHTOK_RECOVERY_ERR ||
        ret == PAM_AUTHTOK_LOCK_BUSY ||
        ret == PAM_AUTHTOK_DISABLE_AGING ||
        ret == PAM_TRY_AGAIN)
        goto cleanup;

    ret = PAM_SERVICE_ERR;

cleanup:
#endif

    return ret;
}
