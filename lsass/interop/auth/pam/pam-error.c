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
            ret = PAM_NEW_AUTHTOK_REQD;
            break;
        case LW_ERROR_USER_CANNOT_CHANGE_PASSWD:
            ret = PAM_PERM_DENIED;
            break;
        case LW_ERROR_INTERNAL:
            ret = PAM_SERVICE_ERR;
            break;
        case ECONNREFUSED:
        case ENOENT:
            if (pPamContext && pPamContext->pamOptions.bUnknownOK)
                ret = PAM_IGNORE;
            else
                ret = PAM_AUTH_ERR;
            break;
        case EACCES:
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
