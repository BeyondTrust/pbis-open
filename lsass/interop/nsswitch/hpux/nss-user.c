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
} LSA_NSS_PASSWD_BACKEND, *PLSA_NSS_PASSWD_BACKEND;

typedef NSS_STATUS (*NSS_ENTRYPOINT)(nss_backend_t*, void*);



static
NSS_STATUS
LsaNssSolarisPasswdDestructor(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PASSWD_BACKEND pLsaBackend = (PLSA_NSS_PASSWD_BACKEND) pBackend;
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
LsaNssSolarisPasswdSetpwent(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PASSWD_BACKEND pLsaBackend = (PLSA_NSS_PASSWD_BACKEND) pBackend;
    PLSA_ENUMUSERS_STATE    pEnumUsersState = &pLsaBackend->enumUsersState;

    return LsaNssCommonPasswdSetpwent(
                                      &pLsaBackend->lsaConnection,
                                      pEnumUsersState);
}

static
NSS_STATUS
LsaNssSolarisPasswdGetpwent(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PASSWD_BACKEND pLsaBackend = (PLSA_NSS_PASSWD_BACKEND) pBackend;
    PLSA_ENUMUSERS_STATE    pEnumUsersState = &pLsaBackend->enumUsersState;
    nss_XbyY_args_t*        pXbyYArgs = (nss_XbyY_args_t*) pArgs;
    struct passwd *         pResultUser = (struct passwd*) pXbyYArgs->buf.result;
    char*                   pszBuf = (char*) pXbyYArgs->buf.buffer;
    size_t                  bufLen = (size_t) pXbyYArgs->buf.buflen;
    int                     err = 0;
    int                     ret;
    int*                    pErrorNumber = &err;

    ret = LsaNssCommonPasswdGetpwent(
                                     &pLsaBackend->lsaConnection,
                                     pEnumUsersState,
                                     pResultUser,
                                     pszBuf,
                                     bufLen,
                                     pErrorNumber);

    if (ret == NSS_STATUS_SUCCESS)
    {
        pXbyYArgs->returnval = pXbyYArgs->buf.result;
    }
    else if (ret == NSS_STATUS_TRYAGAIN  && err == ERANGE)
    {
        pXbyYArgs->erange = 1;
        /* Solaris 8 will call again with the same buffer size if tryagain
         * is returned.
         */
        ret = NSS_STATUS_UNAVAIL;
    }
    else
    {
        errno = err;
    }

    return ret;
}

static
NSS_STATUS
LsaNssSolarisPasswdEndpwent(
    nss_backend_t* pBackend,
    void* pArgs)
{
    PLSA_NSS_PASSWD_BACKEND pLsaBackend = (PLSA_NSS_PASSWD_BACKEND) pBackend;
    PLSA_ENUMUSERS_STATE    pEnumUsersState = &pLsaBackend->enumUsersState;

    return LsaNssCommonPasswdEndpwent(
            &pLsaBackend->lsaConnection,
            pEnumUsersState);
}

static
NSS_STATUS
LsaNssSolarisPasswdGetpwnam(
    nss_backend_t* pBackend,
    void* pArgs)
{
    nss_XbyY_args_t*        pXbyYArgs = (nss_XbyY_args_t*) pArgs;
    int                     ret = NSS_STATUS_SUCCESS;
    int                     err = 0;
    int *                   pErrorNumber = &err;
    const char *            pszLoginId = pXbyYArgs->key.name;
    struct passwd *         pResultUser = (struct passwd*) pXbyYArgs->buf.result;
    char *                  pszBuf = (char*) pXbyYArgs->buf.buffer;
    size_t                  bufLen = (size_t) pXbyYArgs->buf.buflen;
    PLSA_NSS_PASSWD_BACKEND pLsaBackend = (PLSA_NSS_PASSWD_BACKEND) pBackend;

    ret = LsaNssCommonPasswdGetpwnam(
                                     &pLsaBackend->lsaConnection,
                                     pszLoginId,
                                     pResultUser,
                                     pszBuf,
                                     bufLen,
                                     pErrorNumber);

    if (ret == NSS_STATUS_SUCCESS)
    {
        pXbyYArgs->returnval = pXbyYArgs->buf.result;
    }
    else if (ret == NSS_STATUS_TRYAGAIN  && err == ERANGE)
    {
        pXbyYArgs->erange = 1;
        /* Solaris 8 will call again with the same buffer size if tryagain
         * is returned.
         */
        ret = NSS_STATUS_UNAVAIL;
    }
    else if (ret == NSS_STATUS_UNAVAIL && err == ECONNREFUSED)
    {
        /* Librestart on Solaris does not like it when getpwnam_r returns
         * ECONNREFUSED. So instead, we'll treat this case like the user
         * was not found (0 for errno but NULL for result).
         */
        errno = 0;
    }
    else
    {
        errno = err;
    }

    return ret;
}

static
NSS_STATUS
LsaNssSolarisPasswdGetpwuid(
    nss_backend_t* pBackend,
    void* pArgs)
{
    nss_XbyY_args_t*        pXbyYArgs = (nss_XbyY_args_t*) pArgs;
    int                     ret = NSS_STATUS_SUCCESS;
    int                     err = 0;
    int *                   pErrorNumber = &err;
    uid_t                   uid = pXbyYArgs->key.uid;
    struct passwd *         pResultUser = (struct passwd*) pXbyYArgs->buf.result;
    char *                  pszBuf = (char*) pXbyYArgs->buf.buffer;
    size_t                  bufLen = (size_t) pXbyYArgs->buf.buflen;
    PLSA_NSS_PASSWD_BACKEND pLsaBackend = (PLSA_NSS_PASSWD_BACKEND) pBackend;

    ret = LsaNssCommonPasswdGetpwuid(
                                     &pLsaBackend->lsaConnection,
                                     uid,
                                     pResultUser,
                                     pszBuf,
                                     bufLen,
                                     pErrorNumber);

    if (ret == NSS_STATUS_SUCCESS)
    {
        pXbyYArgs->returnval = pXbyYArgs->buf.result;
    }
    else if (ret == NSS_STATUS_TRYAGAIN  && err == ERANGE)
    {
        pXbyYArgs->erange = 1;
        /* Solaris 8 will call again with the same buffer size if tryagain
         * is returned.
         */
        ret = NSS_STATUS_UNAVAIL;
    }
    else if (ret == NSS_STATUS_UNAVAIL && err == ECONNREFUSED)
    {
        /* Librestart on Solaris does not like it when getpwnam_r returns
         * ECONNREFUSED. So instead, we'll treat this case like the user
         * was not found (0 for errno but NULL for result).
         */
        errno = 0;
    }
    else
    {
        errno = err;
    }

    return ret;
}

static
NSS_ENTRYPOINT
LsaNssSolarisPasswdOps[] =
{
    LsaNssSolarisPasswdDestructor,
    LsaNssSolarisPasswdEndpwent,
    LsaNssSolarisPasswdSetpwent,
    LsaNssSolarisPasswdGetpwent,
    LsaNssSolarisPasswdGetpwnam,
    LsaNssSolarisPasswdGetpwuid
};

static
nss_backend_t
LsaNssSolarisPasswdBackend =
{
    .n_ops = 6,
    .ops = LsaNssSolarisPasswdOps
};

nss_backend_t*
LsaNssSolarisPasswdCreateBackend(
    void
    )
{
    PLSA_NSS_PASSWD_BACKEND pLsaBackend = NULL;

    if (LwAllocateMemory(sizeof(*pLsaBackend), (void**) &pLsaBackend))
    {
        return NULL;
    }

    pLsaBackend->base = LsaNssSolarisPasswdBackend;

    return (nss_backend_t*) pLsaBackend;
}
