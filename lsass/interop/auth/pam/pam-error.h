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
 *        pam-error.h
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
#ifndef __PAM_ERROR_H__
#define __PAM_ERROR_H__

#define LSA_PAM_OPENPAM_FILTER_COMMON(ret) \
    if (ret == PAM_SUCCESS || \
        ret == PAM_SERVICE_ERR || \
        ret == PAM_BUF_ERR || \
        ret == PAM_CONV_ERR || \
        ret == PAM_PERM_DENIED || \
        ret == PAM_ABORT || \
        ret == PAM_IGNORE) \
        goto cleanup
int
LsaPamMapErrorCode(
    DWORD       dwError,
    PPAMCONTEXT pPamContext
    );

static
inline
DWORD
LsaPamUnmapErrorCode(
    int iPamError
    )
{
    assert(iPamError <= (_LW_ERROR_PAM_MAX - _LW_ERROR_PAM_BASE));
    return iPamError ? (_LW_ERROR_PAM_BASE + iPamError) : LW_ERROR_SUCCESS;
}

int
LsaPamOpenPamFilterAuthenticate(
    int ret
    );

int
LsaPamOpenPamFilterSetCred(
    int ret
    );

int
LsaPamOpenPamFilterAcctMgmt(
    int ret
    );

int
LsaPamOpenPamFilterOpenSession(
    int ret
    );

int
LsaPamOpenPamFilterCloseSession(
    int ret
    );

int
LsaPamOpenPamFilterChauthtok(
    int ret
    );

#endif /* __PAM_ERROR_H__ */

