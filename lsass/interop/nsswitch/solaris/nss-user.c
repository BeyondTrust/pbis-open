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
 *        nss-user.c
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
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

#ifndef HAVE_STRUCT_NSS_XBYY_ARGS_RETURNLEN
/* In order to support NSS2 using a Sol 8 build
 * we need to define a NSS2 compatible nss_XbyY_args struct
 */
typedef struct nss_XbyY_nss2_args {
    nss_XbyY_buf_t  buf;
    int             stayopen;
#if defined(__STDC__)
    int             (*str2ent)      (const char             *instr,
                                    int                     instr_len,
                                    void *ent, char *buffer, int buflen);
#else
    int             (*str2ent)();
#endif
    union nss_XbyY_key key;
    void            *returnval;
    int             erange;
    int             h_errno;
    nss_status_t    status;
#if defined(__STDC__)
    int             (*key2str)  (void *buffer, size_t buflen,
                                void *key, size_t *len);
#else
    int             (*key2str)();
#endif
	size_t          returnlen;
} nss_XbyY_nss2_args_t;
#endif

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
        if (pResultUser)
        {
            pXbyYArgs->returnval = pResultUser;
        }
        else
        {
            pXbyYArgs->returnval = pszBuf;
#ifdef HAVE_STRUCT_NSS_XBYY_ARGS_RETURNLEN
            pXbyYArgs->returnlen = strlen(pszBuf);
#else
            /* The pXbyYArgs->buf.result was NULL indicating this is NSS2/NSCD
             * Cast the nss_XbyY_args_t to a NSS2 compatible version.
             * NOTE: This is only required if we build on Solaris 8
             */
            ((nss_XbyY_nss2_args_t *)pXbyYArgs)->returnlen = strlen(pszBuf);
#endif
        }
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
        if (pResultUser)
        {
            pXbyYArgs->returnval = pResultUser;
        }
        else
        {
            pXbyYArgs->returnval = pszBuf;
#ifdef HAVE_STRUCT_NSS_XBYY_ARGS_RETURNLEN
            pXbyYArgs->returnlen = strlen(pszBuf);
#else
            /* The pXbyYArgs->buf.result was NULL indicating this is NSS2/NSCD
             * Cast the nss_XbyY_args_t to a NSS2 compatible version.
             * NOTE: This is only required if we build on Solaris 8
             */
            ((nss_XbyY_nss2_args_t *)pXbyYArgs)->returnlen = strlen(pszBuf);
#endif
        }
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
        if (pResultUser)
        {
            pXbyYArgs->returnval = pResultUser;
        }
        else
        {
            pXbyYArgs->returnval = pszBuf;
#ifdef HAVE_STRUCT_NSS_XBYY_ARGS_RETURNLEN
            pXbyYArgs->returnlen = strlen(pszBuf);
#else
            /* The pXbyYArgs->buf.result was NULL indicating this is NSS2/NSCD
             * Cast the nss_XbyY_args_t to a NSS2 compatible version.
             * NOTE: This is only required if we build on Solaris 8
             */
            ((nss_XbyY_nss2_args_t *)pXbyYArgs)->returnlen = strlen(pszBuf);
#endif
        }
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
