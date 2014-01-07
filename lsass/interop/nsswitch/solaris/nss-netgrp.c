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
 *        nss-netgrp.c
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
 *
 *        Handle NSS Net Group Information
 *
 * Authors:  Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "nss-netgrp.h"

typedef struct
{
    nss_backend_t base;
    int inner;
    LSA_NSS_CACHED_HANDLE lsaConnection;
} LSA_NSS_NETGROUP_BACKEND, *PLSA_NSS_NETGROUP_BACKEND;

typedef struct _LSA_NSS_NETGROUP_LIST
{
    PSTR pszGroup;
    struct _LSA_NSS_NETGROUP_LIST* pNext;
} LSA_NSS_NETGROUP_LIST, *PLSA_NSS_NETGROUP_LIST;

typedef struct
{
    LSA_NSS_NETGROUP_BACKEND base;
    char* pBuffer;
    char* pCursor;
    PLSA_NSS_NETGROUP_LIST pExpand;
    PLSA_NSS_NETGROUP_LIST pSeen;
} LSA_NSS_NETGROUP_INNER_BACKEND, *PLSA_NSS_NETGROUP_INNER_BACKEND;

typedef NSS_STATUS (*NSS_ENTRYPOINT)(nss_backend_t*, void*);

static NSS_ENTRYPOINT LsaNssSolarisNetgroupOps[];
static nss_backend_t LsaNssSolarisNetgroupBackend;

static
NSS_STATUS
LsaNssPushNetgroup(
    PLSA_NSS_NETGROUP_LIST* ppList,
    PCSTR pszGroup
    )
{
    int ret = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_LIST pLink = NULL;

    ret = MAP_LSA_ERROR(NULL,
                        LwAllocateMemory(
                            sizeof(*pLink),
                            (void**) &pLink));
    BAIL_ON_NSS_ERROR(ret);

    ret = MAP_LSA_ERROR(NULL,
                        LwAllocateString(
                            pszGroup,
                            &pLink->pszGroup));
    BAIL_ON_NSS_ERROR(ret);

    pLink->pNext = *ppList;
    *ppList = pLink;

error:

    return ret;
}

static
NSS_STATUS
LsaNssPopNetgroup(
    PLSA_NSS_NETGROUP_LIST* ppList,
    PSTR* ppszGroup
    )
{
    int ret = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_LIST pLink = *ppList;

    if (!pLink)
    {
        ret = NSS_STATUS_UNAVAIL;
        BAIL_ON_NSS_ERROR(ret);
    }

    *ppList = pLink->pNext;
    *ppszGroup = pLink->pszGroup;
    LwFreeMemory(pLink);

error:

    return ret;
}

