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
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include "lsanss.h"
#include "nss-user.h"

typedef struct
{
    nss_backend_t base;
    LSA_ENUMUSERS_STATE enumUsersState;
    LSA_NSS_CACHED_HANDLE lsaConnection;
} LSA_NSS_PRPASSWD_BACKEND, *PLSA_NSS_PRPASSWD_BACKEND;

typedef NSS_STATUS (*NSS_ENTRYPOINT)(nss_backend_t*, void*);

static
void
LsaNssHpuxPasswdToPrpasswd(
    struct passwd *in,
    struct pr_passwd *out)
{
    struct pr_default* defaults;

    defaults = getprdfnam("default");

    out->sfld = defaults->prd;
    out->sflg = defaults->prg;

    strncpy(out->ufld.fd_name, in->pw_name, 8);
    out->uflg.fg_name = 1;

    out->ufld.fd_uid = in->pw_uid;
    out->uflg.fg_uid = 1;

    out->ufld.fd_pswduser = in->pw_uid;
    out->uflg.fg_pswduser = 1;

    /* FIXME: we need some way to acquire this info */
    out->ufld.fd_lock = 0;
    out->uflg.fg_lock = 1;

    out->ufld.fd_pw_audid = in->pw_uid + 100;
    out->uflg.fg_pw_audid = 1;

    out->ufld.fd_pw_audflg = 1;
    out->uflg.fg_pw_audflg = 1;

    // HP-UX requires the encrypted password to be explicitly overwritten for
    // each user. HP-UX will not honor the fg_encrypt value by falling back to
    // sfld.fd_encrypt if it is 0.
    out->ufld.fd_encrypt[0] = '*';
    out->ufld.fd_encrypt[1] = 0;
    out->uflg.fg_encrypt = 1;
}

static
NSS_STATUS
LsaNssHpuxPrpasswdDestructor(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PRPASSWD_BACKEND pLsaBackend = (PLSA_NSS_PRPASSWD_BACKEND) pBackend;
    PLSA_ENUMUSERS_STATE    pEnumUsersState = &pLsaBackend->enumUsersState;
    int                     ret = NSS_STATUS_SUCCESS;

    LsaNssClearEnumUsersState(
        &pLsaBackend->lsaConnection,
        pEnumUsersState);
    LsaNssCommonCloseConnection(&pLsaBackend->lsaConnection);
    LsaFreeIgnoreHashes();
    LwFreeMemory(pBackend);

    return ret;
}

static
NSS_STATUS
LsaNssHpuxPrpasswdSetpwent(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PRPASSWD_BACKEND pLsaBackend = (PLSA_NSS_PRPASSWD_BACKEND) pBackend;
    PLSA_ENUMUSERS_STATE    pEnumUsersState = &pLsaBackend->enumUsersState;

    return LsaNssCommonPasswdSetpwent(
                                      &pLsaBackend->lsaConnection,
                                      pEnumUsersState);
}

static
NSS_STATUS
LsaNssHpuxPrpasswdGetpwent(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PRPASSWD_BACKEND pLsaBackend = (PLSA_NSS_PRPASSWD_BACKEND) pBackend;
    PLSA_ENUMUSERS_STATE    pEnumUsersState = &pLsaBackend->enumUsersState;
    nss_XbyY_args_t*        pXbyYArgs = (nss_XbyY_args_t*) pArgs;
    comsec_nss_parms_t*     pComsecParams = (comsec_nss_parms_t*) pXbyYArgs->buf.result;
    struct passwd           resultUser;
    char                    szBuf[2048];
    size_t                  bufLen = sizeof(szBuf);
    int                     err = 0;
    int                     ret;
    int*                    pErrorNumber = &err;

    ret = LsaNssCommonPasswdGetpwent(
                                     &pLsaBackend->lsaConnection,
                                     pEnumUsersState,
                                     &resultUser,
                                     szBuf,
                                     bufLen,
                                     pErrorNumber);

    if (ret == NSS_STATUS_SUCCESS)
    {
        LsaNssHpuxPasswdToPrpasswd(&resultUser, pComsecParams->prpw);
        pXbyYArgs->returnval = pComsecParams->prpw;
    }
    else if (ret == NSS_STATUS_TRYAGAIN && err == ERANGE)
    {
        pXbyYArgs->erange = 1;
    }

    return ret;
}

