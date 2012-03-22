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
 *        pam-error.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 *        Likewise Errors to PAM Error mapping API
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