static
BOOLEAN
LsaNssContainsNetgroup(
    PLSA_NSS_NETGROUP_LIST pList,
    PSTR pszGroup
    )
{
    PLSA_NSS_NETGROUP_LIST pLink = NULL;

    for (pLink = pList; pLink; pLink = pLink->pNext)
    {
        if (!strcmp(pLink->pszGroup, pszGroup))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static
NSS_STATUS
LsaNssFreeNetgroupList(
    PLSA_NSS_NETGROUP_LIST* ppList
    )
{
    int ret = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_LIST pLink, pNext;

    for (pLink = *ppList; pLink; pLink = pNext)
    {
        pNext = pLink->pNext;

        LwFreeMemory(pLink->pszGroup);
        LwFreeMemory(pLink);
    }

    *ppList = NULL;

    return ret;
}

static
NSS_STATUS
LsaNssSolarisNetgroupDestructor(
    nss_backend_t* pBackend,
    void* pArgs)
{
    int ret = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_INNER_BACKEND pLsaBackend = (PLSA_NSS_NETGROUP_INNER_BACKEND) pBackend;

    if (pLsaBackend->base.inner)
    {
        LsaNssFreeNetgroupList(&pLsaBackend->pSeen);
        LsaNssFreeNetgroupList(&pLsaBackend->pExpand);

        LsaNssCommonCloseConnection(&pLsaBackend->base.lsaConnection);
        LsaFreeIgnoreHashes();
        LW_SAFE_FREE_MEMORY(pLsaBackend->pBuffer);
    }

    LwFreeMemory(pBackend);

    return ret;
}

static
NSS_STATUS
LsaNssSolarisNetgroupInnerSetent(
    nss_backend_t* pBackend,
    void* pArgs
    )
{
    return NSS_STATUS_SUCCESS;
}

static
NSS_STATUS
buffer_alloc(
    char** buffer,
    size_t* buflen,
    size_t needed,
    char** chunk,
    enum nss_netgr_status* status)
{
    size_t needed_aligned = ((needed / sizeof(size_t)) + 1) * sizeof(size_t);
    if (needed_aligned > *buflen)
    {
        *status = NSS_NETGR_NOMEM;
        return NSS_STATUS_UNAVAIL;
    }
    else
    {
        *chunk = *buffer;
        *buffer += needed_aligned;
        *buflen -= needed_aligned;
        return NSS_STATUS_SUCCESS;
    }
}

static
NSS_STATUS
LsaNssSolarisNetgroupInnerNext(
    PLSA_NSS_NETGROUP_INNER_BACKEND pBackend
    )
{
    NSS_STATUS ret = NSS_STATUS_SUCCESS;
    PSTR pszGroup = NULL;
    PSTR pszContents = NULL;

    while (pBackend->pExpand)
    {
        LsaNssPopNetgroup(&pBackend->pExpand, &pszGroup);

        ret = LsaNssCommonNetgroupFindByName(
            &pBackend->base.lsaConnection,
            pszGroup,
            &pszContents);
        if (ret == NSS_STATUS_NOTFOUND)
        {
            /* Don't let bad entries stop expansion of valid ones */
            continue;
        }
        BAIL_ON_NSS_ERROR(ret);

        if (pBackend->pBuffer)
        {
            LwFreeMemory(pBackend->pBuffer);
        }
        pBackend->pBuffer = pBackend->pCursor = pszContents;
        goto cleanup;
    }

    ret = NSS_STATUS_NOTFOUND;
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    return ret;

error:

    goto cleanup;
}

static
NSS_STATUS
LsaNssSolarisNetgroupInnerParse(
    PLSA_NSS_NETGROUP_INNER_BACKEND pBackend,
    PSTR* ppszOutHost,
    PSTR* ppszOutUser,
    PSTR* ppszOutDomain,
    enum nss_netgr_status* pStatus
    )
{
    NSS_STATUS ret = NSS_STATUS_SUCCESS;
    PSTR pszHost = NULL;
    PSTR pszUser = NULL;
    PSTR pszDomain = NULL;
    PSTR pszGroup = NULL;
    LSA_NSS_NETGROUP_ENTRY_TYPE type;

    /* Keep parsing until we encounter an error
       or find a triple entry */

    while (1)
    {
        if (!pBackend->pBuffer)
        {
            ret = LsaNssSolarisNetgroupInnerNext(pBackend);
            BAIL_ON_NSS_ERROR(ret);
        }

        ret = LsaNssCommonNetgroupParse(
            &pBackend->pCursor,
            &type,
            &pszHost,
            &pszUser,
            &pszDomain,
            &pszGroup);
        BAIL_ON_NSS_ERROR(ret);

        switch (type)
        {
        case LSA_NSS_NETGROUP_ENTRY_GROUP:
            if (!LsaNssContainsNetgroup(pBackend->pSeen, pszGroup))
            {
                ret = LsaNssPushNetgroup(&pBackend->pExpand, pszGroup);
                BAIL_ON_NSS_ERROR(ret);
                ret = LsaNssPushNetgroup(&pBackend->pSeen, pszGroup);
                BAIL_ON_NSS_ERROR(ret);
            }
            break;
        case LSA_NSS_NETGROUP_ENTRY_TRIPLE:
            *ppszOutUser = pszUser;
            *ppszOutHost = pszHost;
            *ppszOutDomain = pszDomain;
            *pStatus = NSS_NETGR_FOUND;
            goto cleanup;
        case LSA_NSS_NETGROUP_ENTRY_END:
            LwFreeMemory(pBackend->pBuffer);
            pBackend->pBuffer = NULL;
            break;
        }
    }

cleanup:

    return ret;

error:

    if (ret == NSS_STATUS_NOTFOUND)
    {
        LwFreeMemory(pBackend->pBuffer);
        pBackend->pBuffer = NULL;
        *pStatus = NSS_NETGR_NO;
    }

    goto cleanup;
}


static
NSS_STATUS
LsaNssSolarisNetgroupInnerGetent(
    nss_backend_t* pBackend,
    void* pArgs
    )
{
    NSS_STATUS ret = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_INNER_BACKEND pLsaBackend = (PLSA_NSS_NETGROUP_INNER_BACKEND) pBackend;
    PSTR pszUser = NULL;
    PSTR pszHost = NULL;
    PSTR pszDomain = NULL;
    struct nss_getnetgrent_args *pNetArgs = (struct nss_getnetgrent_args *) pArgs;
    PSTR pszBuffer = pNetArgs->buffer;
    size_t dwBufferSize = pNetArgs->buflen;

    ret = LsaNssSolarisNetgroupInnerParse(
        pLsaBackend,
        &pszUser,
        &pszHost,
        &pszDomain,
        &pNetArgs->status);
    BAIL_ON_NSS_ERROR(ret);

    ret = buffer_alloc(&pszBuffer, &dwBufferSize, strlen(pszHost) + 1, &pNetArgs->retp[NSS_NETGR_MACHINE], &pNetArgs->status);
    BAIL_ON_NSS_ERROR(ret);
    strcpy(pNetArgs->retp[NSS_NETGR_MACHINE], pszHost);
    ret = buffer_alloc(&pszBuffer, &dwBufferSize, strlen(pszUser) + 1, &pNetArgs->retp[NSS_NETGR_USER], &pNetArgs->status);
    BAIL_ON_NSS_ERROR(ret);
    strcpy(pNetArgs->retp[NSS_NETGR_USER], pszUser);
    ret = buffer_alloc(&pszBuffer, &dwBufferSize, strlen(pszDomain) + 1, &pNetArgs->retp[NSS_NETGR_DOMAIN], &pNetArgs->status);
    BAIL_ON_NSS_ERROR(ret);
    strcpy(pNetArgs->retp[NSS_NETGR_DOMAIN], pszDomain);

error:

    return ret;
}

static
NSS_STATUS
LsaNssSolarisNetgroupInnerEndent(
    nss_backend_t* pBackend,
    void* pArgs
    )
{
    return NSS_STATUS_SUCCESS;
}

static
NSS_STATUS
LsaNssSolarisNetgroupSetnetgrent(
    nss_backend_t* pBackend,
    void* pArgs
    )
{
    int ret = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_INNER_BACKEND pInnerBackend = NULL;
    PSTR pGroupContents = NULL;
    struct nss_setnetgrent_args *pNetArgs = (struct nss_setnetgrent_args*) pArgs;
    PLSA_NSS_NETGROUP_BACKEND    pLsaBackend = (PLSA_NSS_NETGROUP_BACKEND) pBackend;


    ret = LsaNssCommonNetgroupFindByName(
        &pLsaBackend->lsaConnection,
        pNetArgs->netgroup,
        &pGroupContents);
    BAIL_ON_LSA_ERROR(ret);

    ret = MAP_LSA_ERROR(NULL,
                        LwAllocateMemory(
                            sizeof(*pInnerBackend),
                            (void**) &pInnerBackend));
    pInnerBackend->base.base = LsaNssSolarisNetgroupBackend;
    pInnerBackend->base.inner = 1;
    pInnerBackend->pBuffer = pGroupContents;
    pInnerBackend->pCursor = pInnerBackend->pBuffer;
    pGroupContents = NULL;


    ret = LsaNssPushNetgroup(&pInnerBackend->pSeen, pNetArgs->netgroup);
    BAIL_ON_LSA_ERROR(ret);

    pNetArgs->iterator = (nss_backend_t*) pInnerBackend;

cleanup:

    LW_SAFE_FREE_STRING(pGroupContents);

    return ret;

error:

    if (pInnerBackend)
    {
        LsaNssSolarisNetgroupDestructor((nss_backend_t*) pInnerBackend, NULL);
    }

    goto cleanup;
}

static
NSS_STATUS
LsaNssSolarisNetgroupInnetgr(
    nss_backend_t* pBackend,
    void* pArgs
    )
{
    int ret = NSS_STATUS_SUCCESS;
    PLSA_NSS_NETGROUP_INNER_BACKEND pInnerBackend = NULL;
    struct nss_innetgr_args *pNetArgs = (struct nss_innetgr_args*) pArgs;
    int iGroup;
    int iMatch;
    int iArg;
    int numMatches = 0;
    PSTR pszHost = NULL;
    PSTR pszUser = NULL;
    PSTR pszDomain = NULL;
    PSTR pszMatchHost = NULL;
    PSTR pszMatchUser = NULL;
    PSTR pszMatchDomain = NULL;

    ret = MAP_LSA_ERROR(NULL,
                        LwAllocateMemory(
                            sizeof(*pInnerBackend),
                            (void**) &pInnerBackend));
    pInnerBackend->base.base = LsaNssSolarisNetgroupBackend;
    pInnerBackend->base.inner = 1;

    for(iGroup = 0; iGroup < pNetArgs->groups.argc; iGroup++)
    {
        ret = LsaNssPushNetgroup(&pInnerBackend->pExpand, pNetArgs->groups.argv[iGroup]);
        BAIL_ON_NSS_ERROR(ret);
        ret = LsaNssPushNetgroup(&pInnerBackend->pSeen, pNetArgs->groups.argv[iGroup]);
        BAIL_ON_NSS_ERROR(ret);
    }

    for (iArg = NSS_NETGR_MACHINE; iArg <= NSS_NETGR_DOMAIN; iArg++)
    {
        if (pNetArgs->arg[iArg].argc > numMatches)
        {
            numMatches = pNetArgs->arg[iArg].argc;
        }
    }

    while (1)
    {
        ret = LsaNssSolarisNetgroupInnerParse(
            pInnerBackend,
            &pszHost,
            &pszUser,
            &pszDomain,
            &pNetArgs->status);
        BAIL_ON_NSS_ERROR(ret);

        for (iMatch = 0; iMatch < numMatches; iMatch++)
        {
            pszMatchHost = pNetArgs->arg[NSS_NETGR_MACHINE].argc ? pNetArgs->arg[NSS_NETGR_MACHINE].argv[iMatch] : NULL;
            pszMatchUser = pNetArgs->arg[NSS_NETGR_USER].argc ? pNetArgs->arg[NSS_NETGR_USER].argv[iMatch] : NULL;
            pszMatchDomain = pNetArgs->arg[NSS_NETGR_DOMAIN].argc ? pNetArgs->arg[NSS_NETGR_DOMAIN].argv[iMatch] : NULL;

            if ((pszMatchHost == NULL || !strcmp(pszHost, pszMatchHost)) &&
                (pszMatchUser == NULL || !strcmp(pszUser, pszMatchUser)) &&
                (pszMatchDomain == NULL || !strcmp(pszDomain, pszMatchDomain)))
            {
                pNetArgs->status = NSS_NETGR_FOUND;
                goto cleanup;
            }
        }
    }

    ret = NSS_STATUS_NOTFOUND;
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    LsaNssSolarisNetgroupDestructor((nss_backend_t*) pInnerBackend, NULL);

    return ret;

error:

    if (ret == NSS_STATUS_NOTFOUND)
    {
        pNetArgs->status = NSS_NETGR_NO;
    }

    goto cleanup;
}

static
NSS_ENTRYPOINT
LsaNssSolarisNetgroupOps[] =
{
    LsaNssSolarisNetgroupDestructor,
    LsaNssSolarisNetgroupInnerEndent,
    LsaNssSolarisNetgroupInnerSetent,
    LsaNssSolarisNetgroupInnerGetent,
    LsaNssSolarisNetgroupInnetgr,
    LsaNssSolarisNetgroupSetnetgrent
};

static
nss_backend_t
LsaNssSolarisNetgroupBackend =
{
    .n_ops = 6,
    .ops = LsaNssSolarisNetgroupOps
};

nss_backend_t*
LsaNssSolarisNetgroupCreateBackend(
    void
    )
{
    PLSA_NSS_NETGROUP_BACKEND pLsaBackend = NULL;

    if (LwAllocateMemory(sizeof(*pLsaBackend), (void**) &pLsaBackend))
    {
        return NULL;
    }

    pLsaBackend->base = LsaNssSolarisNetgroupBackend;
    pLsaBackend->inner = 0;

    return (nss_backend_t*) pLsaBackend;
}