static
NSS_STATUS
LsaNssHpuxPrpasswdEndpwent(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PRPASSWD_BACKEND pLsaBackend = (PLSA_NSS_PRPASSWD_BACKEND) pBackend;
    PLSA_ENUMUSERS_STATE    pEnumUsersState = &pLsaBackend->enumUsersState;

    return LsaNssCommonPasswdEndpwent(
            &pLsaBackend->lsaConnection,
            pEnumUsersState);
}

static
NSS_STATUS
LsaNssHpuxPrpasswdGetpwnam(
    nss_backend_t* pBackend,
    void* pArgs)
{
    nss_XbyY_args_t*        pXbyYArgs = (nss_XbyY_args_t*) pArgs;
    int                     ret = NSS_STATUS_SUCCESS;
    int                     err = 0;
    int *                   pErrorNumber = &err;
    const char *            pszLoginId = pXbyYArgs->key.name;
    comsec_nss_parms_t*     pComsecParams = (comsec_nss_parms_t*) pXbyYArgs->buf.result;
    struct passwd           resultUser;
    char                    szBuf[2048];
    size_t                  bufLen = sizeof(szBuf);
    PLSA_NSS_PRPASSWD_BACKEND pLsaBackend = (PLSA_NSS_PRPASSWD_BACKEND) pBackend;

    ret = LsaNssCommonPasswdGetpwnam(
                                     &pLsaBackend->lsaConnection,
                                     pszLoginId,
                                     &resultUser,
                                     szBuf,
                                     bufLen,
                                     pErrorNumber);

    if (ret == NSS_STATUS_SUCCESS)
    {
        LsaNssHpuxPasswdToPrpasswd(&resultUser, pComsecParams->prpw);
        pXbyYArgs->returnval = pComsecParams->prpw;
    }

    return ret;
}

static
NSS_STATUS
LsaNssHpuxPrpasswdGetpwuid(
    nss_backend_t* pBackend,
    void* pArgs)
{
    nss_XbyY_args_t*        pXbyYArgs = (nss_XbyY_args_t*) pArgs;
    int                     ret = NSS_STATUS_SUCCESS;
    int                     err = 0;
    int *                   pErrorNumber = &err;
    uid_t                   uid = pXbyYArgs->key.uid;
    comsec_nss_parms_t*     pComsecParams = (comsec_nss_parms_t*) pXbyYArgs->buf.result;
    struct passwd           resultUser;
    char                    szBuf[2048];
    size_t                  bufLen = sizeof(szBuf);
    PLSA_NSS_PRPASSWD_BACKEND pLsaBackend = (PLSA_NSS_PRPASSWD_BACKEND) pBackend;

    ret = LsaNssCommonPasswdGetpwuid(
                                     &pLsaBackend->lsaConnection,
                                     uid,
                                     &resultUser,
                                     szBuf,
                                     bufLen,
                                     pErrorNumber);

    if (ret == NSS_STATUS_SUCCESS)
    {
        LsaNssHpuxPasswdToPrpasswd(&resultUser, pComsecParams->prpw);
        pXbyYArgs->returnval = pComsecParams->prpw;
    }

    return ret;
}

static
NSS_ENTRYPOINT
LsaNssHpuxPrpasswdOps[] =
{
    LsaNssHpuxPrpasswdDestructor,
    LsaNssHpuxPrpasswdEndpwent,
    LsaNssHpuxPrpasswdSetpwent,
    LsaNssHpuxPrpasswdGetpwent,
    LsaNssHpuxPrpasswdGetpwnam,
    LsaNssHpuxPrpasswdGetpwuid
};

static
nss_backend_t
LsaNssHpuxPrpasswdBackend =
{
    .n_ops = 6,
    .ops = LsaNssHpuxPrpasswdOps
};

nss_backend_t*
LsaNssHpuxPrpasswdCreateBackend(
    void
    )
{
    PLSA_NSS_PRPASSWD_BACKEND pLsaBackend = NULL;

    if (LwAllocateMemory(sizeof(*pLsaBackend), (void**) &pLsaBackend))
    {
        return NULL;
    }

    pLsaBackend->base = LsaNssHpuxPrpasswdBackend;

    return (nss_backend_t*) pLsaBackend;
}
