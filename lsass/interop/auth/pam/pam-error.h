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
 *        pam-error.h
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

