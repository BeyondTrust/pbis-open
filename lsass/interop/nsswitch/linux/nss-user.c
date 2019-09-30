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
 *        nss-user.c
 *
 * Abstract:
 *
 *        Name Server Switch (BeyondTrust LSASS)
 *
 *        Handle NSS User Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "externs.h"

static LSA_ENUMUSERS_STATE gEnumUsersState = {0};

NSS_STATUS
_nss_lsass_setpwent(
    void
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonPasswdSetpwent(&lsaConnection,
                                        &gEnumUsersState);
    
    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getpwent_r(
    struct passwd * pResultUser,
    char *          pszBuf,
    size_t          bufLen,
    int *           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonPasswdGetpwent(
        &lsaConnection,
        &gEnumUsersState,
        pResultUser,
        pszBuf,
        bufLen,
        pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_endpwent(
    void
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonPasswdEndpwent(
        &lsaConnection,
        &gEnumUsersState);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getpwnam_r(
    const char *     pszLoginId,
    struct passwd *  pResultUser,
    char *           pszBuf,
    size_t           bufLen,
    int *            pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonPasswdGetpwnam(&lsaConnection,
                                        pszLoginId,
                                        pResultUser,
                                        pszBuf,
                                        bufLen,
                                        pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getpwuid_r(
    uid_t           uid,
    struct passwd * pResultUser,
    char *          pszBuf,
    size_t          bufLen,
    int *           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonPasswdGetpwuid(&lsaConnection,
                                        uid,
                                        pResultUser,
                                        pszBuf,
                                        bufLen,
                                        pErrorNumber);

    NSS_UNLOCK();

    return status;
}

