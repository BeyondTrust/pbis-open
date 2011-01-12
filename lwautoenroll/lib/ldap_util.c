/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <bail.h>
#include <ldap_util.h>

#include <ldap.h>

#include <lwerror.h>
#include <lwmem.h>
#include <lwnet.h>
#include <lwldap.h>
#include <lwstr.h>

DWORD
LwAutoEnrollFindDomainDn(
        PCSTR dn,
        PCSTR* pDomainDn
        )
{
    DWORD error = LW_ERROR_SUCCESS;

    while (dn != NULL)
    {
        if (!strncasecmp(dn, "dc=", 3))
        {
            break;
        }

        if ((dn = strchr(dn, ',')) != NULL)
        {
            ++dn;
        }
    }

    if (dn == NULL)
    {
        BAIL_WITH_LW_ERROR(LW_ERROR_INVALID_LDAP_DN);
    }

cleanup:
    *pDomainDn = dn;
    return error;
}

DWORD
LwAutoEnrollLdapSearch(
        IN HANDLE ldapConnection,
        IN PCSTR domainDn,
        IN PCSTR searchBase,
        IN int scope,
        IN PCSTR queryString,
        IN PSTR *pAttributeNames,
        OUT LDAPMessage **ppLdapResults
        )
{
    PSTR searchDn = NULL;
    LDAPMessage *pLdapResults = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    error = LwAllocateStringPrintf(&searchDn, "%s,%s", searchBase, domainDn);
    BAIL_ON_LW_ERROR(error);

    error = LwLdapDirectorySearch(
                ldapConnection,
                searchDn,
                scope,
                queryString,
                pAttributeNames,
                &pLdapResults);
    BAIL_ON_LW_ERROR(error);

cleanup:
    if (error)
    {
        if (pLdapResults)
        {
            ldap_msgfree(pLdapResults);
            pLdapResults = NULL;
        }
    }

    LW_SAFE_FREE_STRING(searchDn);

    *ppLdapResults = pLdapResults;
    return error;
}

DWORD
LwAutoEnrollLdapConnect(
        IN PCSTR domainDnsName,
        OUT HANDLE *pLdapConnection
        )
{
    PLWNET_DC_INFO pDcInfo = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    error = LWNetGetDCName(
                NULL,
                domainDnsName,
                NULL,
                DS_ONLY_LDAP_NEEDED,
                &pDcInfo);
    BAIL_ON_LW_ERROR(error);

    error = LwLdapOpenDirectoryServer(
                pDcInfo->pszDomainControllerAddress,
                pDcInfo->pszDomainControllerName,
                0,
                pLdapConnection);
    BAIL_ON_LW_ERROR(
            error,
            ": Could not connect to DC %s [%s] for domain %s",
            pDcInfo->pszDomainControllerName,
            pDcInfo->pszDomainControllerAddress,
            domainDnsName);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    return error;
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
